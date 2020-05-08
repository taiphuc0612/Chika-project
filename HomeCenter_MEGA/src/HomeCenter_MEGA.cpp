//The future is always a blank page
//1002502019002 - CA-SW2, 1002502019003 - CA-SW3, 1002502019004 - PIR , 1002502019005 AQI, 1002502019005 Flame&Gas

#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <PriUint64.h>
#include <math.h>
#include <Adafruit_NeoPixel.h>

#define CE 9
#define CSN 53
#define dht_pin A0
#define dht_type DHT22
#define PIN 6
#define NUMPIXELS 8
#define DELAYVAL 100

typedef struct
{
    int id;              // address device in EEPROM
    char productId[100]; // ID of product and topic too
    char type[20];       // type of device (SR, SS01, SS0X, ....)
    float value[3];      // value of device
    uint64_t RF_Channel; // channel for communicate with orther device using RF signal

} device __attribute__((packed)); // stuct of Chika device help store data esay for managing (haven't known "attribute((packed))" yet :D )

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800); //Object of LED Line
DHT dht(dht_pin, dht_type);                                     // Object of DHT
RF24 radio(CE, CSN);                                            // Object of nRF24
StaticJsonDocument<1000> JsonDoc;

//Adafruit_NeoPixel - FS - First Start:
int FS_red[8] = {255, 255, 255, 0, 0, 75, 128, 255};
int FS_green[8] = {0, 165, 255, 128, 0, 0, 0, 255};
int FS_blue[8] = {0, 0, 0, 0, 255, 130, 128, 255};
int proceed[9] = {40, 40, 40, 40, 40, 40, 190, 220, 255};

//Adafruit_NeoPixel - CS - Chika Start:
int CS_red[9] = {65, 0, 0, 0, 65, 0, 0, 0};
int CS_green[9] = {40, 60, 255, 128, 40, 60, 255, 128};
int CS_blue[9] = {134, 255, 153, 0, 134, 255, 153, 0};

//Adafruit_NeoPixel - NS - Normal State:
int NS_red[9] = {0, 0, 0, 0, 0, 0, 0, 0};
int NS_green[9] = {0, 0, 0, 0, 0, 0, 0, 0};
int NS_blue[9] = {0, 0, 0, 0, 0, 0, 0, 0};

float Device_2_HC[3], HC_2_Device[3]; // Device_2_HC

int numberOfKey = 3; // number of value of HC_2_Device
int numberOfDevice, index;

device CA_device[20]; //We have 20 Chika device :D

uint8_t pipeNum; //It help to classify RF channel

// cause C++
uint64_t stringToUint64(String input);
void initial();
uint64_t iDontKnow(int exp);
void shiftLeft(int a[]);
void shiftRight(int a[]);
void pixelsOff();
void ChikaStartUp();
void StartUp();
void FirstStartUp();
void pixelsOff();
void normalMode();
void processing();

void setup()
{
    Serial.begin(9600);
    Serial3.begin(115200); // the way communicate with ESP
    Serial.println("Serial Mega ready");
    SPI.begin();
    pixels.begin();
    //========================RF========================
    radio.begin();
    radio.setRetries(15, 15);
    radio.setPALevel(RF24_PA_MAX);
    //==================================================
    dht.begin();
    //===================initial========================
    Serial.println(".....initial.....");
    index = 0;
    initial(); // proceed reload data - get data of device user having from server
    delay(1000);
    Serial.println("Hi ! Chika Home Central at your service ");
}

void loop()
{

    //============================= Listen RF =============================
    for (int i = 0; i < numberOfDevice; i++)
    {
        radio.openReadingPipe(i + 1, CA_device[i].RF_Channel); // listen on radio channel (RF)
    }
    radio.startListening();

    if (radio.available(&pipeNum))
    {
        Serial.println(pipeNum);
        radio.read(&Device_2_HC, sizeof(Device_2_HC)); // read data form device
        JsonDoc.clear();
        String typeDevice = CA_device[pipeNum - 1].type; // get type of device recevie
        JsonDoc["type"] = typeDevice;                    // Json type
        JsonDoc["id"] = CA_device[pipeNum - 1].id;       // Json id , it help on esp get index device in EEPROM

        if (typeDevice.equals("SR"))
        {
            JsonDoc["button1"] = CA_device[pipeNum - 1].value[0] = (bool)Device_2_HC[0]; //Json button 1 and store
            JsonDoc["button2"] = CA_device[pipeNum - 1].value[1] = (bool)Device_2_HC[1]; //Json button 2 and store
            JsonDoc["button3"] = CA_device[pipeNum - 1].value[2] = (bool)Device_2_HC[2]; //Json button 3 and store
            String payload;
            serializeJson(JsonDoc, payload); //encode Json to String
            Serial.println(payload);
            Serial3.print(payload); // send String Json to ESP
            Serial3.print('\r');
        }

        if (typeDevice.equals("SS02"))
        {
            JsonDoc["auto"] = CA_device[pipeNum - 1].value[0] = (boolean)Device_2_HC[0];  //Json auto mode and store
            JsonDoc["state"] = CA_device[pipeNum - 1].value[1] = (boolean)Device_2_HC[1]; //Json state and store
            String payload;
            serializeJson(JsonDoc, payload); //encode Json to String
            Serial.println(payload);
            Serial3.print(payload); //send String Json to ESP
            Serial3.print('\r');
        }

        if (typeDevice.equals("SS03"))
        {
            JsonDoc["temperature"] = CA_device[pipeNum - 1].value[0] = (float)Device_2_HC[0]; //Json temperture and store
            JsonDoc["humidity"] = CA_device[pipeNum - 1].value[1] = (float)Device_2_HC[1];    //Json humidity and store
            JsonDoc["AQI"] = CA_device[pipeNum - 1].value[2] = (float)Device_2_HC[2];         //Json AQI and store
            String payload;
            serializeJson(JsonDoc, payload); //encode Json to String
            Serial.println(payload);
            Serial3.print(payload); //send String Json to ESP
            Serial3.print('\r');
        }

        if (typeDevice.equals("SS04"))
        {
            JsonDoc["flameWarning"] = CA_device[pipeNum - 1].value[0] = (boolean)Device_2_HC[0]; //Json flameWarning and store
            JsonDoc["gasWarning"] = CA_device[pipeNum - 1].value[1] = (boolean)Device_2_HC[1];   //Json gasWarning and store
            String payload;
            serializeJson(JsonDoc, payload); //encode Json to String
            Serial.println(payload);
            Serial3.print(payload); //send String Json to ESP
            Serial3.print('\r');
        }
    }
    //============================= End Listen RF =============================

    //====================== Listen Command From MQTT =========================

    if (Serial3.available())
    {
        String payload;
        payload = Serial3.readStringUntil('\r'); //get data from esp
        Serial.println(payload);
        radio.stopListening(); // stop listen for transmite

        JsonDoc.clear();
        deserializeJson(JsonDoc, payload);
        String productID = JsonDoc["productID"]; // get productID from ESP
        int index;
        for (int i = 0; i < numberOfDevice; i++) //find index CA_device by productID
        {
            if (productID.equals(CA_device[i].productId))
            {
                index = i;
            }
        }

        String type = JsonDoc["type"]; //get type for decode
        if (type.equals("CA-SS00"))
        {
            float temperture = JsonDoc["temperture"];
            float humidity = JsonDoc["humidity"];

            if (temperture && humidity) // if temperture and humidity aren't receive null
            {
                Serial.print(temperture);
                Serial.print('\t');
                Serial.println(humidity);
                //set state led of temperture
                if (temperture < 20)
                {
                    pixels.setPixelColor(2, 0, 0, 255);
                    pixels.show();
                }
                else if (temperture > 38)
                {
                    pixels.setPixelColor(2, 255, 0, 0);
                    pixels.show();
                }
                else
                {
                    pixels.setPixelColor(2, 0, 255, 0);
                    pixels.show();
                }

                //set state led of humidity
                if (humidity < 40)
                {
                    pixels.setPixelColor(1, 255, 255, 0);
                    pixels.show();
                }
                else if (humidity > 65)
                {
                    pixels.setPixelColor(1, 0, 0, 255);
                    pixels.show();
                }
                else
                {
                    pixels.setPixelColor(1, 0, 128, 255);
                    pixels.show();
                }
            }
        }
        if (type.equals("CA_SR"))
        {
            String key[numberOfKey] = {"button1", "button2", "button3"}; // array key in Json

            Serial.println("Receive control command for CA_SR: ");
            for (int i = 0; i < numberOfKey; i++) // get value from Json
            {
                JsonVariant checkContainKey = JsonDoc[key[i]];
                if (!checkContainKey.isNull())
                {
                    CA_device[index].value[i] = checkContainKey.as<float>();
                    Serial.println(CA_device[index].value[i]);
                }
                else
                {
                    Serial.println("NULL");
                }
            }

            radio.openWritingPipe(CA_device[index].RF_Channel);                   //start transmit
            radio.write(&CA_device[index].value, sizeof(CA_device[index].value)); // send command data from MQTT
        }

        if (type.equals("CA_SS02"))
        {
            String key[numberOfKey] = {"auto", "delayTime"}; // array key in Json

            Serial.println("Receive control command for CA_SS02: ");
            for (int i = 0; i < numberOfKey; i++) // get value from Json
            {
                JsonVariant checkContainKey = JsonDoc[key[i]];
                if (!checkContainKey.isNull())
                {
                    CA_device[index].value[i] = checkContainKey.as<float>();
                    Serial.println(CA_device[index].value[i]);
                }
                else
                {
                    Serial.println("NULL");
                }
            }

            radio.openWritingPipe(CA_device[index].RF_Channel);                   //start transmit
            radio.write(&CA_device[index].value, sizeof(CA_device[index].value)); // send command data from MQTT
        }

        if (type.equals("CA_SS04"))
        {
            String key[numberOfKey] = {"offWarning"}; // array key in Json

            Serial.println("Receive control command for CA_SS04: ");
            for (int i = 0; i < numberOfKey; i++) // get value from Json
            {
                JsonVariant checkContainKey = JsonDoc[key[i]];
                if (!checkContainKey.isNull())
                {
                    CA_device[index].value[i] = checkContainKey.as<float>();
                    Serial.println(CA_device[index].value[i]);
                }
                else
                {
                    Serial.println("NULL");
                }
            }

            radio.openWritingPipe(CA_device[index].RF_Channel);                   //start transmit
            radio.write(&CA_device[index].value, sizeof(CA_device[index].value)); // send command data from MQTT
        }
    }
    //====================== End Listen Command From MQTT =========================
    delay(100);
}

void initial()
{
    StartUp(); //show led line at start up mode
    pixels.setBrightness(10);
    pixels.clear();
    while (1)
    {
        processing(); // make led line in processing mode

        if (Serial3.available())
        {
            String payload = Serial3.readStringUntil('\r'); //get information from ESP
            JsonDoc.clear();
            deserializeJson(JsonDoc, payload);
            String command = JsonDoc["command"]; // get command
            Serial.print("ESP: ");
            if (command.length() > 4)
            {
                Serial.print(command);
                Serial.print("  ");
            }
            else
                Serial.println(payload);

            if (payload == "On_SmartConfig") // ESP startSmartConfig
            {
                boolean tickLed = true;
                while (1)
                {
                    processing();
                    if (Serial3.available())
                    {
                        String payload = Serial3.readStringUntil('\r');
                        Serial.print("ESP: ");
                        Serial.println(payload);
                        if (payload.equals("."))
                        {
                            tickLed = !tickLed;
                            int rangeRGB = map(tickLed, 0, 1, 0, 255);
                            pixels.setPixelColor(0, rangeRGB, 0, 0);
                            pixels.show();
                        }
                        if (payload.equals("WIFI_CONNECTED"))
                        {
                            NS_red[0] = 0;
                            NS_green[0] = 50;
                            NS_blue[0] = 255;
                            pixels.setPixelColor(0, NS_red[0], NS_green[0], NS_blue[0]);
                            pixels.show();
                            break;
                        }
                        if (payload.equals("SmartConfig_fail"))
                        {
                            NS_red[0] = 255;
                            NS_green[0] = 100;
                            NS_blue[0] = 0;
                            pixels.setPixelColor(0, NS_red[0], NS_green[0], NS_blue[0]);
                            pixels.show();
                            break;
                        }
                    }
                    delay(100);
                }
            }

            if (payload.equals("WIFI_CONNECTED"))
            {
                NS_red[0] = 0;
                NS_green[0] = 50;
                NS_blue[0] = 255;
                pixels.setPixelColor(0, NS_red[0], NS_green[0], NS_blue[0]);
                pixels.show();
            }

            if (payload == "Wrong")
            {
                Serial.print("MEGA: ");
                Serial.println("Everything is alright");
            }

            if (command == "Number_Of_Device")
            {
                numberOfDevice = JsonDoc["value"];
                Serial.println(numberOfDevice);
                Serial.println();
            }

            if (command == "Data_Of_Device")
            {
                int id = JsonDoc["id"];
                Serial.println(id);
                if (CA_device[index].id != id && id != 1) // counter index
                {
                    index++;
                }
                Serial.print("index : ");
                Serial.println(index);

                CA_device[index].id = id;

                String payloadPID = JsonDoc["productId"];                                    // get product id from Json
                payloadPID.toCharArray(CA_device[index].productId, payloadPID.length() + 1); // store data
                Serial.print("productId : ");
                Serial.println(CA_device[index].productId);

                String typeDevice = JsonDoc["type"];                                    // get type from Json
                typeDevice.toCharArray(CA_device[index].type, typeDevice.length() + 1); // store data
                Serial.print("Type : ");
                Serial.println(CA_device[index].type);

                String payloadRF = JsonDoc["RFchannel"];                 // get RF channel from Json
                CA_device[index].RF_Channel = stringToUint64(payloadRF); // translate String to Uint_64t and store data
                Serial.print("RF_channel : ");
                Serial.println(PriUint64<DEC>(CA_device[index].RF_Channel));
                Serial.println();
            }

            if (command == "End_Data_Device")
            {
                Serial.println();
                Serial.print("Mega: ");
                Serial.println();
                Serial.println("check data");
                Serial.println();
                for (int i = 0; i < numberOfDevice; i++) //check all data just got
                {
                    Serial.print("MEGA: id of device : ");
                    Serial.println(CA_device[i].id);
                    Serial.print("MEGA: productId of device : ");
                    Serial.println(CA_device[i].productId);
                    Serial.print("MEGA: RF_channel of device : ");
                    Serial.println(PriUint64<DEC>(CA_device[i].RF_Channel));
                    Serial.println();

                    String payload_check;
                    JsonDoc.clear();
                    JsonDoc["check_id"] = CA_device[i].id;
                    JsonDoc["check_productId"] = CA_device[i].productId;
                    serializeJson(JsonDoc, payload_check);
                    Serial3.print(payload_check);
                    String payloadOK;
                    while (1) // waitting for ESP check data
                    {

                        processing();

                        if (Serial3.available())
                        {
                            payloadOK = Serial3.readStringUntil('\r');
                            Serial.print("ESP: ");
                            Serial.println(payloadOK);
                            if (payloadOK == "OK")
                            {
                                NS_blue[i + 3] = 255; // set state device on led line in normal mode
                                NS_green[i + 3] = 50;
                                NS_red[i + 3] = 0; // 3 first led is WiFi temp and humi, 5 device is next led
                                break;
                            }
                            if (payloadOK == "Wrong")
                            {
                                NS_red[i + 3] = 255;
                                NS_green[i + 3] = 100;
                                NS_blue[i + 3] = 0;
                                Serial.println("Data have wrong");
                                break;
                            }
                        }
                        delay(200);
                    }
                    Serial.println();
                    delay(200);
                }
                Serial3.print("\r");
                Serial3.print("Check_Done");
                Serial3.print("\r");
            }

            if (command == "Finish")
            {
                Serial.println("\nMEGA: Show up");
                break;
            }
        }
        delay(100);
    }

    ChikaStartUp(); // Chika Light mode
    normalMode();   // State mode
}

uint64_t stringToUint64(String input)
{
    uint64_t output = 0;
    for (int i = 0; i < input.length(); i++)
    {
        output += (input[i] - '0') * iDontKnow(input.length() - 1 - i);
        delay(50);
    }
    return output;
}

uint64_t iDontKnow(int exp)
{
    switch (exp)
    {
    case 0:
        return 1;
        break;

    case 1:
        return 10;
        break;

    case 2:
        return 100;
        break;

    case 3:
        return 1000;
        break;

    case 4:
        return 10000;
        break;

    case 5:
        return 100000;
        break;

    case 6:
        return 1000000;
        break;

    case 7:
        return 10000000;
        break;

    case 8:
        return 100000000;
        break;

    case 9:
        return 1000000000;
        break;

    case 10:
        return 10000000000;
        break;

    case 11:
        return 100000000000;
        break;

    case 12:
        return 1000000000000;
        break;

    case 13:
        return 10000000000000;
        break;

    case 14:
        return 100000000000000;
        break;

    default:
        break;
    }
}

//============================================================================================================

//================================= Adafruit_NeoPixel Start Up - BEGIN RANGE =================================//
void swap(int &a, int &b)
{
    int temp = a;
    a = b;
    b = temp;
}

// Right to Left
void shiftLeft(int a[])
{
    for (int i = 0; i < 8; i++)
    {
        swap(a[i], a[i + 1]);
    }
}

// Left to Right
void shiftRight(int a[])
{
    for (int i = 8; i > 0; i--)
    {
        swap(a[i], a[i - 1]);
    }
}

void pixelsOff()
{
    for (int i = 0; i < 8; i++)
    {
        pixels.setPixelColor(i, 0, 0, 0);
    }
    pixels.show();
}

void FirstStartUp()
{
    for (int i = 0; i < 8; i++)
    {
        pixels.setBrightness(pow(2, i) - 1);
        for (int j = 0; j < 7; j++)
        {
            pixels.setPixelColor(j, FS_red[i], FS_green[i], FS_blue[i]);
            shiftLeft(FS_red);
            shiftLeft(FS_green);
            shiftLeft(FS_blue);
        }
        pixels.setPixelColor(7, FS_red[i], FS_green[i], FS_blue[i]);
        pixels.show();
        delay(80);
    }

    for (int i = 7; i > 0; i--)
    {
        pixels.setBrightness(pow(2, i) - 1);
        for (int j = 0; j < 7; j++)
        {
            pixels.setPixelColor(j, FS_red[i], FS_green[i], FS_blue[i]);
            shiftLeft(FS_red);
            shiftLeft(FS_green);
            shiftLeft(FS_blue);
        }
        pixels.setPixelColor(7, FS_red[i], FS_green[i], FS_blue[i]);
        pixels.show();
        delay(80);
    }
}

void ChikaStartUp()
{
    for (int i = 0; i < 9; i++)
    {
        pixels.setBrightness(pow(2, i) - 1);
        for (int j = 0; j < 7; j++)
        {
            pixels.setPixelColor(j, CS_red[i], CS_green[i], CS_blue[i]);
            shiftRight(CS_red);
            shiftRight(CS_green);
            shiftRight(CS_blue);
        }
        pixels.setPixelColor(7, CS_red[i], CS_green[i], CS_blue[i]);
        pixels.show();
        delay(100);
    }

    for (int i = 7; i > 0; i--)
    {
        pixels.setBrightness(pow(2, i) - 1);
        for (int j = 0; j < 7; j++)
        {
            pixels.setPixelColor(j, CS_red[i], CS_green[i], CS_blue[i]);
            shiftRight(CS_red);
            shiftRight(CS_green);
            shiftRight(CS_blue);
        }
        pixels.setPixelColor(7, CS_red[i], CS_green[i], CS_blue[i]);
        pixels.show();
        delay(100);
    }
}

void StartUp()
{
    FirstStartUp();
    delay(50);
    ChikaStartUp();
    delay(200);
    pixelsOff();
    delay(500);
}

void normalMode()
{
    pixels.setBrightness(30);
    for (int i = 0; i < 8; i++)
    {
        pixels.setPixelColor(i, NS_red[i], NS_green[i], NS_blue[i]);
    }
    pixels.show();
}

void processing()
{
    for (int i = 1; i < 8; i++)
    {
        pixels.setPixelColor(i, proceed[i], proceed[i], proceed[i]);
        pixels.show();
    }
    shiftRight(proceed);
}
//================================= Adafruit_NeoPixel Start Up - END RANGE =================================//
