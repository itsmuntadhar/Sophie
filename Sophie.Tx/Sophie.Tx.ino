/*
    Project Sophie (Transmitter device)
    Muntadhar Haydar, 2018.
    
    Description:
    When the program detects the fall, it will trigger one or more
    of the LEDs that are attached to indicate the direction; this
    can be set by setting DEBUG to 3, and will transmit a message 
    through 433MHz to the receiver which will in turn send an SMS
    to a set number.
    By setting DEBUG to 2, the program will send the values in a
    plotting-friendly format.
*/

#include <MPU6050.h>
#include <VirtualWire.h>

#define DEBUG 1 //0 for no debug, 1 for normal debug strings, 2 for Data Plotting and 3 for direction debugging.

#define DEVICE_ID 0x97 //Each "wearable" has an ID to distinguish it.
#define LED_PIN 13
#define MILLIS_BETWEEN_NOTIF 1000 //A second, at least, between every notification.
#define RF_TX_PIN 12
#define READINGS_COUNT 25 //Number of reading to take their average.
#define AXISES_COUNT 6
#define AX 0
#define AY 1
#define AZ 2
#define GX 3
#define GY 4
#define GZ 5
// These values were from a simple test, maybe a better test would yield a better values.
#define AX_THRESHOLD_0 17500
#define AX_THRESHOLD_1 31000
#define AY_THRESHOLD 14500
#define AZ_THRESHOLD 5500

MPU6050 mpu;

bool isDown = false;

unsigned long lastReportTime, lastReadTime;

int16_t ax, ay, az;
int16_t gx, gy, gz;

int32_t readings[AXISES_COUNT][READINGS_COUNT]; // the reading history
int32_t readIndex[AXISES_COUNT];                // the index of the current reading
int32_t total[AXISES_COUNT];                    // the running total
int32_t average[AXISES_COUNT];                  // the average
int32_t leds[4] = {2, 3, 4, 5};

void setup()
{
    lastReportTime = lastReadTime = millis();
    vw_setup(2000); // Bits per sec
    Wire.begin();
    for (byte i = 0; i < 4; i++)
        pinMode(leds[i], OUTPUT);

#if DEBUG > 0
    Serial.begin(115200);
#endif

    // initialize device
    mpu.initialize();

    mpu.setXGyroOffset(-1100);
    mpu.setYGyroOffset(271);
    mpu.setZGyroOffset(-60);
    mpu.setXAccelOffset(-2509);
    mpu.setYAccelOffset(-101);
    mpu.setZAccelOffset(925);

#if DEBUG == 1
    // verify connection
    Serial.println(F("Testing device connections..."));
    Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));
#endif
    // configure LED for output
    pinMode(LED_PIN, OUTPUT);

    // zero-fill all the arrays:
    for (int axis = 0; axis < AXISES_COUNT; axis++)
    {
        readIndex[axis] = 0;
        total[axis] = 0;
        average[axis] = 0;
        for (int i = 0; i < READINGS_COUNT; i++)
        {
            readings[axis][i] = 0;
        }
    }
}

void loop()
{ // read raw accel/gyro measurements from device
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    smooth(AX, ax);
    smooth(AY, ay);
    smooth(AZ, az);
    smooth(GX, gx);
    smooth(GY, gy);
    smooth(GZ, gz);

#if DEBUG == 2
    Serial.print(average[AX]);
    Serial.print("\t");
    Serial.print(average[AY]);
    Serial.print("\t");
    Serial.print(average[AY]);
    Serial.print("\t");
    Serial.print(average[GX]);
    Serial.print("\t");
    Serial.print(average[GY]);
    Serial.print("\t");
    Serial.print(average[GZ]);
    Serial.println("\t");
#endif
    lastReadTime = millis();
    bool isDown = false;
    if (average[AX] <= -12000 && average[AZ] <= 9000)
    {
#if DEBUG == 3
        Serial.println("f");
#endif
        digitalWrite(leds[0], HIGH);
        isDown = true;
    }
    else if (average[AZ] <= -12000)
    {
#if DEBUG == 3
        Serial.println("r");
#endif
        digitalWrite(leds[1], HIGH);
        isDown = true;
    }
    else if (average[AX] >= 6500 && average[AZ] <= 14000)
    {
#if DEBUG == 3
        Serial.println("b");
#endif
        digitalWrite(leds[2], HIGH);
        isDown = true;
    }
    else if (average[AZ] >= 9500)
    {
#if DEBUG == 3
        Serial.println("l");
#endif
        digitalWrite(leds[3], HIGH);
        isDown = true;
    }
    else
    {
        for (byte i = 0; i < 4; i++)
            digitalWrite(leds[i], LOW);
    }
    if (isDown && lastReadTime - lastReportTime >= MILLIS_BETWEEN_NOTIF)
    {
        SendNotification();
        lastReportTime = lastReadTime;
#if DEBUG == 1
        Serial.print("A fall detected: ");
        Serial.println(lastReportTime);
#endif
    }

    digitalWrite(LED_PIN, isDown);
}

void smooth(int axis, int32_t val)
{
    // pop and subtract the last reading:
    total[axis] -= readings[axis][readIndex[axis]];
    total[axis] += val;

    // add value to running total
    readings[axis][readIndex[axis]] = val;
    readIndex[axis]++;

    if (readIndex[axis] >= READINGS_COUNT)
        readIndex[axis] = 0;

    // calculate the average:
    average[axis] = total[axis] / READINGS_COUNT;
}

void SendNotification()
{
#if DEBUG == 1
    Serial.println("preparing message");
#endif
    String msg = "F";
    msg += (char)(((DEVICE_ID & 0xF0) >> 4) + 0x30);
    msg += (char)(((DEVICE_ID & 0x0F)) + 0x30);

    SendMessage(msg.c_str());
}

void SendMessage(char msg[])
{
    int len = strlen(msg);
#if DEBUG == 1
    Serial.print("sendin': ");
    Serial.print(len);
    Serial.print("\t");
    Serial.println(msg);
#endif
    if (len < 1)
        return;
    vw_send((uint8_t *)msg, len);
    vw_wait_tx();
}
