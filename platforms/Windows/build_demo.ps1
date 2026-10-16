param([string]$BuildDirectory = "", [switch]$SkipTests)
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
if ($env:OS -ne "Windows_NT") { throw "Windows x64 and Visual Studio 2022 C++ tools are required." }
if (-not $BuildDirectory) { $BuildDirectory = Join-Path $PSScriptRoot "build" }
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$Sdk = Join-Path $PSScriptRoot "sdk"
function Invoke-Checked([string]$Program, [string[]]$Parameters) {
    & $Program @Parameters
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
foreach ($Role in @("sdk", "demo")) {
    $Source = if ($Role -eq "sdk") { Join-Path $PSScriptRoot "examples" } else { Join-Path $PSScriptRoot "demo/source" }
    $Build = Join-Path $BuildDirectory $Role
    Invoke-Checked "cmake" @("-S", $Source, "-B", $Build, "-G", "Visual Studio 17 2022", "-A", "x64", "-DCMAKE_PREFIX_PATH=$Sdk")
    Invoke-Checked "cmake" @("--build", $Build, "--config", "Release", "--parallel", "2")
    if (-not $SkipTests) {
        Push-Location "$Build/bin/Release"
        try {
            if ($Role -eq "sdk") {
                foreach ($Name in @("hiface_c_consumer", "hiface_cpp_consumer", "hiface_model_consumer")) {
                    Invoke-Checked "./$Name.exe" @()
                }
            } else {
                Invoke-Checked "./hiface_cli.exe" @("self-test")
                $TestData = Join-Path $env:TEMP ("HiFace Demo data " + [Guid]::NewGuid().ToString("N"))
                New-Item -ItemType Directory -Path $TestData | Out-Null
                try {
                    $Database = Join-Path $TestData "faces.db"
                    $Roundtrip = Join-Path $TestData "roundtrip.db"
                    $Export = Join-Path $TestData "features.json"
                    Invoke-Checked "./hiface_cli.exe" @("import-json", $Database, (Join-Path $PSScriptRoot "demo/data/features_10000.json"))
                    Invoke-Checked "./hiface_cli.exe" @("export-json", $Database, $Export)
                    Invoke-Checked "./hiface_cli.exe" @("import-json", $Roundtrip, $Export)
                    $Count = & "./hiface_cli.exe" count $Roundtrip
                    if ($LASTEXITCODE -ne 0 -or ($Count -join "`n").Trim() -ne "count=10000") { throw "AI Demo data roundtrip failed" }
                    Write-Host "AI Demo data: 10000 HF1 entries imported and roundtripped."
                } finally { Remove-Item -LiteralPath $TestData -Recurse -Force }
            }
        } finally { Pop-Location }
    }
}
Write-Host "Demo: $BuildDirectory/demo/bin/Release/hiface_demo.exe"
