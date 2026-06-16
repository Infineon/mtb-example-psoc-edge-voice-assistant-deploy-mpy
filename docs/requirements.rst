.. _requirements:

Requirements
============

.. warning::
   The deployment tool (``deepcraft-voice-assistant-model-deploy.py``) is currently
   supported on **Windows only**. Linux and macOS have not been tested and hence may or may not work on these platforms.

.. contents::
   :local:
   :depth: 1

----

Hardware
--------
   - `PSOC™ Edge E84 AI Kit (KIT_PSE84_AI) <https://www.infineon.com/KIT_PSE84_AI>`_

----

DEEPCRAFT™ cloud tool
---------------------

A model must be created and exported from the
`DEEPCRAFT™ Voice Assistant cloud tool <https://deepcraft.infineon.com/solutions/voice-assistant>`_
before deployment. The tool generates ``.zip`` containing the model assets
that the deployment script installs onto the board.

No cloud tool account is needed if you are using one of the demo models already
bundled in the repository (e.g. ``test_gpio_control``).

----

Deployment tool
---------------

- **Python 3.8+** — verify with ``python --version``
- **requests** *(optional)* — enables auto-download of OpenOCD:

  .. code-block:: console

     pip install requests

  If not installed, download OpenOCD manually and set ``openocd_dir`` in the config file.

----

Build toolchain
---------------

**LLVM Embedded Toolchain for Arm**

The CM55 firmware must be compiled with the
`LLVM Embedded Toolchain for Arm v19.1.5 <https://github.com/ARM-software/LLVM-embedded-toolchain-for-Arm/releases/tag/release-19.1.5>`_.
This is the only supported compiler version and is required regardless of which
enablement you are using. If you are using the deployment tool, this is auto-downloaded if not already present, but can also be installed manually.

----

**GNU Make (MinGW)**

`MinGW <https://sourceforge.net/projects/mingw/>`_ provides ``mingw32-make``, which is required
to compile the firmware on Windows. After installation, add ``<MinGW>\\bin`` to your
system PATH so the tool is reachable from the command line. If you are using the deployment tool, this is auto-downloaded, but can also be installed manually.

----

Application host requirements
------------------------------

Requirements for the application host side depend on the enablement you are using.
Each enablement page lists its own prerequisites.

For the current enablement (CM33 running MicroPython over IPC), see respective section.

----

Middleware licence
------------------

The Audio Enhancement and Voice Assistant middleware shipped with this example
operate under a time-limited evaluation licence:

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Middleware
     - Time limit
   * - Audio Enhancement (AFE)
     - ~15 minutes
   * - Voice Assistant
     - ~30 minutes

For unlimited operation, contact `Infineon support <https://www.infineon.com/support>`_
to obtain a full licence.
