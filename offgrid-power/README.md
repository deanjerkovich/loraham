# Off-grid power

Running nodes where you don't have an outlet

!nocommit

## Power requirements

The following are some simple real-world tests of how long a given type of node will run for a particular battery capacity. These can be used as rough guides to how much power you'll need if you want 24x7 uptime.

| Node | Battery | Notes | Battery life | mAh/day |
|-------|-------|-----| ---- | ---- |
| GPS| 400mAh | Beaconing every 10s |11.5 hours | 834 mAh |
| SD| 400mAh | Beaconing disabled (RX-only) |TODO |
| digipeater|500mAh | Medium/low traffic  | 17.4 hours | 689 mAh
| sleepy beacon|500mAh | Beacon every 10 mins  | WIP |

## Solar garden lights

According to the various adafruit docs ([M0 radio](https://learn.adafruit.com/adafruit-feather-m0-radio-with-lora-radio-module/power-management) & [feather spec](https://learn.adafruit.com/adafruit-feather/feather-specification)), the boards are happy with 5 volt via USB, or somewhere from 3.0 - 4.7 volts via the LiPoly connector. Whilst you can achieve 5v from nearly any voltage with a [boost converter](https://www.adafruit.com/product/4654), it's better to choose a light that already provides the right voltage.

The most common batteries in solar garden lights are either 3.2v or 3.7v, in LiPo or Li-ion, or LiFePO4. Whilst I have successfully deployed (and they're still running!) nodes using 3.2 volt circuits, I prefer to shoot for 3.7 volt setups with larger mAh ratings.

The biggest issue with solar garden lights is the power consumed rather than voltage. The following popular lights have a single LiFePO4 14430 45mAh 3.2V (1.44Wh) cell, and a fairly small PV panel on top. This wouldn't be enough to power a GPS node or repeater without exceptional sunlight duration and quality.

![magnetic solar](/images/solar_magnetic.jpg)
*Need larger PV panels* 
## Adafruit Solar Panel & Charger

tktk

## Hand Crank Power / Dynamo

Eton/Red Cross sell a $9.99 hand cranked flashlight (which also has an uterly useless USB port marketed as a cellphone charger). By soldering in a JST-PH socket to the battery, you can power a Feather board with hand crank - or connect the dynamo up to a constant rotation source.

![marketing shot](/images/hand_crank_original.jpeg)
*Hand crank dynamo* \
\
\
\
![hand crank](/images/crank_power_open.jpg)
*Adding a connector* \
\
\
\
![hand crank runing](/images/crank_power_running.jpg)
*Powered up*