#!/usr/bin/python
import serial
import json
import time

def log(level, message):
    print "[%s]\t%s" % (level, message)

def send_start(s):
    for x in xrange(10):
        s.write(chr(start_byte))

device = '/dev/ttyACM0' # probably should detect this instead of hardcode
baud_rate = 38400 # higher baud rates make things break on this computer at least
timeout = 10
start_byte = 33  # ! (ascii 33) means start of a json line
reset_byte = 38 # & means die, as so many c programs have when they needed it and didn't have it
INFO = "INFO"
ERROR = "ERROR"
DATA = "DATA"


# Open our connection on the usual place
log(INFO, "Opening serial port on %s with %d baud rate and %d seconds timeout" % (device, baud_rate, timeout))
ser = serial.Serial(device, baud_rate, timeout=timeout)
# wait for arduino to startup
time.sleep(5)
log(INFO, "Sending %s (start byte) to initiate communication" % start_byte)
ser.flushOutput() # make sure we only send what we intend to
ser.flushInput()
send_start(ser)
while True:
    data = ser.readline()
    if data == None or len(data) == 0:
        log(ERROR, "Got no data from sensor bridge. Retrying...")
        continue
    data = data.replace("!","")
    data = data.replace("\n","")

    try:
        jdata = json.loads(data)
    except ValueError:
        log(ERROR, "Invalid json recieved. Retrying: %s" % data)
        continue;
    log(DATA, data)
