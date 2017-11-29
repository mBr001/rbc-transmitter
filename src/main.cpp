#include <RFM69.h>
#include <SPI.h>
//#include <SPIFlash.h>

#define NODEID 99
#define NETWORKID 100
#define GATEWAYID 1
#define FREQUENCY RF69_868MHZ  //RF69_433MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define KEY "sampleEncryptKey" //has to be same 16 characters/bytes on all nodes, not more not less!
#define LED 9
#define SERIAL_BAUD 19200
#define ACK_TIME 50 // # of ms to wait for an ack
//#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!

int TRANSMITPERIOD = 200; //transmit a packet to gateway so often (in ms)
int licz = 0;
byte sendSize = 0;
boolean requestACK = false;
//SPIFlash flash(8, 0xEF30); //EF40 for 16mbit windbond chip
RFM69 radio;

typedef struct
{
    uint16_t nodeId; //store this nodeId
    int v0;          //erature maybe?
    int v1;
} Payload;
Payload theData;

String inputString = "";
boolean stringComplete = false;
byte input = 0;
char tmp[6];
int adres = 20;

void Blink(byte PIN, int DELAY_MS)
{
    pinMode(PIN, OUTPUT);
    digitalWrite(PIN, HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN, LOW);
}


void setup()
{
    Serial.begin(SERIAL_BAUD);
    radio.initialize(FREQUENCY, NODEID, NETWORKID);
    //radio.setHighPower(); //uncomment only for RFM69HW!
    radio.encrypt(KEY);
    char buff[50];
    sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY == RF69_433MHZ ? 433 : FREQUENCY == RF69_868MHZ ? 868 : 915);
    Serial.println(buff);
}

long lastPeriod = -1;
void loop()
{
    if (stringComplete)
    {
        Serial.println(inputString);

        if (inputString[0] == 'F')
        {
            for (int i = 0; i < 4; i++)
            {
                tmp[i] = inputString[i + 6];
            }
            Serial.println(tmp);
            theData.v0 = atoi(tmp);
            for (int i = 0; i < 4; i++)
            {
                tmp[i] = inputString[i + 10];
            }
            Serial.println(tmp);
            theData.v1 = atoi(tmp);

            Serial.print(theData.v0);
            Serial.print(" ");
            Serial.println(theData.v1);
        }
        if (inputString[0] == '1')
        {
            TRANSMITPERIOD = 50 * (1);
            if (TRANSMITPERIOD == 0)
                TRANSMITPERIOD = 1000;
            Serial.print("\nChanging delay to ");
            Serial.print(TRANSMITPERIOD);
            Serial.println("ms\n");
        }
        if (inputString[0] == '2')
        {
            TRANSMITPERIOD = 50 * (2);
            if (TRANSMITPERIOD == 0)
                TRANSMITPERIOD = 1000;
            Serial.print("\nChanging delay to ");
            Serial.print(TRANSMITPERIOD);
            Serial.println("ms\n");
        }

        inputString = "";
        stringComplete = false;
    } //serial

    //check for any received packets
    if (radio.receiveDone())
    {
        Serial.print('[');
        Serial.print(radio.SENDERID, DEC);
        Serial.print("] ");
        for (byte i = 0; i < radio.DATALEN; i++)
            Serial.print((char)radio.DATA[i]);
        Serial.print("   [RX_RSSI:");
        Serial.print(radio.readRSSI());
        Serial.print("]");

        if (radio.ACKRequested())
        {
            radio.sendACK();
            Serial.print(" - ACK sent");
            delay(10);
        }
        Blink(LED, 5);
        Serial.println();
    }

    int currPeriod = millis() / TRANSMITPERIOD;
    if (currPeriod != lastPeriod)
    {
        //fill in the struct with new values
        theData.nodeId = 0x20;
        //theData.uptime = millis();

        Serial.print("Sending struct (");
        Serial.print(sizeof(theData));
        Serial.print(" bytes) ... ");
        if (radio.sendWithRetry(GATEWAYID, (const void *)(&theData), sizeof(theData)))
            Serial.print(" ok!");
        else
            Serial.print(" nothing...");
        Serial.println();
        Blink(LED, 3);
        lastPeriod = currPeriod;
    }

} //loop

void serialEvent()
{
    while (Serial.available())
    {
        // get the new byte:
        char inChar = (char)Serial.read();
        // add it to the inputString:
        inputString += inChar;
        // if the incoming character is a newline, set a flag so the main loop can
        // do something about it:
        if (inChar == '\n')
        {
            stringComplete = true;
        }
    }
}