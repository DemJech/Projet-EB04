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

#define GPIO_DHT11 5
#define TRUE 1
#define FALSE 0
#define MAX_CNT	85

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brendan Signarbieux & Tom Ladune");
MODULE_VERSION("Alpha 1.0");
MODULE_DESCRIPTION("This module measures temperature and humidity thanks to the DHT11.");

static struct Measures {      //Structure de donnée obtenue par le capteur DHT11
    int temperature;
    int humidity;
} measures;

static struct Order {        //Structure de données des paramètres de fonctionnement du driver
  int temperature_min;
  int temperature_max;
  int periode_ms;
} order;

static int periode_us = 1000*1000;
module_param(periode_us, int, 0644);

rtdm_task_t task_desc;
static rtdm_mutex_t my_mtx;
static char temperature[2] ;
static char a;

rtdm_task_t task_desc;  //Definition de la tâche
static rtdm_mutex_t my_mtx; //Definition du mutex

void task_measure(void *arg) {
  rtdm_printk(KERN_INFO "%s.%s(): has launched for the first time.\n", THIS_MODULE->name, __FUNCTION__);
  
  while(!rtdm_task_should_stop()){
	  
	int data[5] = { 0, 0, 0, 0, 0 };
 	int dernier_etat = 1 ; 
	uint8_t count = 0;
	uint8_t j = 0, i;
	int err;
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	
	//MCU Start signal
  	if ((err = gpio_direction_output(GPIO_DHT11, 1)) != 0) { //Envoi 1 20ms
		gpio_free(GPIO_DHT11);
		rtdm_printk(KERN_ALERT "%s.%s() : error 1 %d\n", THIS_MODULE->name, __FUNCTION__, err);
  	}
  	rtdm_task_sleep(20000000);

  	if ((err = gpio_direction_output(GPIO_DHT11, 0)) != 0) { //Envoi 0 18ms pour Start
  		gpio_free(GPIO_DHT11);
  		rtdm_printk(KERN_ALERT "%s.%s() : error 2 %d\n", THIS_MODULE->name, __FUNCTION__, err);
  	}
  	rtdm_task_sleep(18000000);
	
	if ((err = gpio_direction_output(GPIO_DHT11, 1)) != 0) { //Remise à 1 pour 40us
		gpio_free(GPIO_DHT11);
		rtdm_printk(KERN_ALERT "%s.%s() : error 3 %d\n", THIS_MODULE->name, __FUNCTION__, err);
  	}
	rtdm_task_sleep(38000);	 //Courte attente 38us
	
  	if ((err = gpio_direction_input(GPIO_DHT11)) != 0) { //Mise en mode lecture
  		gpio_free(GPIO_DHT11);
  		rtdm_printk(KERN_ALERT "%s.%s() : error 4 %d\n", THIS_MODULE->name, __FUNCTION__, err);
  	}
	
	//rtdm_printk(KERN_ALERT "%s.%s() : GPIO waiting for DHT11\n", THIS_MODULE->name, __FUNCTION__);
	
	// Detecte chaque changement et lit donnees recues
	for ( i = 0; i < MAX_CNT; i++ )
	{
		count = 0;
		while ( gpio_get_value(GPIO_DHT11) == dernier_etat ) //attendre tant que le dernier bit recu n'a pas change
		{
			count++;
			rtdm_task_sleep(1000);
			
			if ( count == 255 )
			{
				break;
			}
		}
		dernier_etat = gpio_get_value(GPIO_DHT11) ; //actualise l'etat du dernier bit recu

		if ( count == 255 )
			break;

		//ignore les 3 premieres transitions recues du dht11 (bits start) avant de recevoire les datas utiles
		if ( (i >= 4) && (i % 2 == 0) )
		{
			//envoie chaque bit dans data et les sépare en 5 octets
			data[j / 8] <<= 1;
			if ( count > 8 )
			{	
				data[j / 8] |= 1;
			}
			j++;
		}
	}
	
	 //Verif lecture des 40 bits (8bit x 5 ) + verif checksum
	if ( (j >= 40)  && (data[4] == ( (data[0] + data[1] + data[2] + data[3]) & 0xFF) ) )
	{
		rtdm_printk(KERN_INFO "Humidite = %d.%d %% Temperature = %d.%d *C \n",data[0], data[1], data[2], data[3]);	
		temperature[0]=data[2] + '0';
	    temperature[1]=data[3] + '0';
		
		measures.humidity = data[0]; //Affecte l'octet data[0] pour l'info de l'humidité
  		measures.temperature = data[2]; //Affecte l'octet data[2] pour l'info de la température
		
	}else  {
		rtdm_printk( "Donnee recue erronnee\n" );
	}
	
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
