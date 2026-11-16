param([switch]$Quick, [switch]$SkipBuild, [ValidateRange(1, 1000000)][int]$MaxLive = 48)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    if (-not $SkipBuild) { & "$PSScriptRoot/build.ps1" }
    Get-Content build/oracle/sources.txt
    foreach ($case in @('basic', 'errors', 'seasons', 'wide_ids')) {
        $actual = & ./build/oracle/oracle.exe "oracle/tests/$case.trace"
        if ($LASTEXITCODE -ne 0) { throw "Oracle failed: $case" }
        $expected = Get-Content "oracle/tests/$case.expected"
        if (($actual -join "`n") -cne ($expected -join "`n")) { throw "Golden output mismatch: $case" }
        Write-Output "PASS golden $case"
        foreach ($backend in @('avl','fenwick')) {
            $actual = & ./build/oracle/leaderboard.exe $backend "oracle/tests/$case.trace"
            if ($LASTEXITCODE -ne 0 -or ($actual -join "`n") -cne ($expected -join "`n")) {
                throw "Production $backend replay differed: $case"
            }
            Write-Output "PASS production $backend $case"
        }
        & ./build/oracle/differential.exe --trace "oracle/tests/$case.trace"
        if ($LASTEXITCODE -ne 0) { throw "Differential failed: $case" }
    }
    foreach ($seed in @(253, 20261112, 42)) {
        $operations = if ($Quick) { 10000 } elseif ($seed -eq 253) { 1000000 } else { 100000 }
        & ./build/oracle/differential.exe --seed $seed --ops $operations --max-live $MaxLive
        if ($LASTEXITCODE -ne 0) { throw "Differential failed: seed $seed" }
    }
    $size = if ($Quick) { 10000 } else { 1000000 }
    & ./build/oracle/differential.exe --scale $size
    if ($LASTEXITCODE -ne 0) { throw 'Scale check failed' }
} finally { Pop-Location }
