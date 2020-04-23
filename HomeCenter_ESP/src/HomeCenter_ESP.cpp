#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <DHT.h>
#include <EEPROM.h>
#include <PriUint64.h>
#include <IRutils.h>

#define ledR 16
#define ledB 5
#define btn_config 4
#define DHT_pin 0
#define DHT_type DHT22

uint32_t timer_smartConfig = 0;
uint16_t timer_sendTempHumi = 0;
uint16_t longPressTime = 6000;
int numberOfDevice = 0;
int funDelay = 2000;

boolean buttonActive = false;

char payload_char[500];

const char *mqtt_server = "chika.gq";
const int mqtt_port = 2502;
const char *mqtt_user = "chika";
const char *mqtt_pass = "2502";

const char *HC_ID = "f7a3bde5-5a85-470f-9577-cdbf3be121d4";
// const char *CA_SS00 = "f7a3bde5-5a85-470f-9577-cdbf3be121d4";
// const char *CA_SR = "2b92934f-7a41-4ce1-944d-d33ed6d97e13";
// const char *CA_SR2 = "4a0bfbfe-efff-4bae-927c-c8136df70333";
// const char *CA_SR3 = "ebb2464e-ba53-4f22-aa61-c76f24d3343d";
// const char *CA_SS02 = "9d860c55-7899-465b-9fb3-195ae0c0959a";
// const char *CA_SS03 = "1dd591c2-9080-4dcc-9c14-d9ecf8561248";
// const char *CA_SS04 = "d5ae3121-fb7b-4198-bbb5-a6fc67566452";

Ticker ticker;
WiFiClient esp;
PubSubClient client(esp);
DHT SS00(DHT_pin, DHT_type);

typedef struct
{
  int id;
  char productId[100];
  uint64_t RF_Chanel;
} device __attribute__((packed));

device CA_device[20];

void tick();
void tick2();
void exitSmartConfig();
boolean startSmartConfig();
void longPress();
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();
void loadDataFromEEPROM();
void loadDataFromServer();
void initial();
int checkDataID(int id);

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
    Serial.print("WIFI CONNECTED");
    Serial.print('\r');
    Serial.print(WiFi.SSID());
    Serial.print('\r');
    Serial.print("IP: ");
    Serial.print(WiFi.localIP());
    Serial.print('\r');
  }

  loadDataFromServer();
  initial();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop()
{
  delay(3000);
  //longPress();
  // if (WiFi.status() == WL_CONNECTED)
  // {
  //   digitalWrite(ledB, HIGH);
  //   digitalWrite(ledR, LOW);
  //   if (client.connected())
  //   {
  //     client.loop();
  //     // do something here
  //     timer_sendTempHumi++;
  //     if (timer_sendTempHumi >= 60)
  //     {
  //       timer_sendTempHumi = 0;
  //       float h = SS00.readHumidity();
  //       float t = SS00.readTemperature();

  //       String sendTempHumi;
  //       char payload_sendTempHumi[300];
  //       StaticJsonDocument<300> JsonDoc;
  //       JsonDoc["type"] = "CA-SS00";
  //       JsonDoc["temperture"] = t;
  //       JsonDoc["humidity"] = h;
  //       serializeJson(JsonDoc, sendTempHumi);
  //       sendTempHumi.toCharArray(payload_sendTempHumi, sendTempHumi.length() + 1);
  //       client.publish(HC_ID, payload_sendTempHumi, true);
  //     }
  //     if (Serial.available())
  //     {
  //       String payload_MEGA = Serial.readStringUntil('\r');
  //       // Serial.println(payload);

  //       StaticJsonDocument<500> JsonDoc;
  //       deserializeJson(JsonDoc, payload_MEGA);
  //       payload_MEGA.toCharArray(payload_char, payload_MEGA.length() + 1);
  //       String type = JsonDoc["type"];

  //       if (type == "CA-SWR1")
  //       {
  //         client.publish(CA_SR, payload_char);
  //       }
  //       else if (type == "CA-SWR2")
  //       {
  //         client.publish(CA_SR2, payload_char);
  //       }
  //       else if (type == "CA-SWR3")
  //       {
  //         client.publish(CA_SR3, payload_char);
  //       }
  //       if (type == "CA-SS02s")
  //       {
  //         client.publish(CA_SS02, payload_char);
  //       }
  //       if (type == "CA-SS03s")
  //       {
  //         client.publish(CA_SS03, payload_char);
  //       }
  //       if (type == "CA-SS04s")
  //       {
  //         client.publish(CA_SS04, payload_char);
  //       }
  //     }
  //   }
  //   else
  //   {
  //     reconnect();
  //   }
  // }
  // else
  // {
  //   Serial.println("WiFi Connected Fail");
  //   WiFi.reconnect();
  //   digitalWrite(ledB, LOW);
  //   boolean state = digitalRead(ledR);
  //   digitalWrite(ledR, !state);
  // }
  // delay(5);
}

void reconnect()
{
  Serial.println("Attempting MQTT connection ...");
  String clientId = "ESP8266Client-testX";
  clientId += String(random(0xffff), HEX);
  if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
  {
    Serial.println("connected");
    for (int i = 0; i < numberOfDevice; i++)
    {
      client.subscribe(CA_device[i].productId);
    }

    // client.subscribe(CA_SR);
    // client.subscribe(CA_SR2);
    // client.subscribe(CA_SR3);
    // client.subscribe(CA_SS02);
    // client.subscribe(CA_SS03);
    // client.subscribe(CA_SS04);
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
  // String data;
  // String mtopic = (String)topic;

  for (uint16_t i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void initial()
{
  String payload;
  EEPROM.begin(4095);
  numberOfDevice = EEPROM.read(0);
  EEPROM.end();
  StaticJsonDocument<300> JsonDoc;
  JsonDoc["command"] = "Number of device";
  JsonDoc["value"] = numberOfDevice;
  serializeJson(JsonDoc, payload);
  Serial.print(payload);
  Serial.print('\r');
  delay(2000);

  if (numberOfDevice <= 0)
  {
    // Serial.println("need sever");
    loadDataFromServer(); // if haven't data form HC start request to sever
  }
  else
  {
    // Serial.println("no need sever");
    loadDataFromEEPROM();
  }

  for (int i = 0; i < numberOfDevice; i++)
  {
    payload.clear();
    JsonDoc.clear();
    JsonDoc["command"] = "Data from user";
    JsonDoc["id"] = CA_device[i].id;
    JsonDoc["productId"] = CA_device[i].productId;
    JsonDoc["RFchannel"] = uint64ToString(CA_device[i].RF_Chanel);
    serializeJson(JsonDoc, payload);
    Serial.print(payload);
    Serial.print('\r');
    delay(2000);
  }

  payload.clear();
  JsonDoc.clear();
  JsonDoc["command"] = "end of data user";
  serializeJson(JsonDoc, payload);
  Serial.print(payload);
  Serial.print('\r');

  while (1) // check data
  {
    if (Serial.available())
    {
      String payload_check = Serial.readStringUntil('\r');
      if (payload_check == "Check Done")
        break;
      else
      {
        JsonDoc.clear();
        deserializeJson(JsonDoc, payload_check);
        int check_id = JsonDoc["check_id"];
        int index = checkDataID(check_id);
        String check_productId = JsonDoc["check_productId"];
        delay(200);
        if (check_productId.equals(CA_device[index].productId))
        {
          Serial.print("OK");
          Serial.print('\r');
        }
        else
        {
          Serial.print("Wrong");
          Serial.print('\r');
        }
      }
    }
    delay(200);
  }

  JsonDoc.clear();
  payload.clear();
  JsonDoc["command"] = "Finish";
  serializeJson(JsonDoc, payload);
  Serial.println(payload);
}

void loadDataFromServer()
{
  // get information device from sever

  String CA1 = "2b92934f-7a41-4ce1-944d-d33ed6d97e13";
  String CA2 = "4a0bfbfe-efff-4bae-927c-c8136df70333";
  String CA3 = "ebb2464e-ba53-4f22-aa61-c76f24d3343d";

  CA1.toCharArray(CA_device[0].productId, CA1.length() + 1);
  CA_device[0].RF_Chanel = 1002502019001;

  CA2.toCharArray(CA_device[1].productId, CA2.length() + 1);
  CA_device[1].RF_Chanel = 1002502019002;

  CA3.toCharArray(CA_device[2].productId, CA3.length() + 1);
  CA_device[2].RF_Chanel = 1002502019003;

  numberOfDevice = 3;

  // start store data to EEPROM
  EEPROM.begin(4095);
  int address = 1;
  for (int i = 0; i < numberOfDevice; i++)
  {
    CA_device[i].id = address;
    EEPROM.put(address, CA_device[i]);
    delay(200);
    address += sizeof(CA_device[i]);
  }
  EEPROM.write(0, numberOfDevice);
  EEPROM.commit();
  EEPROM.end();
}

void loadDataFromEEPROM()
{
  EEPROM.begin(4095);
  int address = 1;
  for (int i = 0; i < numberOfDevice; i++)
  {
    EEPROM.get(address, CA_device[i]);
    delay(200);
    address += sizeof(CA_device[i]);
  }
  EEPROM.end();
}

int checkDataID(int id)
{
  int i = 0;
  for (i = 0; i < numberOfDevice; i++)
  {
    if (id == CA_device[i].id)
      return i;
  }
  return -1;
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