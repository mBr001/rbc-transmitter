#include <RFM69.h>
#include <SPI.h>
#include <vector>
//#include <SPIFlash.h>
using namespace std;
// disables or enables printing to serial
#define print(var) Serial.print(var)
// #define print(var)

#define NODEID 99
#define NETWORKID 100
#define GATEWAYID 1
#define FREQUENCY                                                                                                      \
  RF69_868MHZ // RF69_433MHZ //Match this with the version of your Moteino!
              // (others: RF69_433MHZ, RF69_868MHZ)
#define KEY                                                                                                            \
  "sampleEncryptKey" // has to be same 16 characters/bytes on all nodes, not
                     // more not less!
#define LED 9
#define BAUDRATE 57600
#define ACK_TIME 50 // # of ms to wait for an ack
//#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you
// have RFM69W/CW!

int TRANSMITPERIOD = 200; // transmit a packet to gateway so often (in ms)
int licz = 0;
byte sendSize = 0;
boolean requestACK = false;
// SPIFlash flash(8, 0xEF30); //EF40 for 16mbit windbond chip
RFM69 radio;

byte data[61];
vector<byte> received;

boolean is_complete_frame = false;

void blink(byte pin, int delay_ms) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  delay(delay_ms);
  digitalWrite(pin, LOW);
}

void pong() {
  print('[');
  // Sprintln(radio.SENDERID, DEC);
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
  // println();
}

void slice61(vector<byte> received, int i) {
  vector<byte>::const_iterator first = received.begin() + 61 * i;
  vector<byte>::const_iterator last = first + 61;
  copy(first, last, data);
}

void setup() {
  Serial.begin(BAUDRATE);
  radio.initialize(FREQUENCY, NODEID, NETWORKID);
  // radio.setHighPower(); //uncomment only for RFM69HW!
  radio.encrypt(KEY);
  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...\n", FREQUENCY == RF69_433MHZ ? 433 : FREQUENCY == RF69_868MHZ ? 868 : 915);
  print(buff);
}

void send() {
  print("Sending (");
  print(sizeof(data));
  print(" bytes) ... ");
  if (radio.sendWithRetry(GATEWAYID, (const void *)(&data), sizeof(data))) {
    print(" ok!\n");
  } else {
    print(" nothing...\n");
  }

  blink(LED, 3);
}

void clear_data() { memset(&data[0], 0, sizeof(data)); }

long lastPeriod = -1;
void loop() {
  if (is_complete_frame) {
    // accept only our glorious protocol
    if (received.front() == 0xFF) {
      int nr_packets_to_send = (int)ceil(received.size() / 61.0);
      print("ACCEPTED. Need to send: ");
      if (received.size() <= 61) {
        print(nr_packets_to_send);
        print(" packet(s)\n");
        slice61(received, 0);
        send();
        clear_data();
      } else {
        print(nr_packets_to_send);
        print(" packet(s)\n");
        for (int i = 0; i < nr_packets_to_send; i++) {
          slice61(received, 0);
          send();
          clear_data();
        }
      }

      // std::vector<double> v;
      // double *a = &v[0];
      print("\n");
    } else {
      print("REJECTED\n");
    }
    is_complete_frame = false;
    received.clear();
  }

  if (radio.receiveDone()) {
    // pong();
  }

  // int currPeriod = millis() / TRANSMITPERIOD;
  // if (currPeriod != lastPeriod) {
  //   send();
  //   lastPeriod = currPeriod;
  // }
}

void serialEvent() {
  while (Serial.available()) {
    byte incoming_byte = Serial.read();
    received.push_back(incoming_byte);
    // Serial.println(received[i], HEX);
    if (incoming_byte == 0x0A) {
      is_complete_frame = true;
    }
  }
}
