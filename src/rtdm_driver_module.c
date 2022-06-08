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
#define TRUE 1
#define FALSE 0

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brendan Signarbieux & Tom Ladune");
MODULE_VERSION("Alpha 1.0");
MODULE_DESCRIPTION("This module measures temperature and humidity thanks to the DHT11.");

static int count = 0;
static int MAX_CNT = 320;
static int PULSES_CNT = 41;

static struct Measures {      //Structure de donnée obtenue par le capteur DHT11
    int temperature;
    int humidity;
} measures;

static struct Order {        //Structure de données des paramètres de fonctionnement du driver
  int temperature_min;
  int temperature_max;
  int periode_ms;
} order;

rtdm_task_t task_desc;  //Definition de la tâche
static rtdm_mutex_t my_mtx; //Definition du mutex

void task_measure(void *arg) {
  rtdm_printk(KERN_INFO "%s.%s(): has launched for the first time.\n", THIS_MODULE->name, __FUNCTION__);
  while(!rtdm_task_should_stop()){
    //Mesurer la température et l'humidité
    rtdm_printk(KERN_INFO "%s.%s(): will start a new measure.\n", THIS_MODULE->name, __FUNCTION__);
    //*****TEST*****//
    /*rtdm_mutex_lock(&my_mtx);
    measures.temperature ^= 0x01;
    measures.humidity ^= 0x01;
    rtdm_printk(KERN_INFO "%s.%s() : temp=%d, hum=%d\n", THIS_MODULE->name, __FUNCTION__, measures.temperature, measures.humidity);*/
    int err;
  	//unsigned long tempo20ms, tempo18ms, tempo30us;

  	//tempo20ms = 20000000L;
  	//tempo18ms = 18000000L;
    //tempo30us = 30000L;
  	rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);

  	//MCU Start signal
  	if ((err = gpio_direction_output(GPIO_DHT11, 1)) != 0) { //Envoi 1 20ms
  		gpio_free(GPIO_DHT11);
      rtdm_printk(KERN_ALERT "%s.%s() : error 2 %d\n", THIS_MODULE->name, __FUNCTION__, err);
  	}

  	rtdm_task_sleep(20000000);
    
  	if ((err = gpio_direction_output(GPIO_DHT11, 0)) != 0) { //Envoi 0 18ms pour Start
  		gpio_free(GPIO_DHT11);
  		rtdm_printk(KERN_ALERT "%s.%s() : error 3 %d\n", THIS_MODULE->name, __FUNCTION__, err);
  	}

  	rtdm_task_sleep(18000000);
    rtdm_printk(KERN_INFO "%s.%s() : going through error 3 & sleep\n", THIS_MODULE->name, __FUNCTION__);
  	if ((err = gpio_direction_input(GPIO_DHT11)) != 0) { //Mise en mode lecture
  		gpio_free(GPIO_DHT11);
  		rtdm_printk(KERN_ALERT "%s.%s() : error 4 %d\n", THIS_MODULE->name, __FUNCTION__, err);
  	}

  	rtdm_task_sleep(30000); //Courte attente

    rtdm_printk(KERN_ALERT "%s.%s() : GPIO waiting for DHT11\n", THIS_MODULE->name, __FUNCTION__);

  	count = 0;

  	while (gpio_get_value(GPIO_DHT11) != 0) { //Attente de la réponse du DHT11 avec un bit à 0 (sous 20-40us)
  		count++;
  		if (count > MAX_CNT) {
  			rtdm_printk(KERN_ALERT "%s.%s() : error : pullup by host 20-40us failed\n", THIS_MODULE->name, __FUNCTION__);
  		}
  	}

    rtdm_printk(KERN_ALERT "%s.%s() : GPIO will start receiving data\n", THIS_MODULE->name, __FUNCTION__);

  	int pulse_cnt[2*PULSES_CNT];
  	int fix_crc = FALSE;
  	int i;

  	for (i=0; i<=(2*PULSES_CNT); i+=2) {
  		while (gpio_get_value(GPIO_DHT11) == FALSE) {
  			pulse_cnt[i] += 1;
  			if (pulse_cnt[i] > MAX_CNT) {
  				rtdm_printk(KERN_ALERT "%s.%s() : error : pullup by DHT timeout %d\n", THIS_MODULE->name, __FUNCTION__, i);
  			}
  		}
  		while (gpio_get_value(GPIO_DHT11) != 0) {
  			pulse_cnt[i+1] += 1;
  			if (pulse_cnt[i+1] > MAX_CNT) {
  				if (i == 2*(PULSES_CNT-1)) {
  				}
  				else {
  					rtdm_printk(KERN_ALERT "%s.%s() : error : pullup by DHT timeout %d\n", THIS_MODULE->name, __FUNCTION__, i);
  				}
  			}
  		}
  	}
  	int total_cnt = 0;

  	for (i=2; i<=(2*PULSES_CNT); i+=2) {
  		total_cnt += pulse_cnt[i];
  	}

  	int average_cnt = total_cnt/(PULSES_CNT-1); //Mesure la moyenne des signaux a l'état bas et à l'état haut

  	//char data[PULSES_CNT];

  	int m=0;
  	int data0=0, data1=0, data2=0, data3=0, data4=0;
  	for (i=3; i<=(2*PULSES_CNT); i+=2) {
  		int nb;

  		// En comparant avec moyenne, si état haut > état bas, c'est un 1
  		if (pulse_cnt[i] > average_cnt) {
  			nb = 1;
  		}
  		else {
  			nb = 0;
  		}

  		// Sépare les 41 caractères de la chaine en 5 octets séparés data0, data1, data2, data3, data4
  		if (m/8 == 0) {
  			data0 += nb << (m%8);
  		}
  		else if (m/8 == 1) {
  			data1 += nb << (m%8);
  		}
  		else if (m/8 == 2) {
  			data2 += nb << (m%8);
  		}
  		else if (m/8 == 3) {
  			data3 += nb << (m%8);
  		}
  		else if (m/8 == 4) {
  			data4 += nb << (m%8);
  		}
  		m++;
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
  	measures.humidity=-1;
    measures.temperature=-1;

  	if (data4 == ((data0 + data1 + data2 + data3) & 0xFF)) {
  		measures.humidity = data0; //Affecte l'octet data0 pour l'info de l'humidité
  		measures.temperature = data2; //Affecte l'octet data2 pour l'info de la température

  	}
  	else {
  		rtdm_printk(KERN_ALERT "%s.%s() : error : checksum error\n", THIS_MODULE->name, __FUNCTION__);
  	}

  	rtdm_printk(KERN_INFO "humi = %d% , temp = %d°C \n", measures.humidity, measures.temperature);
    rtdm_task_wait_period(NULL);
  }
}

static int my_open_function(struct rtdm_fd *fd, int flags) {
  rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);
  return 0;
}

static void my_close_function(struct rtdm_fd *fd) {
  rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);
}

static int my_read_nrt_function  (struct rtdm_fd *fd, void __user *buffer, size_t lg)
{
  rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);

  rtdm_mutex_lock(&my_mtx);

  if (lg == sizeof(struct Measures)) {
      if (rtdm_safe_copy_to_user(fd, buffer, &measures, lg) != 0) {
        rtdm_printk(KERN_INFO "%s.%s()\n : Error in the copy of the measured data.", THIS_MODULE->name, __FUNCTION__);
        rtdm_mutex_unlock(&my_mtx);
        return -1;
      }
  }

  rtdm_printk("%s: sent %d bytes, \"%.*s\"\n",  __FUNCTION__, lg, lg, buffer);

  rtdm_mutex_unlock(&my_mtx);
  return lg;
}

static int my_write_nrt_function(struct rtdm_fd *fd, const void __user *buffer, size_t lg)
{
  rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);

  rtdm_mutex_lock(&my_mtx); //Demande du mutex pour ecrire dans les structures

  if (lg == sizeof(struct Order)) {
    struct Order neworder;
    neworder.temperature_min = 0;
    neworder.temperature_max = 20;
    neworder.periode_ms = 2000;
    if (rtdm_safe_copy_from_user(fd, &neworder, buffer, lg) != 0) {
      printk(KERN_INFO "%s.%s() : Error in copy of orders", THIS_MODULE->name, __FUNCTION__);
      rtdm_mutex_unlock(&my_mtx);
      return -2;
    }
    if (neworder.temperature_max <= neworder.temperature_min) {
      printk(KERN_INFO "%s.%s() : temperature_max <= temperature_min", THIS_MODULE->name, __FUNCTION__);
      rtdm_mutex_unlock(&my_mtx);
      return -3;
    }
    order.temperature_min = neworder.temperature_min;
    order.temperature_max = neworder.temperature_max;
    order.periode_ms = order.periode_ms;
    rtdm_task_set_period(&task_desc, 0, order.periode_ms*1000000);
  }
  else {
    printk(KERN_INFO "%s.%s() : message is not equivalent to 3 int.", THIS_MODULE->name, __FUNCTION__);
    rtdm_mutex_unlock(&my_mtx);
    return -1;
  }
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
    .label  = "rtdm_DHT11_%d",
};

static int __init initialisation(void) {
  printk(KERN_ALERT "from %s : RTDM DHT11 Driver launched.", THIS_MODULE->name);

  int err;
  //Initialisation de la tâche
  measures.temperature = 0;
  measures.humidity = 10;

  order.temperature_min = 10;
  order.temperature_max = 20;
  order.periode_ms = 2000;

  if ((err = gpio_request(GPIO_DHT11, THIS_MODULE->name)) != 0) {
		return err;
	}
  rtdm_mutex_init(&my_mtx);         //Initialisation du mutex
  rtdm_dev_register(&my_rt_device); //Initialisation du device

  if ( (err = rtdm_task_init(&task_desc, "rtdm-measure-task", task_measure, NULL, 30, order.periode_ms*1000000)) ) {
         rtdm_printk(KERN_INFO "%s.%s() : error rtdm_task_init\n", THIS_MODULE->name, __FUNCTION__);
  }
  else {
    rtdm_printk(KERN_INFO "%s.%s() : success rtdm_task_init\n", THIS_MODULE->name, __FUNCTION__);
  }

  printk(KERN_ALERT "from %s : RTDM DHT11 Driver initialised.", THIS_MODULE->name);
  return 0;
}

static void __exit cloture(void) {
  printk(KERN_ALERT "from %s : RTDM DHT11 Driver currently closing.", THIS_MODULE->name);

  gpio_free(GPIO_DHT11);              //Libération du GPIO
  rtdm_task_destroy(&task_desc);      //Destruction de la tâche
  rtdm_mutex_destroy(&my_mtx);        //Destruction du mutex
  rtdm_dev_unregister(&my_rt_device); //Destruction du device

  printk(KERN_ALERT "from %s : RTDM DHT11 Driver closed.", THIS_MODULE->name);
}

module_init(initialisation);
module_exit(cloture);
