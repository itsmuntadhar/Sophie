/*
    Project Sophie (Receiving Device)
    Muntadhar Haydar, 2018.

    This device will process the data received from the transmitter 
    to send an SMS notification to a select number and turn on (set HIGH)
    8 pins that can be used to trigger alarms for example.

    In the "FASTEST" mode, no debug will be used and will use direct port 
    manipulation. 
    P.S. The values used to manipulate ports are for ATmega328 MCUs.

    There's MILLIS_BETWEEN_SMS, no SMS will be sent if that time hasn't passed yet.
*/


#include <VirtualWire.h>
#include <SoftwareSerial.h>

#define DEBUG
//#define USE_REGISTERS_MANIPULATION
//#define FASTEST

#ifdef FASTEST
#ifndef USE_REGISTERS_MANIPULATION
#define USE_REGISTERS_MANIPULATION
#endif
#ifdef DEBUG
#undef DEBUG
#endif
#endif

#ifdef USE_REGISTERS_MANIPULATION
#ifdef DEBUG
#define PINS_HIGH 0xFC
#define PINS_LOW 0x03
#else
#define PINS_HIGH 0xFF
#define PINS_LOW 0x00
#endif
#else
#ifdef DEBUG
#define START 2
#else
#define START 0
#endif
#endif

#define RF_RX_PIN 11
#define SIM_RX 9 //module's tx
#define SIM_TX 8 //module's rx
#define SIM_RESET 2
#define ALARM_TIME_OUT 3000 //The time the output pins stay on in milliseconds.
#define SMS_RECEIVER "phone number here..."
#define SMS_TEXT " has fallen"
#ifdef DEBUG
#define MILLIS_BETWEEN_SMS 1 * 60 * 1000
#else
#define MILLIS_BETWEEN_SMS 15 * 60 * 1000
#endif

unsigned long timer = 0, lastSMS = 0, _timeout;
String _buffer;
SoftwareSerial SIM(SIM_RX, SIM_TX);

void setup()
{
    vw_setup(2000);
    vw_rx_start();
    SIM.begin(38400);
#ifdef DEBUG
    Serial.begin(115200);
    Serial.println("System has been started...");
#endif
#ifdef USE_REGISTERS_MANIPULATION
    DDRD |= PINS_HIGH; //Set the pins as output.
    PORTD &= PINS_LOW; //Ensure they're at logic low.
#else
    for (int i = START; i < 8; i++)
    {
        pinMode(i, OUTPUT);
        digitalWrite(i, LOW);
    }
#endif
}

void loop()
{
    uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t buflen = VW_MAX_MESSAGE_LEN;

    if (vw_get_message(buf, &buflen)) // Non-blocking
    {
#ifdef DEBUG
        for (int i = 0; i < buflen; i++)
        {
            Serial.print(buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println("");
#endif
        if (buf[0] == 0x46)
        {
            timer = millis();
            if (lastSMS == 0 || millis() - lastSMS >= MILLIS_BETWEEN_SMS)
            {
                sendSMS(new char[2]{buf[1], buf[2]});
            }
#ifdef DEBUG
            Serial.print("A fall was detected by the device: ");
            Serial.print(buf[1] - 0x30, HEX);
            Serial.println(buf[2] - 0x30, HEX);
#endif
#ifdef USE_REGISTERS_MANIPULATION
            PORTD |= PINS_HIGH; //Make them logic high.
#else
            for (int i = START; i < 8; i++)
                digitalWrite(i, HIGH);
#endif
        }
    }

    if (millis() - timer >= ALARM_TIME_OUT)
    {
#ifdef USE_REGISTERS_MANIPULATION
        PORTD &= PINS_LOW; //Make them logic low.
#else
        for (int i = START; i < 8; i++)
            digitalWrite(i, LOW);
#endif
    }
}

void sendSMS(char id[2])
{
#ifdef DEBUG
    Serial.println("sending sms...");
#endif
    lastSMS = millis();
    SIM.print(F("AT+CMGF=1\r\n")); //set sms to text mode
    _buffer = _readSerial();
#ifdef DEBUG
    Serial.println(_buffer);
#endif
    SIM.print(F("AT+CMGS=\"")); // command to send sms
    SIM.print(SMS_RECEIVER);
    SIM.print(F("\"\r\n"));
    _buffer = _readSerial();
#ifdef DEBUG
    Serial.println(_buffer);
#endif
    SIM.print(id[0]);
    SIM.print(id[1]);
    SIM.print(SMS_TEXT);
    SIM.print("\r\n");
    delay(100);
    SIM.print((char)26);
    _buffer = _readSerial();
#ifdef DEBUG
    Serial.println(_buffer);
#endif
    //expect CMGS:xxx   , where xxx is a number,for the sending sms.
    if (((_buffer.indexOf("CMGS")) != -1))
    {
        return true;
    }
    else
    {
        return false;
    }
}

String _readSerial()
{
    _timeout = 0;
    while (!SIM.available() && _timeout < 12000)
    {
        delay(13);
        _timeout++;
    }
    if (SIM.available())
    {
        return SIM.readString();
    }
}