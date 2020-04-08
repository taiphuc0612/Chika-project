#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <PubSubClient.h>

#include <IRremoteESP8266.h>
#include <IRutils.h>
#include <IRrecv.h>
#include <IRsend.h>

#define ledR 16
#define ledG 5
#define btn_config 15
#define ledIR 4
#define ledRecv 14

uint16_t timer = 0; // set first time config
uint16_t longPressTime = 2000;

boolean buttonActive = false;
boolean learn = false;
boolean control = false;
;

const uint16_t kCaptureBufferSize = 1024;
const uint8_t kTimeout = 50;
const uint16_t kFrequency = 38000;

const char *mqtt_server = "chika.gq";
const int mqtt_port = 2502;
const char *mqtt_user = "chika";
const char *mqtt_pass = "2502";

char *IR_module = "CA-IRX0.01";
char *topicLearn = "CA-IRX0.01/learn";
char *topicControl = "CA-IRX0.01/control";
char *topicDone = "CA-IRX0.01/done";
char *topicCancel = "CA-IRX0.01/cancel";

String IR_value;
char data_char[500];
char protocol_char[50];

Ticker ticker;
WiFiClient esp;
PubSubClient client(esp);
IRrecv irrecv(ledRecv, kCaptureBufferSize, kTimeout, false);
IRsend irsend(ledIR);

decode_results results;

void tick();
void exitSmartConfig();
boolean startSmartConfig();
void longPress();
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();

void setup()
{
  Serial.begin(115200);
  Serial.println("IR Device is ready");

  irsend.begin();
  irrecv.enableIRIn(); // Start the receiver

  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);

  pinMode(ledR, OUTPUT);
  pinMode(btn_config, INPUT);
  pinMode(ledG, OUTPUT);

  delay(10000);
  if (!WiFi.isConnected())
  {
    startSmartConfig();
  }
  else
  {
    digitalWrite(ledG, HIGH);
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
  longPress();
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connected())
    {
      client.loop();

      if (control)
      {
        Serial.println("IR_send");
        StaticJsonDocument<500> JsonDoc;
        deserializeJson(JsonDoc, IR_value);
        String protocol_str = JsonDoc["protocol"];
        uint16_t bits = JsonDoc["bit"];
        uint16_t size = JsonDoc["size"];
        uint64_t irVal = JsonDoc["value"];

        uint16_t arraySize = JsonDoc["state"].size();
        uint8_t stateVal[arraySize + 1];
        for (uint16_t i = 0; i < arraySize; i++)
        {
          stateVal[i] = JsonDoc["state"][i].as<uint8_t>();
        }

        protocol_str.toCharArray(protocol_char, protocol_str.length() + 1);
        decode_type_t protocol = strToDecodeType(protocol_char);

        if (hasACState(protocol))
        {
          irsend.send(protocol, stateVal, size / 8);
          Serial.println("Send AC");
        }
        else if (protocol_str == "UNKNOWN")
        {
          irsend.sendNEC(irVal);
          Serial.println("send Unknown");
        }
        else
        {
          irsend.send(protocol, irVal, bits);
          Serial.println("Send has protocol");
        }

        control = false;
      }

      while (learn)
      {
        digitalWrite(ledG, LOW);
        tick();
        client.loop();
        if (irrecv.decode(&results))
        {
          uint16_t size = getCorrectedRawLength(&results);
          StaticJsonDocument<1000> JsonDoc;
          JsonDoc["protocol"] = typeToString(results.decode_type);
          JsonDoc["bit"] = results.bits;
          JsonDoc["size"] = size;
          JsonDoc["value"] = results.value;

          JsonArray arrState = JsonDoc.createNestedArray("state");
          for (uint16_t i = 0; i < results.bits / 8; i++)
          {
            arrState.add(results.state[i]);
          }

          String data_str;
          serializeJson(JsonDoc, data_str);
          Serial.println(data_str);
          data_str.toCharArray(data_char, data_str.length() + 1);
          for (uint16_t i = 0; i < data_str.length() + 1; i++)
          {
            Serial.print(data_char[i]);
          }
          Serial.println();
          client.publish(topicDone, data_char); // publish funtion expect char[]
          client.publish(topicLearn, "DONE");
          learn = false;
          digitalWrite(ledR, HIGH);
          digitalWrite(ledG, LOW);
          irrecv.resume();
          delay(200);
        }
        delay(200);
      }
      digitalWrite(ledG, HIGH);
      digitalWrite(ledR, LOW);
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
  }
  delay(200);
}

void reconnect()
{
  Serial.println("Attempting MQTT connection ...");
  String clientId = "ESP8266Client-testX";
  clientId += String(random(0xffff), HEX);
  if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
  {
    Serial.println("connected");
    client.subscribe(topicLearn);
    client.subscribe(topicControl);
    client.subscribe(topicCancel);
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
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String data;
  String mtopic = (String)topic;

  for (int i = 0; i < length; i++)
  {
    data += (char)payload[i];
  }
  Serial.println(data);

  if (mtopic == topicControl)
  {
    IR_value = data;
    Serial.println(IR_value);
    control = true;
  }

  if (mtopic == topicLearn)
  {
    Serial.println("learn");
    if (data == "DONE")
    {
      learn = false;
    }
    else
      learn = true;
  }

  if (mtopic == "topicCancel")
  {
    learn = false;
  }
}

void tick()
{
  boolean state = digitalRead(ledR);
  digitalWrite(ledR, !state);
}

void exitSmartConfig()
{
  WiFi.stopSmartConfig();
  ticker.detach();
  digitalWrite(ledR, LOW);
  digitalWrite(ledG, HIGH);
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
      timer = millis();
    }

    if (millis() - timer > longPressTime)
    {
      Serial.println("SmartConfig Start");
      digitalWrite(ledG, LOW);
      startSmartConfig();
    }
  }
  else
  {
    buttonActive = false;
  }
}
