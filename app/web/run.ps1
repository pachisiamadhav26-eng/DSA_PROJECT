param([int]$Port = 25353)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path

Push-Location $repo
try {
    New-Item -ItemType Directory -Force -Path 'build' | Out-Null
    $avl = 'src/structA/avl.cpp'
    $fenwick = 'src/structB/fenwick.cpp'

    if (-not (Test-Path -LiteralPath $avl) -or -not (Test-Path -LiteralPath $fenwick)) {
        New-Item -ItemType Directory -Force -Path 'build/include' | Out-Null
        git show 'origin/main:src/structA/avl.cpp' | Set-Content -LiteralPath 'build/avl_backend.cpp' -Encoding utf8
        if ($LASTEXITCODE -ne 0) { throw 'Could not read the AVL code from main. Run git fetch origin first.' }
        git show 'origin/main:src/structB/fenwick.cpp' | Set-Content -LiteralPath 'build/fenwick_backend.cpp' -Encoding utf8
        if ($LASTEXITCODE -ne 0) { throw 'Could not read the Fenwick code from main. Run git fetch origin first.' }
        git show 'origin/main:include/backend_tools.hpp' | Set-Content -LiteralPath 'build/include/backend_tools.hpp' -Encoding utf8
        if ($LASTEXITCODE -ne 0) { throw 'Could not read the shared backend header from main. Run git fetch origin first.' }
        $avl = 'build/avl_backend.cpp'
        $fenwick = 'build/fenwick_backend.cpp'
    }

    g++ -std=c++17 -O2 -Wall -Wextra -Werror -Iinclude -Iapp -Ibuild/include `
        app/web/server.cpp app/replay.cpp app/player_table.cpp `
        $avl $fenwick -lws2_32 -o build/leaderboard_web.exe
    if ($LASTEXITCODE -ne 0) { throw 'The C++ web server did not compile.' }

    Write-Host "Open http://127.0.0.1:$Port in your browser."
    Write-Host 'Press Ctrl+C here to stop the server.'
    & .\build\leaderboard_web.exe $Port
} finally {
    Pop-Location
}
