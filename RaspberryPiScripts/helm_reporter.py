#!/usr/bin/python
import serial
import json

device = '/dev/ttyACM0' #probably should detect this instead of hardcode
baud_rate = 38400 #higher baud rates make things break on this computer at least
timeout = 10 #10 second timeout

#Open our connection on the usual place
ser = serial.Serial(device, baud_rate, timeout=timeout)
#do a hardware reset on a yet to be implemented command 
while True:
    data = ser.readline()
    try:
        jdata = json.loads(data)
    except ValueError:
        print "invalid json, moving on."
        print data
        continue;
    print jdata

