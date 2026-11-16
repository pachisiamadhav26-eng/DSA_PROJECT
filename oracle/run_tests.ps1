param([switch]$Quick, [switch]$SkipBuild, [ValidateRange(1, 1000000)][int]$MaxLive = 48)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    function Check-Output([string]$Executable, [string[]]$Arguments, [string]$Expected, [string]$Label) {
        $actual = & $Executable @Arguments
        if ($LASTEXITCODE -ne 0) {
            throw "$Label exited with code $LASTEXITCODE"
        }
        if (($actual -join "`n") -cne $Expected) {
            throw "$Label output differed from the expected file"
        }
        Write-Output "PASS $Label"
    }

    if (-not $SkipBuild) {
        & "$PSScriptRoot/build.ps1"
    }
    Get-Content build/oracle/sources.txt

    # Run each hand-written fixture through the independent oracle and real app.
    foreach ($case in @('basic', 'errors', 'seasons', 'wide_ids')) {
        $trace = "oracle/tests/$case.trace"
        $expected = (Get-Content "oracle/tests/$case.expected") -join "`n"
        Check-Output './build/oracle/oracle.exe' @($trace) $expected "golden $case"
        foreach ($backend in @('avl','fenwick')) {
            Check-Output './build/oracle/leaderboard.exe' @($backend, $trace) $expected "production $backend $case"
        }
        & ./build/oracle/differential.exe --trace $trace
        if ($LASTEXITCODE -ne 0) { throw "Differential failed: $case" }
    }
    foreach ($seed in @(253, 20261112, 42)) {
        $operations = if ($Quick) { 10000 } elseif ($seed -eq 253) { 1000000 } else { 100000 }
        & ./build/oracle/differential.exe --seed $seed --ops $operations --max-live $MaxLive
        if ($LASTEXITCODE -ne 0) { throw "Differential failed: seed $seed" }
    }
    # This is a separate test with many live players and audits at checkpoints.
    $players = if ($Quick) { 10000 } else { 1000000 }
    & ./build/oracle/differential.exe --scale $players
    if ($LASTEXITCODE -ne 0) { throw 'Scale check failed' }
} finally { Pop-Location }
