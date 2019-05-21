
#
# DRIVERS
#


# HYPER

ifeq '$(CONFIG_HYPER_ENABLED)' '1'
ifeq '$(udma/hyper/version)' '1'
PULP_CFLAGS += -D__RT_HYPER_COPY_ASM=1
PULP_LIB_FC_SRCS_rt += drivers/hyper/hyperram-v$(udma/hyper/version).c
PULP_LIB_FC_ASM_SRCS_rt += drivers/hyper/hyperram-v$(udma/hyper/version)_asm.S
endif
endif

