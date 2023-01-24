/*
   This is a digipeater node

*/

//Please change these two to describe your hardware.
#define CALLSIGN "KD2LYD-67"
#define COMMENTS "test digipeater node"
#define DEBUG 1

#include <SPI.h>
#include <RH_RF95.h>  //See http://www.airspayce.com/mikem/arduino/RadioHead/
#include <string.h>

/* for feather32u4
  #define RFM95_CS 8
  #define RFM95_RST 4
  #define RFM95_INT 7
  #define VBATPIN A9  /**/

/* for feather m0  */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define VBATPIN A7 /**/

//Uncomment this line to use the UART instead of USB in M0.
//#define Serial Serial1

/* for shield
  #define RFM95_CS 10
  #define RFM95_RST 9
  #define RFM95_INT 7
*/


/* for ESP w/featherwing
  #define RFM95_CS  2    // "E"
  #define RFM95_RST 16   // "D"
  #define RFM95_INT 15   // "B"
*/

/* Feather 32u4 w/wing
  #define RFM95_RST     11   // "A"
  #define RFM95_CS      10   // "B"
  #define RFM95_INT     2    // "SDA" (only SDA/SCL/RX/TX have IRQ!)
*/

/* Feather m0 w/wing
  #define RFM95_RST     11   // "A"
  #define RFM95_CS      10   // "B"
  #define RFM95_INT     6    // "D"
*/

/* Teensy 3.x w/wing
  #define RFM95_RST     9   // "A"
  #define RFM95_CS      10   // "B"
  #define RFM95_INT     4    // "C"
*/


// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 434.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Blinky on receipt
#define LED 13

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

// size of message history buffer
#define MSG_HISTORY_SZ 5
// how old a message needs to be to be evicted
#define EVICT_TIME 15000
// how long between doing voluntary evictions
// higher is better because it's less time evicting, more time doing message things
// but too high and you could have stale messages
#define EVICTION_PERIOD 60000

// maximum random() value in ms to wait before RTing
#define BACKOFF_MAX 250

unsigned long lasteviction = 0;

struct message {
  uint8_t content[256];
  unsigned int timereceived;
} messages_history[MSG_HISTORY_SZ];

//Returns the battery voltage as a float.
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
    while (1) {
      Serial.println("LoRa radio init failed");
      delay(10000);
    }
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    while (1) {
      Serial.println("setFrequency failed");
      delay(10000);
    }
    //while (1);
  } else {
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

void radiooff() {
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
}

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // disable DEBUG before release
  if (DEBUG) {
    while (!Serial) delay(10);
  }

  Serial.begin(9600);
  Serial.setTimeout(10);
  delay(100);

  if (DEBUG) {
    freemem((char*)__func__);
  }

  radioon();
  digitalWrite(LED, LOW);


  for (int i = 0; i < MSG_HISTORY_SZ; i++ ) {
    messages_history[i].content[0] = 0;
    messages_history[i].timereceived = 0;
  }
  lasteviction = millis();

  //Beacon once at startup.
  beacon();
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
void beacon() {
  if (DEBUG) {
    Serial.println("beaconing...");
  }
  unsigned long tstart = millis();
  static int packetnum = 0;
  float vcc = voltage();

  char radiopacket[RH_RF95_MAX_MESSAGE_LEN];
  snprintf(radiopacket,
           RH_RF95_MAX_MESSAGE_LEN,
           "BEACON %s %s VCC=%d.%03d count=%d uptime=%ld",
           CALLSIGN,
           COMMENTS,
           (int) vcc,
           (int) (vcc * 1000) % 1000,
           packetnum,
           uptime());

  radiopacket[sizeof(radiopacket)] = 0;

  addtomessageshistory((uint8_t*)radiopacket);

  rf95.send((uint8_t *)radiopacket, strlen((char*) radiopacket));

  rf95.waitPacketSent();
  packetnum++;
  if (DEBUG) {
    freemem((char*)__func__);
    Serial.print("beacon() took (ms): ");
    Serial.println(millis() - tstart);
  }
}

void dohistoryeviction() {
  unsigned long tstart = millis();
  if (DEBUG) {
    Serial.println("Eviction time..");
  }
  for (int i = 0; i < MSG_HISTORY_SZ; i++) {
    Serial.print(i);
    Serial.print(" ");
    Serial.print(messages_history[i].timereceived);
    Serial.print(" ");
    Serial.println((char*)messages_history[i].content);

    if ((messages_history[i].timereceived > 0) && messages_history[i].timereceived < (millis() - EVICT_TIME)) {
      messages_history[i].timereceived = 0;
      messages_history[i].content[0] = 0;
      if (DEBUG) {
        Serial.println("     ^ evicting that one");
      }
    }
  }
  if (DEBUG) {
    Serial.print("[+] eviction took (ms): ");
    Serial.println(millis() - tstart);
  }
}

bool existsinhistory(uint8_t *msg) {
  for (int i = 0; i < MSG_HISTORY_SZ; i++) {
    if (memcmp(messages_history[i].content, msg, strlen((char*)msg)) == 0) {
      return true;
    }
  }
  return false;
}

void addtomessageshistory(uint8_t *msg) {
  // check that it's not already in there
  bool inhistory = existsinhistory(msg);
  if (inhistory) {
    Serial.println("not adding to history: its already there");
    return;
  }

  // find an empty/old slot
  unsigned int oldest = messages_history[0].timereceived;
  unsigned int oldest_idx = 0;
  for (int i = 0; i < MSG_HISTORY_SZ; i++) {
    if (messages_history[i].timereceived < oldest) {
      oldest = messages_history[i].timereceived;
      oldest_idx = i;
    }
  }


  messages_history[oldest_idx].timereceived = millis();
  strcpy((char*)messages_history[oldest_idx].content, (char*)msg);
  Serial.println("> added to history");
  if (DEBUG) {
    if (oldest > 0) {
      Serial.println("> memory was full, so overwrote an item");
    }
  }
}

void printmessageshistory() {
  for (int i = 0; i < MSG_HISTORY_SZ; i++) {
    Serial.print(i);
    Serial.print(" ");
    Serial.print(messages_history[i].timereceived);
    Serial.print(" ");
    Serial.println((char*)messages_history[i].content);
  }
}

//Handles retransmission of the packet.
bool shouldirt(uint8_t *buf, uint8_t len) {
  //Don't RT if the packet is too long.
  if (strlen((char*) buf) > 200) {
    Serial.println("shouldirt? NO: Length is too long.\n");
    return false;
  }

  uint8_t buf2[256];
  memcpy(buf2, buf, 256);
  char *ptr = strchr((char*)buf2, '\n');
  if (ptr != NULL) {
    *ptr = 0;
  }
  Serial.print("chopped: ");
  Serial.println((char*)buf2);
  Serial.println("");

  if (existsinhistory(buf2)) {
    Serial.println("shouldirt? NO: its in our history\n");
    //printmessageshistory();

    return false;
  }

  addtomessageshistory(buf2);
  //printmessageshistory();

  /*
    //Don't RT any packet containing our own callsign.
    if (strstr((char*) buf, CALLSIGN)) { //originally was strcasestr but wouldn't compile TODO: fix that
      Serial.println("shouldirt? NO: I'm in the RT list already\n");
      return false;
    }
  */

  //Don't RT if we've gotten an incoming packet in that time.
  /* if(rf95.available()){
    Serial.println("Interrupted by another packet.");
    return false
    } */

  //No objections.  RT it!
  Serial.println("shouldirt? Yes!\n");

  //Random backoff if we might RT it.
  long rbackoff = random(BACKOFF_MAX);
  if (DEBUG) {
    Serial.print("backing off (ms): ");
    Serial.println(rbackoff);
  }
  delay(rbackoff);

  return true;
}

void freemem(char *func) {
  int m = freeMemory();
  Serial.print("in: ");
  Serial.print(func);
  Serial.print(", free memory: ");
  Serial.println(m);
}

//If a packet is available, digipeat it.  Otherwise, wait.
void digipeat() {
  //digitalWrite(LED, LOW);
  //Try to receive a reply.
  if (rf95.available()) {
    unsigned long tstart = millis();
    if (DEBUG) {
      Serial.println("------ new message received ------");
    }
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    int rssi = 0;
    float vcc = voltage();
    /*
       When we receive a packet, we repeat it after a random
       delay if:
       1. It asks to be repeated.
       2. We've not yet received a different packet.
       3. We've waited a random amount of time.
       4. The first word is not RT.
    */
    if (rf95.recv(buf, &len)) {
      rssi = rf95.lastRssi();
      //digitalWrite(LED, HIGH);
      //RH_RF95::printBuffer("Received: ", buf, len);
      //Serial.print("Got: ");
      buf[len] = 0;
      Serial.print("Received: ");
      Serial.print((char*)buf);
      Serial.println("");

      if (shouldirt(buf, len)) {
        // Retransmit.
        uint8_t data[RH_RF95_MAX_MESSAGE_LEN];
        snprintf((char*) data,
                 RH_RF95_MAX_MESSAGE_LEN,
                 "%s\n" //First line is the original packet.
                 "RT %s rssi=%d VCC=%d.%03d uptime=%ld", //Then we append our call and strength as a repeater.
                 (char*) buf,
                 CALLSIGN,  //Repeater's callsign.
                 (int) rssi, //Signal strength, for routing.
                 (int) vcc,
                 (int) (vcc * 1000) % 1000,
                 uptime()
                );
        rf95.send(data, strlen((char*) data));
        rf95.waitPacketSent();
        Serial.println((char*) data);
        //digitalWrite(LED, LOW);
        Serial.println("");

        if (DEBUG) {
          freemem((char*)__func__);
        }
      } else {
        //Serial.println("Declining to retransmit.\n");
      }
    } else {
      Serial.println("Receive failed");
    }
    if (DEBUG) {
      Serial.print("digipeat() took (ms): ");
      Serial.println(millis() - tstart);
    }
  } else {
    // TODO: why?
    delay(10);
  }
}

void loop() {
  static unsigned long lastbeacon = millis();

  //Only digipeat if the battery is in good shape.
  if (voltage() > 3.25) {
    //Only digipeat when battery is high.
    digipeat();

    //Every ten minutes, we beacon just in case.
    if (millis() - lastbeacon > 10 * 60000) {
      beacon();
      lastbeacon = millis();
    }

    if (millis() - lasteviction > EVICTION_PERIOD) {
      dohistoryeviction();
      lasteviction = millis();
    }
  } else {
    //Transmit a beacon every ten minutes when battery is low.
    radiooff();
    // TODO: go real deep sleep (sleepydog)
    Serial.println("Low battery. Sleeping.");
    delay(10 * 60000);
    radioon();
    beacon();
  };
}
