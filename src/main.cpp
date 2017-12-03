#include <RFM69.h>
#include <SPI.h>
#include <defines.h>
#include <vector>
//#include <SPIFlash.h>
using namespace std;

int licz = 0;
byte sendSize = 0;
boolean requestACK = false;
// SPIFlash flash(8, 0xEF30); //EF40 for 16mbit windbond chip
RFM69 radio;

byte manipulator_data[61];

typedef struct {
  uint16_t nodeId; // store this nodeId
  int pwm1;        // erature maybe?
  int pwm2;
} Payload;
Payload platform_data;
char tmp[6];
// byte platform_data[12];
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
    print(" - ACK sent\n");
    delay(10);
  }
  blink(LED, 5);
  // println();
}

void slice61(byte platform_data[], vector<byte> received, int i) {
  vector<byte>::const_iterator first = received.begin() + 61 * i;
  vector<byte>::const_iterator last = first + 61;
  copy(first, last, platform_data);
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

void send(byte data[], uint8_t data_size) {
  print("Sending (");
  print(data_size);
  print(" bytes) ... ");
  print(data[0]);
  if (radio.sendWithRetry(GATEWAYID, (const void *)(data), data_size)) {
    print(" ok!\n");
  } else {
    print(" nothing...\n");
  }

  blink(LED, 3);
}

void send_struct() {
  print("Sending struct (");
  print(sizeof(platform_data));
  print(" bytes) ... ");
  if (radio.sendWithRetry(GATEWAYID, (const void *)(&platform_data), sizeof(platform_data)))
    print(" ok!\n");
  else
    print(" nothing...\n");
  blink(LED, 3);
}

void clear_data(byte data[], uint8_t data_size) { memset(&data[0], 0, data_size); }

long lastPeriod = -1;
void loop() {
  if (is_complete_frame) {
    int nr_packets_to_send = (int)ceil(received.size() / 61.0);

    // accept only our glorious protocol
    if (received.front() == 0x46) {
      // only platform frame has 12 bytes
      print("ACCEPTED. Platform frame.\n");
      for (int i = 0; i < 4; i++) {
        tmp[i] = received[i + 6];
      }
      platform_data.pwm1 = atoi(tmp);
      for (int i = 0; i < 4; i++) {
        tmp[i] = received[i + 10];
      }
      platform_data.pwm2 = atoi(tmp);

      print(platform_data.pwm1);
      print(" ");
      print(platform_data.pwm2);
      send_struct();
    } else if (received.front() == 0xFF) {
      print("ACCEPTED. Need to send: ");
      print(nr_packets_to_send);
      print(" packet(s)\n");
      for (int i = 0; i < nr_packets_to_send; i++) {
        slice61(manipulator_data, received, i);
        send(manipulator_data, sizeof(manipulator_data));
        clear_data(manipulator_data, sizeof(manipulator_data));
      }
    } else {
      print("REJECTED\n");
    }
    is_complete_frame = false;
    received.clear();
  }

  if (radio.receiveDone()) {
    pong();
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
