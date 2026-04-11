#!/usr/bin/env python3
import argparse
import os
import shutil
import stat
import subprocess
import sys
from pathlib import Path

# -----------------------------------------------------------------------------
# Paths
# -----------------------------------------------------------------------------
PROJECT_ROOT = Path(__file__).resolve().parent
BUILD_DIR = PROJECT_ROOT / "build"
BINARIES_DIR = BUILD_DIR / "bin"


# -----------------------------------------------------------------------------
# Colors / Logging
# -----------------------------------------------------------------------------
class Color:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    RED = "\033[31m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    BLUE = "\033[34m"
    CYAN = "\033[36m"
    DIM = "\033[2m"


LABEL_WIDTH = 12


def _log(label: str, msg: str, color: str = "") -> None:
    print(f"{Color.BOLD}{color}[{label}]".ljust(LABEL_WIDTH) + f"{Color.RESET} {msg}")


def info(msg: str) -> None: _log("INFO", msg, Color.BLUE)


def warn(msg: str) -> None: _log("WARN", msg, Color.YELLOW)


def success(msg: str) -> None: _log("SUCCESS", msg, Color.GREEN)


def error(msg: str) -> None: _log("ERROR", msg, Color.RED)


def configure(msg: str) -> None: _log("CONFIGURE", msg, Color.BLUE)


def build_log(msg: str) -> None: _log("BUILD", msg, Color.BLUE)


def run_log(msg: str) -> None: _log("RUN", msg, Color.CYAN)


def log_cmd(msg: str) -> None: _log("CMD", msg, Color.DIM)


# -----------------------------------------------------------------------------
# Utilities
# -----------------------------------------------------------------------------
def run_cmd(cmd_list, cwd: Path | None = None) -> None:
    log_cmd(" ".join(cmd_list))
    result = subprocess.run(cmd_list, cwd=cwd)
    if result.returncode != 0:
        error(f"Command failed with exit code {result.returncode}")
        sys.exit(result.returncode)


def get_macos_sdk_path() -> str:
    return subprocess.check_output(["xcrun", "--sdk", "macosx", "--show-sdk-path"], text=True).strip()


# -----------------------------------------------------------------------------
# vcpkg
# -----------------------------------------------------------------------------
def get_vcpkg_toolchain() -> Path:
    vcpkg_root = os.environ.get("VCPKG_ROOT")
    if not vcpkg_root:
        error("VCPKG_ROOT environment variable is not set.")
        sys.exit(1)
    toolchain = Path(vcpkg_root) / "scripts" / "buildsystems" / "vcpkg.cmake"
    if not toolchain.exists():
        error(f"vcpkg toolchain not found: {toolchain}")
        sys.exit(1)
    return toolchain


# -----------------------------------------------------------------------------
# Build Steps
# -----------------------------------------------------------------------------
def get_source_files() -> list[Path]:
    exclude_paths = [BUILD_DIR]
    files = []
    for folder in ["src", "include"]:
        for ext in ("*.cpp", "*.hpp"):
            for f in Path(folder).rglob(ext):
                if any(excl in f.parents for excl in exclude_paths):
                    continue
                files.append(f)
    return files


def run_clang_tidy() -> None:
    info("Running clang-tidy")

    cmd = [
        "clang-tidy",
        "-p", str(BUILD_DIR),
        "-quiet",
        "-header-filter=src/.*|include/.*",
        "--warnings-as-errors=*",
    ]

    if sys.platform == "darwin":
        cmd.append("--extra-arg-before=--driver-mode=clang++")
        sdk = get_macos_sdk_path()
        if sdk:
            cmd += ["--extra-arg=-isysroot", f"--extra-arg={sdk}"]

    src_files = get_source_files()
    cmd += [str(f) for f in src_files]

    run_cmd(cmd)


def run_clang_format():
    info("Running clang-format (dry run)")
    src_files = get_source_files()
    for f in src_files:
        run_cmd(["clang-format", "--dry-run", "--Werror", str(f)])


def clean_build_dir() -> None:
    if BUILD_DIR.exists():
        warn(f"Removing {BUILD_DIR}")
        shutil.rmtree(BUILD_DIR)
    BUILD_DIR.mkdir(parents=True, exist_ok=True)


def configure_cmake(toolchain: Path, build_type: str = "Debug") -> None:
    configure(f"Running CMake ({build_type})")
    run_cmd([
        "cmake",
        str(PROJECT_ROOT),
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    ], cwd=BUILD_DIR)


def build_target(target: str | None) -> None:
    toolchain = get_vcpkg_toolchain()
    compile_shaders()
    configure_cmake(toolchain)
    cmd = ["cmake", "--build", "."]
    if target:
        cmd += ["--target", target]
        build_log(f"Building target {target}")
    else:
        build_log(f"Building all targets")
    run_cmd(cmd, cwd=BUILD_DIR)


# -----------------------------------------------------------------------------
# Shader Compilation
# -----------------------------------------------------------------------------
def compile_shaders() -> None:
    """Compile shaders from the assets directory"""
    shaders_dir = PROJECT_ROOT / "assets/shaders"
    out_dir = BINARIES_DIR / "assets/shaders"

    for pipeline_dir in shaders_dir.iterdir():
        if not pipeline_dir.is_dir():
            continue

        pipeline_name = pipeline_dir.name
        info(f"Compiling shaders for pipeline: {pipeline_name}")
        shader_files = list(pipeline_dir.glob("*.vert")) + list(pipeline_dir.glob("*.frag"))
        if not shader_files:
            warn(f"No .vert or .frag files found in pipeline {pipeline_name}")
            continue

        pipeline_out_dir = out_dir / pipeline_name
        pipeline_out_dir.mkdir(parents=True, exist_ok=True)

        for shader_file in shader_files:
            output_file = pipeline_out_dir / f"{shader_file.name}.spv"
            info(f"Compiling {pipeline_name}/{shader_file.name} -> {shader_file.name}.spv")
            run_cmd(["glslc", str(shader_file), "-o", str(output_file)])


# -----------------------------------------------------------------------------
# Executable Handling
# -----------------------------------------------------------------------------
def ensure_executable(path: Path) -> None:
    if not os.access(path, os.X_OK):
        warn(f"Setting executable permission: {path}")
        path.chmod(path.stat().st_mode | stat.S_IXUSR)


def find_executable(target: str) -> Path:
    candidates = [
        BINARIES_DIR / target,
    ]
    for path in candidates:
        if path.is_file():
            ensure_executable(path)
            return path
    error(f"Executable {target} not found. Checked:\n" + "\n".join(f"  - {p}" for p in candidates))
    sys.exit(1)


def run_target(target: str) -> None:
    exe = find_executable(target)
    run_log(f"Running executable: {exe}")
    run_cmd([str(exe)], cwd=exe.parent)


# -----------------------------------------------------------------------------
# Main Workflow
# -----------------------------------------------------------------------------
def main() -> None:
    parser = argparse.ArgumentParser(description="Build script for CrossPlatformVulkanEngine")
    parser.add_argument("target", type=str, help="Name of the executable target to build and run", nargs="?")
    parser.add_argument("--lint", action="store_true", help="Run Clang-Tidy and exit")
    args = parser.parse_args()

    if args.target == "clean":
        clean_build_dir()
        success("Clean completed.")
        return

    info("Starting build workflow")
    build_target(args.target)

    if args.lint:
        run_clang_format()
        run_clang_tidy()

    if not args.target:
        success("Successfully built all targets")
    else:
        success(f"Successfully built target {args.target}")
        run_target(args.target)


if __name__ == "__main__":
    main()
