$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildWeb = Join-Path $Root "Build\Web"
$EmsdkDir = Join-Path $Root "Build\emsdk"

function Write-Step([string]$Message) {
    Write-Host ""
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Ensure-Directory([string]$Path) {
    if (-not (Test-Path $Path)) {
        New-Item -ItemType Directory -Path $Path | Out-Null
    }
}

function Find-Emcc {
    $existing = Get-Command emcc -ErrorAction SilentlyContinue
    if ($existing) {
        return $existing.Source
    }

    $candidate = Join-Path $EmsdkDir "upstream\emscripten\emcc.bat"
    if (Test-Path $candidate) {
        return $candidate
    }

    return $null
}

function Ensure-Emsdk {
    $activate = Join-Path $EmsdkDir "emsdk_env.ps1"
    if (Test-Path $activate) {
        & $activate | Out-Null
    }

    $emccPath = Find-Emcc
    if ($emccPath) {
        Write-Host "Using emcc: $emccPath"
        return
    }

    Write-Step "Emscripten not found. Installing emsdk into Build\emsdk (one-time)"
    Ensure-Directory (Join-Path $Root "Build")

    if (-not (Test-Path $EmsdkDir)) {
        git clone https://github.com/emscripten-core/emsdk.git $EmsdkDir
    }

    Push-Location $EmsdkDir
    try {
        & .\emsdk.bat install latest
        & .\emsdk.bat activate latest
        & .\emsdk_env.ps1 | Out-Null
    }
    finally {
        Pop-Location
    }

    $emccPath = Find-Emcc
    if (-not $emccPath) {
        throw "emcc still not available after emsdk install"
    }
    Write-Host "Using emcc: $emccPath"
}

function Invoke-WebBuild {
    Write-Step "Compiling Robo WebAssembly module"
    Ensure-Directory $BuildWeb

    $sources = @(
        (Join-Path $Root "Compiler\Source\Color.cpp"),
        (Join-Path $Root "Compiler\Source\TextAlign.cpp"),
        (Join-Path $Root "Compiler\Source\FontStyle.cpp"),
        (Join-Path $Root "Compiler\Source\Text.cpp"),
        (Join-Path $Root "XMLCompiler\Source\XmlParser.cpp"),
        (Join-Path $Root "XMLCompiler\Source\XmlRuntime.cpp"),
        (Join-Path $Root "XMLCompiler\Source\HtmlFromXml.cpp"),
        (Join-Path $Root "XMLCompiler\Source\XmlToCpp.cpp"),
        (Join-Path $Root "RoboPlatforms\Web\WebPlatform.cpp"),
        (Join-Path $Root "RoboPlatforms\Web\Main.cpp")
    )

    $includeArgs = @(
        "-I$(Join-Path $Root 'Compiler\Include')",
        "-I$(Join-Path $Root 'XMLCompiler\Include')",
        "-I$(Join-Path $Root 'RoboPlatforms\Web')"
    )

    $outputJs = Join-Path $BuildWeb "Robo.js"

    $emccArgs = @(
        "-O2",
        "-std=c++17",
        "--bind"
    ) + $includeArgs + $sources + @(
        "-sMODULARIZE=1",
        "-sEXPORT_NAME=CreateRoboModule",
        "-sALLOW_MEMORY_GROWTH=1",
        "-sENVIRONMENT=web",
        "-sFILESYSTEM=0",
        "-o", $outputJs
    )

    & em++ @emccArgs
    if ($LASTEXITCODE -ne 0) {
        throw "em++ failed with exit code $LASTEXITCODE"
    }

    Write-Host "Built: $outputJs"
    Write-Host "Built: $(Join-Path $BuildWeb 'Robo.wasm')"
}

Write-Step "RoboDev Web build"
Ensure-Emsdk
Invoke-WebBuild
Write-Step "Build complete"
Write-Host "Run .\RunRoboIDE.ps1 to open the IDE."
