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

RF24 radio(CE,CSN);

const uint64_t address[3] = {1002502019004,1002502019005,1002502019006};
// 1002502019004 - PIR , 1002502019005 AQI, 1002502019005 Flame&Gas

float dhtValue[3];
float sensorValue[3], orderESP[3];

uint32_t timer = 0;

void setup() {
    Serial.begin(9600);
    Serial3.begin(115200);
    Serial.println("Serial Mega ready");
    SPI.begin();
    //========================RF========================
    radio.begin();
    radio.setRetries(15,15);
    radio.setPALevel(RF24_PA_MAX);
    //==================================================
    dht.begin();
}

void loop() {
    delay(100);

    //================= Listen RF ===================
    uint8_t pipeNum;
    radio.openReadingPipe(1, address[0]);
    radio.openReadingPipe(2, address[1]);
    radio.openReadingPipe(3, address[2]);
    radio.startListening();

    if(radio.available(&pipeNum)){  // signal from RF
        radio.read(&sensorValue,sizeof(sensorValue));
        switch (pipeNum)
        {
        case 1: // PIR
        {
            String output;
            output += F("PipeNum: ");
            output += pipeNum;
            output += F("\t waring: ");
            output += sensorValue[0];
            output += F("\t Delay time: ");
            output += sensorValue[1];
            output += F("\t State of device: ");
            output += sensorValue[2];
            // Serial.println(output);

            StaticJsonDocument<500> JsonDoc;
            JsonDoc["type"] = "CA-SS02";
            JsonDoc["warning"] = sensorValue[0];
            JsonDoc["delayTime"] = sensorValue[1];
            JsonDoc["state"] = sensorValue[2];
            String payload;
            serializeJson(JsonDoc, payload);
            Serial3.print(payload);
            break;
        }   

        case 2: // AQI
        {
            String output;
            output += F("PipeNum: ");
            output += pipeNum;
            output += F("\t Temperature : ");
            output += sensorValue[0];
            output += F(" 0C \t Humidity : ");
            output += sensorValue[1];
            output += F(" % \t Air Quality : ");
            output += sensorValue[2];
            output += F(" ppm");
            // Serial.println(output);

            StaticJsonDocument <500> JsonDoc;
            JsonDoc["type"] = "CA-SS03";
            JsonDoc["temperture"] = sensorValue[0];
            JsonDoc["humidity"] = sensorValue[1];
            JsonDoc["AQI"] = sensorValue[2];
            String payload;
            serializeJson(JsonDoc, payload);
            Serial3.print(payload);
            break;
        }

        case 3: // flame & gas
        {
            String output;
            output += F("PipeNum: ");
            output += pipeNum;
            output += F("\t Flame : ");
            output += sensorValue[0];
            output += F("\t gas : ");
            output += sensorValue[1];
            // Serial.println(output);

            StaticJsonDocument <500> JsonDoc;
            JsonDoc["type"] = "CA-SS04";
            JsonDoc["flame"] = sensorValue[0];
            JsonDoc["gas"] = sensorValue[1];
            String payload;
            serializeJson(JsonDoc, payload);
            Serial3.print(payload);
            break;
        }
        
        default:
            break;
        }
    } // end condition of radio RF

    delay(200);

    //==============Listen esp=======================

    if(Serial3.available()){       // order from ESP
        String payload;
        payload = Serial3.readStringUntil('\r');
        Serial.println(payload);
        radio.stopListening();
        
        StaticJsonDocument<500> JsonDoc;
        deserializeJson(JsonDoc,payload);

        String type = JsonDoc["type"];
        if(type == "CA-SS04"){
            boolean warning = JsonDoc["warning"];
            Serial.print("CA-SS04 warning : ");
            Serial.println(warning);
            orderESP[0] = warning;
            radio.openWritingPipe(address[2]);
            radio.write(orderESP, sizeof(orderESP));
            delay(100);
        }
        if(type == "CA-SS02"){
            boolean state = JsonDoc["light"];
            uint16_t delayTime = JsonDoc["delay"];
            Serial.print("CA-SS02 light : ");
            Serial.print(state);
            Serial.print("\t delay time set in : ");
            Serial.println(delayTime);
            orderESP[0] = state;
            orderESP[1] = delayTime;
            radio.openWritingPipe(address[0]);
            radio.write(orderESP, sizeof(orderESP));
            delay(100);
        }
    } // end condition of communicate to ESP

}