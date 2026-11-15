param([string]$Compiler = 'g++')
$ErrorActionPreference = 'Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    New-Item -ItemType Directory -Force build/oracle | Out-Null
    $flags = @('-std=c++17', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Iinclude')
    & $Compiler @flags oracle/main.cpp -o build/oracle/oracle.exe
    if ($LASTEXITCODE -ne 0) { throw 'Oracle compilation failed' }
    & $Compiler @flags oracle/differential.cpp src/structA/avl.cpp src/structB/fenwick.cpp -o build/oracle/differential.exe
    if ($LASTEXITCODE -ne 0) { throw 'Differential compilation failed' }
} finally { Pop-Location }
