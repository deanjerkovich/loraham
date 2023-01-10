"""
Based on code by Brent Rubell for Adafruit Industries
"""
import time
import busio
from digitalio import DigitalInOut, Direction, Pull
import board
import adafruit_rfm9x
import random
import json
import os
import sys
import signal

# Button A
btnA = DigitalInOut(board.D5)
btnA.direction = Direction.INPUT
btnA.pull = Pull.UP

# Button B
btnB = DigitalInOut(board.D6)
btnB.direction = Direction.INPUT
btnB.pull = Pull.UP

# Button C
btnC = DigitalInOut(board.D12)
btnC.direction = Direction.INPUT
btnC.pull = Pull.UP

# Create the I2C interface.
i2c = busio.I2C(board.SCL, board.SDA)

# 128x32 OLED Display
reset_pin = DigitalInOut(board.D4)

# Configure LoRa Radio
CS = DigitalInOut(board.CE1)
RESET = DigitalInOut(board.D25)
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)
rfm9x = adafruit_rfm9x.RFM9x(spi, CS, RESET, 434.0)
rfm9x.tx_power = 23
rfm9x.enable_crc = True

callsign = "TKTK"

debug_print = False

def configure():
    signal.signal(signal.SIGINT, handler)
    global debug_print

    if "debug" in os.environ:
        print("enabling debug")
        debug_print=True
    if os.getenv("debug") == "true":
        debug_print=True
    if "CALLSIGN" not in os.environ:
        print("please set CALLSIGN environment variable")
        sys.exit(1)
    global callsign
    callsign = os.getenv("CALLSIGN")
    if debug_print:
        print("Callsign set to: ",callsign)

def handler(signum, frame):
    print("Caught Ctrl-C, exiting")
    sys.exit(0)

def loop():
    if debug_print:
        print("Pi Gateway Started.")
    while True:
        packet = None

        # check for packet rx
        packet = rfm9x.receive()
            # we got a packet. RT it, send to log and pubsub
        if packet == None:
            time.sleep(0.2)
            continue
        try:
            packet_text = str(packet, "utf-8")
        except Exception:
            print("error decoding packet, skipping\n")
            continue
        #print(packet_text)

        logdata = {}
        logdata["time"] = time.time()
        logdata["packet"] = packet_text
        logdata["direction"] = "rx"
        logdata["rssi"] = "{}".format(rfm9x.last_rssi)
        logdata["snr"] = "{}".format(rfm9x.last_snr)
        out = json.dumps(logdata)
        print(out,flush=True)
        

        delay = random.randrange(1,20)/100
        time.sleep(delay)
        packet_rt = packet_text + "\nRT {} rssi={}".format(callsign, rfm9x.last_rssi)
        rfm9x.send(bytes(packet_rt,"utf-8"))

        logdata = {}
        logdata["time"] = time.time()
        logdata["packet"] = packet_rt
        logdata["direction"] = "tx"
        out = json.dumps(logdata)
        print(out,flush=True)

        if not btnA.value:
            test_data = "BEACON " + callsign + " TEST"
            rfm9x.send(bytes(test_data,"utf-8"))
            print(test_data)
            logdata = {}
            logdata["time"] = time.time()
            logdata["packet"] = test_data
            logdata["direction"] = "tx"
            out = json.dumps(logdata)

        time.sleep(0.1)


configure()
loop()
