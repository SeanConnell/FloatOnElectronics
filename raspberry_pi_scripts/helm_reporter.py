#!/usr/bin/python
import serial
import json
import time
import requests
from cmdline_tools import log, WARN, INFO, ERROR, DEBUG, DATA

class SerialFormatError(Exception): pass
class NoHandlerError(Exception): pass

def _send_cmd(s, cmd, n=1):
    """ Sends a one byte command n number of times over a serial connection """
    for x in xrange(n):
        s.write(chr(cmd))

class _data_source():
    """ Class that wraps either a socket or a file that sources data """
    def __init__(self, data):
        if data is None:
            raise Exception("Couldn't use None data source")
        self.data = data

    def __iter__(self):
        return self

    def next(self):
        data = self.data.readline()
        if data == "":
            raise StopIteration
        else:
            return data

def open_comm(device, baud_rate, timeout):
    """ Open a serial connection with the sensor bridge """
    start_byte = 33  # ! (ascii 33) means start of a json line
    reset_byte = 38 # & means die, as so many c programs have when they needed it and didn't have it
    device_startup_time = 3
    log(INFO, "Opening serial port on %s with %d baud rate and %d seconds timeout" 
            % (device, baud_rate, timeout))
    ser = serial.Serial(device, baud_rate, timeout=timeout)
    ser.flushOutput() # make sure we only send what we intend to
    log(INFO, "Sending %s (reset byte) to clear device state" % reset_byte)
    _send_cmd(ser, reset_byte)
    log(INFO, "Waiting for device to start up")
    time.sleep(device_startup_time)
    log(INFO, "Sending %s (start byte) to initiate communication" % start_byte)
    _send_cmd(ser, start_byte) 
    ser.flushOutput() # make sure we only send what we intend to
    ser.flushInput()
    return ser

def open_test_file(filename):
    log(INFO, "Running with testfile %s instead of live data" % filename)
    return open(filename)

def _parse_line(data):
    """ Validate line of data is in correct format and return if it is  """
    if data == None or len(data) == 0:
        log(ERROR, "Got empty data from sensor bridge. Retrying...")
        raise SerialFormatError 
    if data[0] != '!':
        log(ERROR, "Badly formatted packet, missing ! as starting char.")
        raise SerialFormatError 
    if data[-1] != '\n':
        log(ERROR, "Badly formatted packet, missing newline as final char.")
        raise SerialFormatError 
    return data[1:]

def _handle_INFO_message(msg_info):
    info = msg_info['INFO']
    log(INFO, "%s" % info)

def _handle_WARN_message(msg_warn):
    warn = msg_warn['WARN']
    log(WARN, "%s" % warn)

def _handle_ERROR_message(msg_error):
    error = msg_error['ERROR']
    log(ERROR, "%s" % error)

def _handle_DATA_message(msg_data):
    data = msg_data['DATA']
    #do some msg parsing here
    log(DATA, "%s: %s %s over %s ms" % (data['type'], data['value'], 
        data['unit'], data['period']))
    return msg_data #looks pointless for now, but will be a subset later

def _handle_MANIFEST_message(msg_startup):
    for sensor in msg_startup['SENSORS_MANIFEST']:
        log(INFO, "%s (%s) connected on %s" % (sensor['name'], sensor['url'], 
            sensor['connection']))

def _dispatch(msg, handlers):
    msg_type = _parse_message_type(msg)
    if msg_type in handlers:
        return(handlers[msg_type](msg))
    else:
        raise NoHandlerError("No handler for msg of type %s" % msg_type)

def post_json_message(data, uri):
    json_data = json.dumps(data)
    headers = {'Content-type': 'application/json', 'Accept': 'text/plain',
            'name':'test', 'password':'test'}
    r = requests.post(uri, data=json_data, headers=headers)
    log(DEBUG, "Posted %s to %s with response %s" % (json_data, uri, r))
    return int(r.status_code)

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


def read_data(data_source, reporting_uri):
    #Wrap handler functions as needed
    #Probably a better way to do this
    data_reporter = \
        lambda x: post_json_message(_handle_DATA_message(x), reporting_uri)

    #create handlers to dispatch to
    HANDLERS = {
                'INFO':_handle_INFO_message,
                'WARN':_handle_WARN_message,
                'ERROR':_handle_ERROR_message,
                'DATA':data_reporter, 
                'SENSORS_MANIFEST':_handle_MANIFEST_message}
    recieved_data = _data_source(data_source)

    for data in recieved_data:
        try:
            valid_data = _parse_line(data)
        except SerialFormatError:
            continue

        try:
            jdata = json.loads(valid_data)
        except ValueError:
            log(ERROR, "Invalid json recieved. Retrying: %s" % data)
            continue;
        try:
            _dispatch(jdata, HANDLERS)
        except NoHandlerError as handle_error:
            log(ERROR, "Error in dispatch: %s" % handle_error)
            continue;
        except Exception as error:
            log(ERROR, "Error in dispatch: %s" % error)
            continue;
