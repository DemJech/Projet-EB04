import time
from sense_hat import SenseHat

sense = SenseHat()

while (True) :
    fic = open("server.txt", 'r') #Récupération des données du fichier
    ligne = fic.readlines()
    fic.close()
    ligne = ligne[-1]
    data = ligne.split(';') #humidity;temperature
    data[0] = float(data[0])
    data[1] = float(data[1])
    data.append(sense.get_humidity())
    data.append(sense.get_temperature())
    data.append(sense.get_pressure())
    sense.show_message("Hext={:3.1f}%, Text={:5.1f}*C, Hint={:3.1f}%, Tint={:5.1f}*C, P={:4.0f}hPa".format(data[0], data[1], data[2], data[3], data[4]))
    fic2 = open("internSensor.txt", 'a')
    fic2.write("{2};{3};{4}".format(data))
    fic2.close()
