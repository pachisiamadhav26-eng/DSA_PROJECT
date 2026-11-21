param([switch]$Quick, [string]$Compiler = 'g++')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $arguments = @('scripts/run_all.py', '--compiler', $Compiler)
    if ($Quick) { $arguments += '--quick' }
    python @arguments
    if ($LASTEXITCODE -ne 0) { throw "Pipeline failed with exit code $LASTEXITCODE" }
} finally { Pop-Location }
