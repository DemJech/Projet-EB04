# Description du driver
---

Le DHT11 est branché sur la pin 6

Le mutex permet de ne pas éxécuter les commandes lire et ecrire en même temps

La mesure et la réaction qui lui est associé tournent dans le même thread :
 - Dans un premier temps on mesure;
 - dans un second temps on réagit à la mesure.

Dans le cas d'une lecture, la fonction my_read_nrt_function est appelé et retourne la structure mesures
