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

#define CE 9
#define CSN 53
#define dht_pin A0
#define dht_type DHT22

DHT dht(dht_pin, dht_type); // declare DHT object

RF24 radio(CE, CSN); // declare RF object

typedef struct
{
    int id;
    char productId[100];
    uint64_t RF_Chanel;
} device __attribute__((packed));

uint64_t address[5] = {1002502019001, 1002502019003, 1002502019004, 1002502019005, 1002502019006}; // RF address
//1002502019002 - CA-SW2, 1002502019003 - CA-SW3, 1002502019004 - PIR , 1002502019005 AQI, 1002502019005 Flame&Gas

float Device_2_HC[3], HC_2_Device[3]; // Device_2_HC

int numberOfDevice, index;

device CA_device[20];

uint8_t pipeNum;

uint32_t timer = 0;

uint64_t stringToUint64(String input);
void initial();
uint64_t iDontKnow(int exp);

void setup()
{
    Serial.begin(9600);
    Serial3.begin(115200);
    Serial.println("Serial Mega ready");
    SPI.begin();
    //========================RF========================
    radio.begin();
    radio.setRetries(15, 15);
    radio.setPALevel(RF24_PA_MAX);
    //==================================================
    dht.begin();
    Serial.println(".....initial.....");
    index = 0;
    initial();
}

void loop()
{
    delay(2000);
    Serial.println("hello");

    // //================= Listen RF ===================
    // radio.openReadingPipe(1, address[0]);
    // radio.openReadingPipe(2, address[1]);
    // radio.openReadingPipe(3, address[2]);
    // radio.openReadingPipe(4, address[3]);
    // radio.openReadingPipe(5, address[4]);
    // radio.startListening();

    // if (radio.available(&pipeNum))
    // {
    //     radio.read(&Device_2_HC, sizeof(Device_2_HC)); // read data form device
    //     switch (pipeNum)
    //     {
    //     case 1: // CA-SWR1
    //     {
    //         StaticJsonDocument<500> JsonDoc;
    //         JsonDoc["type"] = "CA-SR1";
    //         JsonDoc["button1"] = (boolean)Device_2_HC[0];
    //         // JsonDoc["button_2"] = (boolean)Device_2_HC[1];
    //         String payload;
    //         serializeJson(JsonDoc, payload);
    //         Serial.println(payload);
    //         Serial3.print(payload);
    //         Serial3.print('\r');
    //         break;
    //     }
    //     case 2: // CA-SWR2
    //     {
    //         StaticJsonDocument<500> JsonDoc;
    //         JsonDoc["type"] = "CA-SR3";
    //         JsonDoc["button1"] = (boolean)Device_2_HC[0];
    //         JsonDoc["button2"] = (boolean)Device_2_HC[1];
    //         JsonDoc["button3"] = (boolean)Device_2_HC[2];
    //         String payload;
    //         serializeJson(JsonDoc, payload);
    //         Serial.println(payload);
    //         Serial3.println(payload);
    //         break;
    //     }
    //     case 3: // PIR
    //     {
    //         String output;
    //         output += F("PipeNum: ");
    //         output += pipeNum;
    //         output += F("\t waring: ");
    //         output += Device_2_HC[0];
    //         output += F("\t Delay time: ");
    //         output += Device_2_HC[1];
    //         output += F("\t State of device: ");
    //         output += Device_2_HC[2];
    //         // Serial.println(output);

    //         StaticJsonDocument<500> JsonDoc;
    //         JsonDoc["type"] = "CA-SS02";
    //         JsonDoc["auto"] = Device_2_HC[0];
    //         JsonDoc["delayTime"] = Device_2_HC[1] / 1000;
    //         JsonDoc["state"] = Device_2_HC[2];
    //         String payload;
    //         serializeJson(JsonDoc, payload);
    //         Serial3.print(payload);
    //         Serial3.println();
    //         break;
    //     }

    //     case 4: // AQI
    //     {
    //         String output;
    //         output += F("PipeNum: ");
    //         output += pipeNum;
    //         output += F("\t Temperature : ");
    //         output += Device_2_HC[0];
    //         output += F(" 0C \t Humidity : ");
    //         output += Device_2_HC[1];
    //         output += F(" % \t Air Quality : ");
    //         output += Device_2_HC[2];
    //         output += F(" ppm");
    //         // Serial.println(output);

    //         StaticJsonDocument<500> JsonDoc;
    //         JsonDoc["type"] = "CA-SS03";
    //         JsonDoc["temperture"] = Device_2_HC[0];
    //         JsonDoc["humidity"] = Device_2_HC[1];
    //         JsonDoc["AQI"] = Device_2_HC[2];
    //         String payload;
    //         serializeJson(JsonDoc, payload);
    //         Serial3.print(payload);
    //         Serial3.println();
    //         break;
    //     }

    //     case 5: // flame & gas
    //     {
    //         String output;
    //         output += F("PipeNum: ");
    //         output += pipeNum;
    //         output += F("\t Flame : ");
    //         output += Device_2_HC[0];
    //         output += F("\t gas : ");
    //         output += Device_2_HC[1];
    //         output += F("\t warning : ");
    //         output += Device_2_HC[2];
    //         // Serial.println(output);

    //         StaticJsonDocument<500> JsonDoc;
    //         JsonDoc["type"] = "CA-SS04";
    //         JsonDoc["flame"] = Device_2_HC[0];
    //         JsonDoc["gas"] = Device_2_HC[1];
    //         JsonDoc["warning"] = Device_2_HC[2];
    //         String payload;
    //         serializeJson(JsonDoc, payload);
    //         Serial3.print(payload);
    //         Serial3.println();
    //         break;
    //     }

    //     default:
    //         break;
    //     }
    // } // end condition of radio RF
    //   // Serial.println("Delay time is starting");
    // delay(10);

    // //==============Listen Control Command From MQTT=======================

    // if (Serial3.available())
    // {
    //     String payload;
    //     payload = Serial3.readStringUntil('\r');
    //     Serial.println(payload);
    //     radio.stopListening();

    //     StaticJsonDocument<500> JsonDoc;
    //     deserializeJson(JsonDoc, payload);
    //     String type = JsonDoc["type"];

    //     if (type == "CA-SR1c")
    //     {
    //         int buttonIndex = JsonDoc["button"];
    //         boolean state   = JsonDoc["state"];

    //         Serial.println("Receive control command from CA-SWR2: ");
    //         Serial.print("Button ");
    //         Serial.print(buttonIndex);
    //         Serial.print(" : ");
    //         Serial.println(state);

    //         HC_2_Device[buttonIndex - 1] = state;
    //         radio.openWritingPipe(address[0]);
    //         radio.write(&HC_2_Device, sizeof(HC_2_Device));
    //     }

    //     if (type == "CA-SR2c")
    //     {
    //         boolean button_1 = JsonDoc["button_1"];
    //         boolean button_2 = JsonDoc["button_2"];

    //         Serial.println("Receive control command from CA-SWR2: ");
    //         Serial.print("Button 1: ");
    //         Serial.println(button_1);
    //         Serial.print("Button 2: ");
    //         Serial.println(button_2);

    //         HC_2_Device[0] = button_1;
    //         HC_2_Device[1] = button_2;
    //         radio.openWritingPipe(address[0]);
    //         radio.write(&HC_2_Device, sizeof(HC_2_Device));
    //     }

    //     if (type == "CA-SR3c")
    //     {
    //         Serial.println("Receive control command from CA-SWR3: ");

    //         JsonVariant checkContainKey = JsonDoc["button1"];
    //         Serial.print("Button 1: ");
    //         if(!checkContainKey.isNull()){
    //             HC_2_Device[0] = checkContainKey.as<float>();
    //             Serial.println(HC_2_Device[0]);

    //         }else Serial.println("NULL");

    //         checkContainKey = JsonDoc["button2"];
    //         Serial.print("Button 2: ");
    //         if(!checkContainKey.isNull()){
    //             HC_2_Device[1] = checkContainKey.as<float>();
    //             Serial.println(HC_2_Device[1]);

    //         }else Serial.println("NULL");

    //         checkContainKey = JsonDoc["button3"];
    //         Serial.print("Button 3: ");
    //         if(!checkContainKey.isNull()){
    //             HC_2_Device[2] = checkContainKey.as<float>();
    //             Serial.println(HC_2_Device[2]);

    //         }else Serial.println("NULL");

    //         uint64_t address_RF = JsonDoc["RF"];

    //         radio.openWritingPipe(address_RF);
    //         radio.write(&HC_2_Device, sizeof(HC_2_Device));
    //     }

    //     if (type == "CA-SS02c")
    //     {
    //         boolean state = JsonDoc["auto"];
    //         uint16_t delayTime = JsonDoc["delayTime"];
    //         Serial.print("CA-SS02 light : ");
    //         Serial.print(state);
    //         Serial.print("\t delay time set in : ");
    //         Serial.println(delayTime);
    //         HC_2_Device[0] = state;
    //         HC_2_Device[1] = delayTime * 1000; // đôi lúc không gửi delayTime
    //         radio.openWritingPipe(address[2]);
    //         radio.write(&HC_2_Device, sizeof(HC_2_Device));
    //         delay(5);
    //     }

    //     if (type == "CA-SS04c")
    //     {
    //         boolean warning = JsonDoc["warning"];
    //         Serial.print("CA-SS04 warning : ");
    //         Serial.println(warning);
    //         HC_2_Device[0] = warning;
    //         radio.openWritingPipe(address[4]);
    //         radio.write(&HC_2_Device, sizeof(HC_2_Device));
    //         delay(5);
    //     }
    // } // end condition of communicate to ESP
    // // Serial.println("Delay Time is end");
}

void initial()
{
    while (1)
    {
        if (Serial3.available())
        {
            String payload = Serial3.readStringUntil('\r');
            StaticJsonDocument<300> JsonDoc;
            deserializeJson(JsonDoc, payload);
            String command = JsonDoc["command"];
            Serial.print("ESP: ");
            if (command.length() > 4)
            {
                Serial.print(command);
                Serial.print("  ");
            }
            else
                Serial.println(payload);

            if(payload == "Wrong"){
                Serial.print("MEGA: ");
                Serial.println("Everything is alright");
            }

            if (command == "Number of device")
            {
                numberOfDevice = JsonDoc["value"];
                Serial.println(numberOfDevice);
                Serial.println();
            }

            if (command == "Data from user")
            {
                int id = JsonDoc["id"];
                Serial.println(id);
                if (CA_device[index].id != id && id != 1)
                {
                    index++;
                }
                Serial.print("index : ");
                Serial.println(index);

                CA_device[index].id = id;

                String payloadPID = JsonDoc["productId"];
                payloadPID.toCharArray(CA_device[index].productId, payloadPID.length() + 1);
                Serial.print("productId : ");
                Serial.println(CA_device[index].productId);

                String payloadRF = JsonDoc["RFchannel"];
                CA_device[index].RF_Chanel = stringToUint64(payloadRF);
                Serial.print("RF_channel : ");
                Serial.println(PriUint64<DEC>(CA_device[0].RF_Chanel));
                Serial.println();
            }

            if (command == "end of data user")
            {
                Serial.println();
                Serial.print("Mega: ");
                Serial.println("check data");
                delay(1000);
                for (int i = 0; i < numberOfDevice; i++)
                {
                    Serial.print("id of device : ");
                    Serial.println(CA_device[i].id);
                    Serial.print("productId of device : ");
                    Serial.println(CA_device[i].productId);
                    Serial.print("RF_chanel of device : ");
                    Serial.println(PriUint64<DEC>(CA_device[i].RF_Chanel));
                    Serial.println();

                    String payload_check;
                    JsonDoc.clear();
                    JsonDoc["check_id"] = CA_device[i].id;
                    JsonDoc["check_productId"] = CA_device[i].productId;
                    serializeJson(JsonDoc, payload_check);
                    Serial3.print(payload_check);
                    String payloadOK;
                    while (1)
                    {
                        if (Serial3.available())
                        {
                            payloadOK = Serial3.readStringUntil('\r');
                            Serial.print("ESP: ");
                            Serial.println(payloadOK);
                            if (payloadOK == "OK")
                                break;
                            if (payloadOK == "Wrong")
                            {
                                Serial.println("Data have wrong");
                                break;
                            }
                        }
                        delay(200);
                    }
                    Serial.println();
                    delay(2000);
                }
                Serial3.print("\r");
                Serial3.print("Check Done");
                Serial3.print("\r");
            }

            if (command == "Finish")
            {
                Serial.println("\nMEGA: Show up");
                return;
            }
        }
        delay(100);
    }
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
