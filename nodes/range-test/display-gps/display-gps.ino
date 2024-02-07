#include <Adafruit_GPS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <TinyGPSPlus.h>


// what's the name of the hardware serial port?
//#define GPSSerial Serial1

#define CALLSIGN "KD2LYD-55"
#define COMMENTS "comment='GPS Distance Display'"

#include <SPI.h>
#include <RH_RF95.h>  //See http://www.airspayce.com/mikem/arduino/RadioHead/

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define VBATPIN A7

#define RF95_FREQ 434.0

#define LED 13
uint8_t led_status = 0;

#define BEACON_DELAY 10000 // milliseconds

RH_RF95 rf95(RFM95_CS, RFM95_INT);

Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);

// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&Wire);


// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO false

uint32_t timer = millis();
double maxdistance = 0.0;

int packetCtr = 0;

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
  Serial.println("gps displayer test 001");

  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(0x10);  // The I2C address to use is 0x10
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

  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  delay(100);

  radioon();
  digitalWrite(LED, LOW);

  display.begin(0x3C, true); // Address 0x3C default
  display.display();
  display.clearDisplay();
  display.display();

  display.setRotation(1);

  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.print("Starting...");
  display.display();

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
void beacon(float latitude, float longitude, char lat, char lon, float altitude, int satellites, int fixquality, char* timedate) {
  static int packetnum = 0;
  float vcc = voltage();

  char radiopacket[RH_RF95_MAX_MESSAGE_LEN];
  snprintf(radiopacket,
           RH_RF95_MAX_MESSAGE_LEN,
           "BEACON %s %s VCC=%d.%03d count=%d uptime=%ld gps=%f%c,%f%c,%0.2f,%d,%d gdate=%s",
           CALLSIGN,
           COMMENTS,
           (int) vcc,
           (int) (vcc * 1000) % 1000,
           packetnum,
           uptime(),
           latitude,
           lat,
           longitude,
           lon,
           altitude,
           satellites,
           fixquality,
           timedate);

  radiopacket[sizeof(radiopacket)] = 0;

  Serial.println("Sending..."); delay(10);
  rf95.send((uint8_t *)radiopacket, strlen((char*) radiopacket));

  rf95.waitPacketSent();
  packetnum++;
}

void loop() // run over and over again
{
  // Try and read our own GPS device
  char c = GPS.read();
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA())) {
      Serial.println("parsing our own GPS failed. Will try again next time");
      return;
    }
    /*
      if (GPS.fix) {
      Serial.print("Location: ");
      Serial.print(GPS.latitude, 4); Serial.print(GPS.lat);
      Serial.print(", ");
      Serial.print(GPS.longitude, 4); Serial.println(GPS.lon);
      Serial.print("Speed (knots): "); Serial.println(GPS.speed);
      Serial.print("Angle: "); Serial.println(GPS.angle);
      Serial.print("Altitude: "); Serial.println(GPS.altitude);
      Serial.print("Satellites: "); Serial.println((int)GPS.satellites);
      }
    */
  }

  // have we received a LoRa message?
  if (rf95.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    int rssi = 0;
    float vcc = voltage();

    if (rf95.recv(buf, &len)) {
      rssi = rf95.lastRssi();
      //digitalWrite(LED, HIGH);
      //RH_RF95::printBuffer("Received: ", buf, len);
      //Serial.print("Got: ");

      Serial.println("We have received a LoRa message");
      buf[len] = 0;
      if (strstr((char *)buf, "gps=") && !strstr((char*)buf, "RT") && !strstr((char*)buf, "gps=0.0")) {
        Serial.print("It's GPS: ");

        if (led_status) {
          // led is on, turn off
          digitalWrite(LED, LOW);
          led_status = 0;
        } else {
          // led is off, turn on
          digitalWrite(LED, HIGH);
          led_status = 1;
        }
        
        packetCtr++;
        String recvd = String((char*)buf);
        String gpsRecvd = recvd.substring(recvd.indexOf("gps=") + 4);
        int idx1 = gpsRecvd.indexOf(",");
        int idx2 = gpsRecvd.indexOf(",", idx1 + 1);
        gpsRecvd = gpsRecvd.substring(0, idx2);

        Serial.println(gpsRecvd);

        float latr = gpsRecvd.substring(0, gpsRecvd.indexOf(",")).toFloat();
        float lonr = gpsRecvd.substring(gpsRecvd.indexOf(",") + 1).toFloat();

        //Serial.println(latr,10);
        //Serial.println(lonr,10);

        //Serial.println((char*)buf);
        if (GPS.fix) {
          Serial.print("Our location: ");
          //Serial.print(GPS.latitude, 6); Serial.print(GPS.lat); //.lat and .lon are the N/E/W/S
          //Serial.print(", ");
          //Serial.print(GPS.longitude, 6); Serial.println(GPS.lon);
          Serial.print(GPS.latitudeDegrees, 8);
          Serial.print(",");
          Serial.println(GPS.longitudeDegrees, 8);

          double distance = TinyGPSPlus::distanceBetween(GPS.latitudeDegrees, GPS.longitudeDegrees, latr, lonr);
          Serial.print("distance: ");
          Serial.println(distance);

          if (distance > maxdistance) {
            maxdistance = distance;
          }

          char buffer[20];
          sprintf(buffer, "%f,%f", GPS.latitudeDegrees, GPS.longitudeDegrees);

          //display.setTextSize(1);
          //display.setTextColor(SH110X_WHITE);
          display.clearDisplay();
          display.setCursor(0, 0);
          display.print("This unit:\n");
          display.print(buffer);
          display.print("\nBeacon:\n");
          display.print(gpsRecvd);
          display.print("\nDistance: ");
          display.print(distance);
          display.print("m\nMax: ");
          display.print(maxdistance);
          display.print("\nOur sats: ");
          display.print(GPS.satellites);
          display.print("\nRSSI: ");
          display.print(rssi);
          display.print("  pkts: ");
          display.print(packetCtr);
          display.display();
        } else {
          Serial.println("not updating display because we have no GPS Fix");
        }
      } else {
        Serial.println("It's not valid GPS data");
      }
    }
  }
}


/*
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
  if (GPSECHO){
    Serial.print(GPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
  }
  if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
    return; // we can fail to parse a sentence in which case we should just wait for another
  }

  // approximately every X seconds or so, print out the current stats
  unsigned int td_len = 32;
  char timedate[td_len];
  if (millis() - timer > BEACON_DELAY) {
  timer = millis(); // reset the timer
  Serial.print("\nTime: ");
  if (GPS.hour < 10) { Serial.print('0'); }
  Serial.print(GPS.hour, DEC); Serial.print(':');
  if (GPS.minute < 10) { Serial.print('0'); }
  Serial.print(GPS.minute, DEC); Serial.print(':');
  if (GPS.seconds < 10) { Serial.print('0'); }
  Serial.print(GPS.seconds, DEC); Serial.print('.');
  if (GPS.milliseconds < 10) {
    Serial.print("00");
  } else if (GPS.milliseconds > 9 && GPS.milliseconds < 100) {
    Serial.print("0");
  }
  Serial.println(GPS.milliseconds);
  Serial.print("Date: ");
  Serial.print(GPS.day, DEC); Serial.print('/');
  Serial.print(GPS.month, DEC); Serial.print("/20");
  Serial.println(GPS.year, DEC);
  Serial.print("Fix: "); Serial.print((int)GPS.fix);
  Serial.print(" quality: "); Serial.println((int)GPS.fixquality);

  snprintf(timedate, td_len, "%u-%u-%u_%u:%u:%u", GPS.year, GPS.month, GPS.day, GPS.hour, GPS.minute, GPS.seconds);
  Serial.print("djdate:");
  Serial.println(timedate);

  if (GPS.fix) {
    Serial.print("Location: ");
    Serial.print(GPS.latitude, 4); Serial.print(GPS.lat);
    Serial.print(", ");
    Serial.print(GPS.longitude, 4); Serial.println(GPS.lon);
    Serial.print("Speed (knots): "); Serial.println(GPS.speed);
    Serial.print("Angle: "); Serial.println(GPS.angle);
    Serial.print("Altitude: "); Serial.println(GPS.altitude);
    Serial.print("Satellites: "); Serial.println((int)GPS.satellites);
      radioon();
      //Transmit a beacon once every five minutes.
      snprintf(timedate, td_len, "%u-%u-%u_%02u:%02u:%02u", GPS.year, GPS.month, GPS.day, GPS.hour, GPS.minute, GPS.seconds);
      Serial.print("djdate:");
      Serial.println(timedate);
      beacon(GPS.latitude, GPS.longitude, GPS.lat, GPS.lon, GPS.altitude, (int)GPS.satellites, GPS.fixquality, timedate);
      //Then turn the radio off to save power.
      radiooff();
  } else {
      radioon();
      //Transmit a beacon once every five minutes.
      snprintf(timedate, td_len, "%u-%u-%u_%u:%u:%u", 0,0,0,0,0,0);
      Serial.print("djdate:");
      Serial.println(timedate);
      //void beacon(float latitude, float longitude, char lat, char lon, float altitude, int satellites, int fixquality, char* timedate){
      beacon(0.0, 0.0, 'X', 'X',0.0, 0,0, timedate);
      //Then turn the radio off to save power.
      radiooff();
  }
  }
*/
