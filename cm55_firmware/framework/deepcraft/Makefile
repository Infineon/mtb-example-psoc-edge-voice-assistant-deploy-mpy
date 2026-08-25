################################################################################
# framework/deepcraft/Makefile -- DeepCraft framework build implementation
#
# This file contains only framework-specific sources and build rules.
# All hardware/BSP/toolchain config is in board.mk; invoke via the project
# entry point (cm55_firmware/Makefile) or standalone with board.mk available.
#
# Usage (standalone):
#   make BOARD_DIR=/path/to/cm55_firmware [CONFIG=Debug|Release]
#   make clean
#   make deploy
#
# Prerequisites:
#   - LLVM Embedded Toolchain for Arm 19.1.5 on PATH
#   - deps/psoc-edge/ submodules populated
#   - sources/bsp-cfg/cy_afe_configurator_settings.c generated
################################################################################

APPNAME          ?= cm55_firmware
CONFIG           ?= Debug
BOARD            ?= KIT_PSE84_AI
DEEPCRAFT_PROJECT ?= test_gpio_control
FRAMEWORK        ?= deepcraft

# BOARD_DIR/BUILD_DIR are set by the project Makefile; provide fallbacks for
# standalone invocation from inside the framework directory.
BOARD_DIR ?= $(abspath ../..)
BUILD_DIR ?= $(BOARD_DIR)/build/$(CONFIG)

# Pull in all hardware/BSP/toolchain config from the canonical board file.
include $(BOARD_DIR)/board.mk

################################################################################
# Framework-specific paths and sources
################################################################################

DEEPCRAFT_DIR           := $(REPO_ROOT)/deps/deepcraft/micropython-deepcraft-model-interface/deepcraft
DEEPCRAFT_FRAMEWORK_DIR := $(BOARD_DIR)/framework/deepcraft
DEEPCRAFT_VA_MODELS_DIR := $(DEEPCRAFT_FRAMEWORK_DIR)/va_models

FRAMEWORK_C_SRCS ?= \
    $(DEEPCRAFT_FRAMEWORK_DIR)/main.c \
    $(DEEPCRAFT_FRAMEWORK_DIR)/audio/voice_assistant.c \
    $(DEEPCRAFT_VA_MODELS_DIR)/$(DEEPCRAFT_PROJECT)/$(DEEPCRAFT_PROJECT)_config.c \
    $(DEEPCRAFT_VA_MODELS_DIR)/$(DEEPCRAFT_PROJECT)/$(DEEPCRAFT_PROJECT)_ifx_va_config_prms.c \
    $(DEEPCRAFT_VA_MODELS_DIR)/$(DEEPCRAFT_PROJECT)/$(DEEPCRAFT_PROJECT)_U55_CMDmodel.c \
    $(DEEPCRAFT_VA_MODELS_DIR)/$(DEEPCRAFT_PROJECT)/$(DEEPCRAFT_PROJECT)_U55_WWmodel.c \
    $(DEEPCRAFT_DIR)/deepcraft_engine.c

# Framework-specific include paths (appended to the board INCLUDES)
INCLUDES += \
    -I$(DEEPCRAFT_FRAMEWORK_DIR) \
    -I$(DEEPCRAFT_FRAMEWORK_DIR)/audio \
    -I$(DEEPCRAFT_VA_MODELS_DIR)/$(DEEPCRAFT_PROJECT) \
    -I$(DEEPCRAFT_DIR)

################################################################################
# Collect all objects
################################################################################

ALL_C_SRCS   := $(APP_C_SRCS) $(FRAMEWORK_C_SRCS) $(BSP_C_SRCS) $(LIB_C_SRCS)
ALL_CXX_SRCS := $(LIB_CXX_SRCS)
ALL_S_SRCS   := $(LIB_S_SRCS)

_src_to_obj = $(BUILD_DIR)/obj/$(subst :,_,$(subst /,_,$(subst $(REPO_ROOT)/,,$(abspath $1)))).o

C_OBJS   := $(foreach s,$(ALL_C_SRCS),$(call _src_to_obj,$(s)))
CXX_OBJS := $(foreach s,$(ALL_CXX_SRCS),$(call _src_to_obj,$(s)))
S_OBJS   := $(foreach s,$(ALL_S_SRCS),$(call _src_to_obj,$(s)))

ALL_OBJS := $(C_OBJS) $(CXX_OBJS) $(S_OBJS)
DEPS     := $(ALL_OBJS:.o=.d)

################################################################################
# Output artifacts
################################################################################

ELF := $(BUILD_DIR)/$(APPNAME).elf
HEX := $(BUILD_DIR)/$(APPNAME).hex
BIN := $(BUILD_DIR)/$(APPNAME).bin

################################################################################
# Build rules
################################################################################

.PHONY: all clean deploy size

all: $(HEX) $(BIN) size

$(BUILD_DIR)/obj:
	$(if $(filter $(OS),Windows_NT),if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)",mkdir -p $@)

define C_RULE
$(call _src_to_obj,$(1)): $(1) | $(BUILD_DIR)/obj
	@echo   CC  $(notdir $(1))
	@$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MF $$(@:.o=.d) -MT $$@ -o $$@ $$<
endef
$(foreach s,$(ALL_C_SRCS),$(eval $(call C_RULE,$(s))))

define CXX_RULE
$(call _src_to_obj,$(1)): $(1) | $(BUILD_DIR)/obj
	@echo   CXX $(notdir $(1))
	@$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -MF $$(@:.o=.d) -MT $$@ -o $$@ $$<
endef
$(foreach s,$(ALL_CXX_SRCS),$(eval $(call CXX_RULE,$(s))))

define S_RULE
$(call _src_to_obj,$(1)): $(1) | $(BUILD_DIR)/obj
	@echo   AS  $(notdir $(1))
	@$(AS) $(ASFLAGS) $(DEFINES) $(INCLUDES) -MF $$(@:.o=.d) -MT $$@ -o $$@ $$<
endef
$(foreach s,$(ALL_S_SRCS),$(eval $(call S_RULE,$(s))))

OBJS_RSP := $(BUILD_DIR)/objects.rsp
$(ELF): $(ALL_OBJS) $(LDLIBS) | $(BUILD_DIR)/obj
	@echo   LD  $@
	$(file >$(OBJS_RSP),$(ALL_OBJS))
	@$(LD) $(LDFLAGS) -o $@ \
	    -Wl,--start-group @$(OBJS_RSP) $(LDLIBS) -Wl,--end-group $(LD_EXTRA_LIBS)

$(HEX): $(ELF)
	@echo   HEX $@
	@$(OBJCOPY) -O ihex $< $@

$(BIN): $(ELF)
	@echo   BIN $@
	@$(OBJCOPY) -O binary $< $@

size: $(ELF)
	@$(SIZE) $<

clean:
	$(RM_RF) $(BUILD_DIR)

BUILD ?= $(BUILD_DIR)
deploy: $(BUILD)/$(APPNAME).elf
	$(call flash_target,$<)

-include $(DEPS)
