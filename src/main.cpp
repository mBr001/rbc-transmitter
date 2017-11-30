#include <RFM69.h>
#include <SPI.h>
// #include <list>
//#include <SPIFlash.h>

// disables or enables printing to serial
#define print(var) Serial.print(var)
// #define print(var)

#define NODEID 99
#define NETWORKID 100
#define GATEWAYID 1
#define FREQUENCY                                                              \
  RF69_868MHZ // RF69_433MHZ //Match this with the version of your Moteino!
              // (others: RF69_433MHZ, RF69_868MHZ)
#define KEY                                                                    \
  "sampleEncryptKey" // has to be same 16 characters/bytes on all nodes, not
                     // more not less!
#define LED 9
#define BAUDRATE 57600
#define ACK_TIME 50 // # of ms to wait for an ack
//#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you
// have RFM69W/CW!

int TRANSMITPERIOD = 200; // transmit a packet to gateway so often (in ms)
boolean requestACK = false;
// SPIFlash flash(8, 0xEF30); //EF40 for 16mbit windbond chip
RFM69 radio;

typedef struct {
  byte data[255];
} Payload;
Payload payload;

byte received[255];
boolean is_complete = false;

void blink(byte pin, int delay_ms) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  delay(delay_ms);
  digitalWrite(pin, LOW);
}

void pong() {
  print('[');
  print(radio.SENDERID);
  print("] ");
  for (byte i = 0; i < radio.DATALEN; i++)
    print((char)radio.DATA[i]);
  print("   [RX_RSSI:");
  print(radio.readRSSI());
  print("]");

  if (radio.ACKRequested()) {
    radio.sendACK();
    print(" - ACK sent");
    delay(10);
  }
  blink(LED, 5);
}

void clean_payload_data() {
  int data_length = sizeof(payload.data) / sizeof(payload.data[0]);
  for (int i = 0; i < data_length; i++) {
    payload.data[i] = null;
  }
}

void setup() {
  Serial.begin(BAUDRATE);
  radio.initialize(FREQUENCY, NODEID, NETWORKID);
  // radio.setHighPower(); //uncomment only for RFM69HW!
  radio.encrypt(KEY);
  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...",
          FREQUENCY == RF69_433MHZ ? 433
                                   : FREQUENCY == RF69_868MHZ ? 868 : 915);
  print(buff);
}

void send() {
  print("Sending struct (");
  print(sizeof(payload));
  print(" bytes) ... ");
  if (radio.sendWithRetry(GATEWAYID, (const void *)(&payload), sizeof(payload)))
    print(" ok!\n");
  else
    print(" nothing...\n");
  blink(LED, 3);
}

long lastPeriod = -1;
void loop() {
  if (is_complete) {
    // accept only our glorious protocol
    if (received[0] == 0xFF) {
      print("ACCEPTED: ");
      for (int i = 0; i < 255; i++) {
        payload.data[i] = received[i];
        print(payload.data[i]);
      }
      print("\n");
    } else {
      print("REJECTED\n");
    }
    is_complete = false;
  }

  // check for any received packets
  if (radio.receiveDone()) {
    pong();
  }

  int currPeriod = millis() / TRANSMITPERIOD;
  if (currPeriod != lastPeriod) {
    send();
    lastPeriod = currPeriod;
    clean_payload_data();
  }

} // loop

void serialEvent() {
  int i = 0;
  while (Serial.available()) {
    received[i] = Serial.read();
    // Serial.println(received[i], HEX);
    if (received[i] == 0x0A) {
      is_complete = true;
    }
    i++;
  }
}
