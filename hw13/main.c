#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "pingpong"
#define CLASS_NAME "pingpong"
#define BUF_SIZE 1024

static dev_t devno;
static struct cdev cdev_obj;
static struct class *dev_class;
static struct device *dev;

static char buffer[BUF_SIZE];
static size_t data_size = 0;

static int my_open(struct inode *inode, struct file *file) { return 0; }

static int my_release(struct inode *inode, struct file *file) { return 0; }

static ssize_t my_read(struct file *file, char __user *buf, size_t count,
                       loff_t *ppos) {
  if (*ppos >= data_size)
    return 0; // конец данных

  if (count > data_size - *ppos)
    count = data_size - *ppos;

  if (copy_to_user(buf, buffer + *ppos, count))
    return -EFAULT;

  *ppos += count;
  return count;
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count,
                        loff_t *ppos) {
  if (count > BUF_SIZE)
    count = BUF_SIZE;

  if (copy_from_user(buffer, buf, count))
    return -EFAULT;

  data_size = count;
  return count;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static int __init my_init(void) {
  int ret;

  ret = alloc_chrdev_region(&devno, 0, 1, DEVICE_NAME);
  if (ret < 0) {
    pr_err("alloc_chrdev_region failed\n");
    return ret;
  }

  cdev_init(&cdev_obj, &fops);
  cdev_obj.owner = THIS_MODULE;

  ret = cdev_add(&cdev_obj, devno, 1);
  if (ret < 0) {
    pr_err("cdev_add failed\n");
    unregister_chrdev_region(devno, 1);
    return ret;
  }

  dev_class = class_create(THIS_MODULE, CLASS_NAME);
  if (IS_ERR(dev_class)) {
    cdev_del(&cdev_obj);
    unregister_chrdev_region(devno, 1);
    return PTR_ERR(dev_class);
  }

  dev = device_create(dev_class, NULL, devno, NULL, DEVICE_NAME);
  if (IS_ERR(dev)) {
    class_destroy(dev_class);
    cdev_del(&cdev_obj);
    unregister_chrdev_region(devno, 1);
    return PTR_ERR(dev);
  }

  pr_info("pingpong device loaded: /dev/%s\n", DEVICE_NAME);
  return 0;
}

static void __exit my_exit(void) {
  device_destroy(dev_class, devno);
  class_destroy(dev_class);
  cdev_del(&cdev_obj);
  unregister_chrdev_region(devno, 1);
  pr_info("pingpong device unloaded\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Артур");
MODULE_DESCRIPTION("Simple ping-pong char device");
