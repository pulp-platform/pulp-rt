ifndef GAP_SDK_HOME
    $(error Please run 'source sourceme.sh' in gap_sdk first)
endif

CD = cd
CP = cp -rf
RM = rm -rf
MKDIR = mkdir

# The C program compiler.
CXX           = riscv32-unknown-elf-g++
CC            = riscv32-unknown-elf-gcc
AR            = riscv32-unknown-elf-ar
OBJDUMP       = riscv32-unknown-elf-objdump

LDFLAGS       = -T$(GAP_SDK_HOME)/tools/ld/link.gap8.ld

# The pre-processor and compiler options.
# Users can override those variables from the command line.
COMMON        = -march=rv32imcxgap8 -mPE=8 -mFC=1 -D__riscv__ -Os -g -fno-jump-tables -Werror -fno-tree-loop-distribute-patterns  -Wextra -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wundef -fdata-sections -ffunction-sections

ASMFLAGS      = $(COMMON) -DLANGUAGE_ASSEMBLY -MMD -MP -c

CFLAGS        = $(COMMON) -MMD -MP -c -DCONFIG_GAP

# Final binary
#------------------------------------------
RT_LIB        = $(TARGET_INSTALL_DIR)/libs/librt.a
IO_LIB		  = $(TARGET_INSTALL_DIR)/libs/libio.a

HEADER_DIR    = $(TARGET_INSTALL_DIR)/include
BUILD_RT  	  ?= $(BUILD_DIR)/rt
COMMON_SRC	  += $(RUNTIME_PATH)/pulp-rt/rt_conf.c $(RUNTIME_PATH)/pulp-rt/rt_pad_conf.c

HEADERS       = $(wildcard $(shell find $(RUNTIME_PATH)/include -name "*.h"))

RT_S_OBJECTS     = $(patsubst %.S, $(BUILD_RT)/%.o, $(wildcard $(shell find $(RUNTIME_PATH)/pulp-rt -name "*.S" | sort)))

RT_C_OBJECTS     = $(patsubst %.c, $(BUILD_RT)/%.o, $(wildcard $(shell find $(RUNTIME_PATH)/pulp-rt/kernel -name "*.c") $(shell find $(RUNTIME_PATH)/pulp-rt/drivers -name "*.c") $(COMMON_SRC)))

RT_OBJECTS       = $(RT_S_OBJECTS) $(RT_C_OBJECTS)

IO_C_OBJECTS     = $(patsubst %.c, $(BUILD_RT)/%.o, $(wildcard $(shell find $(RUNTIME_PATH)/pulp-rt/libs/io -name "*.c")))

INC_DEFINE    = -include $(TARGET_INSTALL_DIR)/include/rt/chips/$(TARGET_NAME)/config.h

INC           = $(TARGET_INSTALL_DIR)/include/ \
				$(TARGET_INSTALL_DIR)/include/io \
				$(INSTALL_DIR)/include

INC_PATH      = $(foreach d, $(INC), -I$d)  $(INC_DEFINE)

$(HEADER_DIR):
	mkdir -p $@

install_headers: $(HEADER_DIR)
	$(CP) $(GAP_SDK_HOME)/pulp-os/include/* $(HEADER_DIR)
	$(CP) $(GAP_SDK_HOME)/pulp-os/include/archi $(INSTALL_DIR)/include
	install -D $(GAP_SDK_HOME)/pulp-os/include/Gap8.h $(TARGET_INSTALL_DIR)/include
	install -D $(GAP_SDK_HOME)/pulp-os/include/gap8_emul.h $(TARGET_INSTALL_DIR)/include

install_rt_gap8: install_headers
	make -C  $(GAP_SDK_HOME)/pulp-os/pulp-rt MK_ROOT=$(GAP_SDK_HOME)/pulp-os/pulp-rt/mk/gap header pmsis build install

install_rt_vega: install_headers
	make -C  $(GAP_SDK_HOME)/pulp-os/pulp-rt MK_ROOT=$(GAP_SDK_HOME)/pulp-os/pulp-rt/mk/vega header pmsis build install

# Rules for creating the libs.
#------------------------------------------
gap: install_headers install_rt_gap8

gap_rev1: install_headers install_rt_gap8

gap9: install_headers install_rt_vega

#all: install_headers $(TARGET_CHIP)





