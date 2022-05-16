//RTDM Kernel module and char driver
//Measure both temperature and humidity of DHT11 and communicate them to the user domain

#include <linux/init.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/fs.h>

#include <asm/uaccess.h>

#include <rtdm/rtdm.h>
#include <rtdm/driver.h>

#define GPIO_DHT11 6

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brendan Signarbieux & Tom Ladune");
MODULE_VERSION("Alpha 1.0");
MODULE_DESCRIPTION("This module measures temperature and humidity thanks to the DHT11.");

rtdm_task_t task_desc;  //Definition de la tâche


void task_measure(void *arg) {

}

static int my_open_function(struct rtdm_fd *fd, int flags)
{
  rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);
  return 0;
}



static void my_close_function(struct rtdm_fd *fd)
{
  rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);
}



static int my_read_nrt_function  (struct rtdm_fd *fd, void __user *buffer, size_t lg)
{
  rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);

  rtdm_mutex_lock(&my_mtx);

  if (lg > 0) {
      if (rtdm_safe_copy_to_user(fd, buffer, &periode_char, lg) != 0) {
          rtdm_mutex_unlock(&my_mtx);
          return -EFAULT;
      }
  }

  rtdm_printk("%s: sent %d bytes, \"%.*s\"\n",  __FUNCTION__, lg, lg, buffer);

  rtdm_mutex_unlock(&my_mtx);
  return lg;
}



static int my_write_nrt_function(struct rtdm_fd *fd, const void __user *buffer, size_t lg)
{
  rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);

  rtdm_mutex_lock(&my_mtx);

  rtdm_mutex_unlock(&my_mtx);

  return lg;
}

static struct rtdm_driver my_rt_driver = {

    .profile_info = RTDM_PROFILE_INFO(my_example, RTDM_CLASS_TESTING, 1, 1),

    .device_flags   = RTDM_NAMED_DEVICE,
    .device_count   = 1,
    .context_size   = 0,

    .ops = {
        .open      = my_open_function,
        .close     = my_close_function,
        .read_nrt  = my_read_nrt_function,
        .write_nrt = my_write_nrt_function,
    },
};


static struct rtdm_device my_rt_device = {

    .driver = &my_rt_driver,
    .label  = "rtdm_oscillateur_%d",
};

static int __init initialisation(void) {
  printk(KERN_ALERT "from %s : RTDM DHT11 Driver launched.", THIS_MODULE->name);


  //Initialisation de la tâche
  if ( (err = rtdm_task_init(&task_desc, "rtdm-measure-task", task_measure, NULL, 30, periode_us*1000)) ) {
         rtdm_printk(KERN_INFO "%s.%s() : error rtdm_task_init\n", THIS_MODULE->name, __FUNCTION__);
  }
  else {
    rtdm_printk(KERN_INFO "%s.%s() : success rtdm_task_init\n", THIS_MODULE->name, __FUNCTION__);
  }

  //Initialisation du mutex

  //Initialisation du device

  printk(KERN_ALERT "from %s : RTDM DHT11 Driver initialised.", THIS_MODULE->name);
  return 0;
}

static void __exit cloture(void) {
  printk(KERN_ALERT "from %s : RTDM DHT11 Driver currently closing.", THIS_MODULE->name);

  //Libération du GPIO
  gpio_free(GPIO_DHT11);

  //Destruction de la tâche
  rtdm_task_destroy(&task_desc);

  //Destruction du mutex

  //Destruction du device

  printk(KERN_ALERT "from %s : RTDM DHT11 Driver closed.", THIS_MODULE->name);
}
<<<<<<< HEAD

module_init(initialisation);
module_exit(cloture);
=======
>>>>>>> 981b89b60030ce748b748561f85da4e5b272b574
