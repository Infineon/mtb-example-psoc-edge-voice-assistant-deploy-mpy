# MicroPython PSOC™ Edge AI Model

This repo hosts MicroPython-based deployment enablements for running embedded AI models on **PSOC™ Edge** boards. It currently supports [DEEPCRAFT™ Voice Assistant](https://deepcraft.infineon.com/solutions/voice-assistant) models.

## DEEPCRAFT™ Voice Assistant

A single Python script that takes a DEEPCRAFT™ Voice Assistant model and gets it running on a **PSOC™ Edge KIT_PSE84_AI** board — firmware is built and flashed in one command.

---

## Setup

Set your LLVM toolchain path in `deepcraft-voice-assistant-model-deploy.ini`:

```ini
[tools]
llvm_dir = C:/llvm/LLVM-ET-Arm-19.1.5-Windows-x86_64
```

That's the only required config. OpenOCD is downloaded automatically if not found.

---

## Deploy a model

```powershell
python deepcraft-voice-assistant-model-deploy.py \
    --config-file deepcraft-voice-assistant-model-deploy.ini \
    all path\to\your_model
```

`all` clones the firmware repo, builds with your model, and flashes to the board.
The model can be a folder or a `.zip` exported from the [DEEPCRAFT™ cloud tool](https://deepcraft.infineon.com/solutions/voice-assistant).

Other commands:

| Command | Description |
|---------|-------------|
| `build <MODEL>` | Build only (also clones/updates the repo) |
| `flash` | Flash the last built `.hex` |
| `clean` | Remove build artifacts for a clean rebuild |

---

## Build Firmware

Choose one of the following workflows.

### Option 1: Clone On The Host

Use this workflow when Git is installed on the host. Clone the firmware and
open its root directory before running Docker.

```sh
git clone --recurse-submodules https://github.com/Infineon/mtb-example-psoc-edge-voice-assistant-deploy-mpy.git

cd mtb-example-psoc-edge-voice-assistant-deploy-mpy
```

From this directory, which contains `cm55_firmware/`, run the following
commands. Docker mounts the checkout at `/workspace`; `-w` selects the
project's build directory inside it.

WSL with docker installed:

```sh
docker run --rm \
	-v "$PWD":/workspace \
	-w /workspace/cm55_firmware \
	ifxmakers/psoc-embedded-ai-toolchain:deepcraft-0.1.0 \
	make CONFIG=Debug FRAMEWORK=deepcraft
```

### Option 2: Clone Inside Docker

```sh
mkdir firmware-build

docker run --rm \
	-v "$PWD/firmware-build":/workspace \
	--entrypoint sh \
	ifxmakers/psoc-embedded-ai-toolchain:deepcraft-0.1.0 \
	-c 'git clone --recurse-submodules https://github.com/Infineon/mtb-example-psoc-edge-voice-assistant-deploy-mpy.git . &&
	     cd cm55_firmware &&
	     make CONFIG=Debug FRAMEWORK=deepcraft'
```

The checkout and build outputs remain in `firmware-build` after the container
exits.

## Flash Firmware

The image includes Infineon's OpenOCD fork, which supports the KitProg3
interface and PSE84 target configs.

Flashing requires USB access to the board's debug probe, which build commands
do not need. On Linux, pass the USB bus through explicitly:

```sh
docker run --rm \
	--device=/dev/bus/usb \
	-v "$PWD":/workspace \
	-w /workspace/cm55_firmware \
	ifxmakers/psoc-embedded-ai-toolchain:deepcraft-0.1.0 \
	make CONFIG=Debug FRAMEWORK=deepcraft flash
```

On WSL, attach the debug probe to the WSL distribution first using
[usbipd-win](https://github.com/dorssel/usbipd-win) before running the command
above. Direct USB passthrough to Windows Docker Desktop containers is not
generally supported.

---

## Documentation

| Guide | Contents |
|-------|----------|
| [Quick Start](https://mpy-va-deploy.readthedocs.io/en/latest/enablement_cm33_mpy_ipc.html#quickstart) | CM33 MicroPython over IPC — requirements, steps, example code |
| [Deployment Tool](https://mpy-va-deploy.readthedocs.io/en/latest/deployment_tool.html) | Installation, commands, options, config file |
| [Requirements](https://mpy-va-deploy.readthedocs.io/en/latest/requirements.html) | Hardware, toolchain, and software prerequisites |
| [MicroPython Interface](https://github.com/Infineon/micropython-deepcraft-model-interface) | CM33-side MicroPython API |

---

## Version

`deepcraft-voice-assistant-model-deploy.py` v1.0.0
