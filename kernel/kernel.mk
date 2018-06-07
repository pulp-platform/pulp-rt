
# In case the architecture does not have any fabric controller, the FC code will run
# on pe0 of cluster0
fc_archi = $(fc/archi)
ifeq '$(fc/archi)' ''
fc_archi = $(pe/archi)
endif


PULP_LIB_FC_SRCS_rt     += kernel/init.c kernel/alloc.c kernel/alloc_extern.c \
  kernel/thread.c kernel/events.c kernel/dev.c kernel/irq.c kernel/debug.c \
  kernel/utils.c kernel/error.c kernel/bridge.c
PULP_LIB_FC_ASM_SRCS_rt += kernel/$(fc_archi)/crt0.S kernel/$(fc_archi)/thread.S

ifeq '$(CONFIG_TIME_ENABLED)' '1'
PULP_LIB_FC_SRCS_rt     += kernel/time.c kernel/time_irq.c
endif



ifneq '$(fc_archi)' 'or1k'
PULP_LIB_FC_ASM_SRCS_rt += kernel/$(fc_archi)/sched.S kernel/$(fc_archi)/vectors.S
endif

ifneq '$(udma)' ''
PULP_LIB_FC_SRCS_rt     += kernel/periph-v$(udma/archi).c
PULP_LIB_FC_ASM_SRCS_rt += kernel/$(fc_archi)/udma.S
endif

ifneq '$(soc/fll)' ''
ifneq '$(pulp_chip)' 'gap'
PULP_LIB_FC_SRCS_rt     += kernel/fll-v$(fll/version).c
endif
PULP_LIB_FC_SRCS_rt     += kernel/freq-v$(fll/version).c
endif

ifneq '$(soc_eu/version)' ''
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
PULP_LIB_FC_SRCS_rt += kernel/vega/maestro.c
endif

ifeq '$(pulp_chip)' 'gap'
PULP_LIB_FC_SRCS_rt += kernel/gap/maestro.c kernel/gap/pmu_driver.c
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


INSTALL_TARGETS += $(PULP_SDK_INSTALL)/lib/$(pulp_chip)/crt0.o


ifeq '$(pulp_chip)' 'oprecompkw'

$(PULP_SDK_INSTALL)/lib/$(pulp_chip)/crt0.o: $(CONFIG_BUILD_DIR)/rt/fc/kernel/oprecompkw/crt0.o
	install -D $< $@

else

$(PULP_SDK_INSTALL)/lib/$(pulp_chip)/crt0.o: $(CONFIG_BUILD_DIR)/rt/fc/kernel/$(fc_archi)/crt0.o
	install -D $< $@

endif
