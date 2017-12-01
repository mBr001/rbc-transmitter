#define DEBUG true

#if DEBUG
#define print(var) Serial.print(var)
#else
#define print(var)
#endif

#define NODEID 99
#define NETWORKID 100
#define GATEWAYID 1
#define FREQUENCY RF69_868MHZ  // RF69_433MHZ (others: RF69_433MHZ, RF69_868MHZ)
#define KEY "sampleEncryptKey" // has to be same 16 characters/bytes on all nodes
#define LED 9
#define BAUDRATE 57600
#define ACK_TIME 50 // # of ms to wait for an ack
#define TRANSMITPERIOD 200 // transmit a packet to gateway so often (in ms)