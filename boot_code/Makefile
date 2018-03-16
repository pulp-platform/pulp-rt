PULP_PROPERTIES = pulp_chip pulpChip
include $(PULP_SDK_HOME)/install/rules/pulp_properties.mk

ifeq '$(pulp_chip)' ''
pulp_chip = $(pulpChip)
endif

PULP_APP = boot-$(pulp_chip)
PULP_APP_FC_SRCS = boot_code.c
PULP_APP_FC_ASM_SRCS = crt0-riscv.fc.S

PULP_APP_HOST_SRCS = boot_code.c
PULP_APP_HOST_ASM_SRCS = crt0-ariane.S

ifeq '$(pulp_chip)' 'gap'
PULP_APP_FC_SRCS += AesLib.c
PULP_APP_FC_SRCS += CheckXtal.c
endif

ifeq '$(pulp_chip)' 'GAP'
PULP_APP_FC_SRCS += AesLib.c
PULP_APP_FC_SRCS += CheckXtal.c
endif

ifeq '$(pulp_chip)' 'wolfe'
PULP_APP_FC_SRCS += CheckXtal.c
endif

PULP_LDFLAGS += -Tlink.$(pulp_chip).ld

ifeq '$(pulp_chip)' ''
noCrt0 = 1
noLinkScript = 1
noRt = 1
else
CONFIG_OPT += options/rt/no-crt0=true options/rt/no-link-script=true options/rt/no-rt=true
endif

PULP_CFLAGS += -Os -g -DUSE_AES -fno-jump-tables

PULP_LDFLAGS += -nostdlib

include $(PULP_SDK_HOME)/install/rules/pulp_rt.mk
