.. _versions:

Versions & Compatibility
========================

The table below lists the middleware and library versions the project was built and tested with. The deployment tool and toolchain versions are also listed for reference.

.. list-table::
   :header-rows: 1
   :widths: 40 30 30

   * - Component
     - Version
     - Notes
   * - Voice Assistant
     - 2.1.0.76
     - ~30 min time limit (evaluation). See :ref:`requirements` for licence details.
   * - Audio Front-End (AFE)
     - 1.0.1.362
     - ~15 min time limit (evaluation). See :ref:`requirements` for licence details.
   * - Audio Voice Core
     - 2.0.0.563
     -
   * - ML Middleware
     - 3.3.0.16533
     -
   * - ML TFLite Micro
     - 3.3.0.16533
     -
   * - Speech Onset Detection
     - 1.0.1.213
     -
   * - CMSIS
     - 6.1.0.171
     -
   * - PSE8XXGP Device Support (mtb-dsl-pse8xxgp)
     - 1.3.0.950
     -

Toolchain
---------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Tool
     - Required version
   * - LLVM Embedded Toolchain for Arm
     - 19.1.5 (only tested version)
   * - OpenOCD (Infineon fork)
     - 5.11.0.4042 (auto-downloaded by the deployment tool)
   * - Python
     - 3.8 or later

Deployment tool
---------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Tool
     - Version
   * - ``deepcraft-voice-assistant-model-deploy.py``
     - 0.1.0
