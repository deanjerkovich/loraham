#include <SPI.h>
#include <SD.h>
#include <RH_RF95.h>  //See http://www.airspayce.com/mikem/arduino/RadioHead/
#include <string.h>

const int chipSelect = 10;
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define VBATPIN A7 /**/

#define LED 13

#define MAX_JSON_LEN RH_RF95_MAX_MESSAGE_LEN+36
#define MAX_FSIZE 10000

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 434.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

int fileNum = 0;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  /*while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }*/
  
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  digitalWrite(8,HIGH);
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");
  radioon();
  digitalWrite(LED, HIGH);

}

void radioon(){
  Serial.println("Enabling radio.");
  
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
 
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    //while (1);
  }
  Serial.println("LoRa radio init OK!");
 
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    //while (1);
  }else{
    Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  }
 
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
 
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
  Serial.println("Set power to 23.");
  Serial.print("Max packet length: "); Serial.println(RH_RF95_MAX_MESSAGE_LEN);
}


void radiooff(){
  // manual reset
  Serial.println("Disabling radio");
  digitalWrite(RFM95_RST, LOW);
  digitalWrite(RFM95_CS,HIGH);
  delay(10);
}

void writelog(String s, int rssi){
   Serial.println("Logging to SD");

  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }

  // 8.3 filenames in use; cannot call it .json
  char fName[12];
  sprintf(fName, "data-%d.txt", fileNum);
  File dataFile = SD.open(fName, FILE_WRITE);
  while (dataFile.size() > MAX_FSIZE) {
    dataFile.close();
    fileNum++;
    sprintf(fName, "data-%d.txt", fileNum);
    dataFile = SD.open(fName, FILE_WRITE);
  }

  Serial.print("writing to: ");
  Serial.println(fName);

  // if the file is available, write to it:
  if (dataFile) {
    //dataFile.println(s);
    dataFile.print("{\"packet\": \"");
    dataFile.print(s);
    dataFile.print("\", \"direction\":\"rx\", \"rssi\":\"");
    dataFile.print(rssi);
    dataFile.print("\"}\n");
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening log file.");
  }
}

void listen() {
   if (rf95.available()){
    // Should be a message for us now  
    Serial.println("+ Message available\n"); 
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    int rssi=0;
    //float vcc=voltage();

    if (rf95.recv(buf, &len)){
      rssi=rf95.lastRssi();
      Serial.print("Got: ");
      buf[len]=0;
      Serial.println((char*)buf);
      Serial.print("Len: ");
      Serial.println(len);
      Serial.println("");

      radiooff();
      writelog((const char *)buf, rssi);
      radioon();

    }else{
      Serial.println("+ Receive failed");
    }
  }else{
    delay(10);
  }
}

void loop() {
  // make a string for assembling the data to log
  listen();
}
