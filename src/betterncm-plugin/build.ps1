<#
.SYNOPSIS
    构建脚本
.DESCRIPTION
    将分散的模块文件合并为单一的 main.js 文件
.AUTHOR
    Taskbar Lyrics Plugin Developer
.DATE
    2025-12-01
#>

$srcDir = "$PSScriptRoot\src"
$modulesDir = "$srcDir\modules"
$outputFile = "$PSScriptRoot\main.js"

# 定义构建文件顺序
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

# 清空输出内容
$content = ""
foreach ($file in $files) {
    if (Test-Path $file) {
        $fileContent = Get-Content $file -Raw -Encoding UTF8
        # 移除 "use strict"; 声明
        $fileContent = $fileContent -replace '(?m)^\s*["'']use strict["''];\s*$', ''
        $content += $fileContent + "`n"
    } else {
        Write-Error "File not found: $file"
    }
}

# 添加全局严格模式声明
$finalContent = '"use strict";' + "`n" + $content
Set-Content -Path $outputFile -Value $finalContent -Encoding UTF8

Write-Host "Build complete: $outputFile"
