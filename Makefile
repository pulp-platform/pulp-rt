PULP_LIBS = rt

PULP_PROPERTIES += fc/archi pe/archi pulp_chip pulp_chip_family soc/cluster
PULP_PROPERTIES += host/archi fc_itc udma/hyper udma udma/cam udma/i2c/version soc/fll
PULP_PROPERTIES += udma/i2s udma/uart event_unit/version perf_counters
PULP_PROPERTIES += fll/version soc/spi_master soc/apb_uart padframe/version
PULP_PROPERTIES += udma/spim udma/spim/version gpio/version rtc udma/archi
PULP_PROPERTIES += soc_eu/version compiler

include $(PULP_SDK_HOME)/install/rules/pulp_properties.mk

ifndef PULP_RT_CONFIG
PULP_RT_CONFIG = configs/pulpos.mk
endif

include $(PULP_RT_CONFIG)


ifdef CONFIG_IO_ENABLED
PULP_CFLAGS += -D__RT_USE_IO=1
endif

ifdef CONFIG_ASSERT_ENABLED
PULP_CFLAGS += -D__RT_USE_ASSERT=1
endif

ifdef CONFIG_TRACE_ENABLED
PULP_CFLAGS += -D__RT_USE_TRACE=1
endif

ifdef CONFIG_CFLAGS
PULP_CFLAGS += $(CONFIG_CFLAGS)
endif

PULP_CFLAGS += -Os -g -fno-jump-tables -Werror
ifneq '$(compiler)' 'llvm'
PULP_CFLAGS += -fno-tree-loop-distribute-patterns
endif

INSTALL_FILES += $(shell find include -name *.h)
INSTALL_FILES += $(shell find rules -name *.ld)
WS_INSTALL_FILES += include/rt/data/rt_data_bridge.h



include kernel/kernel.mk
include drivers/drivers.mk
include libs/libs.mk


include $(PULP_SDK_HOME)/install/rules/pulp_rt.mk
