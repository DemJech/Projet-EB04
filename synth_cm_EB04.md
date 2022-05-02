# Notes des CM

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
