.. _troubleshooting:

Troubleshooting
===============

.. list-table::
   :header-rows: 1
   :widths: 45 55

   * - Symptom
     - Fix
   * - ``LLVM Embedded Toolchain for Arm not found``
     - Set ``llvm_dir`` in the config file or export ``LLVM_DIR``.
   * - ``make: command not found`` / ``mingw32-make not found``
     - Install MinGW-w64 and add ``<MinGW>\bin`` to PATH, or set ``make_cmd`` in config.
   * - ``The filename or extension is too long`` (linker)
     - Response-file fix applied in the Makefile — reapply if the Makefile is reset by git.
   * - ``couldn't open C:Users...`` in OpenOCD
     - Backslash path passed to Tcl — fixed in the script; update to the latest version.
   * - Verify fails after flash
     - Expected — SRAM sections cannot be verified. The flash itself was successful.
   * - Build is very slow
     - The ``-j`` flag defaults to your CPU core count. Check that MinGW is not adding overhead;
       try ``--make-cmd make`` if you have a native make available.
