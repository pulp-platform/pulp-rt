PULP_LIBS = rt rtio bench omp

PULP_PROPERTIES += fc/archi pe/archi pulp_chip pulp_chip_family soc/cluster
PULP_PROPERTIES += host/archi fc_itc udma/hyper udma udma/cam udma/i2c soc/fll
PULP_PROPERTIES += udma/i2s udma/uart event_unit/version perf_counters
PULP_PROPERTIES += fll/version soc/spi_master soc/apb_uart padframe/version
PULP_PROPERTIES += udma/spim udma/spim/version gpio/version rtc

include $(PULP_SDK_HOME)/install/rules/pulp_properties.mk

# In case the architecture does not have any fabric controller, the FC code will run
# on pe0 of cluster0
fc_archi = $(fc/archi)
ifeq '$(fc/archi)' ''
fc_archi = $(pe/archi)
endif



PULP_CFLAGS += -Os -g -fno-jump-tables -fno-tree-loop-distribute-patterns -Werror

INSTALL_FILES += $(shell find include -name *.h)
WS_INSTALL_FILES += include/rt/data/rt_data_bridge.h
INSTALL_TARGETS += $(PULP_SDK_INSTALL)/lib/$(pulp_chip)/crt0.o

ifneq '$(host/archi)' ''

PULP_LIB_HOST_SRCS_rt     += kernel/init.c
PULP_LIB_HOST_ASM_SRCS_rt += kernel/$(host/archi)/crt0.S

PULP_LIB_HOST_SRCS_rtio   += libs/io/tinyprintf.c libs/io/io.c kernel/freq-v$(fll/version).c kernel/fll-v$(fll/version).c kernel/utils.c

$(PULP_SDK_INSTALL)/lib/$(pulp_chip)/crt0.o: $(CONFIG_BUILD_DIR)/rt/host/kernel/riscv/crt0.o
	@mkdir -p `dirname $@`
	install -D $< $@

PULP_LIB_HOST_SRCS_bench   += libs/bench/bench.c

else

PULP_LIB_FC_SRCS_rt     += kernel/init.c kernel/alloc.c kernel/alloc_extern.c \
  kernel/thread.c kernel/events.c kernel/dev.c kernel/irq.c kernel/debug.c \
  kernel/time.c kernel/time_irq.c kernel/utils.c kernel/error.c kernel/bridge.c
PULP_LIB_FC_ASM_SRCS_rt += kernel/$(fc_archi)/crt0.S kernel/$(fc_archi)/thread.S

ifneq '$(fc_archi)' 'or1k'
PULP_LIB_FC_ASM_SRCS_rt += kernel/$(fc_archi)/sched.S kernel/$(fc_archi)/vectors.S
endif

ifneq '$(udma)' ''
PULP_LIB_FC_SRCS_rt     += kernel/periph.c
PULP_LIB_FC_ASM_SRCS_rt += kernel/$(fc_archi)/udma.S
endif

ifneq '$(soc/fll)' ''
PULP_LIB_FC_SRCS_rt     += kernel/fll-v$(fll/version).c kernel/freq-v$(fll/version).c
endif

ifneq '$(udma)' ''
ifneq '$(fc_itc)' ''
PULP_LIB_FC_ASM_SRCS_rt += kernel/$(fc_archi)/soc_event_itc.S
else
PULP_LIB_FC_ASM_SRCS_rt += kernel/$(fc_archi)/soc_event_eu.S
endif
endif

PULP_LIB_CL_ASM_SRCS_rt += kernel/$(fc_archi)/pe-eu-v$(event_unit/version).S

ifeq '$(event_unit/version)' '1'
PULP_LIB_FC_SRCS_rt += kernel/riscv/pe-eu-v1-entry.c
endif

ifeq '$(pulp_chip_family)' 'wolfe'
PULP_LIB_FC_SRCS_rt += kernel/wolfe/maestro.c
endif

ifeq '$(pulp_chip_family)' 'devchip'
PULP_LIB_FC_SRCS_rt += kernel/wolfe/maestro.c
endif

ifeq '$(pulp_chip_family)' 'vega'
PULP_LIB_FC_SRCS_rt += kernel/wolfe/maestro.c
endif

ifeq '$(pulp_chip)' 'gap'
PULP_LIB_FC_SRCS_rt += kernel/gap/maestro.c
endif


PULP_LIB_FC_SRCS_rt += kernel/cluster.c

ifneq '$(perf_counters)' ''
PULP_LIB_FC_SRCS_rt += kernel/perf.c
endif

ifneq '$(soc/cluster)' ''
ifneq '$(event_unit/version)' '1'
PULP_LIB_CL_SRCS_rt += kernel/sync_mc.c
endif
endif




#
# DRIVERS
#


# PADS

ifneq '$(padframe/version)' ''
PULP_LIB_FC_SRCS_rt += drivers/pads/pads-v$(padframe/version).c
endif


# SPIM

ifneq '$(soc/spi_master)' ''
PULP_LIB_FC_SRCS_rt += drivers/spim/spim-v0.c
endif

ifneq '$(udma/spim)' ''
PULP_LIB_FC_SRCS_rt += drivers/spim/spim-v$(udma/spim/version).c drivers/spim/spiflash-v$(udma/spim/version).c drivers/spim/spiflash-v$(udma/spim/version)-asm.c
endif


# HYPER

ifneq '$(udma/hyper)' ''
PULP_LIB_FC_SRCS_rt += drivers/hyper/hyperram.c drivers/hyper/hyperflash.c
endif


# UART

ifneq '$(udma/uart)' ''
PULP_LIB_FC_SRCS_rt += drivers/uart/uart.c
endif

ifneq '$(soc/apb_uart)' ''
PULP_LIB_FC_SRCS_rt += drivers/uart/uart-v0.c
endif


# I2S

ifneq '$(udma/i2s)' ''
PULP_LIB_FC_SRCS_rt += drivers/i2s/i2s.c
endif


# CAM

ifneq '$(udma/cam)' ''
PULP_LIB_FC_SRCS_rt += drivers/camera/himax.c drivers/camera/ov7670.c drivers/camera/camera.c
endif


# I2C

ifneq '$(udma/i2c)' ''
PULP_LIB_FC_SRCS_rt += drivers/i2c/i2c.c
endif


# RTC

ifneq '$(rtc)' ''
PULP_LIB_FC_SRCS_rt += drivers/dolphin/rtc.c
PULP_LIB_FC_ASM_SRCS_rt += drivers/dolphin/rtc_asm.S
endif


# GPIO

ifneq '$(gpio/version)' ''
PULP_LIB_FC_SRCS_rt += drivers/gpio/gpio-v$(gpio/version).c
endif



PULP_LIB_FC_SRCS_rt += drivers/flash.c drivers/read_fs.c

PULP_LIB_FC_SRCS_rtio   += libs/io/tinyprintf.c libs/io/io.c

PULP_LIB_FC_SRCS_bench   += libs/bench/bench.c

ifeq '$(pulp_chip)' 'oprecompkw'

$(PULP_SDK_INSTALL)/lib/$(pulp_chip)/crt0.o: $(CONFIG_BUILD_DIR)/rt/fc/kernel/oprecompkw/crt0.o
	install -D $< $@

else

$(PULP_SDK_INSTALL)/lib/$(pulp_chip)/crt0.o: $(CONFIG_BUILD_DIR)/rt/fc/kernel/$(fc_archi)/crt0.o
	install -D $< $@

endif

endif



ifneq '$(event_unit/version)' '1'
PULP_LIB_CL_ASM_SRCS_omp   += libs/omp/omp_asm.S
PULP_LIB_CL_SRCS_omp   += libs/omp/omp_wrapper_gcc.c
endif



include $(PULP_SDK_HOME)/install/rules/pulp_rt.mk
