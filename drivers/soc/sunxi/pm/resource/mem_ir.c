/*
 * drivers/soc/sunxi/pm/resource/mem_ir.c
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Yanggq <yanggq@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/types.h>

__s32 mem_ir_init(void)
{
	return 0;
}

__s32 mem_ir_exit(void)
{
	return 0;
}

__s32 mem_ir_detect(void)
{
	return 0;
}

__s32 mem_ir_verify(void)
{
	return -1;
}
