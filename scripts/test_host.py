# SPDX-License-Identifier: MIT
"""Run pure C++ logic tests with MSVC on Windows or a C++17 compiler on POSIX; no hardware simulation.
在 Windows MSVC 上运行纯逻辑测试，不连接或模拟硬件。
"""
import os
import shutil
from pathlib import Path
import subprocess
import sys


def main():
    root = Path(__file__).resolve().parent.parent
    # POSIX CI uses the system C++17 compiler; no hardware is simulated.
    # POSIX CI 使用系统 C++17 编译器；不模拟硬件。
    if os.name != "nt":
        output = root / "build/host"
        output.mkdir(parents=True, exist_ok=True)
        compiler = os.environ.get("CXX", "g++")
        for name in ("test_identity", "test_radio_math", "test_replay", "test_web_policy"):
            executable = output / name
            subprocess.run([
                compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-pedantic",
                "-I", str(root / "include"), str(root / f"tests/{name}.cpp"),
                "-o", str(executable),
            ], cwd=root, check=True)
            subprocess.run([str(executable)], check=True)
        return 0
    vswhere = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)"))
    vswhere /= "Microsoft Visual Studio/Installer/vswhere.exe"
    if not vswhere.is_file():
        raise RuntimeError("需要 Visual Studio C++ Build Tools（未自动安装）。")
    installation = subprocess.check_output(
        [str(vswhere), "-latest", "-products", "*", "-requires",
         "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
        text=True,
    ).strip()
    if not installation:
        raise RuntimeError("找不到 MSVC C++ 工具链。")
    vcvars = Path(installation) / "VC/Auxiliary/Build/vcvars64.bat"
    # Capture only the compiler environment; do not log the full environment.
    # 仅捕获编译器所需环境，不把整份环境变量写入日志。
    result = subprocess.run(
        f'cmd.exe /d /s /c ""{vcvars}" >nul && set"',
        capture_output=True, text=True, check=True,
    )
    compiler_env = {key.upper(): value for key, value in os.environ.items()}
    for line in result.stdout.splitlines():
        key, separator, value = line.partition("=")
        if separator and key:
            compiler_env[key.upper()] = value
    output = root / "build/host"
    output.mkdir(parents=True, exist_ok=True)
    compiler = shutil.which("cl.exe", path=compiler_env.get("PATH", ""))
    if not compiler:
        raise RuntimeError("MSVC 环境初始化后仍找不到 cl.exe。")
    for name in ("test_identity", "test_radio_math", "test_replay", "test_web_policy"):
        executable = output / f"{name}.exe"
        command = [
            compiler, "/nologo", "/std:c++17", "/W4", "/WX", "/EHsc", "/utf-8",
            f"/I{root / 'include'}", str(root / f"tests/{name}.cpp"),
            f"/Fe{executable}", f"/Fo{output / (name + '.obj')}",
        ]
        subprocess.run(command, env=compiler_env, cwd=output, check=True)
        subprocess.run([str(executable)], check=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
