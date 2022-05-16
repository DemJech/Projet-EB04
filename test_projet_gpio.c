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

static int periode_us = 1000;
static char periode_char;
module_param(periode_us, int, 0644);

rtdm_task_t task_desc;

static rtdm_mutex_t my_mtx;

static int count = 0;

static int MAX_CNT = 320;
static int PULSES_CNT = 41;

#define TRUE 1
#define FALSE 0

// Pin 5 du GPIO du Raspberry
#define GPIO_DHT11 6

void task_oscillateur(void *arg){

    static int value = 0;

    while(!rtdm_task_should_stop()){

        gpio_set_value(GPIO_OSCILLATEUR, value);
        value = 1 - value;

        rtdm_task_wait_period(NULL);
    }
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

    if (lg > 0) {
        if (rtdm_safe_copy_from_user(fd,
                          &periode_char,
                          buffer, lg) != 0) {
            rtdm_mutex_unlock(&my_mtx);
            return -EFAULT;
        }

        switch (periode_char){
            case '1':
                periode_us=50;
                break;
            case '2':
                periode_us=40;
                break;
            case '3':
                periode_us=30;
                break;
            case '4':
                periode_us=20;
                break;
            case '5':
                periode_us=10;
                break;
            default:
                periode_us=50;
        }

        rtdm_printk(KERN_INFO "%s.%s() : driver receive %c, periode_us = %d\n", THIS_MODULE->name, __FUNCTION__, periode_char, periode_us);

        rtdm_task_set_period(&task_desc, 0, periode_us*1000);

    }
    else {
        rtdm_printk(KERN_INFO "%s.%s() : device receive error\n", THIS_MODULE->name, __FUNCTION__);
    }

    rtdm_printk("==> Received from Linux %d bytes : %.*s\n", lg, lg, buffer);

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


static int __init init_driver_oscillateur (void)
{
	int err;

	rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);

	if ((err = gpio_request(GPIO_DHT11, THIS_MODULE->name)) != 0) {
		return err;
	}
	if ((err = gpio_direction_output(GPIO_DHT11, 1)) != 0) { //Envoi 1 20ms 
		gpio_free(GPIO_DHT11);
		return err;
	}
	rtdm_task_wait_period(200000000);

	if ((err = gpio_direction_output(GPIO_DHT11, 0)) != 0) { //Envoi 0 18ms pour Start
		gpio_free(GPIO_DHT11);
		return err;
	}
	rtdm_task_wait_period(180000000);

	if ((err = gpio_direction_input(GPIO_DHT11)) != 0) { //Mise en mode lecture
		gpio_free(GPIO_DHT11);
		return err;
	}

	rtdm_task_wait_period(1000000000); //Courte attente

	count = 0;

	while (gpio_get_value(GPIO_DHT11) != 0) {
		count++;
		if (count > MAX_CNT) {
			rtdm_printk(KERN_INFO "pullup by host 20-40us failed\n");
			return none;
		}
	}

	int pulse_cnt[2*PULSES_CNT];
	int fix_crc = FALSE;

	for (i=0; i<=(2*PULSES_CNT); i+=2) {
		while (gpio_get_value(GPIO_DHT11) == FALSE) {
			pulse_cnt[i] += 1;
			if (pulse_cnt[i] > MAX_CNT) {
				rtdm_printk(KERN_INFO "pullup by DHT timeout %d\n", i);
				return none;   		
			}
		}
		while (gpio_get_value(GPIO_DHT11) != 0) {
			pulse_cnt[i+1] += 1;
			if (pulse_cnt[i+1] > MAX_CNT) {
				if (i == 2*(PULSES_CNT-1)) {
				}
				else {
					rtdm_printk(KERN_INFO "pullup by DHT timeout %d\n", i);
					return none;   	
				}	
			}
		}  	    	
	}
	int total_cnt = 0;
	
	for (i=2; i<=(2*PULSES_CNT); i+=2) {
		total_cnt += pulse_cnt[i];
	}

	int average_cnt = total_cnt/(PULSES_CNT-1); //Mesure la moyenne des signaux a l'état bas et à l'état haut

	char data[PULSES_CNT];

	int m=0;
	int data0, data1, data2, data3, data4;
	int pow=1;
	for (i=3; i<=(2*PULSES_CNT); i+=2) {	
		int nb;
		
		// En comparant avec moyenne, si état haut > état bas c'est un 1, sinon c'est un 0
		if (pulse_cnt[i] > average_cnt) { 
			nb = 1;
		}
		else {
			nb = 0;
		}
		
		// Sépare les 41 caractères de la chaine en 5 octets séparés data0, data1, data2, data3, data4
		if (m/8 == 0) { 	
			data0 += nb*pow;
		}
		else if (m/8 == 1) {
			data1 += nb*pow;
		}		
		else if (m/8 == 2) {
			data2 += nb*pow;
		}
		else if (m/8 == 3) {
			data3 += nb*pow;
		}
		else if (m/8 == 4) {
			data4 += nb*pow;
		}		
		m++;
		pow = pow*2;
	}

	/*if ((fix_crc == TRUE) && (data4 != ((data0 + data1 + data2 + data3) & 0xFF)) {
		data4 = data4 ^ 0x01; //Pair ou impair ?
		if ((data4 & 0x01) == TRUE) {
			data = ; //a completer (ligne 182 du seeed_dht.py)
		}			
		else {
			data = ; //a completer (ligne 182 du seeed_dht.py)
		}
	}*/

	if (data4 == ((data0 + data1 + data2 + data3) & 0xFF)) {
		int humi = data0; //Affecte l'octet data0 pour l'info de l'humidité
		int temp = data2; //Affecte l'octet data2 pour l'info de la température	
	}
	else {
		rtdm_printk(KERN_INFO "checksum error \n");
		return none;
	}

	rtdm_printk(KERN_INFO "humi = %d% , temp = %d°C \n", humi, temp);
	return humi, temp; 
	

    if ( (err = rtdm_task_init(&task_desc, "rtdm-oscillateur-task", task_oscillateur, NULL, 30, periode_us*1000)) ) {
         rtdm_printk(KERN_INFO "%s.%s() : error rtdm_task_init\n", THIS_MODULE->name, __FUNCTION__);
    }

    else  rtdm_printk(KERN_INFO "%s.%s() : success rtdm_task_init\n", THIS_MODULE->name, __FUNCTION__);

    rtdm_mutex_init(&my_mtx);
    rtdm_dev_register(&my_rt_device);

    return 0;
}


static void __exit exit_driver_oscillateur (void)
{
    rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);

    gpio_free(GPIO_OSCILLATEUR);

    rtdm_task_destroy(&task_desc);

    rtdm_dev_unregister(&my_rt_device);
    rtdm_mutex_destroy(&my_mtx);
}


module_init(init_driver_oscillateur);
module_exit(exit_driver_oscillateur);
MODULE_LICENSE("GPL");

