#include <Adafruit_GPS.h>

#define GPSSerial Serial1

#define CALLSIGN "KD2LYD-45"
#define COMMENTS "GPS Beacon"

#include <SPI.h>
#include <RH_RF95.h>  //See http://www.airspayce.com/mikem/arduino/RadioHead/

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define VBATPIN A7

#define RF95_FREQ 434.0

#define LED 13

#define BEACON_DELAY 1 //how many seconds to wait between beacons

RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO false

uint32_t timer = millis();

float voltage() {
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  return measuredvbat;
}

void radioon() {
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);


  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
  Serial.println("Set power to 23.");
  Serial.print("Max packet length: "); Serial.println(RH_RF95_MAX_MESSAGE_LEN);
}

void radiooff() {
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
}

void setup()
{
  //while (!Serial);  // uncomment to have the sketch wait until Serial is ready

  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  Serial.begin(115200);
  Serial.println("Adafruit GPS library basic parsing test!");

  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);

  delay(1000);

  // Ask for firmware version
  GPSSerial.println(PMTK_Q_RELEASE);

  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  delay(100);

  radioon();
  digitalWrite(LED, LOW);

}

//! Uptime in seconds, correcting for rollover.
long int uptime() {
  static unsigned long rollover = 0;
  static unsigned long lastmillis = millis();

  //Account for rollovers, every ~50 days or so.
  if (lastmillis > millis()) {
    rollover += (lastmillis >> 10);
    lastmillis = millis();
  }

  return (rollover + (millis() >> 10));
}

//Transmits one beacon and returns.
void beacon(float latitude, float longitude, float altitude, int satellites, int fixquality, char* timedate) {
  static int packetnum = 0;
  float vcc = voltage();

  char radiopacket[RH_RF95_MAX_MESSAGE_LEN];
  snprintf(radiopacket,
           RH_RF95_MAX_MESSAGE_LEN,
           "BEACON %s %s VCC=%d.%03d count=%d uptime=%ld gps=%f,%f,%0.2f,%d,%d gdate=%s",
           CALLSIGN,
           COMMENTS,
           (int) vcc,
           (int) (vcc * 1000) % 1000,
           packetnum,
           uptime(),
           latitude,
           longitude,
           altitude,
           satellites,
           fixquality,
           timedate);

  radiopacket[sizeof(radiopacket)] = 0;

  Serial.println("Sending packet.."); delay(10);
  rf95.send((uint8_t *)radiopacket, strlen((char*) radiopacket));

  rf95.waitPacketSent();
  packetnum++;
}

void loop() // run over and over again
{
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if (GPSECHO)
    if (c) Serial.print(c);

  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trying to print out data
    if (GPSECHO) {
      Serial.print(GPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
    }
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }

  unsigned int td_len = 32;
  char timedate[td_len];
  if (millis() - timer > (BEACON_DELAY * 1000)) {
    timer = millis(); // reset the timer

    Serial.print("gps antenna: ");
    if (GPS.antenna == 2) {
      Serial.println("Internal");
    } else if (GPS.antenna = 3) {
      Serial.println("External");
    } else {
      Serial.println("Error");
    }

    if (GPS.fix) {
      Serial.println("We have a fix");
      Serial.print(GPS.latitudeDegrees, 10);
      Serial.print(",");
      Serial.println(GPS.longitudeDegrees, 10);
      Serial.print("Speed (knots): "); Serial.println(GPS.speed);
      Serial.print("Angle: "); Serial.println(GPS.angle);
      Serial.print("Altitude: "); Serial.println(GPS.altitude);
      Serial.print("Satellites: "); Serial.println((int)GPS.satellites);
      snprintf(timedate, td_len, "%u-%u-%u_%02u:%02u:%02u", GPS.year, GPS.month, GPS.day, GPS.hour, GPS.minute, GPS.seconds);
      Serial.print("date:");
      Serial.println(timedate);
      //radioon();
      beacon(GPS.latitudeDegrees, GPS.longitudeDegrees, GPS.altitude, (int)GPS.satellites, GPS.fixquality, timedate);
      //radiooff();
    } else {
      snprintf(timedate, td_len, "%u-%u-%u_%u:%u:%u", 0, 0, 0, 0, 0, 0);
      Serial.print("timedate:");
      Serial.println(timedate);
      beacon(0.0, 0.0, 0.0, 0, 0, timedate);
    }
    Serial.println("");
  }
}
