#include "rt/rt_api.h"

rt_dev_t __rt_devices[] = {
  {"camera", 0x509, (void *)&himax_desc, {{}}},
  {"microphone", 0x8, (void *)&i2s_desc, {{}}},
  {"microphone0", 0x8, (void *)&i2s_desc, {{}}},
  {"microphone1", 0x18, (void *)&i2s_desc, {{}}},
};

int __rt_nb_devices = 4;
