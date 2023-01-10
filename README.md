# LoRaHam

This project includes things to communicate data over the 70cm ham radio band using LoRa radios,  inexpensive off-the-shelf hardware, and code examples designed for easy deployment.

This is all entirely built on the wonderful work of Travis Goodspeed (KK4VCZ) located at [loraham.org](https://loraham.org) and [github](https://github.com/travisgoodspeed/loraham/).

# Getting Started

You'll need 

- Two radio boards
- This code
- An amateur radio license

# The Protocol

The protocol is a simple ASCII text based messaging format sent over LoRa with fairly standard RF parameters.

For now, see [Travis' description](https://tktk)

In future, further details will be at: [/protocol/](/protocol/)

# Hardware

The current board of choice for these experiments is the Adafruit Feather M0 RFM96 LoRa Radio, specifically in 433MHz/70cm band [available here](https://www.adafruit.com/product/3179).

The primary chip on these boards is the ATSAMD21G18 ARM Cortex M0 processor, which has:
- 48 MHz clock
- 3.3V logic, 
- 256K of FLASH 
- 32K of RAM

This chip is then connected to a SX127x module with 100mW of transmit power.

![image](images/feather-boards.jpg)

The above image shows four feather boards, three which have simple coil antennas and featherwings (an OLED screen, a GPS receiver, and an SD logger). Also shown is a large 6600mAh battery and a couple of 3d printed cases. Hidden from view are 400mAh batteries that fit in between the feather & featherwing boards.

Some benefits of using these boards:

- They're entirely compatible with the arduino ecosystem, so writing and loading code is simple
- There are a huge variety of "featherwings" that can be added on top to extend functionality - GPS, SD card, environmental sensors, OLED displays, you name it
- It's really easy to hook up i2c other peripheral devices, especially using i2c via the Stemma/Qwicc connectors (basically, a standardized way of doing i2c supported by Adafruit & [Sparkfun](https://www.sparkfun.com/qwiic))
- The library support is fantastic, they're well documented & typically have a great guide to get up and running
- A wide range of power sources available off the shelf: solar, LiPo batteries from miniscule to fairly large, USB, etc.
- They're easy to design 3D printable cases for; a few already exist for free
- Low power consumption, with a fair ability to tune for very low (although, will never be comparable to purpose-specific hardware obviously)

## Solar power

There are numerous ways you can have these boards running entirely off solar power. The easiest way is to buy a panel, charge circuit, and battery specifically designed for these boards. But by far the cheaper and more convenient way is to purchase an outdoor solar light and wire your board into that.

You'll get:
- A weather sealed enclosure
- A solar panel
- A battery (often of decent capacity)
- A charge controller
- Mounting hardware
- Spare LEDs for other projects

All for about $10 - $20!

See [offgrid-power](/offgrid-power/) for more details

# Software

The software run on each device is typically node specific, but reuses a fair amount (most of it should be shifted to a library).

All of the arduino code on the Feathers is simple C; the raspberry pi code is python. 

See the individual nodes for actual code.

# Nodes

- [sd-logger-node](/sd-logger-node/): A mobile node that logs all traffic to SD card in JSON format.
- [oled-node](/oled-node/): A mobile node with an OLED screen for displaying messages
- [loraham-pi](/loraham-pi/): A node built around a raspberry pi for increased processing power, storage, and WiFi/Bluetooth capability. less mobile, but still extremely mobile compared to most things.

# Enclosures

### Project case

tktk something

### Plastidip

tktk something

### 3d printed

tktk something

# Experiments

## Battery life / Power consumption

See [off-grid power](/offgrid-power/)


## Range Tests

All range tests are done with the [range test](/nodes/range-test/) nodes which calculate distance between two GPS modules.

| Radio | Antennas |Tx Power |Distance  |  RSSI|  Notes | 
|-------|-------|---|--------| ---- | ---- |
| Feather m0| [Coil](https://www.adafruit.com/product/4394) on both ends | 23db / 100mW |1007 meters | -104| Not quite line of sight | 

Todo
- TX: coil RX: Tuned omni
- TX: yagi RX: Tuned omni
- TX: 5w amp RX: Tuned omni

<!--- 
## Solar Power

todo

## GPS Topo mapping

todo

--> 