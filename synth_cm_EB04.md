# Notes des CM
---

## CM n°3

### Diapo n°10

Une tâche temps réel correspond à un module (éxécuté dans l'espace noyau)

### Ex : RTLinux (Diapo n°11)

## CM n°4

### Les interruptions (Diapo n°21)

Les interruptions sont controlées par le KernelRT

### Gestion du temps (Diapo n°24)

- Récupérer l'heure courante
- Récupérer le temps ecouler depuis l'allumage
-

### Communication inter-tâches

- FIFO
- Mémoire partagée
- Files de messages

#### FIFO (Diapo n°26-30)

- Communication entre taches RT et nRT
- Tampon à R/W non bloquant dans l'espace d'addresse du noyau
- R/W s'éxécutent ou renvoie une erreur

#### Mémoire partagée (Diapo n°31-32)

- Allocation d'une part de la RAM initié au démarrage (grub.conf)
- Accés depuis nRT avec l'addresse mémoire.
- Accés depuis RT avec un pointeur sur mémoire physique

#### Files de messages (Diapo n°33-34)

- Communication pour les tâches dans le domaine RT
- File de message avec temps avant autodestruction
- Création, Read, Write, destruction

### Outils de synchronisation

- Sémaphores
- Mutex
- Les variables conditions

#### Sémaphores (Diapo n°36-37)

- Sémaphores = distributeur de tickets
- Un process peut demander un ticket
- Un process peut vendre un ticket, l'un des demandeur est réveillé et est activé

#### Mutex (Diapo n°38-39)

Sémaphores dont la file est de une place

#### Variables de condition

Mise en attente d'un process RT suivant une variable de condition

### Ports d'entrée/sortie

Ecrire et lire les ports d'entrée sortie.

## CM n°5

Exemples de noyaux RT : RTLinux, RTAI, Xenomai

### Structure d'un système Linux RT : Xenomai (Diapo n°9)

### Core Xenomai

- Objet thread temps réel
- objet générique : interruption
- Allocateur de mémoire avec temps de réponse prédictible
- objets de synchronisation dont sémaphores, mutex, files de messages
- objets de gestion du temps

### Exmples de Skins

- POSIX
- VxWorks
- pSOS+
- VRTX
- uiTron
- RTAI
- "Native"
- RTDM

### Coexistence de Linux et Xenomai

Real time shadowing pour les taches nRT qui deviennent RT. `Objet RT Shadow`

les structures task_struct et xnthread sont partagées par les ordonnaceurs.

### Threads Linux & Xenomai

Les threads RT peuvent s'éxécuter dans l'espace noyau (forme moduule) et dans
l'espace utilisateur (Thread Xenomai).

Les threads Xenomai :
 - Migre du domaine primaire au domaine secondaire en faisant appel à un service Linux
 - et migre du domaine secondaire en faisant appel a l'API Xenomai.

## CM n°6 : Programmation multi-tâches

### Les threads

un thread :
 - 1 état des registres
 - 1 compteur ordinal

 les threads s'éxécutent de manière concurrente. l'espace d'addressage est
 partagé ainsi que les variables globales.

 main() est le thread initial d'un programme.

Création :
 - rt_task_create()
 - pthread_create()

Destruction :
 - si arrivé à la fin de son fil d'instruction.
 - rt_task_delete()
 - pthread_exit()
 - Si son thread père finit.
 - pthread_cancel()

 Pour attendre la fin d'un thread fils : `rt_task_join()` et `pthread_join()`

A la céation de la tâche priorité (0 à 99 + basse à + haute).
Structure POSIX : pthread_attr_t

### IPC : Mutex et sémaphore

La compétition se produit lorsque 2 tâches utilisent une ressource de manière non
synchronisée.

Résolution du conflit par les mutex et les sémaphores.

#### Mutex

2 états : LIBRE (1), FERME (0)

Un processus peut entrer en liste d'attente pour l'acquerir ou le relacher.

Fonctions:
 - `rt_mutex_create()` ou `pthread_mutex_init()` pour la création
 - `Héritage de la priorité` ou `pthread_mutex_attr_init()` pour définir la priorité
 - `rt_mutex_delete()` ou `pthread_mutex_destroy()` pour détruire le mutex
 - `rt_mutex_lock()` ou `pthread_mutex_lock()` pour acquérir le mutex
 - `rt_mutex_release()` ou `pthread_mutex_unlock` libérer le mutex

 ATTENTION : au problème de blocage

#### Sémaphore

 - `rt_sem_create()` ou `sem_init()` pour la création
 - `rt_sem_delete()` ou `sem_destroy()` pour suppr
 - `rt_sem_p()` ou `sem_wait()` acquérir le sémaphore
 - `rt_sem_broadcast()` ou `voir objet barrier` débloquer toutes les tâches

### IPC : messages et mémoire partagée

#### file de msg : `queue`

Communication entre 2 taches RT
Taille de file définit à la création, lecture bloquante ou non.
Quand il n'y a pas de message, la tâche est bloquée.
Si la file est pleine, impossible d'écrire

Fonctions :
 - `rt_queue_create()` ou `mq_open()` pour la création
 - `rt_queue_delete()` ou `mq_close()` pour définir la priorité
 - `rt_queue_write()` ou `mq_send()` pour détruire le mutex
 - `rt_queue_read()` ou `mq_receive()` pour lire le message

#### File de message ("pipe")

Communication entre un processus Linux et un processus Xenomai.

Utilise un objet RT_PIPE (n° de mineur et device)

Fonctions :
 - `rt_pipe_create()` ou `open()` pour la création
 - `rt_pipe_delete()` ou `close()` pour supprimer une file
 - `rt_pipe_write()` ou `send()` pour envoyer un message
 - `rt_pipe_read()` ou `read()` pour lire le message

#### Mémoire partagée

Zone de mémoire partagé par des tâches. Allocation dans un tas de mémoire de
taille fixe défini au démarrage. Utiliser un mutex pour un accés cohérent à la mémoire.
Pour la communication inter-tâches, préférer la file de message

- `rt_heap_create()` ou `shm_open()` pour la création
- `rt_heap_delete()` ou `shm_close()` pour supprimer la mém partagée
- `rt_heap_alloc()` ou `shm_nmap()` pour allouer la mémoire
- `rt_heap_free()` ou `shm_unmap()` pour libérer la mémoire

#### Le temps

les horloges dispo clkid_t :
CLOCK_MONOTONIC, tps depuis la mise sous tension
CLOCK_REALTIME, tps depuis le 1er jan. 1970 (UTC)
CLOCK_PROCESS_CPUTIME_ID, temps CPU consommé par le process

##### En Linux
clock_nanosleep(clockid_t, struct timespec *res,)
clock_gettime(,) obtenir le tps écoulé
clock_getres(,) obtenir la résolution de l'horloge

##### En Xenomai

RTIME, un long long int tps en us.
RTIME rt_timer_read(void) : tps courant
rt_task_sleep(RTIME time) : fait dormir la tâche time us

##### Tâche périodique sous Xenomai

rt_task_set_periodic(date, period) / pthread_make_periodic_np()
rt_task_wait_period(&overrun) /pthread_wait_np()

## CM n° 7 : Modules Linux

Module : programme éxécuté dans l'espace noyau.
Ajout dynamique :
 - `insmod`
 - `rmmod`
 - `lsmod`

Fonction d'initialisation

Avertir le noyau, mettre en place les structures nécessaires

déclaration dans la macros module_init

Fonction de terminaison

Supprimer les structures mises en places pour servir les requêtes

Déclaration dans la macros module_exit

### Programmation d'un module

`#include <linux/init.h>`
`#include <linux/module.h>`
MODULE_LICENSE("GPL");

static int __init A(void) {
  ...
  printk(KERN_ALERT "from %s : COUCOU.", THIS_MODULE->name);
  ...}

static void __exit B(void) {...}

module_init(A);
module_exit(B);

Les modules sont localisés dans `/proc/modules` et accessibles via `lsmod`

Avantages :
 - Seul les app les + utilisées sont chargées statiquement
 - les pilotes de périphériques sont des modules le + svt
 - Les modules sont modifiables sans recompilation du noyau

Les sorties d'un module sont archivées dans `/var/log/message`

### Documentation

Fonctions utiles, définies dans `#include <linux/module.h>`, accessible avec
la commande `modinfo Nom_Module.ko`:
 - MODULE_AUTHOR();
 - MODULE_VERSION();
 - MODULE_DESCRIPTION();
 - MODULE_SUPPORTED_DEVICE();

### Passage d'arguments au chargement du module

Instanciation en C : `#include <linux/moduleparam.h>` et `module_param(nom, type, permission)`
Déclaration en Bash : `insmod mymodule.ko myvariable=10`
Pour enrichir modinfo : `MODULE_PARAM_DESC(nom, "description");`

## CM n°8 : Pilotes de périphériques (Device Drivers)

Drivers : interface logicielle entre l'OS et le périphérique

3 types de pilotes :
 - caractère : flot d'octet e.g. : console, port série, port parallèle
 - bloc : 1 k octet
 - réseau

ls -l /dev/ttyS0

-> permission ___ ___ majeur, mineur date nom_du_fic

majeur : le pilote
mineur : l'instance

### Pilote caractère : identification

En C : dans `<linux/kdev_t.h>`
 - `MAJOR(dev_t dev)` retourne le majeur de dev (int)
 - `MINOR(dev_t dev)` retourne le mineur de dev (int)
 - `MKDEV(int major, int minor)` retourne un objet de type dev_t
