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

#define GPIO_DHT11 5  //Definition du pin GPIO du capteur DHT11
#define GPIO_SERVO 12 //Definition du pin GPIO du servo moteur
#define TRUE 1
#define FALSE 0
#define MAX_CNT	85
#define MSG_SIZE 4

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brendan Signarbieux & Tom Ladune");
MODULE_VERSION("Alpha 1.0");
MODULE_DESCRIPTION("This module measures temperature and humidity thanks to the DHT11 and activate the servomotor depending on the humidity treshold.");

static char measures[4];

rtdm_task_t task_desc_dht11;  //Definition de la tâche dht11
static rtdm_mutex_t my_mtx; //Definition du mutex

static int seuil_humi = 60; //Definition du seuil d humidite demande par l utilisateur
static char seuil_char;

void task_measure(void *arg) {
  rtdm_printk(KERN_INFO "%s.%s(): task_measure has launched for the first time.\n", THIS_MODULE->name, __FUNCTION__);

  while(!rtdm_task_should_stop()) {
  	char data[5] = { 0, 0, 0, 0, 0 };
   	int dernier_etat = 1 ;
  	uint8_t count = 0;
  	uint8_t j = 0, i;
  	int err;

	   //Signal "start" envoye au DHT11 depuis la Rpi
  	if ((err = gpio_direction_output(GPIO_DHT11, 1)) != 0) { //Envoi 1 sur 20ms
		gpio_free(GPIO_DHT11);
		rtdm_printk(KERN_ALERT "%s.%s() : error 1 %d\n", THIS_MODULE->name, __FUNCTION__, err);
  	}
  	rtdm_task_sleep(20000000);

  	if ((err = gpio_direction_output(GPIO_DHT11, 0)) != 0) { //Envoi 0 sur 18ms pour Start
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

  	for ( i = 0; i < MAX_CNT; i++ ) {// Detecte chaque changement et lit donnees recues
  		count = 0;
  		while ( gpio_get_value(GPIO_DHT11) == dernier_etat ) {//attendre tant que le dernier bit recu n'a pas change
  			count++;
  			rtdm_task_sleep(1000);

  			if ( count == 255 ) {
  				break;
  			}
  		}
  		dernier_etat = gpio_get_value(GPIO_DHT11) ; //actualise l'etat du dernier bit recu
  		if ( count == 255 )
  			break;

  		if ( (i >= 4) && (i % 2 == 0) ) //ignore les 3 premieres transitions recues du dht11 (bits start) avant de recevoire les datas utiles
  		{
  			data[j / 8] <<= 1; //envoie chaque bit dans data et les sépare en 5 octets
  			if ( count > 8 )
  			{
  				data[j / 8] |= 1;
  			}
  			j++;
  		}
    }

  	if ( (j >= 40)  && (data[4] == ( (data[0] + data[1] + data[2] + data[3]) & 0xFF))) {//Verif lecture des 40 bits (8bit x 5 ) + verif checksum
  		rtdm_printk(KERN_INFO "Humidite = %d.%d %% Temperature = %d.%d *C \n",data[0], data[1], data[2], data[3]);
  		measures[0] = data[0]; //Affecte l'octet data[0] pour l'info de l'humidité
  		measures[1] = data[1];
  		measures[2] = data[2]; //Affecte l'octet data[2] pour l'info de la température
  		measures[3] = data[3];
  	} else {
  		rtdm_printk( "Donnee recue erronnee\n" );
  	}

  	//PWM SERVO
  	uint32_t PERIODE_TOT = 20000000; //Definition periode totale du signal envoye au servo (20ms)
  	uint32_t ETAT_HAUT = 1000000; //Definition de la duree a l etat haut du signal envoye au servo (par defaut 1ms pour 0 degre d'inclinaison)
  	int nb_tour;
  	nb_tour = 0;

  	while (nb_tour < 20)
  	{
  		if (measures[0] > seuil_humi) //Si humidite > seuil demande
  		{
  			ETAT_HAUT = 2000000; //Etat haut de la PWM a 2ms pour une inclinaison du servo de 180 degres

  		}else{
  			ETAT_HAUT = 1000000; //Etat haut de la PWM a 1ms pour une inclinaison du servo de 0 degre
  		}

  		//Mise a 1 du signal PWM sur duree ETAT_HAUT
  		if ((err = gpio_direction_output(GPIO_SERVO, 1)) != 0) {
  			gpio_free(GPIO_SERVO);
  			rtdm_printk(KERN_ALERT "%s.%s() : error servo 1 %d\n", THIS_MODULE->name, __FUNCTION__, err);
  		}
  		rtdm_task_sleep(ETAT_HAUT);

  		//Remise a 0 du signal PWM sur duree (PERIODE_TOT - ETAT_HAUT)
  		if ((err = gpio_direction_output(GPIO_SERVO, 0)) != 0) {
  			gpio_free(GPIO_SERVO);
  			rtdm_printk(KERN_ALERT "%s.%s() : error servo 2 %d\n", THIS_MODULE->name, __FUNCTION__, err);
  		}
  		rtdm_task_sleep(PERIODE_TOT-ETAT_HAUT);

  		nb_tour+=1;
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

static int my_read_nrt_function (struct rtdm_fd *fd, void __user *buffer, size_t lg) {
  rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);
  //char msg[MSG_SIZE];
  rtdm_mutex_lock(&my_mtx);

  if (lg == MSG_SIZE) { //Si la longueur demandé est egale à la longueur du message
      //Copier dans buffer, les MSG_SIZE octets à l'adresse de measures
      if (rtdm_safe_copy_to_user(fd, buffer, &measures, MSG_SIZE) != 0) {
        rtdm_printk(KERN_INFO "%s.%s()\n : Error in the copy of the measured data.", THIS_MODULE->name, __FUNCTION__);
        rtdm_mutex_unlock(&my_mtx);
        return -1;
      }
  }
  rtdm_printk("%s: sent %d bytes, \"%.*s\"\n",  __FUNCTION__, lg, lg, buffer);
  rtdm_mutex_unlock(&my_mtx);
  return lg;
}

static int my_write_nrt_function(struct rtdm_fd *fd, const void __user *buffer, size_t lg) {
	rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);

	rtdm_mutex_lock(&my_mtx); //Demande du mutex pour ecrire dans les structures

	if (lg > 0) {//Test de compatibilité des paramètres d'ecriture
		if (rtdm_safe_copy_from_user(fd,
						  &seuil_char,
						  buffer, 1) != 0) {
			rtdm_mutex_unlock(&my_mtx);
			return -EFAULT;
		}
    //Sélection du seuil d'humidité
		switch (seuil_char){
			case '1':
				seuil_humi=40;
				break;
			case '2':
				seuil_humi=50;
				break;
			case '3':
				seuil_humi=60;
				break;
			case '4':
				seuil_humi=70;
				break;
			case '5':
				seuil_humi=80;
				break;
			default:
				seuil_humi=60;
		}

		rtdm_printk(KERN_INFO "%s.%s() : driver receive %c, seuil_humi = %d\n", THIS_MODULE->name, __FUNCTION__, seuil_char, seuil_humi);

	}
	else {
		rtdm_printk(KERN_INFO "%s.%s() : device receive error\n", THIS_MODULE->name, __FUNCTION__);
    return -1;
	}

	rtdm_printk("Received from Linux %d bytes : %.*s\n", lg, lg, buffer);

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
  measures[0] = 0;
  measures[1] = 0;
  measures[2] = 10;
  measures[3] = 0;

  order.temperature_min = 10;
  order.temperature_max = 20;
  order.periode_ms = 2000;

  if ((err = gpio_request(GPIO_DHT11, THIS_MODULE->name)) != 0) {
	return err;
  }
  rtdm_mutex_init(&my_mtx);         //Initialisation du mutex
  rtdm_dev_register(&my_rt_device); //Initialisation du device

  if ( (err = rtdm_task_init(&task_desc_dht11, "rtdm-measure-task", task_measure, NULL, 30, order.periode_ms*1000000)) ) {
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
  rtdm_task_destroy(&task_desc_dht11);      //Destruction de la tâche dht11

  rtdm_mutex_destroy(&my_mtx);        //Destruction du mutex
  rtdm_dev_unregister(&my_rt_device); //Destruction du device

  printk(KERN_ALERT "from %s : RTDM DHT11 Driver closed.", THIS_MODULE->name);
}

module_init(initialisation);
module_exit(cloture);
