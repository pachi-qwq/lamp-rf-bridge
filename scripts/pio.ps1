# SPDX-License-Identifier: MIT
# Use the project's virtual environment/cache regardless of the caller's directory.
# 从任意目录调用，始终使用项目隔离环境与项目内缓存。
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectPython = Join-Path $ProjectRoot '.venv\Scripts\python.exe'
if (-not (Test-Path -LiteralPath $ProjectPython)) {
    throw '缺少项目 Python 环境，请先按 README 安装开发工具。'
}
Push-Location $ProjectRoot
try {
    $env:PLATFORMIO_CORE_DIR = Join-Path $ProjectRoot '.pio-core'
    & $ProjectPython -m platformio @args
    $ToolExitCode = $LASTEXITCODE
} finally {
    Pop-Location
}
exit $ToolExitCode
