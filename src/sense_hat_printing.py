import time
from sense_hat import SenseHat

sense = SenseHat()

while (True) :
    fic = open("server.txt", 'r')
    #Lire le fichier
    ligne = fic.readlines()
    ligne = ligne[-1]
    data = ligne.split(';') #humidity;temperature
    data.append(sense.getHumidity())
    data.append(sense.getTemperature())
    data.append(sense.getPressure()/100)
    sense.showMessage("Hext={2.0f}, Text={5.1f}, Hint={2.0f}, Tint={5.1f}, P={4.0f}".format(data[0], data[1], data[2], data[3], data[4]))
    fic.close()
