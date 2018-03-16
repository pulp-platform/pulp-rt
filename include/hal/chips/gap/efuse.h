/*
 * Copyright (C) 2018 GreenWaves Technologies
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __HAL_CHIPS_GAP_EFUSE_H__
#define __HAL_CHIPS_GAP_EFUSE_H__

#include "hal/pulp.h"


#define GAP_EFUSE_INFO_REG          0
#define GAP_EFUSE_INFO2_REG         1
#define GAP_EFUSE_AES_KEY_FIRST_REG 2
#define GAP_EFUSE_AES_KEY_NB_REGS   16
#define GAP_EFUSE_AES_IV_FIRST_REG  18
#define GAP_EFUSE_AES_IV_NB_REGS    8

#define GAP_EFUSE_WAIT_XTAL_DELTA_REG_LSB     26
#define GAP_EFUSE_WAIT_XTAL_DELTA_REG_MSB     27

#define GAP_EFUSE_WAIT_XTAL_MIN_REG        28
#define GAP_EFUSE_WAIT_XTAL_MAX_REG        29

#define GAP_EFUSE_INFO_PLT_BIT    0
#define GAP_EFUSE_INFO_PLT_WIDTH  3

#define GAP_EFUSE_INFO_BOOT_BIT    3
#define GAP_EFUSE_INFO_BOOT_WIDTH  3

#define GAP_EFUSE_INFO_ENCRYPTED_BIT     6
#define GAP_EFUSE_INFO_ENCRYPTED_WIDTH   1

#define GAP_EFUSE_INFO_WAIT_XTAL_BIT     7
#define GAP_EFUSE_INFO_WAIT_XTAL_WIDTH   1

static inline unsigned int plp_efuse_info_get() {
  return plp_efuse_readByte(GAP_EFUSE_INFO_REG);
}

static inline unsigned int plp_efuse_info2_get() {
  return plp_efuse_readByte(GAP_EFUSE_INFO2_REG);
}

static inline unsigned int plp_efuse_platform_get(unsigned int infoValue) {
  return ARCHI_REG_FIELD_GET(infoValue, GAP_EFUSE_INFO_PLT_BIT, GAP_EFUSE_INFO_PLT_WIDTH);
}

static inline unsigned int plp_efuse_bootmode_get(unsigned int infoValue) {
  return ARCHI_REG_FIELD_GET(infoValue, GAP_EFUSE_INFO_BOOT_BIT, GAP_EFUSE_INFO_BOOT_WIDTH);
}

static inline unsigned int plp_efuse_encrypted_get(unsigned int infoValue) {
  return ARCHI_REG_FIELD_GET(infoValue, GAP_EFUSE_INFO_ENCRYPTED_BIT, GAP_EFUSE_INFO_ENCRYPTED_WIDTH);
}

static inline unsigned int plp_efuse_aesKey_get(int word) {
  return plp_efuse_readByte(GAP_EFUSE_AES_KEY_FIRST_REG + word);
}

static inline unsigned int plp_efuse_aesIv_get(int word) {
  return plp_efuse_readByte(GAP_EFUSE_AES_IV_FIRST_REG + word);
}

static inline unsigned int plp_efuse_wait_xtal_get(unsigned int infoValue) {
  return ARCHI_REG_FIELD_GET(infoValue, GAP_EFUSE_INFO_WAIT_XTAL_BIT, GAP_EFUSE_INFO_WAIT_XTAL_WIDTH);
}

static inline unsigned int plp_efuse_wait_xtal_delta_get() {
  return plp_efuse_readByte(GAP_EFUSE_WAIT_XTAL_DELTA_REG_LSB) | (plp_efuse_readByte(GAP_EFUSE_WAIT_XTAL_DELTA_REG_MSB) << 8);
}

static inline unsigned int plp_efuse_wait_xtal_min_get() {
  return plp_efuse_readByte(GAP_EFUSE_WAIT_XTAL_MIN_REG);
}

static inline unsigned int plp_efuse_wait_xtal_max_get() {
  return plp_efuse_readByte(GAP_EFUSE_WAIT_XTAL_MAX_REG);
}

#endif
