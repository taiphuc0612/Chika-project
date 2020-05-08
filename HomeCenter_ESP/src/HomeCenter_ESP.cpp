#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
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
String tokenUser;

const char *mqtt_server = "chika.gq";
const int mqtt_port = 2502;
const char *mqtt_user = "chika";
const char *mqtt_pass = "2502";

const char *HC_ID = "f7a3bde5-5a85-470f-9577-cdbf3be121d4";

Ticker ticker;
WiFiClient esp;
PubSubClient client(esp);
DHT SS00(DHT_pin, DHT_type);
HTTPClient httpClient;
StaticJsonDocument<2000> JsonDoc;

typedef struct
{
  int id;                         // address device in EEPROM
  char productId[100];            // ID of product and topic too
  char RF_Chanel[50];             // channel for communicate with orther device using RF signal
  char type[20];                  // type of device (SR, SS01, SS0X, ....)
} device __attribute__((packed)); // stuct of Chika device help store data esay for managing (and I haven't known "attribute((packed))" yet :D )

device CA_device[20]; //We have 20 Chika device and it's maxium from now :D

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
String httpGET(HTTPClient &http, String address, String token);

void setup()
{
  Serial.begin(115200);
  delay(4000);
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
    Serial.print("WIFI_CONNECTED");
    Serial.print('\r');
    Serial.print("SSID: ");
    Serial.print(WiFi.SSID());
    Serial.print('\r');
    Serial.print("IP: ");
    Serial.print(WiFi.localIP());
    Serial.print('\r');
  }
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  while (!client.connected())
  {
    reconnect(); // Need connect to MQTT for getting token user
    delay(100);
  }

  initial(); // proceed reload data - get data of device user having from server
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
      // ESP send data temperture and humidity per 5 sencond
      timer_sendTempHumi++;
      if (timer_sendTempHumi >= 500)
      {
        timer_sendTempHumi = 0;
        float h = SS00.readHumidity();
        float t = SS00.readTemperature();

        String sendTempHumi;
        char payload_sendTempHumi[300];
        JsonDoc.clear();
        JsonDoc["type"] = "CA-SS00";
        JsonDoc["temperture"] = t;
        JsonDoc["humidity"] = h;
        serializeJson(JsonDoc, sendTempHumi);
        Serial.print(sendTempHumi); // ESP send data to MEGA
        Serial.print('\r');
        sendTempHumi.toCharArray(payload_sendTempHumi, sendTempHumi.length() + 1);
        client.publish(HC_ID, payload_sendTempHumi, false); // ESP send data to MQTT
      }
      //=========================================================================
      //================MQTT send data command for Home central==================
      if (Serial.available())
      {
        String payload_MEGA = Serial.readStringUntil('\r');

        JsonDoc.clear(); // clear Object JsonStaticDocument
        deserializeJson(JsonDoc, payload_MEGA);
        payload_MEGA.toCharArray(payload_char, payload_MEGA.length() + 1);

        int id_device = JsonDoc["id"];
        int index = checkDataID(id_device); // get index of array CA_device
        client.publish(CA_device[index].productId, payload_char, false);
      }
      //==========================================================================
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

//==============================MQTT==============================

void reconnect()
{
  Serial.print("Attempting MQTT connection ...");
  Serial.print('\r');
  String clientId = "ESP8266Client-testX";
  clientId += String(random(0xffff), HEX);
  if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
  {
    client.subscribe(HC_ID);
    Serial.print("connected");
    Serial.print('\r');

    for (int i = 0; i < numberOfDevice; i++)
    {
      client.subscribe(CA_device[i].productId);   // MQTT control RF device by productID
    }
  }
  else
  {
    Serial.print("MQTT Connected Fail, rc = ");
    Serial.print(client.state());
    Serial.print('\r');
    Serial.print("try again in 5 seconds");
    Serial.print('\r');
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

  if (mtopic == HC_ID)
  {
    tokenUser = data;   // Token for get user device on server
  }

  for (int i = 0; i < numberOfDevice; i++)
  {
    if (mtopic.equals(CA_device[i].productId))
    {
      // Topic is product id
      // add productID to Json sending to MEGA for controlling
      // Helping Mega can find index of device in array stuct CA_device, hence Mega can get RF_channel to control
      JsonDoc.clear();
      deserializeJson(JsonDoc, data);
      JsonDoc["productID"] = mtopic;    
      String payload;
      serializeJson(JsonDoc, payload);
      Serial.print(payload);
      Serial.print('\r');
    }
  }
}

//====================================================================

//============================initial=================================

void initial()
{
  loadDataFromServer();

  String payload;
  JsonDoc.clear();
  JsonDoc["command"] = "Number_Of_Device";
  JsonDoc["value"] = numberOfDevice;
  serializeJson(JsonDoc, payload);
  Serial.print(payload);
  Serial.print('\r');
  delay(2000);

  for (int i = 0; i < numberOfDevice; i++)
  {
    payload.clear();
    JsonDoc.clear();
    delay(1000);
    JsonDoc["command"] = "Data_Of_Device";
    JsonDoc["id"] = CA_device[i].id;
    JsonDoc["type"] = CA_device[i].type;
    JsonDoc["productId"] = CA_device[i].productId;
    JsonDoc["RFchannel"] = CA_device[i].RF_Chanel;
    serializeJson(JsonDoc, payload);

    Serial.print(payload);
    Serial.print('\r');
    delay(1000);
  }

  payload.clear();
  JsonDoc.clear();
  delay(1000);
  JsonDoc["command"] = "End_Data_Device";
  serializeJson(JsonDoc, payload);
  Serial.print(payload);
  Serial.print('\r');

  while (1) // check data
  {
    if (Serial.available())
    {
      String payload_check = Serial.readStringUntil('\r');
      if (payload_check == "Check_Done")
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
  delay(1000);
  JsonDoc["command"] = "Finish";
  serializeJson(JsonDoc, payload);
  Serial.println(payload);
}

void loadDataFromServer()
{
  //get information device from sever
  //get token from MQTT
  int timer = 0;
  while (1)
  {
    timer++;
    delay(100);
    client.loop();
    if (tokenUser.length() > 4)
      break;
    if (timer > 100)
    {
      loadDataFromEEPROM();
      return;
    }
  }
  //Using token to get data from server
  String dataUser = httpGET(httpClient, "http://chika-server.herokuapp.com/product/rf", tokenUser);

  //if GET fail using data on cache (EEPROM)
  if (dataUser.equals("Fail"))
  {
    Serial.print("Fail to get data server");
    Serial.print('\r');
    loadDataFromEEPROM();
    return;
  }
  //========================================
  // if ESP has user's device
  JsonDoc.clear();
  deserializeJson(JsonDoc, dataUser);

  numberOfDevice = JsonDoc.size();    // user's device is Json array

  // store data to array stuct 'device' and sudcribe topic product id at the same time
  for (int i = 0; i < JsonDoc.size(); i++)
  {
    String CA_product = JsonDoc[i]["id"];
    CA_product.toCharArray(CA_device[i].productId, CA_product.length() + 1);
    String CA_rfChannel = JsonDoc[i]["rfChannel"];
    CA_rfChannel.toCharArray(CA_device[i].RF_Chanel, CA_rfChannel.length() + 1);
    String CA_type = JsonDoc[i]["type"];
    CA_type.toCharArray(CA_device[i].type, CA_type.length() + 1);
    client.subscribe(CA_device[i].productId);
    delay(400);
  }

  // start store data to EEPROM
  EEPROM.begin(4095);
  int address = 1;
  for (int i = 0; i < numberOfDevice; i++)
  {
    CA_device[i].id = address;
    EEPROM.put(address, CA_device[i]);
    address += sizeof(CA_device[i]);
    delay(200);
  }
  EEPROM.write(0, numberOfDevice);
  EEPROM.commit();
  EEPROM.end();
}

void loadDataFromEEPROM()
{
  Serial.print("Load from EEPROM");
  EEPROM.begin(4095);
  numberOfDevice = EEPROM.read(0);
  int address = 1;
  // get data from EEPROM
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

//========================================================================================

//===================================SmartConfig==========================================
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
  Serial.print("On_SmartConfig");
  Serial.print('\r');
  WiFi.beginSmartConfig();
  delay(500);
  ticker.attach(0.1, tick);
  while (WiFi.status() != WL_CONNECTED)
  {
    t++;
    Serial.print(".");
    Serial.print('\r');
    delay(200);
    if (t > 100)
    {
      Serial.print("SmartConfig_fail");
      Serial.print('\r');
      ticker.attach(0.5, tick);
      delay(3000);
      exitSmartConfig();
      return false;
    }
  }
  Serial.print("WIFI_CONNECTED");
  Serial.print('\r');
  delay(1000);
  Serial.print("SSID: ");
  Serial.print(WiFi.SSID());
  Serial.print('\r');
  Serial.print("IP: ");
  Serial.print(WiFi.localIP());
  Serial.print('\r');
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

//============================================================================

//============================HTTP requests method============================

String httpGET(HTTPClient &http, String address, String token)
{
  String payload;
  // Serial.print("GET Method/Procedure");
  // Serial.print('\r');
  // Serial.println(token);
  http.begin(address);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", token);

  int httpCode = http.GET();
  if (httpCode > 0)
  {
    payload = http.getString();
    // Serial.println(payload);
  }
  else
  {
    Serial.print("Parsing GET procedure fail");
    Serial.print('\r');
    return "Fail";
  }

  http.end();
  return payload;
}