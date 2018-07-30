/*
 * drivers/mpp/sunxi-mpp.c
 *
 * Copyright (c) 2014 softwinner.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/cpu.h>
#include <linux/of.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/mpp.h>

enum {
	DEBUG_NONE = 0,
	DEBUG_MPP = 1,
};
static int debug_mask = DEBUG_NONE;
module_param(debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);
#define MPP_DBG(mask, format, args...)				\
	do {								\
		if (mask & debug_mask)					\
			pr_info("[mpp] "format, ##args);		\
	} while (0)

#define MPP_ERR(format, args...) \
	pr_err("[mpp] error:"format, ##args)

#ifdef CONFIG_DEBUG_FS
struct dentry *debugfs_mpp_root;
EXPORT_SYMBOL_GPL(debugfs_mpp_root);

static int __init mpp_debugfs_init(void)
{
	MPP_DBG(DEBUG_MPP, "call %s. \n", __func__);
	WARN_ON(!debugfs_initialized());

	debugfs_mpp_root = debugfs_create_dir("mpp", 0);
	if (!debugfs_mpp_root)
		return -ENOMEM;

	return 0;
}

static void __exit mpp_debugfs_exit(void)
{
	MPP_DBG(DEBUG_MPP, "call %s. \n", __func__);
	debugfs_remove_recursive(debugfs_mpp_root);
}

subsys_initcall(mpp_debugfs_init);
module_exit(mpp_debugfs_exit);
#endif /* CONFIG_DEBUG_FS */

MODULE_DESCRIPTION("mpp driver for sunxi SOCs");
MODULE_LICENSE("GPL");
