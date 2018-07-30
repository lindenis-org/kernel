/*
 * The key define in SUNXI Efuse.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * Matteo <duanmintao@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __SUNXI_SID_EFUSE_H__
#define __SUNXI_SID_EFUSE_H__

#include <linux/sunxi-sid.h>

struct efuse_key_map {
	char name[SUNXI_KEY_NAME_LEN];
	int offset;
	int size;	 /* the number of key in bits */
	int read_flag_shift;
	int burn_flag_shift;
	int public;
	int reserve[3];
};

#if defined(CONFIG_ARCH_SUN8IW11)

#define EFUSE_IS_PUBLIC

static struct efuse_key_map key_maps[] = {
/* Name                  Offset Size ReadFlag BurnFlag Public Reserve */
{EFUSE_CHIPID_NAME,      0x0,   128, -1,      -1,      1,     {0} },
{EFUSE_IN_NAME,          0x10,  256, 23,      11,      0,     {0} },
{EFUSE_SSK_NAME,         0x30,  128, 22,      10,      0,     {0} },
{EFUSE_THM_SENSOR_NAME,  0x40,  32,  -1,      -1,      1,     {0} },
{EFUSE_FT_ZONE_NAME,     0x44,  64,  -1,      -1,      1,     {0} },
{EFUSE_TV_OUT_NAME,      0x4C,  128, -1,      -1,      0,     {0} },
{EFUSE_RSSK_NAME,        0x5C,  256, 20,      8,       0,     {0} },
{EFUSE_HDCP_HASH_NAME,   0x7C,  128, -1,      9,       0,     {0} },
{EFUSE_RESERVED_NAME,    0x90,  896, -1,      -1,      0,     {0} },
{"",                     0,     0,   0,       0,       0,     {0} },
};

static struct efuse_key_map key_map_rd_pro = {
EFUSE_RD_PROTECT_NAME,  0x8C,   32,  -1,      -1,      1,     {0} };
static struct efuse_key_map key_map_wr_pro = {
EFUSE_WR_PROTECT_NAME,  0x8C,   32,  -1,      -1,      1,     {0} };

#elif defined(CONFIG_ARCH_SUN8IW12)

static struct efuse_key_map key_maps[] = {
/* Name                  Offset Size ReadFlag BurnFlag Public Reserve */
{EFUSE_CHIPID_NAME,	 0x0,	128, 16,       0,       1,     {0} },
{EFUSE_BROM_CONF_NAME,	 0x10,	16,  17,       1,       1,     {0} },
{EFUSE_BROM_TRY_NAME,	 0x12,	16,  17,       1,       1,     {0} },
{EFUSE_THM_SENSOR_NAME,  0x14,  96,  18,       2,       1,     {0} },
{EFUSE_FT_ZONE_NAME,	 0x20,	128, 19,       3,       1,     {0} },
{EFUSE_ROTPK_NAME,	 0x30,	256, 20,       4,       0,     {0} },
{EFUSE_NV1_NAME,	 0x50,	32,  21,       5,       0,     {0} },
{EFUSE_TVE_NAME,	 0x54,	32,  22,       6,       0,     {0} },
{EFUSE_ANTI_BLUSH_NAME,  0x58,  32,  23,       7,       0,     {0} },
{EFUSE_RESERVED_NAME,	 0x5C,	768, 24,       8,       0,     {0} },
{"",			 0,	0,   0,        0,       0,     {0} },
};

static struct efuse_key_map key_map_rd_pro = {
EFUSE_RD_PROTECT_NAME,  0xBC,   32,   25,      9,      1,     {0} };
static struct efuse_key_map key_map_wr_pro = {
EFUSE_WR_PROTECT_NAME,  0xBC,   32,   25,      9,      1,     {0} };

#else

#define EFUSE_IS_PUBLIC
#define EFUSE_HAS_NO_RW_PROTECT

static struct efuse_key_map key_maps[] = {
/* Name                  Offset Size ReadFlag BurnFlag Public Reserve */
{EFUSE_CHIPID_NAME,      0x0,   128, -1,      -1,      1,     {0} },
{"",                     0,     0,   0,       0,       0,     {0} }
};

#endif

#endif /* end of __SUNXI_SID_EFUSE_H__ */

