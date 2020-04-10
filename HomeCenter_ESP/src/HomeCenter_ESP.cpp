#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <DHT.h>

#define ledR 16
#define ledB 5
#define btn_config 4
#define DHT_pin 0
#define DHT_type DHT22

uint32_t timer_smartConfig = 0;
uint16_t timer_sendTempHumi = 0 ;
uint16_t longPressTime = 6000;

boolean buttonActive = false;

char payload_char[500];

const char *mqtt_server = "chika.gq";
const int mqtt_port = 2502;
const char *mqtt_user = "chika";
const char *mqtt_pass = "2502";

const char *HomeCenter = "f7a3bde5-5a85-470f-9577-cdbf3be121d4";
const char *CA_SS00 = "f7a3bde5-5a85-470f-9577-cdbf3be121d4/temphumi";
const char *CA_SWR = "2b92934f-7a41-4ce1-944d-d33ed6d97e13/stateDevice";
const char *CA_SWR2 = "4a0bfbfe-efff-4bae-927c-c8136df70333/stateDevice";
const char *CA_SWR3 = "ebb2464e-ba53-4f22-aa61-c76f24d3343d/stateDevice";
const char *CASS02 = "9d860c55-7899-465b-9fb3-195ae0c0959a/stateLight";
const char *CASS03 = "1dd591c2-9080-4dcc-9c14-d9ecf8561248/AQIdata";
const char *CASS04 = "d5ae3121-fb7b-4198-bbb5-a6fc67566452/flameWarning";

Ticker ticker;
WiFiClient esp;
PubSubClient client(esp);
DHT SS00(DHT_pin,DHT_type);

void tick();
void tick2();
void exitSmartConfig();
boolean startSmartConfig();
void longPress();
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();

void setup()
{
  Serial.begin(115200);

  WiFi.setAutoConnect(true);   // auto connect when start
  WiFi.setAutoReconnect(true); // auto reconnect the old WiFi when leaving internet

  pinMode(ledR, OUTPUT);      // led red set on
  pinMode(ledB, OUTPUT);      // led blue set on
  pinMode(btn_config, INPUT); // btn_config is ready

  ticker.attach(1, tick2); // initial led show up

  uint16_t i = 0;
  while (!WiFi.isConnected()) // check WiFi is connected
  {
    i++;
    delay(100);
    if (i >= 100) // timeout and break while loop
      break;
  }

  if (!WiFi.isConnected()) // still not connected
  {
    startSmartConfig(); // start Smartconfig
  }
  else
  {
    ticker.detach();         // shutdown ticker
    digitalWrite(ledR, LOW); // show led
    Serial.println("WIFI CONNECTED");
    Serial.println(WiFi.SSID());
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop()
{
  //longPress();
  if (WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(ledB, HIGH);
    digitalWrite(ledR, LOW);
    if (client.connected())
    {
      client.loop();
      // do something here
      timer_sendTempHumi++;
      if(timer_sendTempHumi >= 60){
        timer_sendTempHumi = 0 ;
        float h = SS00.readHumidity();
        float t = SS00.readTemperature();

        while(isnan(h) || isnan(t))
        {
          h = SS00.readHumidity();
          t = SS00.readTemperature();
          delay(10);
        }

        String sendTempHumi;
        char payload_sendTempHumi[300];
        StaticJsonDocument<300> JsonDoc;
        JsonDoc["type"] = "CA-SS00";
        JsonDoc["temperture"] = t;
        JsonDoc["humidity"] = h;
        serializeJson(JsonDoc,sendTempHumi);
        // Serial.println(sendTempHumi);
        sendTempHumi.toCharArray(payload_sendTempHumi,sendTempHumi.length() + 1);
        client.publish(CA_SS00,payload_sendTempHumi,true);

      }
      if (Serial.available())
      { 
        String payload_MEGA = Serial.readStringUntil('\r');
        // Serial.println(payload);

        StaticJsonDocument<500> JsonDoc;
        deserializeJson(JsonDoc, payload_MEGA);
        payload_MEGA.toCharArray(payload_char, payload_MEGA.length() + 1);
        String type = JsonDoc["type"];

        if (type == "CA-SWR")
        {
          client.publish(CA_SWR, payload_char);
        }
        else if (type == "CA-SWR2")
        {
          client.publish(CA_SWR2, payload_char);
        }
        else if (type == "CA-SWR3")
        {
          client.publish(CA_SWR3, payload_char);
        }
        if (type == "CA-SS02")
        {
          client.publish(CASS02, payload_char);
        }
        if(type == "CA-SS03"){
          client.publish(CASS03, payload_char);
        }
        if(type == "CA-SS04"){
          client.publish(CASS04, payload_char);
        }
      }
    }
    else
    {
      reconnect();
    }
  }
  else
  {
    Serial.println("WiFi Connected Fail");
    WiFi.reconnect();
    digitalWrite(ledB, LOW);
    boolean state = digitalRead(ledR);
    digitalWrite(ledR, !state);
  }
  delay(100);
}

void reconnect()
{
  Serial.println("Attempting MQTT connection ...");
  String clientId = "ESP8266Client-testX";
  clientId += String(random(0xffff), HEX);
  if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
  {
    Serial.println("connected");
    client.subscribe(HomeCenter);
  }
  else
  {
    Serial.print("MQTT Connected Fail, rc = ");
    Serial.print(client.state());
    Serial.println("try again in 5 seconds");
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  // Serial.print("Message arrived [");
  // Serial.print(topic);
  // Serial.print("] ");
  String data;
  String mtopic = (String)topic;

  for (uint16_t i = 0; i < length; i++)
  {
    data += (char)payload[i];
  }
  
  if (mtopic == HomeCenter)
  {
    // Serial.print(length);
    Serial.println(data);
    Serial.flush();
  }
}

void tick()
{
  boolean state = digitalRead(ledR);
  digitalWrite(ledR, !state);
}

void tick2()
{
  boolean state = digitalRead(ledR);
  digitalWrite(ledR, !state);
  digitalWrite(ledB, !state);
}

void exitSmartConfig()
{
  WiFi.stopSmartConfig();
  ticker.detach();
  digitalWrite(ledR, LOW);
  digitalWrite(ledB, HIGH);
}

boolean startSmartConfig()
{
  uint16_t t = 0;
  Serial.println("On SmartConfig ");
  WiFi.beginSmartConfig();
  delay(500);
  ticker.attach(0.1, tick);
  while (WiFi.status() != WL_CONNECTED)
  {
    t++;
    Serial.print(".");
    delay(500);
    if (t > 100)
    {
      Serial.println("Smart Config fail");
      ticker.attach(0.5, tick);
      delay(3000);
      exitSmartConfig();
      return false;
    }
  }
  Serial.println("WiFi connected ");
  Serial.print("IP :");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.SSID());
  exitSmartConfig();
  return true;
}

void longPress()
{
  if (digitalRead(btn_config) == HIGH)
  {
    if (buttonActive == false)
    {
      buttonActive = true;
      timer_smartConfig = millis();
      Serial.println(timer_smartConfig);
    }

    if (millis() - timer_smartConfig > longPressTime)
    {
      Serial.println("SmartConfig Start");
      digitalWrite(ledB, LOW);
      startSmartConfig();
    }
  }
  else
  {
    buttonActive = false;
  }
}