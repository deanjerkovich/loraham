# Range Test

This is a pair of nodes: a beacon & a receiver. The beacon transmits its GPS location every second, the receiver compares the transmitted GPS co-ordinates to its own location and calculates a distance, which is displayed along with RSSI and actual lat/long of both stations on the Oled screen.

The key idea here is a simple setup to encourage distance testing of various antennas and equipment.

### Nodes

[beacon-gps](beacon-gps/): read from GPS and transmit location regularly

[display-gps](display-gps/): recieve GPS points from beacon, compare to location from own GPS module, display stats on OLED screen such as distance, RSSI, lat/long of both.