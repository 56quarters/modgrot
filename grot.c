#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Yes, hello");
MODULE_AUTHOR("56quarters");
MODULE_VERSION("0.1");
MODULE_SUPPORTED_DEVICE("grot");

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

static int major;

struct grot_info {
    struct cdev cdev;

    char msg[GROT_MSG_SIZE];
    char custom;

    char busy;
    char eof;
};

static ssize_t grot_info_write_msg(struct grot_info *g, const char *buf, ssize_t len)
{
    if (len >= GROT_MSG_SIZE - 1) {
        pr_alert("grot: input too large");
        return -EINVAL;
    }

    if (copy_from_user(g->msg, buf, len)) {
        pr_alert("grot: copy_from_user");
    } else {
        g->msg[len] = '\0';
        g->custom = 1;
    }
    
    return len;
}

static ssize_t grot_info_read_msg(struct grot_info *g, char *buf, size_t n)
{
    ssize_t len;
    if (g->eof) {
        return 0;
    }

    if (!g->custom) {
        pr_info("grot: setting default msg");
        strcpy(g->msg, GROT_DEFAULT_MSG);
    }

    len = strlen(g->msg);
    if (copy_to_user(buf, g->msg, len)) {
        pr_alert("grot: copy_to_user");
    }

    g->custom = 0;
    g->eof = 1;
    return len;
}

static ssize_t grot_dev_read(struct file *f, char *buf, size_t n, loff_t *of)
{
    struct grot_info *info;
    info = (struct grot_info *)f->private_data;
    return grot_info_read_msg(info, buf, n);
}

static ssize_t grot_dev_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    struct grot_info *info;
    info = (struct grot_info *)f->private_data;    
    return grot_info_write_msg(info, buf, len);
}

static int grot_dev_open(struct inode *ino, struct file *f)
{    
    struct grot_info *info;
    info = container_of(ino->i_cdev, struct grot_info, cdev);
    if (info->busy) {
        return -EBUSY;
    }

    info->eof = 0;
    info->busy = 1;
    
    f->private_data = info;
    try_module_get(THIS_MODULE); /* tell the system that we're live */
    pr_info("grot: open");
    return 0;
}

static int grot_dev_release(struct inode *ino, struct file *f)
{
    struct grot_info *info;
    info = (struct grot_info *)f->private_data;

    info->busy = 0;
    
    module_put(THIS_MODULE); /* we're finished */
    pr_info("grot: release");
    return 0;
}

static void kexit(void)
{
    unregister_chrdev(major, "grot");
    return;
}

static int kinit(void)
{
    /* register as a character device */
    major = register_chrdev(0, "grot", &fops);

    if (major < 0) {
        pr_alert("grot: register_chrdev %d", major);
    }

    return 0;
}

module_init(kinit);
module_exit(kexit);
