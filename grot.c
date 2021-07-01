/* modgrot - Yes, hello
 *
 * Copyright (C) 2021 Nick Pillitteri
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/kdev_t.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Yes, hello");
MODULE_AUTHOR("56quarters");
MODULE_VERSION("0.1");
MODULE_SUPPORTED_DEVICE("grot");

#define GROT_DEV_NAME "grot"
#define GROT_DEV_CLASS "grot"
#define GROT_MSG_SIZE 255
#define GROT_DEFAULT_MSG "Yes, hello\n"

static ssize_t grot_dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t grot_dev_write(struct file *, const char *, size_t, loff_t *);
static int     grot_dev_open(struct inode *, struct file *);
static int     grot_dev_release(struct inode *, struct file *);

/* callbacks for file operations */
static struct file_operations fops = {
    .read = grot_dev_read,
    .write = grot_dev_write,
    .open = grot_dev_open,
    .release = grot_dev_release
};


/* major / minor device number */
static dev_t dev = 0;

/* device class created on module insert */
static struct class *dev_class;

/* lock for protecting the info struct */
static struct mutex lock;

struct grot_info {
    struct cdev cdev;

    /* has one-time initialization of this struct been performed */
    u8 init;

    /* has all data been read from the device */
    u8 eof;

    /* is there a custom message to use instead of the default */
    u8 custom;

    /* custom or default message to emit */
    char *msg;
};

static ssize_t grot_dev_read(struct file *f, char *buf, size_t n, loff_t *of)
{
    struct grot_info *info = f->private_data;
    char *msg = "";
    ssize_t len = 0;

    if (mutex_lock_interruptible(&lock)) {
        return -EINTR;
    }

    if (!info->eof) {
        if (info->custom) {
            msg = info->msg;
        } else {
            msg = GROT_DEFAULT_MSG;
        }

        len = strlen(msg);
        if (copy_to_user(buf, msg, len)) {
            pr_alert("grot: copy_to_user");
            len = -EFAULT;
        }
    }

    info->custom = 0;
    info->eof = 1;

    mutex_unlock(&lock);
    return len;
}

static ssize_t grot_dev_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    struct grot_info *info = f->private_data;
    ssize_t written = 0;

    if (len >= GROT_MSG_SIZE - 1) {
        pr_alert("grot: input too large");
        return -EINVAL;
    }

    if (mutex_lock_interruptible(&lock)) {
        return -EINTR;
    }

    if (copy_from_user(info->msg, buf, len)) {
        pr_alert("grot: copy_from_user");
        written = -EFAULT;
    } else {
        info->msg[len] = '\0';
        info->custom = 1;
        written = len;
    }

    mutex_unlock(&lock);
    return written;
}


static int grot_dev_open(struct inode *ino, struct file *f)
{
    struct grot_info *info;

    if (mutex_lock_interruptible(&lock)) {
        return -EINTR;
    }

    info = container_of(ino->i_cdev, struct grot_info, cdev);

    if (!info->init) {
        pr_info("grot: one-time init");
        info->msg = kmalloc(GROT_MSG_SIZE, GFP_KERNEL);
        info->init = 1;
    }

    info->eof = 0;
    f->private_data = info;
    try_module_get(THIS_MODULE);
    pr_info("grot: open");

    mutex_unlock(&lock);
    return 0;
}

static int grot_dev_release(struct inode *ino, struct file *f)
{
    module_put(THIS_MODULE);
    pr_info("grot: release");

    return 0;
}

static void kexit(void)
{
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    unregister_chrdev(dev, GROT_DEV_NAME);
}

static int kinit(void)
{

    int major = register_chrdev(0, GROT_DEV_NAME, &fops);
    if (major < 0) {
        pr_alert("grot: register_chrdev %d", major);
        return major;
    }

    dev = MKDEV(major, 0);
    dev_class = class_create(THIS_MODULE, GROT_DEV_CLASS);
    if (NULL == dev_class) {
        pr_alert("grot: class_create");
        goto out_class;
    }

    if (NULL == device_create(dev_class, NULL, dev, NULL, GROT_DEV_NAME)) {
        pr_alert("grot: device_create");
        goto out_device;
    }

    pr_info("grot: major=%d minor=%d", MAJOR(dev), MINOR(dev));
    mutex_init(&lock);
    return 0;

out_device:
    class_destroy(dev_class);

out_class:
    unregister_chrdev(dev, GROT_DEV_NAME);
    return -1;
}

module_init(kinit);
module_exit(kexit);
