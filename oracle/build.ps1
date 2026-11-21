param([string]$Compiler = 'g++')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $backends = @('src/structA/avl.cpp', 'src/structB/fenwick.cpp')
    $app = @('app/leaderboard.cpp', 'app/replay.cpp', 'app/player_table.cpp')
    $sources = @('include/leaderboard.hpp', 'include/backend_tools.hpp',
        'app/replay.hpp', 'app/player_table.hpp') + $backends + $app
    foreach ($path in $sources) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required source missing from this checkout: $path"
        }
    }
    New-Item -ItemType Directory -Force 'build/oracle' | Out-Null

    function Build-Executable([string]$Name, [string[]]$Sources) {
        $flags = @('-std=c++17', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Iinclude')
        & $Compiler @flags @Sources -o "build/oracle/$Name.exe"
        if ($LASTEXITCODE -ne 0) { throw "$Name compilation failed" }
    }

    Build-Executable 'oracle' @('oracle/main.cpp')
    Build-Executable 'differential' (@('oracle/differential.cpp') + $backends)
    Build-Executable 'leaderboard' ($app + $backends)

    # Record the actual files compiled, including any uncommitted changes.
    $record = @('Source mode: current checkout (no dependency export or fetch)')
    foreach ($path in $sources) {
        $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        $record += "SHA256 $hash $path"
    }
    $record | Set-Content -Encoding ascii 'build/oracle/sources.txt'
} finally { Pop-Location }
