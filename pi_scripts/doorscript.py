import serial
import wiringpi2
import time
import logging

port='/dev/ttyUSB0'
baud=9600
accessfile='access.txt'
logfile='access.log'

logging.basicConfig(filename=logfile, level=logging.DEBUG,
        format='%(asctime)s  %(message)s')
wiringpi2.wiringPiSetup()
wiringpi2.pinMode(11, wiringpi2.GPIO.OUTPUT)
wiringpi2.digitalWrite(11, 0)
s = serial.Serial(port, baud, timeout=30)
while(True):
    cardid = s.readline().strip()
    if (len(cardid) > 3):
        fd = open(accessfile);
        lines = fd.readlines()
        lines2 = []
        for l in lines:
            lines2.append(l.split()[0].strip())
        if cardid in lines2:
            print("Access Granted: {}".format(lines[lines2.index(cardid)].strip()))
            logging.info("Access Granted: %s", lines[lines2.index(cardid)].strip())
            s.write('g')
            wiringpi2.digitalWrite(11, 1)
            time.sleep(5)
            wiringpi2.digitalWrite(11,0)
            s.flushInput()
        else:
            print("Access Denied: {}".format(cardid))
            logging.info("Access Denied: {}".format(cardid))
            s.write('d')
        fd.close()
