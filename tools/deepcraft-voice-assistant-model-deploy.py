#!/usr/bin/env python3
"""
deepcraft-voice-assistant-model-deploy.py  v0.2.0  --  Generic Makefile-driven
build/flash/deploy helper for DEEPCRAFT CM55 Voice-Assistant projects.

Subcommands
-----------
    build  MODEL_ASSETS_PATH  Install model assets, then run Make target(s)
    flash                    Run Make target(s) for flashing
    all    MODEL_ASSETS_PATH  Install model assets, then run Make target(s)
    clean                    Run Make target(s) for cleanup
  help               Show detailed usage information

MODEL_ASSETS_PATH
----------
  Path to the DEEPCRAFT model directory or a .zip file.
  Timestamped export names (e.g. test_gpio_control_14-04-2026_15042026_135817.zip)
  are supported -- the timestamp is stripped automatically.

Configuration is stored in deepcraft-voice-assistant-model-deploy.ini next to this script, or ~/.deepcraft-voice-assistant-model-deploy.ini.

This script intentionally delegates build/flash/clean behavior to Make targets
so it can be reused with other repositories that expose the same targets.
"""

import argparse
import configparser
import multiprocessing
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time
import zipfile

try:
    import requests as _requests
except ImportError:
    _requests = None

# ---------------------------------------------------------------------------
VERSION  = "0.2.0"
REPO_URL = "https://github.com/Infineon/mtb-example-psoc-edge-voice-assistant-deploy-mpy.git"
PROJ_CM55_REL  = "proj_cm55"
VA_MODELS_REL  = os.path.join(PROJ_CM55_REL, "va_models")
MAKEFILE       = "Makefile"
APPNAME        = "proj_cm55"
BUILD_DIR_BASE = "build"
_WIN = sys.platform in ("win32", "cygwin")
DEFAULT_MAKE_CMD    = "mingw32-make" if _WIN else "make"
_SCRIPT_DIR         = os.path.dirname(os.path.abspath(__file__))
_REPO_FOLDER_NAME   = "mtb-example-psoc-edge-voice-assistant-deploy-mpy"
DEFAULT_REPO_DIR    = os.path.join(_SCRIPT_DIR, _REPO_FOLDER_NAME)
_LOCAL_CONFIG_FILE  = os.path.join(_SCRIPT_DIR, "deepcraft-voice-assistant-model-deploy.ini")
DEFAULT_CONFIG_FILE = _LOCAL_CONFIG_FILE if os.path.isfile(_LOCAL_CONFIG_FILE) else os.path.join(os.path.expanduser("~"), ".deepcraft-voice-assistant-model-deploy.ini")
_OCD_DOWNLOAD_DIR   = os.path.join(_SCRIPT_DIR, "openocd")
_OCD_VERSION_TAG    = "release-v5.11.0"
_OCD_VERSION_STR    = "5.11.0.4042"
_OCD_BASE_NAME      = f"openocd-{_OCD_VERSION_STR}-"
_OCD_URL_BASE       = f"https://github.com/Infineon/openocd/releases/download/{_OCD_VERSION_TAG}/"
_OCD_SUPPORTED_VERS = ["0.12.0+dev-5.8.0.3960", "0.12.0+dev-5.11.0.4042", "0.12.0+dev-5.12.0.4170"]
_LLVM_VERSION = "19.1.5"
_LLVM_BASE_NAME = f"LLVM-ET-Arm-{_LLVM_VERSION}-Windows-x86_64"
_LLVM_DOWNLOAD_URL = f"https://github.com/ARM-software/LLVM-embedded-toolchain-for-Arm/releases/download/release-{_LLVM_VERSION}/{_LLVM_BASE_NAME}.zip"
_LLVM_INSTALL_DIR = os.path.join(os.path.expanduser("~"), "llvm")
# Well-known default install locations to auto-discover before prompting
_LLVM_SEARCH_PATHS = [
    os.path.join("C:\\", "llvm", _LLVM_BASE_NAME),
    os.path.join(os.path.expanduser("~"), "llvm", _LLVM_BASE_NAME),
    os.path.join(_SCRIPT_DIR, _LLVM_BASE_NAME),
]
def print_f(*a, **kw): print(*a, **kw, flush=True)

def _section(title):
    """Print a clear section header."""
    print_f(f"\n>> {title}")
    print_f("   " + "-" * 50)

def _fatal(msg):
    print_f(f"[dc-va] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)

def _prompt_yes_no(question):
    """Prompt user for yes/no and return True/False."""
    while True:
        resp = input(f"{question} (y/n): ").strip().lower()
        if resp in ("y", "yes"):
            return True
        elif resp in ("n", "no"):
            return False
        print("   Please answer 'y' or 'n'")

def _prompt_text(question):
    return input(f"{question}: ").strip().strip('"')

def _prompt_llvm_dir():
    raw = _prompt_text("Enter LLVM install path (for example C:\\llvm\\LLVM-ET-Arm-19.1.5-Windows-x86_64)")
    if not raw:
        return None
    candidate = os.path.abspath(raw)
    if os.path.basename(candidate).lower() == "bin":
        candidate = os.path.dirname(candidate)
    if os.path.isdir(candidate) and os.path.isdir(os.path.join(candidate, "bin")):
        return candidate
    return None

def _prompt_make_cmd_path(default_cmd):
    raw = _prompt_text(f"Enter full path to {default_cmd} (or a directory containing it)")
    if not raw:
        return None
    if os.path.isdir(raw):
        candidate = os.path.join(raw, f"{default_cmd}.exe" if _WIN else default_cmd)
        if os.path.isfile(candidate):
            return candidate
    if os.path.isfile(raw):
        return os.path.abspath(raw)
    found = shutil.which(raw)
    if found:
        return found
    return None

def _download_file(url, dest_path):
    """Download a file with progress feedback."""
    if _requests is None:
        _fatal("The 'requests' package is required to auto-download tools.\nInstall it with:  pip install requests")
    print_f(f"[dc-va] Downloading {os.path.basename(dest_path)} ...")
    print_f(f"         {url}")
    resp = _requests.get(url, stream=True)
    if resp.status_code != 200:
        _fatal(f"Download failed (HTTP {resp.status_code}): {url}")
    with open(dest_path, "wb") as fh:
        for chunk in resp.iter_content(chunk_size=1 << 16):
            fh.write(chunk)
    print_f("[dc-va] Download complete")

def _unique_install_dir(base_name):
    """Generate a unique install directory, avoiding collisions with locks/existing folders."""
    base_dir = os.path.join(os.path.expanduser("~"), base_name)
    if not os.path.exists(base_dir):
        return base_dir
    stamp = int(time.time())
    idx = 1
    while True:
        candidate = f"{base_dir}-auto-{stamp}-{idx}"
        if not os.path.exists(candidate):
            return candidate
        idx += 1

def _install_llvm():
    """Download, extract, and install LLVM 19.1.5 to ~/llvm/."""
    archive = os.path.join(_SCRIPT_DIR, f"{_LLVM_BASE_NAME}.zip")
    _download_file(_LLVM_DOWNLOAD_URL, archive)
    print_f("[dc-va] Extracting LLVM ...")
    os.makedirs(_LLVM_INSTALL_DIR, exist_ok=True)
    with zipfile.ZipFile(archive) as zf:
        zf.extractall(_LLVM_INSTALL_DIR)
    os.remove(archive)
    llvm_path = os.path.join(_LLVM_INSTALL_DIR, _LLVM_BASE_NAME)
    if not os.path.isdir(llvm_path):
        _fatal(f"LLVM extraction failed: expected folder not found at {llvm_path}")
    print_f(f"[dc-va] LLVM installed -> {llvm_path}")
    return llvm_path

def _install_mingw():
    """Download, extract, and install MinGW-w64 to a unique folder in ~/."""
    if not _WIN:
        _fatal("MinGW auto-installation is only supported on Windows")
    mingw_url = "https://github.com/brechtsanders/winlibs_mingw/releases/download/14.2.0posix-18.1.8-12.0.0-ucrt-r1/winlibs-x86_64-posix-seh-gcc-14.2.0-mingw-w64ucrt-12.0.0-r1.zip"
    print_f("[dc-va] Downloading MinGW-w64 14.2.0 (winlibs) ...")
    print_f("         This may take a few minutes (~300MB)")
    archive = os.path.join(_SCRIPT_DIR, "mingw-w64.zip")
    _download_file(mingw_url, archive)
    print_f("[dc-va] Extracting MinGW (this may take a few minutes) ...")
    try:
        with tempfile.TemporaryDirectory(dir=_SCRIPT_DIR, prefix="mingw_extract_") as extract_root:
            with zipfile.ZipFile(archive) as zf:
                members = zf.infolist()
                total = len(members)
                for i, member in enumerate(members, 1):
                    zf.extract(member, extract_root)
                    if i % 500 == 0 or i == total:
                        pct = int(i * 100 / total)
                        print_f(f"\r[dc-va] Extracting ... {pct}% ({i}/{total} files)", end="")
                print_f("")  # newline after progress
            extracted_path = os.path.join(extract_root, "mingw64")
            if not os.path.isdir(extracted_path):
                _fatal("MinGW extraction failed: expected 'mingw64' folder not found")
            # Use unique dir to avoid Windows lock contention
            target_dir = _unique_install_dir("mingw64")
            print_f(f"[dc-va] Installing MinGW into: {target_dir}")
            for attempt in range(1, 6):
                try:
                    shutil.move(extracted_path, target_dir)
                    break
                except PermissionError:
                    print_f(f"[dc-va] Move blocked by file lock; retrying ({attempt}/5)")
                    time.sleep(2)
                except (FileExistsError, shutil.Error):
                    target_dir = _unique_install_dir("mingw64")
                    print_f(f"[dc-va] Install directory collision; using fallback: {target_dir}")
            else:
                _fatal("MinGW installation failed: unable to finalize directory after retries")
        os.remove(archive)
    except Exception as e:
        _fatal(f"MinGW installation failed: {e}")
    # Handle nested layout: some zips extract to mingw64/mingw64/bin/...
    nested_root = os.path.join(target_dir, "mingw64")
    if not os.path.isfile(os.path.join(target_dir, "bin", "mingw32-make.exe")) and \
       os.path.isfile(os.path.join(nested_root, "bin", "mingw32-make.exe")):
        target_dir = nested_root
    mingw_make = os.path.join(target_dir, "bin", "mingw32-make.exe")
    if not os.path.isfile(mingw_make):
        _fatal(f"MinGW validation failed: mingw32-make not found at {mingw_make}")
    print_f(f"[dc-va] MinGW installed -> {target_dir}")
    return target_dir

# ---------------------------------------------------------------------------
def load_config(path):
    cfg = configparser.ConfigParser()
    if os.path.isfile(path):
        cfg.read(path)
    return cfg

def cfg_get(cfg, section, key, fallback=None):
    try:
        return cfg.get(section, key) or fallback
    except (configparser.NoSectionError, configparser.NoOptionError):
        return fallback

def ensure_llvm(cfg, config_path, cli_override=None):
    # CLI flag or config file takes priority — no prompts needed
    if cli_override:
        return cli_override
    value = cfg_get(cfg, "tools", "llvm_dir") or os.environ.get("LLVM_DIR")
    if value and os.path.isdir(value):
        return value

    # Always ask the user first
    if _prompt_yes_no(f"   Have you already manually installed LLVM Embedded Toolchain for Arm {_LLVM_VERSION}?"):
        # Try well-known locations silently first, then ask for path
        for candidate in _LLVM_SEARCH_PATHS:
            if os.path.isdir(candidate):
                print_f(f"   ✓ Great! Found LLVM at: {candidate}")
                return candidate
        # Not found in known locations — ask user to provide path
        print_f("   LLVM not found in default locations. Please provide the install path.")
        manual_llvm = _prompt_llvm_dir()
        if manual_llvm:
            print_f(f"   ✓ Got it! Using LLVM path: {manual_llvm}")
            return manual_llvm
        print_f("   Provided path is invalid or missing a bin/ subfolder.")
        if not _prompt_yes_no("   Would you like me to install LLVM for you?"):
            _fatal(
                "LLVM Embedded Toolchain for Arm not found.\n"
                "  Set the LLVM_DIR environment variable, or add\n"
                "  [tools] llvm_dir = <path>  in deepcraft-voice-assistant-model-deploy.ini."
            )
    else:
        print_f("   Got it! Sit tight — fetching and installing LLVM for you...")

    llvm_path = _install_llvm()
    print_f(f"   ✓ LLVM is ready!")
    return llvm_path

# ---------------------------------------------------------------------------
def _ocd_exe_name():
    return "openocd.exe" if _WIN else "openocd"

def _ocd_local_bin():
    return os.path.join(_OCD_DOWNLOAD_DIR, "bin", _ocd_exe_name())

def _ocd_in_path():
    exe = shutil.which(_ocd_exe_name())
    if not exe:
        return None
    try:
        r = subprocess.run([exe, "--version"], capture_output=True, timeout=10)
        out = (r.stdout + r.stderr).decode(errors="replace")
        if any(v in out for v in _OCD_SUPPORTED_VERS):
            return exe
    except Exception:
        pass
    return None

def _ocd_download_and_install():
    if _requests is None:
        _fatal("The 'requests' package is required to auto-download OpenOCD.\nInstall it with:  pip install requests\nOr set [tools] openocd = <path>  in your config file.")
    if _WIN:
        suffix, ext = "windows", ".zip"
    elif sys.platform == "darwin":
        suffix, ext = "macos", ".zip"
    else:
        suffix, ext = "linux", ".tar.gz"
    file_name = _OCD_BASE_NAME + suffix + ext
    url       = _OCD_URL_BASE + file_name
    archive   = os.path.join(_SCRIPT_DIR, file_name)
    print_f(f"[dc-va] Downloading OpenOCD {_OCD_VERSION_STR} ...")
    print_f(f"         {url}")
    resp = _requests.get(url, stream=True)
    if resp.status_code != 200:
        _fatal(f"OpenOCD download failed (HTTP {resp.status_code}): {url}")
    with open(archive, "wb") as fh:
        for chunk in resp.iter_content(chunk_size=1 << 16):
            fh.write(chunk)
    print_f("[dc-va] Extracting OpenOCD ...")
    if ext == ".tar.gz":
        with tarfile.open(archive) as tf:
            tf.extractall(_SCRIPT_DIR)
    else:
        with zipfile.ZipFile(archive) as zf:
            zf.extractall(_SCRIPT_DIR)
    os.remove(archive)
    exe = _ocd_local_bin()
    if not os.path.isfile(exe):
        _fatal(f"OpenOCD extraction succeeded but binary not found at: {exe}")
    if sys.platform == "darwin":
        os.chmod(exe, 0o755)
    bin_dir = os.path.dirname(exe)
    os.environ["PATH"] = bin_dir + os.pathsep + os.environ["PATH"]
    print_f(f"[dc-va] OpenOCD installed -> {exe}")
    return exe

def ensure_openocd(cfg, config_path, cli_override=None):
    explicit = cli_override or cfg_get(cfg, "tools", "openocd")
    if explicit and os.path.isfile(explicit):
        return explicit
    local = _ocd_local_bin()
    if os.path.isfile(local):
        bin_dir = os.path.dirname(local)
        os.environ["PATH"] = bin_dir + os.pathsep + os.environ["PATH"]
        print_f(f"[dc-va] Using local OpenOCD: {local}")
        return local
    sys_ocd = _ocd_in_path()
    if sys_ocd:
        print_f(f"[dc-va] Using system OpenOCD: {sys_ocd}")
        return sys_ocd
    return _ocd_download_and_install()

def get_make_cmd(cfg, cli_override=None):
    # CLI flag or config file takes priority — no prompts needed
    cmd = cli_override or cfg_get(cfg, "tools", "make_cmd", DEFAULT_MAKE_CMD)
    configured_explicitly = bool(cli_override or cfg_get(cfg, "tools", "make_cmd"))
    if configured_explicitly and shutil.which(cmd):
        return cmd

    # Always ask the user first
    if _prompt_yes_no(f"   Have you already manually installed {DEFAULT_MAKE_CMD}?"):
        # Try PATH first
        found = shutil.which(cmd)
        if found:
            print_f(f"   ✓ Great! Found {DEFAULT_MAKE_CMD} in PATH")
            return found
        # Not found in PATH — ask user for path
        print_f(f"   {DEFAULT_MAKE_CMD} not found in PATH. Please provide the install path.")
        manual_make = _prompt_make_cmd_path(DEFAULT_MAKE_CMD)
        if manual_make:
            print_f(f"   ✓ Got it! Using: {manual_make}")
            return manual_make
        print_f("   Provided path is invalid or not found.")
        if not _prompt_yes_no("   Would you like me to install MinGW and mingw32-make for you?"):
            _fatal(f"'{cmd}' not found. Please install MinGW or set [tools] make_cmd in your config file.")
    else:
        print_f("   Got it! Sit tight — fetching and installing MinGW for you...")

    if _WIN:
        mingw_dir = _install_mingw()
        os.environ["PATH"] = os.path.join(mingw_dir, "bin") + os.pathsep + os.environ["PATH"]
        installed_cmd = os.path.join(mingw_dir, "bin", f"{DEFAULT_MAKE_CMD}.exe")
        print_f(f"   ✓ MinGW is ready!")
        return installed_cmd

    _fatal(
        f"'{cmd}' not found in PATH.\n"
        f"  Install MinGW (https://www.mingw-w64.org/) and add its bin\\ folder\n"
        f"  to your system PATH environment variable, then retry.\n"
        f"  Or override with [tools] make_cmd = <full path>  in your config file."
    )



def get_make_targets(cfg, command):
    """Return one or more make targets (comma-separated values are supported)."""
    # Defaults preserve current repository behavior while remaining configurable
    # for other repositories with their own target naming.
    defaults = {
        "build": "all",
        "flash": "deploy",
        "clean": "clean",
        "all": "all,deploy",
    }
    value = cfg_get(cfg, "make_targets", command, defaults[command])
    targets = [t.strip() for t in str(value).split(",") if t.strip()]
    if not targets:
        _fatal(f"No make target configured for command '{command}'")
    return targets


def get_make_var_names(cfg):
    return {
        "project": cfg_get(cfg, "make_variables", "project", "DEEPCRAFT_PROJECT"),
        "config": cfg_get(cfg, "make_variables", "config", "CONFIG"),
        "llvm": cfg_get(cfg, "make_variables", "llvm", "LLVM_DIR"),
        "serial": cfg_get(cfg, "make_variables", "serial", "DEV_SERIAL_NUMBER"),
        "openocd": cfg_get(cfg, "make_variables", "openocd", "OPENOCD"),
    }


def run_make_target(*, proj_cm55_dir, make_cmd, target, llvm_dir=None, jobs=None, var_map=None):
    cmd = [make_cmd, "-f", MAKEFILE, target]
    if jobs:
        cmd += ["-j", str(jobs)]
    if var_map:
        for key, value in var_map.items():
            if value is not None and str(value) != "":
                cmd.append(f"{key}={value}")
    _run(cmd, cwd=proj_cm55_dir, env=_build_env(llvm_dir))

# ---------------------------------------------------------------------------
def _git(*args, cwd=None):
    _run(["git"] + list(args), cwd=cwd)

def _git_quiet(*args, cwd=None):
    """Execute git command silently, capturing output instead of printing."""
    try:
        r = subprocess.run(["git"] + list(args), cwd=cwd, capture_output=True, text=True)
        if r.returncode != 0:
            _fatal(f"Git command failed (exit {r.returncode})\n{r.stderr}")
    except FileNotFoundError:
        _fatal("git executable not found")
    except KeyboardInterrupt:
        print_f("\n[dc-va] Interrupted by user", file=sys.stderr)
        sys.exit(1)

def _ensure_git_longpaths():
    if not _WIN:
        return
    try:
        r = subprocess.run(["git", "config", "--global", "core.longpaths"], capture_output=True, text=True)
        if r.stdout.strip() == "true":
            return
    except Exception:
        pass
    print_f("[dc-va] Enabling git core.longpaths = true (Windows long-path support)")
    _run(["git", "config", "--global", "core.longpaths", "true"])

def clone_or_update(repo_url, dest):
    """Clone the repo if missing; otherwise refresh it to match the remote.
    """
    #ToDo: Should be main once merged.
    branch = "en-automation-py-script"
    _ensure_git_longpaths()
    if os.path.isdir(os.path.join(dest, ".git")):
        _git_quiet("fetch", "origin", branch, cwd=dest)
        _git_quiet("checkout", branch, cwd=dest)
        _git_quiet("reset", "--hard", f"origin/{branch}", cwd=dest)
    else:
        parent = os.path.dirname(os.path.abspath(dest))
        os.makedirs(parent, exist_ok=True)
        _git_quiet("clone", "--branch", branch, "--single-branch", "--recurse-submodules", repo_url, dest)

# ---------------------------------------------------------------------------
_TIMESTAMP_RE = re.compile(r'_\d{2}-\d{2}-\d{4}_\d{8}_\d{6}$')

def _strip_timestamp(name):
    return _TIMESTAMP_RE.sub('', name)

def _project_name(model_path):
    base = os.path.basename(model_path.rstrip("/\\"))
    return base[:-4] if base.lower().endswith(".zip") else base

def _zip_top_dir(zip_path):
    with zipfile.ZipFile(zip_path) as zf:
        tops = {n.split("/")[0] for n in zf.namelist() if n.split("/")[0]}
    return tops.pop() if len(tops) == 1 else None

def install_model(model_path, va_models_dir, force=False):
    model_path = os.path.abspath(model_path)
    if not os.path.exists(model_path):
        _fatal(f"Model path does not exist: {model_path}")
    is_zip = os.path.isfile(model_path) and zipfile.is_zipfile(model_path)
    raw_name = (_zip_top_dir(model_path) or _project_name(model_path)) if is_zip else _project_name(model_path)
    project_name = _strip_timestamp(raw_name)
    dest = os.path.join(va_models_dir, project_name)
    print_f(f"   Installing model '{project_name}'...")
    if os.path.exists(dest):
        if not force:
            print_f(f"   ✓ Model '{project_name}' is already in place")
            return project_name
        shutil.rmtree(dest)
    if is_zip:
        with tempfile.TemporaryDirectory() as tmp:
            with zipfile.ZipFile(model_path) as zf:
                zf.extractall(tmp)
            src = os.path.join(tmp, raw_name)
            if not os.path.isdir(src):
                src = os.path.join(tmp, project_name)
            if not os.path.isdir(src):
                src = tmp
            shutil.copytree(src, dest)
    else:
        shutil.copytree(model_path, dest)
    print_f(f"   ✓ Model is ready!")
    return project_name

def _build_env(llvm_dir=None):
    env = os.environ.copy()
    if llvm_dir:
        llvm_bin = os.path.join(llvm_dir, "bin")
        if os.path.isdir(llvm_bin):
            env["PATH"] = llvm_bin + os.pathsep + env["PATH"]
    return env

def _run(cmd, cwd=None, env=None):
    print_f(f"[dc-va] $ {' '.join(str(c) for c in cmd)}")
    try:
        r = subprocess.run(cmd, cwd=cwd, env=env)
    except FileNotFoundError:
        _fatal(
            f"Executable not found: '{cmd[0]}'\n"
            f"On Windows, set [tools] make_cmd = <path>  in your config file."
        )
    except KeyboardInterrupt:
        print_f("\n[dc-va] Interrupted by user", file=sys.stderr)
        sys.exit(1)
    if r.returncode != 0:
        _fatal(f"Command failed (exit {r.returncode})")

def print_help(parser=None):
    print_f(f"""
deepcraft-voice-assistant-model-deploy.py  v{VERSION}
One-stop helper to build and flash DEEPCRAFT CM55 Voice-Assistant firmware
with your own model. It fetches the firmware sources, sets up the required
toolchain, drops your model in, and drives the Makefile build/flash for you.

USAGE
-----
  python deepcraft-voice-assistant-model-deploy.py [--config-file PATH] COMMAND [options]

COMMANDS
--------
  build  MODEL_ASSETS_PATH  [options]
      Fetch sources, install your model, and compile the firmware.

      Required:
        MODEL_ASSETS_PATH        Path to the DEEPCRAFT model folder or .zip file.
                                 Timestamped export names are handled for you.

      Options:
        --llvm-dir  PATH         LLVM toolchain dir (skips the setup prompt)
        --make-cmd  CMD          GNU make executable (default: mingw32-make)
        -j / --jobs N            Parallel make jobs (default: CPU count)
        --force                  Re-install the model even if it is already there

  flash  [options]
      Flash the already-built firmware to a connected board.

      Options:
        --make-cmd  CMD          GNU make executable (default: mingw32-make)
        --openocd   PATH         OpenOCD executable (auto-downloaded if missing)
        --serial    SN           KitProg3 adapter serial (needed with multiple boards)

  all    MODEL_ASSETS_PATH  [options]
      Do everything: fetch sources, install your model, build, then flash.

      Required:
        MODEL_ASSETS_PATH        Path to the DEEPCRAFT model folder or .zip file.

      Options:
        --llvm-dir  PATH         LLVM toolchain dir (skips the setup prompt)
        --make-cmd  CMD          GNU make executable (default: mingw32-make)
        -j / --jobs N            Parallel make jobs (default: CPU count)
        --force                  Re-install the model even if it is already there
        --openocd   PATH         OpenOCD executable (auto-downloaded if missing)
        --serial    SN           KitProg3 adapter serial (needed with multiple boards)

  clean  [options]
      Remove build artifacts.

      Options:
        --make-cmd  CMD          GNU make executable (default: mingw32-make)

  help
      Show this help text.

GLOBAL OPTIONS
--------------
  --config-file PATH   Optional config file (default: deepcraft-voice-assistant-model-deploy.ini next to script)
  --version            Print version and exit

CONFIG FILE  (optional -- most users never need this)
-----------------------------------------------------
  By default the script discovers/installs tools and uses sensible Make
  targets, so no config is required. Create deepcraft-voice-assistant-model-deploy.ini
  next to the script only if you want to pin tool paths or customize targets:

  [tools]
  llvm_dir      = C:\\llvm\\LLVM-ET-Arm-{_LLVM_VERSION}-Windows-x86_64
  make_cmd      = mingw32-make
  openocd       = C:\\path\\to\\openocd.exe

  [make_targets]            ; override only if your repo uses different names
  build         = all
  flash         = deploy
  clean         = clean
  all           = all,deploy

  [board]
  serial_number = KitProg3 serial to flash (set this when multiple boards are connected)

ENVIRONMENT VARIABLES
---------------------
  LLVM_DIR    Path to the LLVM Embedded Toolchain for Arm directory.
              Used when [tools] llvm_dir is not set and --llvm-dir is not passed.
""")


def _parser():
    p = argparse.ArgumentParser(prog="deepcraft-voice-assistant-model-deploy.py", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--version", action="version", version=f"deepcraft-voice-assistant-model-deploy {VERSION}")
    p.add_argument("--config-file", default=DEFAULT_CONFIG_FILE, metavar="PATH",
        help=f"Config file path (default: {DEFAULT_CONFIG_FILE})")
    sub = p.add_subparsers(dest="command", metavar="COMMAND")
    sub.required = True

    def _build_args(sp):
        sp.add_argument("--llvm-dir", metavar="PATH",
            help="LLVM Embedded Toolchain for Arm directory")
        sp.add_argument("--make-cmd", metavar="CMD",
            help="GNU make executable (default: make)")
        sp.add_argument("-j", "--jobs", type=int, metavar="N",
            help="Parallel make jobs")

    def _flash_args(sp):
        sp.add_argument("--openocd", metavar="PATH",
            help="OpenOCD executable path (passed to make variable OPENOCD by default)")
        sp.add_argument("--serial", metavar="SN",
            help="KitProg3 adapter serial number")

    h = sub.add_parser("help", help="Show detailed usage information")
    h.set_defaults(command="help")

    cl = sub.add_parser("clean", help="Run configured make clean target(s)")
    cl.add_argument("--make-cmd", metavar="CMD",
        help="GNU make executable (default: make)")

    b = sub.add_parser("build", help="Install model assets and build")
    b.add_argument("MODEL_ASSETS_PATH", metavar="MODEL_ASSETS_PATH",
        help="Path to model directory or .zip file")
    b.add_argument("--force", action="store_true",
        help="Overwrite model assets if they already exist in va_models/")
    _build_args(b)

    f = sub.add_parser("flash", help="Run configured make flash target(s)")
    f.add_argument("--make-cmd", metavar="CMD",
        help="GNU make executable (default: make)")
    _flash_args(f)

    a = sub.add_parser("all", help="Install model, then run configured make target(s)")
    a.add_argument("MODEL_ASSETS_PATH", metavar="MODEL_ASSETS_PATH",
        help="Path to model directory or .zip file")
    a.add_argument("--force", action="store_true",
        help="Overwrite model assets if they already exist in va_models/")
    _build_args(a)
    _flash_args(a)

    return p

# ---------------------------------------------------------------------------
def main():
    parser = _parser()

    if len(sys.argv) == 1:
        print_help(parser)
        sys.exit(0)

    args = parser.parse_args()

    if args.command == "help":
        print_help(parser)
        return

    cfg = load_config(args.config_file)

    repo_dir = DEFAULT_REPO_DIR
    build_config = cfg_get(cfg, "project", "build_config") or "Debug"
    serial    = getattr(args, "serial",    None) or cfg_get(cfg, "board", "serial_number")
    jobs      = getattr(args, "jobs", None) or multiprocessing.cpu_count()
    force     = getattr(args, "force",     False)

    proj_cm55_dir = os.path.join(repo_dir, PROJ_CM55_REL)
    va_models_dir = os.path.join(repo_dir, VA_MODELS_REL)

    if args.command in ("build", "all"):
        _section("Getting the latest sources")
        clone_or_update(REPO_URL, repo_dir)
        print_f("   ✓ Sources are ready!")

    _section("Let's set up required tools")
    make_cmd = get_make_cmd(cfg, getattr(args, "make_cmd", None))
    var_names = get_make_var_names(cfg)
    llvm_dir = None
    project_name = None

    if args.command in ("build", "all"):
        llvm_dir = ensure_llvm(cfg, args.config_file, getattr(args, "llvm_dir", None))
        _section("Installing model")
        project_name = install_model(args.MODEL_ASSETS_PATH, va_models_dir, force=force)

    openocd = None
    if args.command in ("flash", "all"):
        openocd = ensure_openocd(cfg, args.config_file, getattr(args, "openocd", None))

    make_vars = {
        var_names["config"]: build_config,
    }
    if llvm_dir:
        make_vars[var_names["llvm"]] = llvm_dir
    if project_name:
        make_vars[var_names["project"]] = project_name
    if serial:
        make_vars[var_names["serial"]] = serial
    if openocd:
        make_vars[var_names["openocd"]] = openocd

    targets = get_make_targets(cfg, args.command)
    _section("Final step... Building and flashing to your device!")
    for target in targets:
        run_make_target(
            proj_cm55_dir=proj_cm55_dir,
            make_cmd=make_cmd,
            target=target,
            llvm_dir=llvm_dir,
            jobs=jobs if args.command in ("build", "all") else None,
            var_map=make_vars,
        )
    print_f("\n   ✓ All done! Your build is ready to roll.")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print_f("\n[dc-va] Interrupted by user", file=sys.stderr)
        sys.exit(1)