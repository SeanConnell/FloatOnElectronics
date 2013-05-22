#!/usr/bin/python
"""
Actual program entry point
"""
from cmdline_tools import parse_config, init_parser, log, INFO
from helm_reporter import read_data, open_comm, open_test_file

args = init_parser()
config_values = parse_config(args.config)

#Default values to config values
reporting_uri = config_values.get("Network", "reporting_uri")
com_port = config_values.get("Serial", "com_port")
baud_rate = int(config_values.get("Serial", "baud_rate"))
timeout = int(config_values.get("Serial", "timeout"))
test_file = None
data_source = None

#Override anything passed as a cmdline arg 
if args.report_uri is not None:
    reporting_uris = args.report_uri
if args.com_port is not None:
    com_port = args.com_port
if args.baud_rate is not None:
    baud_rate = args.baud_rate
if args.timeout is not None:
    timeout = args.timeout

data_source = open_comm(com_port, baud_rate, timeout)

#test mode
if args.test_file is not None:
    test_file = args.test_file
    reporting_uris = "http://localhost:8080"
    data_source = open_test_file(test_file)
    log(INFO, "Doing a test run with %s for input" % test_file)

read_data(data_source, reporting_uri)
