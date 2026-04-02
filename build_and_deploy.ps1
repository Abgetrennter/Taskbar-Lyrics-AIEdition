$ErrorActionPreference = "Stop"

Write-Host "Searching for MSBuild..."
$msbuild = Get-Command "msbuild" -ErrorAction SilentlyContinue
if (-not $msbuild) {
    # Try vswhere
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $path = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe
        if ($path) {
            $msbuild = $path
        }
    }
}

if (-not $msbuild) {
    Write-Error "MSBuild not found. Please ensure Visual Studio is installed."
    exit 1
}

Write-Host "Using MSBuild: $msbuild"

Write-Host "Building Solution..."
& $msbuild "Taskbar Lyrics.sln" /p:Configuration=Release /p:Platform=x86 /t:Rebuild

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed."
    exit 1
}

$dist = "$PSScriptRoot\dist"
if (-not (Test-Path $dist)) {
    New-Item -ItemType Directory -Path $dist | Out-Null
}

$pluginDist = "$dist\betterncm-plugin"
if (Test-Path $pluginDist) {
    Remove-Item $pluginDist -Recurse -Force
}
New-Item -ItemType Directory -Path $pluginDist | Out-Null

Write-Host "Copying plugin files..."
Copy-Item "$PSScriptRoot\src\betterncm-plugin\*" "$pluginDist" -Recurse

Write-Host "Build and deploy complete at $dist"
