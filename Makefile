PULP_LIBS = rt

PULP_PROPERTIES += fc/archi pe/archi pulp_chip pulp_chip_family chip/cluster
PULP_PROPERTIES += host/archi fc_itc udma/hyper udma/hyper/version udma udma/cpi udma/i2c/version soc/fll
PULP_PROPERTIES += udma/i2s/version udma/uart event_unit/version perf_counters
PULP_PROPERTIES += fll/version soc/spi_master soc/apb_uart padframe/version
PULP_PROPERTIES += udma/spim udma/spim/version gpio/version rtc udma/archi
PULP_PROPERTIES += soc_eu/version compiler rtc/version l2/size

include $(TARGET_INSTALL_DIR)/rules/pulp_properties.mk

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

ifdef CONFIG_PROFILE_ENABLED
PULP_CFLAGS += -D__RT_USE_PROFILE=1
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





HAL_FILES := $(shell plpfiles copy --item=hal_src_files)
PULP_LIB_FC_SRCS_rt += $(HAL_FILES)



include kernel/kernel.mk
include drivers/drivers.mk
include libs/libs.mk


include $(TARGET_INSTALL_DIR)/rules/pulp_rt.mk


define halSrcRules

$(CONFIG_BUILD_DIR)/fc/$(1): $(TARGET_INSTALL_DIR)/src/$(2)
	@mkdir -p `dirname $$@`
	$(PULP_FC_CC) $(rt_cl_cflags) -MMD -MP -c $$< -o $$@

endef

$(foreach file, $(HAL_FILES), $(eval $(call halSrcRules,$(patsubst %.c,%.o,$(file)),$(file))))


ifeq '$(pulp_chip_family)' 'vega'
CHIP_TARGETS += gen_linker_script
endif


build_rt: build $(CHIP_TARGETS)

clean_all:
	make fullclean
	make PULP_RT_CONFIG=configs/pulpos_profile.mk fullclean

build_all:
	make build_rt install
	make PULP_RT_CONFIG=configs/pulpos_profile.mk build install

gen_linker_script:
	./rules/process_linker_script --input=rules/$(pulp_chip_family)/link.ld.in --output=$(TARGET_INSTALL_DIR)/rules/vega/link.ld

vega: $(CONFIG_BUILD_DIR)/link.ld