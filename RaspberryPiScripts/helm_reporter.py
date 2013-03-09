#!/usr/bin/python
import serial

#Open our connection on the usual place
ser = serial.Serial('/dev/ttyACM0',115200, timeout=10)
#do a hardware reset on a yet to be implemented command 
while True:
    print ser.readline()
