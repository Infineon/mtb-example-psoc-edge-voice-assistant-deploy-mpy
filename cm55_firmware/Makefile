################################################################################
# Standalone Makefile for CM55 Voice Assistant project
# (No ModusToolbox / MTB workflow required)
#
# Usage:
#   make                    # build (Debug)
#   make CONFIG=Release
#   make clean
#   make deploy             # flash via OpenOCD
#
# Prerequisites:
#   - LLVM Embedded Toolchain for Arm 19.1.5 installed and added to path (example: export PATH=/opt/Tools/LLVM-ET-Arm-19.1.5-Linux-x86_64/bin:$PATH)
#   - All library submodules present (deps/psoc-edge/ populated)
#   - cy_afe_configurator_settings.c generated into sources/bsp-cfg/
#     (copy from sources/afe/GeneratedSource/ if the AFE configurator has been run)
################################################################################

################################################################################
# Project settings
################################################################################

APPNAME          ?= cm55_firmware
CONFIG           ?= Debug
BOARD            ?= KIT_PSE84_AI
DEEPCRAFT_PROJECT ?= test_gpio_control

# Directory containing this Makefile (cm55_firmware/)
BOARD_DIR        := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

# Build output directory
BUILD_DIR := build/$(CONFIG)

################################################################################
# Cross-platform setup
################################################################################

RM_RF := $(if $(filter $(OS),Windows_NT),cmd /c rd /s /q,rm -rf)

ifeq ($(OS),Windows_NT)
    _IS_WSL := $(wildcard /proc/version)
    ifneq ($(_IS_WSL),)
        SHELL := /bin/sh
        ifneq (,$(findstring :,$(LLVM_DIR)))
            override undefine LLVM_DIR
        endif
    endif
else
    SHELL := /bin/sh
endif

################################################################################
# Toolchain
################################################################################

CC      := clang
CXX     := clang++
AS      := clang
LD      := clang
AR      := llvm-ar
OBJCOPY := llvm-objcopy
SIZE    := llvm-size
OPENOCD := openocd

ifdef LLVM_DIR
    CC      := $(LLVM_DIR)/bin/clang
    CXX     := $(LLVM_DIR)/bin/clang++
    AS      := $(LLVM_DIR)/bin/clang
    LD      := $(LLVM_DIR)/bin/clang
    AR      := $(LLVM_DIR)/bin/llvm-ar
    OBJCOPY := $(LLVM_DIR)/bin/llvm-objcopy
    SIZE    := $(LLVM_DIR)/bin/llvm-size
    export PATH := $(subst \,/,$(LLVM_DIR))/bin:$(PATH)
endif

################################################################################
# Paths derived from WORKSPACE_DIR
################################################################################

# PSoC Edge SDK libraries (psoc-edge/ inside the app repo)
# Repository root -- used to keep object-file paths short (Windows MAX_PATH)
REPO_ROOT := $(abspath ..)

PSE_DIR := $(REPO_ROOT)/deps/psoc-edge

# Application root (this Makefile lives in cm55_firmware/)
APP_DIR := $(abspath .)

# Shared source across CM33/CM55
SHARED_DIR := $(abspath ../shared)

# MicroPython DeepCraft model interface
DEEPCRAFT_DIR := $(abspath ../deps/deepcraft/micropython-deepcraft-model-interface/deepcraft)

# DeepCraft framework assets selected for this firmware build
DEEPCRAFT_FRAMEWORK_DIR := $(APP_DIR)/framework/deepcraft
DEEPCRAFT_VA_MODELS_DIR := $(DEEPCRAFT_FRAMEWORK_DIR)/va_models

# BSP (board-specific files from psoc-edge)
BSP_DIR := $(PSE_DIR)/TARGET_KIT_PSE84_AI

# Library paths (all under psoc-edge/)
LIB_ABS_RTOS    := $(PSE_DIR)/abstraction-rtos
LIB_ASYNC       := $(PSE_DIR)/async-transfer
LIB_AFE         := $(PSE_DIR)/audio-front-end
LIB_AVC         := $(PSE_DIR)/audio-voice-core
LIB_BTFW        := $(PSE_DIR)/bt-fw-mur-cyw55513
LIB_CLIB        := $(PSE_DIR)/clib-support
LIB_CMSIS       := $(PSE_DIR)/cmsis
LIB_CONN_UTILS  := $(PSE_DIR)/connectivity-utilities
LIB_CORELIB     := $(PSE_DIR)/core-lib
LIB_FREERTOS    := $(PSE_DIR)/freertos
LIB_ML_MW       := $(PSE_DIR)/ml-middleware
LIB_TFLM        := $(PSE_DIR)/ml-tflite-micro
LIB_PSE         := $(PSE_DIR)/mtb-dsl-pse8xxgp
LIB_IPC         := $(PSE_DIR)/mtb-ipc
LIB_SRF         := $(PSE_DIR)/mtb-srf
LIB_RETARGET    := $(PSE_DIR)/retarget-io
LIB_SE_RT       := $(PSE_DIR)/se-rt-services-utils
LIB_SOD         := $(PSE_DIR)/speech-onset-detection
LIB_VA          := $(PSE_DIR)/voice-assistant

################################################################################
# Compiler flags
################################################################################

CPU_FLAGS := \
    --target=arm-none-eabi \
    -mcpu=cortex-m55 \
    -mthumb \
    -mfloat-abi=hard \
    -mfpu=fp-armv8-fullfp16-d16

ifeq ($(CONFIG),Debug)
    OPT_FLAGS := -O0 -g
    DEFINES_CFG := -DCOMPONENT_Debug -DDEBUG
else ifeq ($(CONFIG),Release)
    OPT_FLAGS := -Os
    DEFINES_CFG := -DCOMPONENT_Release -DNDEBUG
else
    OPT_FLAGS := -O0 -g
    DEFINES_CFG := -DCOMPONENT_Debug -DDEBUG
endif

COMMON_FLAGS := \
    $(CPU_FLAGS) \
    $(OPT_FLAGS) \
    -ffunction-sections \
    -fdata-sections \
    -fshort-enums \
    -Wall \
    -fintegrated-cc1 \
    -fintegrated-objemitter \
    -DMTB_LLVM_EMBEDDED_ARM

CFLAGS := $(COMMON_FLAGS) -c -MMD -MP
CXXFLAGS := $(COMMON_FLAGS) -c -MMD -MP -std=c++17 -fno-exceptions -fno-rtti

ASFLAGS := \
    $(CPU_FLAGS) \
    $(OPT_FLAGS) \
    -c -MMD -MP \
    -DMTB_LLVM_EMBEDDED_ARM

LDFLAGS := \
    $(CPU_FLAGS) \
    $(OPT_FLAGS) \
    -ffunction-sections \
    -fdata-sections \
    -Wall \
    -Wl,--gc-sections \
    -Xlinker -L$(BSP_DIR)/COMPONENT_CM55/TOOLCHAIN_LLVM_ARM \
    -Xlinker -L$(CURDIR)/sources/bsp-cfg \
    -T$(BSP_DIR)/COMPONENT_CM55/TOOLCHAIN_LLVM_ARM/pse84_ns_cm55.ld \
    -Wl,-Map,$(BUILD_DIR)/$(APPNAME).map

################################################################################
# Preprocessor defines
################################################################################

DEFINES := \
    $(DEFINES_CFG) \
    -DBLHS_SUPPORT \
    -DCOMPONENT_55500 \
    -DCOMPONENT_55500A1 \
    -DCOMPONENT_APP_KIT_PSE84_AI \
    -DCOMPONENT_AVC_DEMO \
    -DCOMPONENT_CM55 \
    -DCOMPONENT_CM55_0 \
    -DCOMPONENT_CMSIS_DSP \
    -DCOMPONENT_FREERTOS \
    -DCOMPONENT_HARDFP \
    -DCOMPONENT_LLVM_ARM \
    -DCOMPONENT_ML_INT8x8 \
    -DCOMPONENT_ML_TFLM \
    -DCOMPONENT_MTB_DEVICE_SUPPORT \
    -DCOMPONENT_MTB_HAL \
    -DCOMPONENT_MW_ASYNC_TRANSFER \
    -DCOMPONENT_MW_DSNS_MODEL \
    -DCOMPONENT_MW_MDL_DSES \
    -DCOMPONENT_MW_MDL_DSNS \
    -DCOMPONENT_MW_MDL_VA \
    -DCOMPONENT_MW_MTB_DSL_PSE8XXGP \
    -DCOMPONENT_MW_MTB_IPC \
    -DCOMPONENT_MW_MTB_SRF \
    -DCOMPONENT_MW_SE_RT_SERVICES_UTILS \
    -DCOMPONENT_NON_SECURE_DEVICE \
    -DCOMPONENT_PSE84 \
    -DCOMPONENT_RTOS_AWARE \
    -DCOMPONENT_U55 \
    -DCORE_NAME_CM55_0=1 \
    -DCYBSP_MCUBOOT_HEADER_SIZE=0x400 \
    -DCY_APPNAME_$(APPNAME) \
    -DCY_ML_MODEL_MEM=.cy_socmem_data \
    -DCY_RETARGET_IO_CONVERT_LF_TO_CRLF \
    -DCY_RTOS_AWARE \
    -DCY_SUPPORTS_DEVICE_VALIDATION \
    -DCY_TARGET_BOARD=APP_KIT_PSE84_AI \
    -DDSNS_ENABLE_MTB_ML \
    -DENABLE_AFE_MW_SUPPORT \
    -DENABLE_IFX_PRE_PROCESS_HPF \
    -DENABLE_IFX_SOD \
    -DENABLE_IFX_VA_CMD \
    -DENABLE_IFX_VA_WWD \
    -DPROJECT_PREFIX=$(DEEPCRAFT_PROJECT) \
    -DPSE846GPS2DBZC4A \
    -DTARGET_KIT_PSE84_AI \
    -DTF_LITE_STATIC_MEMORY \
    -DTRXV5 \
    -DUSE_AUDIO_ENHANCEMENT \
    -DUSE_KIT_PSE84_AI \
    -DUSE_MTB_ML \
    -DFLASH_BOOT \
    -DCY_PDL_FLASH_BOOT

################################################################################
# Include paths
################################################################################

INCLUDES := \
    -I$(APP_DIR) \
    -I$(APP_DIR)/includes \
    -I$(APP_DIR)/sources \
    -I$(APP_DIR)/sources/bsp-cfg \
    -I$(APP_DIR)/adapters/deepcraft \
    -I$(DEEPCRAFT_FRAMEWORK_DIR) \
    -I$(DEEPCRAFT_FRAMEWORK_DIR)/audio \
    -I$(DEEPCRAFT_VA_MODELS_DIR)/$(DEEPCRAFT_PROJECT) \
    -I$(SHARED_DIR)/include \
    -I$(DEEPCRAFT_DIR) \
    \
    -I$(BSP_DIR) \
    -I$(BSP_DIR)/bluetooth \
    -I$(CURDIR)/sources/bsp-cfg \
    \
    -I$(LIB_ABS_RTOS)/include \
    -I$(LIB_ABS_RTOS)/include/COMPONENT_FREERTOS \
    \
    -I$(LIB_ASYNC)/include \
    \
    -I$(LIB_AFE)/include \
    -I$(LIB_AFE)/source/include \
    \
    -I$(LIB_AVC)/lib/SP_ENH/COMPONENT_CM55/inc \
    -I$(LIB_AVC)/lib/SP_ENH/COMPONENT_CM55/src/inc \
    -I$(LIB_AVC)/lib/SP_ENH/COMPONENT_CM55/MODEL/COMPONENT_ML_TFLM \
    \
    -I$(LIB_CLIB)/include \
    -I$(LIB_CLIB)/source/TOOLCHAIN_LLVM_ARM \
    \
    -I$(LIB_CMSIS)/Core/Include \
    -I$(LIB_CMSIS)/Core/Include/a-profile \
    -I$(LIB_CMSIS)/Core/Include/m-profile \
    -I$(LIB_CMSIS)/Core/Include/r-profile \
    -I$(LIB_CMSIS)/COMPONENT_CMSIS_DSP/Include \
    -I$(LIB_CMSIS)/COMPONENT_CMSIS_DSP/PrivateInclude \
    \
    -I$(LIB_CORELIB)/include \
    \
    -I$(LIB_CONN_UTILS) \
    -I$(LIB_CONN_UTILS)/JSON_parser \
    -I$(LIB_CONN_UTILS)/cy_log \
    -I$(LIB_CONN_UTILS)/cy_string \
    -I$(LIB_CONN_UTILS)/linked_list \
    -I$(LIB_CONN_UTILS)/network \
    \
    -I$(LIB_FREERTOS)/Source/include \
    -I$(LIB_FREERTOS)/Source/portable/COMPONENT_CM55/TOOLCHAIN_LLVM_ARM \
    \
    -I$(LIB_ML_MW)/include \
    -I$(LIB_ML_MW)/source/COMPONENT_ML_TFLM \
    \
    -I$(LIB_TFLM)/COMPONENT_ML_TFLM/include \
    -I$(LIB_TFLM)/COMPONENT_ML_TFLM/include/flatbuffers/include \
    -I$(LIB_TFLM)/COMPONENT_ML_TFLM/include/gemmlowp \
    -I$(LIB_TFLM)/COMPONENT_ML_TFLM/include/ethos-u-core-driver \
    \
    -I$(LIB_PSE)/device-utils/syspm/hobto/pdm_pcm/audioss_pdm \
    -I$(LIB_PSE)/device-utils/syspm/hobto/pdm_pcm/mxpdm \
    -I$(LIB_PSE)/device-utils/syspm/include \
    -I$(LIB_PSE)/hal/include \
    -I$(LIB_PSE)/nnkernel/COMPONENT_CM33/include \
    -I$(LIB_PSE)/pdl/devices/include \
    -I$(LIB_PSE)/pdl/devices/include/ip \
    -I$(LIB_PSE)/pdl/drivers/include \
    -I$(LIB_PSE)/pdl/drivers/third_party/ethernet/include \
    -I$(LIB_PSE)/pdl/drivers/third_party/COMPONENT_U55/include \
    \
    -I$(LIB_IPC)/include \
    \
    -I$(LIB_SRF)/include \
    -I$(LIB_SRF)/include/COMPONENT_NON_SECURE_DEVICE \
    -I$(LIB_SRF)/include/COMPONENT_NON_SECURE_DEVICE/COMPONENT_MW_MTB_IPC \
    \
    -I$(LIB_RETARGET)/include \
    \
    -I$(LIB_SE_RT) \
    \
    -I$(LIB_SOD)/include \
    -I$(LIB_SOD)/source/inc \
    \
    -I$(LIB_VA)/include \
    -I$(LIB_VA)/source/va_core

################################################################################
# Application source files
################################################################################

APP_C_SRCS := \
    $(APP_DIR)/main.c \
    $(APP_DIR)/sources/audio/audio_enhancement.c \
    $(APP_DIR)/adapters/deepcraft/wrapper.c \
    $(APP_DIR)/sources/transport/ipc.c \
    $(APP_DIR)/sources/audio/pdm_mic.c \
    $(APP_DIR)/sources/platform/profiler.c \
    $(APP_DIR)/sources/platform/retarget_io_init.c \
    $(DEEPCRAFT_FRAMEWORK_DIR)/audio/voice_assistant.c \
    $(APP_DIR)/sources/bsp-cfg/cy_afe_configurator_settings.c \
    $(APP_DIR)/sources/bsp-cfg/cycfg.c \
    $(APP_DIR)/sources/bsp-cfg/cycfg_clocks.c \
    $(APP_DIR)/sources/bsp-cfg/cycfg_dmas.c \
    $(APP_DIR)/sources/bsp-cfg/cycfg_peripheral_clocks.c \
    $(APP_DIR)/sources/bsp-cfg/cycfg_peripherals.c \
    $(APP_DIR)/sources/bsp-cfg/cycfg_pins.c \
    $(APP_DIR)/sources/bsp-cfg/cycfg_protection.c \
    $(APP_DIR)/sources/bsp-cfg/cycfg_qspi_memslot.c \
    $(APP_DIR)/sources/bsp-cfg/cycfg_routing.c \
    $(APP_DIR)/sources/bsp-cfg/cycfg_system.c \
    $(SHARED_DIR)/source/COMPONENT_CM55/cm55_ipc_communication.c \
    $(DEEPCRAFT_DIR)/deepcraft_engine.c

# VA model sources (project-specific)
APP_C_SRCS += \
    $(DEEPCRAFT_VA_MODELS_DIR)/$(DEEPCRAFT_PROJECT)/$(DEEPCRAFT_PROJECT)_config.c \
    $(DEEPCRAFT_VA_MODELS_DIR)/$(DEEPCRAFT_PROJECT)/$(DEEPCRAFT_PROJECT)_ifx_va_config_prms.c \
    $(DEEPCRAFT_VA_MODELS_DIR)/$(DEEPCRAFT_PROJECT)/$(DEEPCRAFT_PROJECT)_U55_CMDmodel.c \
    $(DEEPCRAFT_VA_MODELS_DIR)/$(DEEPCRAFT_PROJECT)/$(DEEPCRAFT_PROJECT)_U55_WWmodel.c

################################################################################
# BSP source files
################################################################################

BSP_C_SRCS := \
    $(BSP_DIR)/cybsp.c \
    $(BSP_DIR)/system_edge.c \
    $(BSP_DIR)/bluetooth/cybsp_bt_config.c \
    $(BSP_DIR)/COMPONENT_CM55/COMPONENT_NON_SECURE_DEVICE/ns_start_pse84.c \
    $(BSP_DIR)/COMPONENT_CM55/COMPONENT_NON_SECURE_DEVICE/ns_system_pse84.c

# NOTE: BSP config/GeneratedSource/cycfg*.c are excluded here because the
# project's bsp-cfg/cycfg*.c files (in APP_C_SRCS) override them and defining
# both would cause duplicate symbol linker errors.

################################################################################
# Library source files
################################################################################

# abstraction-rtos (FreeRTOS backend)
LIB_C_SRCS := \
    $(LIB_ABS_RTOS)/source/cy_worker_thread.c \
    $(LIB_ABS_RTOS)/source/COMPONENT_FREERTOS/cyabs_freertos_common.c \
    $(LIB_ABS_RTOS)/source/COMPONENT_FREERTOS/cyabs_freertos_helpers.c \
    $(LIB_ABS_RTOS)/source/COMPONENT_FREERTOS/cyabs_rtos_dsram.c \
    $(LIB_ABS_RTOS)/source/COMPONENT_FREERTOS/cyabs_rtos_freertos.c

# async-transfer
LIB_C_SRCS += \
    $(LIB_ASYNC)/source/mtb_async_transfer.c

# audio-front-end
LIB_C_SRCS += \
    $(LIB_AFE)/source/cy_afe.c \
    $(LIB_AFE)/source/cy_afe_audio_bd_calc.c \
    $(LIB_AFE)/source/cy_afe_audio_debug.c \
    $(LIB_AFE)/source/cy_afe_audio_process.c \
    $(LIB_AFE)/source/cy_afe_audio_speech_enh.c \
    $(LIB_AFE)/source/cy_afe_audio_task.c \
    $(LIB_AFE)/source/cy_afe_audio_utils.c \
    $(LIB_AFE)/source/cy_afe_profiler.c \
    $(LIB_AFE)/source/cy_afe_tuner_commands.c \
    $(LIB_AFE)/source/cy_afe_tuner_process.c \
    $(LIB_AFE)/source/cy_afe_tuner_task.c

# audio-voice-core (C sources + C++ model file compiled separately)
LIB_C_SRCS += \
    $(LIB_AVC)/lib/SP_ENH/COMPONENT_CM55/src/cy_sp_enh.c \
    $(LIB_AVC)/lib/SP_ENH/COMPONENT_CM55/src/ifx_pre_post_process.c \
    $(LIB_AVC)/lib/SP_ENH/COMPONENT_CM55/src/ifx_sp_enh.c \
    $(LIB_AVC)/lib/SP_ENH/COMPONENT_CM55/src/ifx_sp_enh_process.c \
    $(LIB_AVC)/lib/SP_ENH/COMPONENT_CM55/src/ifx_sp_utils.c \
    $(LIB_AVC)/lib/SP_ENH/COMPONENT_CM55/MODEL/COMPONENT_MW_MDL_DSES/DSES_LSTM_tflm_model_int16x8.c \
    $(LIB_AVC)/lib/SP_ENH/COMPONENT_CM55/MODEL/COMPONENT_MW_MDL_DSNS/DSNS_LSTM_tflm_model_int16x8.c \
    $(LIB_AVC)/lib/SP_ENH/COMPONENT_CM55/MODEL/COMPONENT_MW_MDL_VA/VA_AM_INT8_tflm_model_int8x8.c

LIB_CXX_SRCS := \
    $(LIB_AVC)/lib/SP_ENH/COMPONENT_CM55/MODEL/COMPONENT_ML_TFLM/mtb_ml_model_tflm_16x8.cpp

# bt-fw (compiled as C stub; actual firmware is a binary blob linked via .hcd)
LIB_C_SRCS += \
    $(LIB_BTFW)/COMPONENT_MURATA-2FY/btfw.c

# clib-support
LIB_C_SRCS += \
    $(LIB_CLIB)/source/cy_time.c \
    $(LIB_CLIB)/source/COMPONENT_FREERTOS/cy_mutex_pool.c \
    $(LIB_CLIB)/source/TOOLCHAIN_LLVM_ARM/cy_clib_support_llvm_arm_picolibc.c \
    $(LIB_CLIB)/source/TOOLCHAIN_LLVM_ARM/COMPONENT_FREERTOS/cy_clib_support_llvm_arm_freertos.c

# cmsis-dsp — all sources under COMPONENT_CMSIS_DSP/Source/
CMSIS_DSP_INC_DIRS := $(LIB_CMSIS)/COMPONENT_CMSIS_DSP/Include
CMSIS_DSP_SRCS := $(wildcard $(LIB_CMSIS)/COMPONENT_CMSIS_DSP/Source/*.c) \
              $(wildcard $(LIB_CMSIS)/COMPONENT_CMSIS_DSP/Source/*/*.c)
LIB_C_SRCS += $(CMSIS_DSP_SRCS)

# connectivity-utilities
LIB_C_SRCS += \
    $(LIB_CONN_UTILS)/JSON_parser/cy_json_parser.c \
    $(LIB_CONN_UTILS)/cy_log/cy_log.c \
    $(LIB_CONN_UTILS)/cy_string/cy_string_utils.c \
    $(LIB_CONN_UTILS)/linked_list/cy_linked_list.c \
    $(LIB_CONN_UTILS)/network/cy_nw_helper_common.c

# FreeRTOS kernel
LIB_C_SRCS += \
    $(LIB_FREERTOS)/Source/croutine.c \
    $(LIB_FREERTOS)/Source/event_groups.c \
    $(LIB_FREERTOS)/Source/list.c \
    $(LIB_FREERTOS)/Source/queue.c \
    $(LIB_FREERTOS)/Source/stream_buffer.c \
    $(LIB_FREERTOS)/Source/tasks.c \
    $(LIB_FREERTOS)/Source/timers.c \
    $(LIB_FREERTOS)/Source/portable/MemMang/heap_3.c \
    $(LIB_FREERTOS)/Source/portable/COMPONENT_CM55/TOOLCHAIN_LLVM_ARM/port.c \
    $(LIB_FREERTOS)/Source/portable/COMPONENT_CM55/TOOLCHAIN_LLVM_ARM/portasm.c \
    $(LIB_FREERTOS)/Source/portable/COMPONENT_CM55/TOOLCHAIN_LLVM_ARM/mpu_wrappers_v2_asm.c

# ml-middleware
LIB_C_SRCS += \
    $(LIB_ML_MW)/source/mtb_ml.c \
    $(LIB_ML_MW)/source/mtb_ml_nnlite.c \
    $(LIB_ML_MW)/source/mtb_ml_rtos.c \
    $(LIB_ML_MW)/source/mtb_ml_utils.c \
    $(LIB_ML_MW)/source/COMPONENT_U55/mtb_ml_ethosu.c

LIB_CXX_SRCS += \
    $(LIB_ML_MW)/source/COMPONENT_ML_TFLM/mtb_ml_model.cpp

# PSoC Edge PDL drivers
# (79 of 91 sources; 12 v1-crypto + cy_pdl_srf confirmed GC'd in build/Debug/cm55_firmware.map)
LIB_C_SRCS += \
    $(LIB_PSE)/pdl/drivers/source/cy_autanalog.c \
    $(LIB_PSE)/pdl/drivers/source/cy_autanalog_ac.c \
    $(LIB_PSE)/pdl/drivers/source/cy_autanalog_ctb.c \
    $(LIB_PSE)/pdl/drivers/source/cy_autanalog_dac.c \
    $(LIB_PSE)/pdl/drivers/source/cy_autanalog_ptc.c \
    $(LIB_PSE)/pdl/drivers/source/cy_autanalog_sar.c \
    $(LIB_PSE)/pdl/drivers/source/cy_axidmac.c \
    $(LIB_PSE)/pdl/drivers/source/cy_canfd.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_aes_v2.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_cmac_v2.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_crc_v2.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_des_v2.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_ecc_domain_params.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_ecc_ec25519.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_ecc_ecdsa.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_ecc_eddsa.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_ecc_key_gen.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_ecc_nist_p.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_hkdf_v2.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_hmac_v2.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_hw.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_mem_v2.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_prng_v2.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_rsa.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_sha_v2.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_trng.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_core_vu.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_server.c \
    $(LIB_PSE)/pdl/drivers/source/cy_crypto_utils.c \
    $(LIB_PSE)/pdl/drivers/source/cy_dma.c \
    $(LIB_PSE)/pdl/drivers/source/cy_ephy.c \
    $(LIB_PSE)/pdl/drivers/source/cy_ethif.c \
    $(LIB_PSE)/pdl/drivers/source/cy_gpio.c \
    $(LIB_PSE)/pdl/drivers/source/cy_i3c.c \
    $(LIB_PSE)/pdl/drivers/source/cy_ipc_drv.c \
    $(LIB_PSE)/pdl/drivers/source/cy_ipc_pipe.c \
    $(LIB_PSE)/pdl/drivers/source/cy_ipc_sema.c \
    $(LIB_PSE)/pdl/drivers/source/cy_lpcomp.c \
    $(LIB_PSE)/pdl/drivers/source/cy_lvd.c \
    $(LIB_PSE)/pdl/drivers/source/cy_mcwdt.c \
    $(LIB_PSE)/pdl/drivers/source/cy_mipidsi.c \
    $(LIB_PSE)/pdl/drivers/source/cy_mpc.c \
    $(LIB_PSE)/pdl/drivers/source/cy_ms_ctl.c \
    $(LIB_PSE)/pdl/drivers/source/cy_nnlite.c \
    $(LIB_PSE)/pdl/drivers/source/cy_pdl_srf_common.c \
    $(LIB_PSE)/pdl/drivers/source/cy_pdm_pcm_v2.c \
    $(LIB_PSE)/pdl/drivers/source/cy_ppc.c \
    $(LIB_PSE)/pdl/drivers/source/cy_rram.c \
    $(LIB_PSE)/pdl/drivers/source/cy_rtc.c \
    $(LIB_PSE)/pdl/drivers/source/cy_scb_common.c \
    $(LIB_PSE)/pdl/drivers/source/cy_scb_ezi2c.c \
    $(LIB_PSE)/pdl/drivers/source/cy_scb_i2c.c \
    $(LIB_PSE)/pdl/drivers/source/cy_scb_spi.c \
    $(LIB_PSE)/pdl/drivers/source/cy_scb_uart.c \
    $(LIB_PSE)/pdl/drivers/source/cy_sd_host.c \
    $(LIB_PSE)/pdl/drivers/source/cy_smartio.c \
    $(LIB_PSE)/pdl/drivers/source/cy_smif.c \
    $(LIB_PSE)/pdl/drivers/source/cy_smif_hb_flash.c \
    $(LIB_PSE)/pdl/drivers/source/cy_smif_memnum.c \
    $(LIB_PSE)/pdl/drivers/source/cy_smif_memslot.c \
    $(LIB_PSE)/pdl/drivers/source/cy_smif_sfdp.c \
    $(LIB_PSE)/pdl/drivers/source/cy_sysclk_v2.c \
    $(LIB_PSE)/pdl/drivers/source/cy_sysfault.c \
    $(LIB_PSE)/pdl/drivers/source/cy_sysint_v2.c \
    $(LIB_PSE)/pdl/drivers/source/cy_syslib.c \
    $(LIB_PSE)/pdl/drivers/source/cy_syspm_pdcm.c \
    $(LIB_PSE)/pdl/drivers/source/cy_syspm_ppu.c \
    $(LIB_PSE)/pdl/drivers/source/cy_syspm_v4.c \
    $(LIB_PSE)/pdl/drivers/source/cy_systick_v2.c \
    $(LIB_PSE)/pdl/drivers/source/cy_systrimm.c \
    $(LIB_PSE)/pdl/drivers/source/cy_tcpwm_counter.c \
    $(LIB_PSE)/pdl/drivers/source/cy_tcpwm_pwm.c \
    $(LIB_PSE)/pdl/drivers/source/cy_tcpwm_quaddec.c \
    $(LIB_PSE)/pdl/drivers/source/cy_tcpwm_shiftreg.c \
    $(LIB_PSE)/pdl/drivers/source/cy_tdm.c \
    $(LIB_PSE)/pdl/drivers/source/cy_trigmux.c \
    $(LIB_PSE)/pdl/drivers/source/cy_wdt.c \
    $(LIB_PSE)/pdl/drivers/source/ppu_v1.c

# PSoC Edge PDL LLVM toolchain assembly
LIB_S_SRCS := \
    $(LIB_PSE)/pdl/drivers/source/TOOLCHAIN_LLVM_ARM/cy_syslib_ext.S

# PSoC Edge PDL device source
LIB_C_SRCS += \
    $(LIB_PSE)/pdl/devices/source/cy_device.c

# PSoC Edge HAL (all 23 sources confirmed live in build/Debug/cm55_firmware.map)
LIB_C_SRCS += \
    $(LIB_PSE)/hal/source/mtb_hal_adc_sar.c \
    $(LIB_PSE)/hal/source/mtb_hal_clock.c \
    $(LIB_PSE)/hal/source/mtb_hal_comp.c \
    $(LIB_PSE)/hal/source/mtb_hal_dma.c \
    $(LIB_PSE)/hal/source/mtb_hal_dma_dmac.c \
    $(LIB_PSE)/hal/source/mtb_hal_dma_dw.c \
    $(LIB_PSE)/hal/source/mtb_hal_gpio.c \
    $(LIB_PSE)/hal/source/mtb_hal_i2c.c \
    $(LIB_PSE)/hal/source/mtb_hal_lptimer.c \
    $(LIB_PSE)/hal/source/mtb_hal_lptimer_common.c \
    $(LIB_PSE)/hal/source/mtb_hal_lptimer_mcwdt.c \
    $(LIB_PSE)/hal/source/mtb_hal_memoryspi.c \
    $(LIB_PSE)/hal/source/mtb_hal_nvm.c \
    $(LIB_PSE)/hal/source/mtb_hal_nvm_mxs22rramc.c \
    $(LIB_PSE)/hal/source/mtb_hal_pwm.c \
    $(LIB_PSE)/hal/source/mtb_hal_rtc.c \
    $(LIB_PSE)/hal/source/mtb_hal_sdhc.c \
    $(LIB_PSE)/hal/source/mtb_hal_spi.c \
    $(LIB_PSE)/hal/source/mtb_hal_syspm.c \
    $(LIB_PSE)/hal/source/mtb_hal_system.c \
    $(LIB_PSE)/hal/source/mtb_hal_timer.c \
    $(LIB_PSE)/hal/source/mtb_hal_uart.c \
    $(LIB_PSE)/hal/source/mtb_hal_utils_impl.c

# PSoC Edge device-utils syspm callbacks (all 7 confirmed live in build/Debug/cm55_firmware.map)
LIB_C_SRCS += \
    $(LIB_PSE)/device-utils/syspm/source/mtb_syspm_callbacks_dma.c \
    $(LIB_PSE)/device-utils/syspm/source/mtb_syspm_callbacks_i2s_tdm.c \
    $(LIB_PSE)/device-utils/syspm/source/mtb_syspm_callbacks_nvm.c \
    $(LIB_PSE)/device-utils/syspm/source/mtb_syspm_callbacks_pdm_pcm.c \
    $(LIB_PSE)/device-utils/syspm/source/mtb_syspm_callbacks_scb_uart.c \
    $(LIB_PSE)/device-utils/syspm/source/mtb_syspm_callbacks_smif.c \
    $(LIB_PSE)/device-utils/syspm/source/mtb_syspm_callbacks_tcpwm.c

# PSoC Edge stdlib stubs
LIB_C_SRCS += \
    $(LIB_PSE)/support/source/mtb_stdlib_stubs.c

# PSoC Edge Ethos-U55 driver
LIB_C_SRCS += \
    $(LIB_PSE)/pdl/drivers/third_party/COMPONENT_U55/src/ethosu_device_u55_u65.c \
    $(LIB_PSE)/pdl/drivers/third_party/COMPONENT_U55/src/ethosu_device_u85.c \
    $(LIB_PSE)/pdl/drivers/third_party/COMPONENT_U55/src/ethosu_driver.c \
    $(LIB_PSE)/pdl/drivers/third_party/COMPONENT_U55/src/ethosu_pmu.c

# PSoC Edge Ethernet third-party: all 3 files (edd.c, edd_rx.c, edd_tx.c) confirmed
# fully GC'd in build/Debug/cm55_firmware.map (only .debug_* sections present).
# CM55 voice assistant does not use Ethernet. Excluded.

# mtb-ipc
LIB_C_SRCS += \
    $(LIB_IPC)/source/mtb_ipc.c

# mtb-srf
LIB_C_SRCS += \
    $(LIB_SRF)/source/mtb_srf.c \
    $(LIB_SRF)/source/COMPONENT_NON_SECURE_DEVICE/mtb_srf_pool.c \
    $(LIB_SRF)/source/COMPONENT_NON_SECURE_DEVICE/COMPONENT_MW_MTB_IPC/mtb_srf_ipc.c

# retarget-io
LIB_C_SRCS += \
    $(LIB_RETARGET)/source/cy_retarget_io.c

# se-rt-services-utils
LIB_C_SRCS += \
    $(LIB_SE_RT)/ifx_se_crc32.c \
    $(LIB_SE_RT)/ifx_se_fih.c \
    $(LIB_SE_RT)/ifx_se_platform.c \
    $(LIB_SE_RT)/ifx_se_psacrypto.c \
    $(LIB_SE_RT)/ifx_se_ss_wifi.c \
    $(LIB_SE_RT)/ifx_se_syscall.c \
    $(LIB_SE_RT)/ifx_se_syscall_builtin.c

# speech-onset-detection
LIB_C_SRCS += \
    $(LIB_SOD)/source/cy_sod.c \
    $(LIB_SOD)/source/cy_sod_private.c \
    $(LIB_SOD)/source/cy_sod_profiler.c

# voice-assistant
LIB_C_SRCS += \
    $(LIB_VA)/source/mtb_nlu.c \
    $(LIB_VA)/source/mtb_wwd.c \
    $(LIB_VA)/source/MODELS/AM_LSTM_tflm_model_int16x8.c \
    $(LIB_VA)/source/MODELS/U55_NMBmodel.c \
    $(LIB_VA)/source/va_core/va_ml_model.c \
    $(LIB_VA)/source/va_core/va_timer.c \
    $(LIB_VA)/source/va_core/va_utils.c

################################################################################
# Pre-built static libraries
################################################################################

LDLIBS := \
    $(LIB_AVC)/lib/SP_ENH/COMPONENT_CM55/COMPONENT_AVC_DEMO/COMPONENT_HARDFP/TOOLCHAIN_LLVM_ARM/ifx_va.a \
    $(LIB_AVC)/lib/SP_ENH/COMPONENT_CM55/COMPONENT_AVC_DEMO/COMPONENT_HARDFP/TOOLCHAIN_LLVM_ARM/sp_enh.a \
    $(LIB_TFLM)/COMPONENT_ML_TFLM/COMPONENT_U55/TOOLCHAIN_LLVM_ARM/libtensorflow-microlite.a

# Extra linker flags (not file deps) -- C++ runtime for operator new/delete
LD_EXTRA_LIBS := -lc++ -lc++abi

################################################################################
# Collect all objects
################################################################################

ALL_C_SRCS   := $(APP_C_SRCS) $(BSP_C_SRCS) $(LIB_C_SRCS)
ALL_CXX_SRCS := $(LIB_CXX_SRCS)
ALL_S_SRCS   := $(LIB_S_SRCS)

# Derive object paths: map each source file to BUILD_DIR/obj/<mangled-path>.o
_src_to_obj = $(BUILD_DIR)/obj/$(subst :,_,$(subst /,_,$(subst $(REPO_ROOT)/,,$(abspath $1)))).o

C_OBJS   := $(foreach s,$(ALL_C_SRCS),$(call _src_to_obj,$(s)))
CXX_OBJS := $(foreach s,$(ALL_CXX_SRCS),$(call _src_to_obj,$(s)))
S_OBJS   := $(foreach s,$(ALL_S_SRCS),$(call _src_to_obj,$(s)))

ALL_OBJS := $(C_OBJS) $(CXX_OBJS) $(S_OBJS)

################################################################################
# Dependency files
################################################################################

DEPS := $(ALL_OBJS:.o=.d)

################################################################################
# Output artifacts
################################################################################

ELF := $(BUILD_DIR)/$(APPNAME).elf
HEX := $(BUILD_DIR)/$(APPNAME).hex
BIN := $(BUILD_DIR)/$(APPNAME).bin

################################################################################
# Build rules
################################################################################

.PHONY: all clean flash size submodules

all: $(HEX) $(BIN) size

$(BUILD_DIR)/obj:
	$(if $(filter $(OS),Windows_NT),if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)",mkdir -p $@)

# C compilation rule — uses a static pattern over ALL_C_SRCS
define C_RULE
$(call _src_to_obj,$(1)): $(1) | $(BUILD_DIR)/obj
	@echo   CC  $(notdir $(1))
	@$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MF $$(@:.o=.d) -MT $$@ -o $$@ $$<
endef
$(foreach s,$(ALL_C_SRCS),$(eval $(call C_RULE,$(s))))

# C++ compilation rule
define CXX_RULE
$(call _src_to_obj,$(1)): $(1) | $(BUILD_DIR)/obj
	@echo   CXX $(notdir $(1))
	@$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -MF $$(@:.o=.d) -MT $$@ -o $$@ $$<
endef
$(foreach s,$(ALL_CXX_SRCS),$(eval $(call CXX_RULE,$(s))))

# Assembly rule
define S_RULE
$(call _src_to_obj,$(1)): $(1) | $(BUILD_DIR)/obj
	@echo   AS  $(notdir $(1))
	@$(AS) $(ASFLAGS) $(DEFINES) $(INCLUDES) -MF $$(@:.o=.d) -MT $$@ -o $$@ $$<
endef
$(foreach s,$(ALL_S_SRCS),$(eval $(call S_RULE,$(s))))

# Link
OBJS_RSP := $(BUILD_DIR)/objects.rsp
$(ELF): $(ALL_OBJS) $(LDLIBS) | $(BUILD_DIR)/obj
	@echo   LD  $@
	$(file >$(OBJS_RSP),$(ALL_OBJS))
	@$(LD) $(LDFLAGS) -o $@ \
            -Wl,--start-group @$(OBJS_RSP) $(LDLIBS) -Wl,--end-group $(LD_EXTRA_LIBS)
# Post-processing
$(HEX): $(ELF)
	@echo   HEX $@
	@$(OBJCOPY) -O ihex $< $@

$(BIN): $(ELF)
	@echo   BIN $@
	@$(OBJCOPY) -O binary $< $@

size: $(ELF)
	@$(SIZE) $<

# Clean
clean:
	$(RM_RF) $(BUILD_DIR)

# Initialize all psoc-edge submodules
submodules:
	git -C $(abspath ..) submodule update --init --jobs 8 $(abspath ..)/deps/psoc-edge

## Openocd reusable flashing target

# Deploy on an specific serial device if passed
ifndef DEV_SERIAL_NUMBER
SERIAL_ADAPTER_CMD =
else
SERIAL_ADAPTER_CMD = -c "adapter serial $(DEV_SERIAL_NUMBER)"
endif

OPENOCD_OPEN_CMDS = \
	-s $(BOARD_DIR)/sources/bsp-cfg \
	-c "set QSPI_FLASHLOADER $(BOARD_DIR)/sources/bsp-cfg/PSE84_SMIF.FLM" \
	-c "source [find interface/kitprog3.cfg]" \
	$(SERIAL_ADAPTER_CMD) \
	-c "transport select swd" \
 	-c "source [find target/infineon/pse84xgxs2.cfg]" \
	-c "init" \
	-c "reset init" \
	-c "adapter speed 12000" \

OPENOCD_CLOSE_CMDS = \
	-c "reset run; shutdown" \


define flash_target
    $(Q)$(OPENOCD) \
        $(OPENOCD_OPEN_CMDS) \
        -c "flash write_image erase $(1)" \
        $(OPENOCD_CLOSE_CMDS)
endef

BUILD ?= build/Debug
deploy: $(BUILD)/$(APPNAME).elf
	$(call flash_target,$<)

# Pull in generated dependency files
-include $(DEPS)
