/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * /proc/hardware/warp files to get information about your CS-Lab Warp
 * accelerator board hardware status.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>

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

static int __init warp_init(void) {
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

static void __exit warp_cleanup(void) {
	pr_info("%s::%d\n", __func__, __LINE__);

	remove_proc_entry(PROC_FILE, NULL);
	remove_proc_entry(PROC_DIR, NULL);
}

module_init(warp_init);
module_exit(warp_cleanup);

MODULE_AUTHOR("Paolo Pisati <p.pisati@gmail.com>");
MODULE_LICENSE("GPL");
