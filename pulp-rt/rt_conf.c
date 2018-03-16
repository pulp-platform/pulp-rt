#include "rt/rt_api.h"

rt_dev_t __rt_devices[] = {
  {"camera", 0x509, (void *)&himax_desc, {{}}},
};

int __rt_nb_devices = 1;
