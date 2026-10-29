"""Provision Wi-Fi over USB without putting credentials in the source tree."""
import argparse
import getpass
import json
import time

import serial

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--port', required=True, help='For example COM8 or /dev/ttyUSB0')
parser.add_argument('--ssid', help='Wi-Fi network name')
args = parser.parse_args()
ssid = args.ssid or input('Wi-Fi network: ')
password = getpass.getpass('Wi-Fi password: ')
connection = serial.Serial()
connection.port = args.port
connection.baudrate = 115200
connection.timeout = 0.5
connection.dtr = False
connection.rts = False
connection.open()
payload = ('WIFI ' + json.dumps({'ssid': ssid, 'password': password}) + '\n').encode()
connection.write(payload)
deadline = time.monotonic() + 12
while time.monotonic() < deadline:
    line = connection.readline().decode('utf-8', errors='replace').strip()
    if line == 'WIFI_SAVED':
        print('Wi-Fi details saved on the board.')
        connection.close()
        break
else:
    connection.close()
    raise SystemExit('No acknowledgment. Check the port and restart the board, then retry.')
