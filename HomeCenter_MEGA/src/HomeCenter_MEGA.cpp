#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define CE 9
#define CSN 53
#define dht_pin A0
#define dht_type DHT22

DHT dht(dht_pin, dht_type);

RF24 radio(CE, CSN);

const uint64_t address[5] = {1002502019002, 1002502019003, 1002502019004, 1002502019005, 1002502019006};
//1002502019002 - CA-SW2, 1002502019003 - CA-SW3, 1002502019004 - PIR , 1002502019005 AQI, 1002502019005 Flame&Gas

float dhtValue[3];
float incomingValue[3], HCCommand[3];
uint8_t pipeNum;

uint32_t timer = 0;

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
}

void loop()
{
    delay(100);

    //================= Listen RF ===================
    radio.openReadingPipe(1, address[0]);
    radio.openReadingPipe(2, address[1]);
    radio.openReadingPipe(3, address[2]);
    radio.openReadingPipe(4, address[3]);
    radio.openReadingPipe(5, address[4]);
    radio.startListening();

    if (radio.available(&pipeNum))
    { // signal from RF
        radio.read(&incomingValue, sizeof(incomingValue));
        switch (pipeNum)
        {
        case 1:
        {
            StaticJsonDocument<500> JsonDoc;
            JsonDoc["type"] = "CA-SWR2";
            JsonDoc["button_1"] = (boolean)incomingValue[0];
            JsonDoc["button_2"] = (boolean)incomingValue[1];
            String payload;
            serializeJson(JsonDoc, payload);
            Serial.println(payload);
            Serial3.println(payload);
            Serial3.flush();
            break;
        }
        case 2:
        {
            StaticJsonDocument<500> JsonDoc;
            JsonDoc["type"] = "CA-SWR3";
            JsonDoc["button_1"] = (boolean)incomingValue[0];
            JsonDoc["button_2"] = (boolean)incomingValue[1];
            JsonDoc["button_3"] = (boolean)incomingValue[2];
            String payload;
            serializeJson(JsonDoc, payload);
            Serial.println(payload);
            Serial3.println(payload);
            Serial3.flush();
            break;
        }
        case 3: // PIR
        {
            String output;
            output += F("PipeNum: ");
            output += pipeNum;
            output += F("\t waring: ");
            output += incomingValue[0];
            output += F("\t Delay time: ");
            output += incomingValue[1];
            output += F("\t State of device: ");
            output += incomingValue[2];
            // Serial.println(output);

            StaticJsonDocument<500> JsonDoc;
            JsonDoc["type"] = "CA-SS02";
            JsonDoc["warning"] = incomingValue[0];
            JsonDoc["delayTime"] = incomingValue[1];
            JsonDoc["state"] = incomingValue[2];
            String payload;
            serializeJson(JsonDoc, payload);
            Serial3.print(payload);
            Serial3.println();
            Serial3.flush();
            break;
        }

        case 4: // AQI
        {
            String output;
            output += F("PipeNum: ");
            output += pipeNum;
            output += F("\t Temperature : ");
            output += incomingValue[0];
            output += F(" 0C \t Humidity : ");
            output += incomingValue[1];
            output += F(" % \t Air Quality : ");
            output += incomingValue[2];
            output += F(" ppm");
            Serial.println(output);

            StaticJsonDocument<500> JsonDoc;
            JsonDoc["type"] = "CA-SS03";
            JsonDoc["temperture"] = incomingValue[0];
            JsonDoc["humidity"] = incomingValue[1];
            JsonDoc["AQI"] = incomingValue[2];
            String payload;
            serializeJson(JsonDoc, payload);
            Serial3.print(payload);
            Serial3.println();
            Serial3.flush();
            break;
        }

        case 5: // flame & gas
        {
            String output;
            output += F("PipeNum: ");
            output += pipeNum;
            output += F("\t Flame : ");
            output += incomingValue[0];
            output += F("\t gas : ");
            output += incomingValue[1];
            output += F("\t warning : ");
            output += incomingValue[2];
            // Serial.println(output);

            StaticJsonDocument<500> JsonDoc;
            JsonDoc["type"] = "CA-SS04";
            JsonDoc["flame"] = incomingValue[0];
            JsonDoc["gas"] = incomingValue[1];
            String payload;
            serializeJson(JsonDoc, payload);
            Serial3.print(payload);
            Serial3.println();
            Serial3.flush();
            break;
        }

        default:
            break;
        }
    } // end condition of radio RF
    // Serial.println("Delay time is starting");
     delay(200);

    //==============Listen esp=======================

    if (Serial3.available())
    { // order from ESP
        String payload;
        payload = Serial3.readStringUntil('\r');
        Serial.println(payload);
        radio.stopListening();

        StaticJsonDocument<500> JsonDoc;
        deserializeJson(JsonDoc, payload);

        String type = JsonDoc["type"];
        if (type == "CA-SS04")
        {
            boolean warning = JsonDoc["warning"];
            Serial.print("CA-SS04 warning : ");
            Serial.println(warning);
            HCCommand[0] = warning;
            radio.openWritingPipe(address[4]);
            radio.write(&HCCommand, sizeof(HCCommand));
            delay(100);
        }
        if (type == "CA-SS02")
        {
            boolean state = JsonDoc["light"];
            uint16_t delayTime = JsonDoc["delay"];
            Serial.print("CA-SS02 light : ");
            Serial.print(state);
            Serial.print("\t delay time set in : ");
            Serial.println(delayTime);
            HCCommand[0] = state;
            HCCommand[1] = delayTime;
            radio.openWritingPipe(address[2]);
            radio.write(&HCCommand, sizeof(HCCommand));
            delay(100);
        }

        if (type == "CA-SWR1")
        {
            boolean button_1 = JsonDoc["button_1"];

            Serial.println("Receive control command from CA-SWR2: ");
            Serial.print("Button 1: ");
            Serial.println(button_1);

            HCCommand[0] = button_1;
            radio.openWritingPipe(address[0]);
            radio.write(&HCCommand, sizeof(HCCommand));
        }

        if (type == "CA-SWR2")
        {
            boolean button_1 = JsonDoc["button_1"];
            boolean button_2 = JsonDoc["button_2"];

            Serial.println("Receive control command from CA-SWR2: ");
            Serial.print("Button 1: ");
            Serial.println(button_1);
            Serial.print("Button 2: ");
            Serial.println(button_2);

            HCCommand[0] = button_1;
            HCCommand[1] = button_2;
            radio.openWritingPipe(address[0]);
            radio.write(&HCCommand, sizeof(HCCommand));
        }
        if (type == "CA-SWR3")
        {
            boolean button_1 = JsonDoc["button_1"];
            boolean button_2 = JsonDoc["button_2"];
            boolean button_3 = JsonDoc["button_3"];
            
            Serial.println("Receive control command from CA-SWR3: ");
            Serial.print("Button 1: ");
            Serial.println(button_1);
            Serial.print("Button 2: ");
            Serial.println(button_2);
            Serial.print("Button 3: ");
            Serial.println(button_3);

            HCCommand[0] = button_1;
            HCCommand[1] = button_2;
            HCCommand[2] = button_3;
            radio.openWritingPipe(address[1]);
            radio.write(&HCCommand, sizeof(HCCommand));
        }

    } // end condition of communicate to ESP
    // Serial.println("Delay Time is end");
}