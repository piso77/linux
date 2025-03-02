/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * /proc/hardware/warp files to get information about your CS-Lab Warp
 * accelerator board hardware status.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/zorro.h>

static struct zorro_driver_data {
	const char *name;
	unsigned long offset;
	int absolute;	/* offset is absolute address */
} warp_driver_data[] = {
	{ .name = "PowerUP 603e+", .offset = 0xf40000, .absolute = 1 },
	{ .name = "WarpEngine 40xx", .offset = 0x40000 },
	{ .name = "A4091", .offset = 0x800000 },
	{ .name = "GForce 040/060", .offset = 0x40000 },
	{ .name = "CS-LAB Warp 1260", .offset = 0x40000 },
	{ 0 }
};

static struct zorro_device_id warp_zorro_tbl[] = {
	{
		.id = ZORRO_PROD_PHASE5_BLIZZARD_603E_PLUS,
		.driver_data = (unsigned long)&warp_driver_data[0],
	},
	{
		.id = ZORRO_PROD_MACROSYSTEMS_WARP_ENGINE_40xx,
		.driver_data = (unsigned long)&warp_driver_data[1],
	},
	{
		.id = ZORRO_PROD_CBM_A4091_1,
		.driver_data = (unsigned long)&warp_driver_data[2],
	},
	{
		.id = ZORRO_PROD_CBM_A4091_2,
		.driver_data = (unsigned long)&warp_driver_data[2],
	},
	{
		.id = ZORRO_PROD_GVP_GFORCE_040_060,
		.driver_data = (unsigned long)&warp_driver_data[3],
	},
	{
		.id = ZORRO_PROD_CSLAB_WARP_1260,
		.driver_data = (unsigned long)&warp_driver_data[4],
	},
	{ 0 }
};
MODULE_DEVICE_TABLE(zorro, warp_zorro_tbl);


#define PROC_DIR      "warp"
#define PROC_FILE     "info"

static ssize_t proc_read(struct file *file, char __user *buf,
				size_t count, loff_t *pos) {
	pr_info("%s::%d\n", __func__, __LINE__);

	return 0;
}

static ssize_t proc_write(struct file *file, const char __user *buf,
				 size_t len, loff_t * pos) {
	pr_info("%s::%d\n", __func__, __LINE__);

	return 0;
}

static const struct proc_ops proc_fops =
{
	.proc_read = proc_read,
	.proc_write = proc_write,
};

static int warp_init_one(struct zorro_dev *z,
				const struct zorro_device_id *ent) {
	static struct proc_dir_entry *dir;
	static struct proc_dir_entry *file;

	pr_info("%s::%d\n", __func__, __LINE__);
	dir = proc_mkdir(PROC_DIR, NULL);
	if (!dir) {
		pr_err("can't create %s\n", PROC_DIR);
		return -ENOMEM;
	}
	file = proc_create(PROC_FILE, 0644, dir,
			   &proc_fops);
	if (!file) {
		pr_err("can't create %s\n", PROC_FILE);
		remove_proc_entry(PROC_DIR, NULL);
		return -ENOMEM;
	}
	return 0;

}

static void warp_remove_one(struct zorro_dev *z) {
	pr_info("%s::%d\n", __func__, __LINE__);

	remove_proc_entry(PROC_FILE, NULL);
	remove_proc_entry(PROC_DIR, NULL);
}

static struct zorro_driver warp_driver = {
	.name	  = "cslab_warp",
	.id_table = warp_zorro_tbl,
	.probe	  = warp_init_one,
	.remove	  = warp_remove_one,
};

static int __init warp_init(void)
{
	return zorro_register_driver(&warp_driver);
}

static void __exit warp_exit(void)
{
	zorro_unregister_driver(&warp_driver);
}

module_init(warp_init);
module_exit(warp_exit);

MODULE_AUTHOR("Paolo Pisati <p.pisati@gmail.com>");
MODULE_LICENSE("GPL");
