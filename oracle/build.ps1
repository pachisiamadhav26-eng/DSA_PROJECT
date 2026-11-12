param([string]$Compiler = 'g++')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    New-Item -ItemType Directory -Force build/oracle/deps | Out-Null
    # Immutable teammate commits, exported without checking out/merging branches.
    $sources = @(
        @('2db28739e291a07bf65c68b44db6a86fbd187b91', 'avl.cpp'),
        @('de0ab032ce8599d1ef7c9be048377c23cd571327', 'Fenvick.cpp')
    )
    foreach ($source in $sources) {
        $content = git show ($source[0] + ':' + $source[1])
        if ($LASTEXITCODE -ne 0) { throw 'Missing source commit: run git fetch origin first.' }
        [IO.File]::WriteAllLines((Join-Path $root ('build/oracle/deps/' + $source[1])),
            [string[]]$content, [Text.UTF8Encoding]::new($false))
    }
    & $Compiler -std=c++17 -O2 -Wall -Wextra -Wpedantic oracle/main.cpp -o build/oracle/oracle.exe
    if ($LASTEXITCODE -ne 0) { throw 'Oracle compilation failed' }
    & $Compiler -std=c++17 -O2 -Wall -Wextra -Wpedantic -I build/oracle/deps oracle/differential.cpp -o build/oracle/differential.exe
    if ($LASTEXITCODE -ne 0) { throw 'Differential compilation failed' }
} finally { Pop-Location }
