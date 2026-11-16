param(
    [string]$Compiler = 'g++',
    [string]$ArchitectRef = 'a4dc4382b11d60ed85697fb9560574d24a8d2366',
    [string]$AvlRef = '03f3f57186f96bac6b2ad8f22acbedd7da2de153',
    [string]$FenwickRef = '0d5ad686ea2349147a4b6eb1ca2b64eb52feae38'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    function Resolve-Source([string]$Ref) {
        $sha = git rev-parse --verify --end-of-options "$Ref^{commit}"
        if ($LASTEXITCODE -ne 0) { throw "Missing dependency $Ref. Run git fetch origin." }
        return $sha
    }

    function Read-Source([string]$Sha, [string]$Path) {
        $content = git show "${Sha}:$Path"
        if ($LASTEXITCODE -ne 0) { throw "Unable to read ${Sha}:$Path" }
        return $content
    }

    function Export-Source([string]$Sha, [string]$Path) {
        $destination = Join-Path $root "build/oracle/deps/$Path"
        New-Item -ItemType Directory -Force (Split-Path $destination -Parent) | Out-Null
        [IO.File]::WriteAllLines($destination, [string[]](Read-Source $Sha $Path),
            [Text.UTF8Encoding]::new($false))
    }

    function Build-Executable([string]$Name, [string[]]$Sources) {
        & $Compiler -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude -Ibuild/oracle/deps/include @Sources -o "build/oracle/$Name.exe"
        if ($LASTEXITCODE -ne 0) { throw "$Name compilation failed" }
    }

    $architect = Resolve-Source $ArchitectRef
    $avl = Resolve-Source $AvlRef
    $fenwick = Resolve-Source $FenwickRef
    $sharedHeader = (Read-Source $architect 'include/leaderboard.hpp') -join "`n"
    if (((Get-Content include/leaderboard.hpp) -join "`n") -cne $sharedHeader) {
        throw "Oracle's shared header differs from the Architect header at $architect."
    }
    Export-Source $avl 'avl.cpp'
    Export-Source $fenwick 'include/backend_tools.hpp'
    Export-Source $fenwick 'src/structB/fenwick.cpp'
    foreach ($path in @('leaderboard.cpp','replay.cpp','replay.hpp','player_table.cpp','player_table.hpp')) {
        Export-Source $architect "app/$path"
    }
    $backends = @('build/oracle/deps/avl.cpp','build/oracle/deps/src/structB/fenwick.cpp')
    $app = @('leaderboard.cpp','replay.cpp','player_table.cpp') | ForEach-Object { "build/oracle/deps/app/$_" }
    Build-Executable 'oracle' @('oracle/main.cpp')
    Build-Executable 'differential' (@('oracle/differential.cpp') + $backends)
    Build-Executable 'leaderboard' ($app + $backends)
    @("Architect $architect","AVL $avl","Fenwick $fenwick") |
        Set-Content -Encoding ascii build/oracle/sources.txt
} finally { Pop-Location }
