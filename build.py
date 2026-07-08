#!/usr/bin/env python3
import argparse
import os
import shutil
import stat
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

# -----------------------------------------------------------------------------
# Paths
# -----------------------------------------------------------------------------
PROJECT_ROOT = Path(__file__).resolve().parent
BUILD_DIR = PROJECT_ROOT / "build"
XCODE_BUILD_DIR = PROJECT_ROOT / "build-xcode"
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
def run_cmd(cmd_list, cwd: Path | None = None) -> int:
    log_cmd(" ".join(cmd_list))
    result = subprocess.run(cmd_list, cwd=cwd)
    if result.returncode != 0:
        error(f"Command failed with exit code {result.returncode}")
        return result.returncode
    return 0


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
    for folder in ["runtime/src", "tools/editor/src"]:
        for ext in ("*.cpp", "*.hpp"):
            for f in Path(folder).rglob(ext):
                if any(excl in f.parents for excl in exclude_paths):
                    continue
                files.append(f)
    return files


def run_clang_tidy() -> bool:
    info("Running clang-tidy")
    base_cmd = [
        "clang-tidy",
        "-p", str(BUILD_DIR),
        "-quiet",
        "-header-filter=runtime/src/.*|tools/editor/src/.*",
        "--warnings-as-errors=*",
    ]
    if sys.platform == "darwin":
        base_cmd.append("--extra-arg-before=--driver-mode=clang++")
        sdk = get_macos_sdk_path()
        if sdk:
            base_cmd += ["--extra-arg=-isysroot", f"--extra-arg={sdk}"]
    src_files = get_source_files()
    commands = [base_cmd + [str(f)] for f in src_files]
    failed = False
    with ThreadPoolExecutor() as executor:
        futures = [executor.submit(run_cmd, cmd) for cmd in commands]
        for future in as_completed(futures):
            if future.result() != 0:
                failed = True
    if failed:
        error("clang-tidy found issues. See logs above for more details")
        return False
    return True


def run_clang_format(apply_fixes: bool = False) -> bool:
    src_files = [str(f) for f in get_source_files()]
    info("Running clang-format")
    return_code = run_cmd(["clang-format", "--dry-run", "--Werror", *src_files])
    if return_code != 0:
        if apply_fixes:
            warn("clang-format found issues, attempting automatic fixes...")
            return_code = run_cmd(["clang-format", "-i", *src_files])
            if return_code != 0:
                error("Failed to apply clang-format fixes")
                return False
            warn("Formatting issues were found but have been automatically fixed")
            return True
    success("No formatting issues found")
    return True


def clean_build_dir() -> None:
    if BUILD_DIR.exists():
        warn(f"Removing {BUILD_DIR}")
        shutil.rmtree(BUILD_DIR)
    BUILD_DIR.mkdir(parents=True, exist_ok=True)


def configure_cmake(toolchain: Path, build_type: str = "Debug", tracy: bool = False, xcode: bool = False) -> bool:
    build_dir = XCODE_BUILD_DIR if xcode else BUILD_DIR
    label = f"Xcode project" if xcode else build_type
    configure(f"Running CMake ({label})")
    cmd = [
        "cmake",
        str(PROJECT_ROOT),
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        f"-DTRACY_ENABLE={'ON' if tracy else 'OFF'}",
    ]
    if xcode:
        cmd += ["-G", "Xcode"]
    return run_cmd(cmd, cwd=build_dir) == 0


def generate_xcode(toolchain: Path) -> bool:
    if not XCODE_BUILD_DIR.exists():
        XCODE_BUILD_DIR.mkdir(parents=True, exist_ok=True)
    if not compile_shaders():
        return False
    return configure_cmake(toolchain, xcode=True)


def build_target(target: str | None, tracy: bool = False) -> bool:
    if not BUILD_DIR.exists():
        BUILD_DIR.mkdir(parents=True, exist_ok=True)
    toolchain = get_vcpkg_toolchain()
    if not compile_shaders():
        return False
    if not configure_cmake(toolchain, tracy=tracy):
        return False
    cmd = ["cmake", "--build", "."]
    if target:
        cmd += ["--target", target]
        build_log(f"Building target {target}")
    else:
        build_log(f"Building all targets")
    return_code = run_cmd(cmd, cwd=BUILD_DIR)
    return return_code == 0


def run_tests() -> bool:
    if not BUILD_DIR.exists():
        error("Build directory does not exist. Run build first.")
        return False
    build_log("Running tests")
    return_code = run_cmd(["ctest", "--output-on-failure", "-V"], cwd=BUILD_DIR)
    return return_code == 0


# -----------------------------------------------------------------------------
# Shader Compilation
# -----------------------------------------------------------------------------
def compile_shaders() -> bool:
    """Compile all shaders found under PROJECT_ROOT"""
    shader_files = []
    for ext in ("*.vert", "*.frag", "*.comp"):
        shader_files.extend(PROJECT_ROOT.rglob(ext))

    for shader_file in shader_files:
        output_file = shader_file.parent / f"{shader_file.name}.spv"
        info(f"Compiling {shader_file.relative_to(PROJECT_ROOT)} -> {output_file.name}")
        return_code = run_cmd(["glslc", str(shader_file), "-o", str(output_file)])
        if return_code != 0:
            return False
    return True


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


def run_target(target: str, extra_args: list[str] | None = None) -> None:
    exe = find_executable(target)
    run_log(f"Running executable: {exe}")
    cmd = [str(exe)]
    if extra_args:
        cmd.extend(extra_args)
    run_cmd(cmd, cwd=exe.parent)


# -----------------------------------------------------------------------------
# Main Workflow
# -----------------------------------------------------------------------------
def main() -> None:
    parser = argparse.ArgumentParser(description="Build script for CrossPlatformVulkanEngine")
    parser.add_argument("target", type=str, help="Name of the executable target to build and run", nargs="?")
    parser.add_argument("--tidy", action="store_true", help="Run clang-tidy after building")
    parser.add_argument("--xcode", action="store_true", help="Generate Xcode project in build-xcode and build")
    parser.add_argument("--tracy", action="store_true", help="Build with Tracy profiler enabled")
    args, unknown = parser.parse_known_args()

    # Extract extra args after '--'
    extra_args = []
    if '--' in sys.argv:
        dash_idx = sys.argv.index('--')
        extra_args = sys.argv[dash_idx + 1:]

    if args.target == "clean":
        clean_build_dir()
        success("Clean completed.")
        return

    if args.xcode:
        toolchain = get_vcpkg_toolchain()
        if not generate_xcode(toolchain):
            error("Xcode project generation failed")
            sys.exit(1)
        build_log("Building via Xcode")
        cmd = ["cmake", "--build", str(XCODE_BUILD_DIR)]
        if args.target:
            cmd += ["--target", args.target]
        if run_cmd(cmd) != 0:
            error("Xcode build failed")
            sys.exit(1)
        success(f"Xcode build completed in {XCODE_BUILD_DIR}")
        return

    # ----------------------------------------
    # Build workflow
    # ----------------------------------------
    info("Starting build workflow")
    start_time = time.perf_counter()

    if not build_target(args.target, tracy=args.tracy):
        error("Build failed")
        sys.exit(1)

    run_clang_format(apply_fixes=True)

    if not args.tidy:
        warn("clang-tidy was skipped. Run with --tidy to run clang-tidy")
    else:
        run_clang_tidy()

    end_time = time.perf_counter()

    # ----------------------------------------
    # Build workflow summary
    # ----------------------------------------
    if not args.target:
        success(f"Successfully built all targets in {end_time - start_time:.2f}s")
    else:
        success(f"Successfully built target {args.target} in {end_time - start_time:.2f}s")

    if not run_tests():
        error("Tests failed")
        sys.exit(1)

    if args.target:
        run_target(args.target, extra_args)


if __name__ == "__main__":
    main()
