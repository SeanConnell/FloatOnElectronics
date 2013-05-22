#!/usr/bin/python
"""
Command line parsing setup functions
"""
import ConfigParser
import argparse

WARN = "WARN"
DEBUG = "DEBUG"
INFO = "INFO"
ERROR = "ERROR"
DATA = "DATA"

def parse_config(filename):
    config = ConfigParser.RawConfigParser()
    config.read("config.cfg")
    return config

def init_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument("config", help="Config filename with extension to read \
            values from. Should be in the same dir. Example is config.cfg")
    parser.add_argument("-u", "--report_uri", help="URI to send json reports from \
            the sensor to. Example is http://localhost:8080")
    parser.add_argument("-c", "--com_port" , help="Serial port to communicate with \
            the device on. Example is /dev/ttyACM0 on linux or COM0 on windows")
    parser.add_argument("-b", "--baud_rate",
            help="Baud rate to use for serial")
    parser.add_argument("-t", "--timeout", type=int, help="Timeout in seconds \
            to wait for new readlines from serial port")
    parser.add_argument("-f", "--test_file", help="Test filename with extension  \
            to run a test on and print out results. Test file must be in same dir.")
    args = parser.parse_args()
    return args

def log(level, message):
    """ Wrapper for whichever logging solution I end up plugging in """
    print "[%s]\t%s" % (level, message)
