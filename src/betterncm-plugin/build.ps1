$srcDir = "$PSScriptRoot\src"
$modulesDir = "$srcDir\modules"
$outputFile = "$PSScriptRoot\main.js"

$files = @(
    "$modulesDir\constants.js",
    "$modulesDir\utils.js",
    "$modulesDir\config.js",
    "$modulesDir\api.js",
    "$modulesDir\lyric.js",
    "$modulesDir\backend.js",
    "$modulesDir\view.js",
    "$srcDir\index.js"
)

# Clear output file and ensure UTF8 without BOM (optional, but good practice)
$content = ""
foreach ($file in $files) {
    if (Test-Path $file) {
        $fileContent = Get-Content $file -Raw -Encoding UTF8
        # Remove "use strict"; lines to avoid redundancy, add it once at top
        $fileContent = $fileContent -replace '"use strict";', ''
        $content += $fileContent + "`n"
    } else {
        Write-Error "File not found: $file"
    }
}

$finalContent = '"use strict";' + "`n" + $content
Set-Content -Path $outputFile -Value $finalContent -Encoding UTF8

Write-Host "Build complete: $outputFile"
