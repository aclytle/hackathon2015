import serial
import wiringpi2
import time
import logging

port='/dev/ttyUSB0'
baud=9600

wiringpi2.wiringPiSetup()
wiringpi2.pinMode(11, wiringpi2.GPIO.OUTPUT)
wiringpi2.digitalWrite(11, 0)
s = serial.Serial(port, baud, timeout=30)
s.write('g')
wiringpi2.digitalWrite(11, 1)
time.sleep(5)
wiringpi2.digitalWrite(11,0)
s.flushInput()
