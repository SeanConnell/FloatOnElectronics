#!/usr/bin/python
import serial
import json
import time
import sys
import requests

class SerialFormatError(Exception): pass

def log(level, message):
    """ wrapper for whichever logging solution I end up plugging in """
    print "[%s]\t%s" % (level, message)

def send_cmd(s, cmd, n=1):
    for x in xrange(n):
        s.write(chr(cmd))

class data_source():
    def __init__(self, data):
        if data is None:
            raise Exception("Couldn't open data source %s" % data)
        self.data = data

    def __iter__(self):
        return self

    def next(self):
        data = self.data.readline()
        if data == "":
            raise StopIteration
        else:
            return data

dbaud_rate = 38400 # higher baud rates make things break on this computer at least
dtimeout = 10
ddevice = '/dev/ttyACM0' # probably should detect this instead of hardcode
device_startup_time = 3
start_byte = 33  # ! (ascii 33) means start of a json line
reset_byte = 38 # & means die, as so many c programs have when they needed it and didn't have it
WARN = "WARN"
DEBUG = "DEBUG"
INFO = "INFO"
ERROR = "ERROR"
DATA = "DATA"

def open_comm(device=ddevice, baud_rate=dbaud_rate, timeout=dtimeout):
    log(INFO, "Opening serial port on %s with %d baud rate and %d seconds timeout" % (device, baud_rate, timeout))
    ser = serial.Serial(device, baud_rate, timeout=timeout)
    ser.flushOutput() # make sure we only send what we intend to
    log(INFO, "Sending %s (reset byte) to clear device state" % reset_byte)
    send_cmd(ser, reset_byte)
    log(INFO, "Waiting for device to start up")
    time.sleep(device_startup_time)
    log(INFO, "Sending %s (start byte) to initiate communication" % start_byte)
    send_cmd(ser, start_byte) 
    ser.flushOutput() # make sure we only send what we intend to
    ser.flushInput()
    return ser

def open_test_file(filename):
    log(INFO, "Running with testfile %s instead of live data" % filename)
    return open(filename)

def parse_line(data):
    if data == None or len(data) == 0:
        log(ERROR, "Got empty data from sensor bridge. Retrying...")
        raise SerialFormatError 
    if data[0] != '!':
        log(ERROR, "Badly formatted packet, missing ! as starting char. Dropping and Retrying...")
        raise SerialFormatError 
    if data[-1] != '\n':
        log(ERROR, "Badly formatted packet, missing newline as final char. Dropping and Retrying...")
        raise SerialFormatError 
    return data[1:]

def get_data_source():
    if len(sys.argv) == 2:
        try:
            return data_source(open_test_file(sys.argv[1]))
        except Exception:
            log(ERROR, "Couldn't open file.")
            sys.exit(1)
    else:
        try:
            return data_source(open_comm())
        except Exception:
            log(ERROR, "Couldn't open comm with device")
            sys.exit(1)

def handle_INFO_message(msg_info):
    info = msg_info['INFO']
    log(INFO, "%s" % info)

def handle_WARN_message(msg_warn):
    warn = msg_warn['WARN']
    log(WARN, "%s" % warn)

def handle_ERROR_message(msg_error):
    error = msg_error['ERROR']
    log(ERROR, "%s" % error)

def handle_DATA_message(msg_data):
    data = msg_data['DATA']
    post_json_message(msg_data, "http://localhost:8080/")
    log(DATA, "%s: %s %s over %s ms" % (data['type'], data['value'], 
        data['unit'], data['period']))

def handle_MANIFEST_message(msg_startup):
    for sensor in msg_startup['SENSORS_MANIFEST']:
        log(INFO, "%s (%s) connected on %s" % (sensor['name'], sensor['url'], 
            sensor['connection']))

def dispatch(msg, handlers):
    return(handlers[_parse_message_type(msg)](msg))

def post_json_message(msg, address):
    headers = {'Content-type': 'application/json', 'Accept': 'text/plain',
            'name':'test', 'password':'test'}
    r = requests.post(address, data=json.dumps(msg), headers=headers)
    log(DEBUG, "Posted %s to %s with response %s" % (msg, address, r))

def _parse_message_time(msg):
    msg_type = filter(lambda x: x == 'TIME', msg.keys())
    if len(msg_type) != 1: 
        raise SerialFormatError("No TIME header")
    return msg_type[0]

def _parse_message_type(msg):
    msg_type = filter(lambda x: x != 'TIME', msg.keys())
    if len(msg_type) != 1: 
        raise SerialFormatError("More than one message type")
    return msg_type[0]

HANDLERS = {'INFO':handle_INFO_message,
            'WARN':handle_WARN_message,
            'ERROR':handle_ERROR_message,
            'DATA':handle_DATA_message, 
            'SENSORS_MANIFEST':handle_MANIFEST_message,}

data_source = get_data_source()
for data in data_source:
    try:
        data = parse_line(data)
    except SerialFormatError:
        continue

    try:
        jdata = json.loads(data)
    except ValueError:
        log(ERROR, "Invalid json recieved. Retrying: %s" % data)
        continue;
    dispatch(jdata, HANDLERS)
