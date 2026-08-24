.. _user_guide:

Script Reference
================

Complete reference for ``deepcraft-voice-assistant-model-deploy.py``.

.. contents:: On this page
   :local:
   :depth: 2

----

Invocation
----------

.. code-block:: console

   python deepcraft-voice-assistant-model-deploy.py [--config-file PATH] COMMAND [options]

Pass ``help`` as the command to print the full built-in help text:

.. code-block:: console

   python deepcraft-voice-assistant-model-deploy.py help

----

Commands
--------

build
~~~~~

Install a model and compile the firmware. The repository is cloned or updated automatically
before building.

.. code-block:: powershell

   python deepcraft-voice-assistant-model-deploy.py \
       --config-file deepcraft-voice-assistant-model-deploy.ini \
       build path\to\my_model

The model argument can be:

* a **folder** on disk (e.g. the ``test_gpio_control`` folder shipped in this repo)
* a ``.zip`` file exported from the `DEEPCRAFT™ cloud tool <https://deepcraft.infineon.com/solutions/voice-assistant>`_
  — timestamped names are handled automatically

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``--llvm-dir PATH``
     - LLVM toolchain path. Overrides ``[tools] llvm_dir`` in the config file.
   * - ``--make-cmd CMD``
     - make executable to use (default: ``mingw32-make`` on Windows, ``make`` on Linux/macOS).
   * - ``-j N`` / ``--jobs N``
     - Parallel make jobs. Defaults to your CPU core count.
   * - ``--force``
    - Overwrite model assets that already exist in ``framework/deepcraft/va_models/``.

Output hex: ``proj_cm55/build/<build_config>/proj_cm55.hex``

flash
~~~~~

Program the board with the last built hex. Connect the KIT_PSE84_AI via USB (KitProg3) first.
OpenOCD (Infineon fork v5.11.0) is **auto-downloaded** if not already present.

.. code-block:: powershell

   python deepcraft-voice-assistant-model-deploy.py \
       --config-file deepcraft-voice-assistant-model-deploy.ini flash

When multiple boards are connected at the same time, target one by serial number:

.. code-block:: powershell

   python deepcraft-voice-assistant-model-deploy.py \
       --config-file deepcraft-voice-assistant-model-deploy.ini \
       flash --serial 0D161698012D2400

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``--openocd PATH``
     - Path to an OpenOCD binary. Skips auto-download.
   * - ``--serial SN``
     - KitProg3 adapter serial number.

all
~~~

The most common entry point. Clones or updates the repo, builds, then flashes — all in one go:

.. code-block:: powershell

   python deepcraft-voice-assistant-model-deploy.py \
       --config-file deepcraft-voice-assistant-model-deploy.ini \
       all path\to\my_model

Accepts every option from both ``build`` and ``flash``.

clean
~~~~~

Remove compiled model objects and link artifacts so the next ``build`` starts fresh for that
model. BSP and middleware objects are kept, so incremental rebuilds stay fast.

.. code-block:: powershell

   python deepcraft-voice-assistant-model-deploy.py clean

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``--all``
     - Remove the entire build directory, including all cached objects.
   * - ``--model MODEL``
    - Also delete that model's assets from ``framework/deepcraft/va_models/``.

----

Config file
-----------

The script looks for ``deepcraft-voice-assistant-model-deploy.ini`` next to itself first,
then falls back to ``~/.deepcraft-voice-assistant-model-deploy.ini``.
Use ``--config-file PATH`` to point to a different location.

.. code-block:: ini

   [tools]
   llvm_dir  = C:/llvm/LLVM-ET-Arm-19.1.5-Windows-x86_64
   # openocd  = C:/path/to/openocd.exe   ; skip auto-download
   # make_cmd = mingw32-make             ; auto-detected if omitted

   [project]
   build_config = Debug                  ; or Release

   [board]
   # serial_number = 0D161698012D2400    ; only needed with multiple boards

Command-line options always take precedence over the config file.
The ``LLVM_DIR`` environment variable is also accepted as a fallback for ``llvm_dir``.
