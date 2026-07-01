.. _deployment_tool:

Deployment Tool
===============

``deepcraft-voice-assistant-model-deploy.py`` is a generic utility that takes any
DEEPCRAFT™ Voice Assistant model — as a folder or a cloud-exported ``.zip`` — combines
it with the core firmware, compiles it, and produces a deployable ``.hex`` file
ready to be flashed to the board.

Currently the tool drives the CM55 firmware project: it installs model assets,
invokes the project's Makefile to build the hex, and flashes it to the device.
The design is not tied to a specific core or project — any firmware project with
a compatible Makefile structure could be driven by the same utility.

----

Installation
------------

Download the tool into a local folder with ``curl``:

.. code-block:: console

   curl -s -L -o deepcraft-voice-assistant-model-deploy.py https://raw.githubusercontent.com/Infineon/mtb-example-psoc-edge-voice-assistant-deploy-mpy/main/tools/deepcraft-voice-assistant-model-deploy.py

----

What the tool does
------------------

.. list-table::
   :widths: 20 80

   * - **Clone**
     - Fetches the CM55 firmware repository and all submodules. If already cloned,
       updates to the latest version.
   * - **Build**
     - Installs your model assets in the project, compiles the firmware with the
       correct model name, and produces ``.hex``.
   * - **Flash**
     - Programs the ``.hex`` onto the board via OpenOCD (auto-downloaded if not present).
   * - **All**
     - Runs clone → build → flash in a single command.
   * - **Clean**
     - Removes compiled model objects and link artifacts for a clean rebuild.

The tool is independent of what runs on the application host side. Any model generated
by the `DEEPCRAFT™ cloud tool <https://deepcraft.infineon.com/solutions/voice-assistant>`_
can be deployed with it.

Following are the full command reference, options, and config file documentation.

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

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``--llvm-dir PATH``
     - LLVM toolchain path. Overrides ``[tools] llvm_dir`` in the config file.
   * - ``--make-cmd CMD``
     - make executable to use (default: ``mingw32-make`` on Windows).
   * - ``-j N`` / ``--jobs N``
     - Parallel make jobs. Defaults to your CPU core count.
   * - ``--force``
     - Overwrite model assets that already exist in ``va_models/``.

flash
~~~~~

Program the board with the last built hex. Connect the KIT_PSE84_AI via USB (KitProg3) first.
OpenOCD is **auto-downloaded** if not already present.

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
     - Also delete that model's assets from the project.

show-models
~~~~~~~~~~~

List all available models in the project.

.. code-block:: powershell

   python deepcraft-voice-assistant-model-deploy.py --show-models
