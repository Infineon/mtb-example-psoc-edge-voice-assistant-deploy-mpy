# DEEPCRAFT Voice Assistant Model Deploy

A single Python script that takes a [DEEPCRAFT™ Voice Assistant](https://deepcraft.infineon.com/solutions/voice-assistant) model and gets it running on a **PSOC™ Edge KIT_PSE84_AI** board — firmware is built and flashed in one command.

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

## Documentation

| Guide | Contents |
|-------|----------|
| [Quick Start](mtb-example-psoc-edge-voice-assistant-deploy-mpy/docs/enablement_cm33_mpy_ipc.rst) | CM33 MicroPython over IPC — requirements, steps, example code |
| [Deployment Tool](mtb-example-psoc-edge-voice-assistant-deploy-mpy/docs/deployment_tool.rst) | Installation, commands, options, config file |
| [Requirements](mtb-example-psoc-edge-voice-assistant-deploy-mpy/docs/requirements.rst) | Hardware, toolchain, and software prerequisites |
| [MicroPython Interface](https://github.com/Infineon/micropython-deepcraft-model-interface) | CM33-side MicroPython API |

---

## Version

`deepcraft-voice-assistant-model-deploy.py` v1.0.0
