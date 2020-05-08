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
#define ledB 4
#define btn_config 14
#define ledIR 5
#define ledRecv 13

uint32_t timer = 0;
uint16_t longPressTime = 6000;

boolean buttonActive = false;
boolean learn = false;
boolean control = false;

const uint16_t kCaptureBufferSize = 1024;
const uint8_t kTimeOut = 50;
const uint16_t kFrequency = 38000;

const char *mqtt_server = "chika.gq";
const int mqtt_port = 2502;
const char *mqtt_user = "chika";
const char *mqtt_pass = "2502";

char *topicLearn = "CA-IRX0.01/learn";
char *topicControl = "CA-IRX0.01/control";
char *topicCancel = "CA-IRX0.01/cancel";

String IR_value;

Ticker ticker;
WiFiClient esp;
PubSubClient client(esp);
IRrecv irrecv(ledRecv, kCaptureBufferSize, kTimeOut, false);
IRsend irsend(ledIR);
decode_results results;
StaticJsonDocument<2000> JsonDoc;

void tick();
void tick2();
void exitSmartConfig();
boolean startSmartConfig();
void longPress();
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();
uint64_t stringToUint64(String input);
uint64_t iDontKnow(int exp);

void setup()
{
  Serial.begin(115200);
  Serial.println("IR Device is ready");

  irsend.begin();

  WiFi.setAutoConnect(true);   // auto connect when start
  WiFi.setAutoReconnect(true); // auto reconnect the old WiFi when leaving internet

  pinMode(ledR, OUTPUT);
  pinMode(btn_config, INPUT);
  pinMode(ledB, OUTPUT);
  pinMode(ledIR, OUTPUT);

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
      // do something hereSerial
      if (control)
      {
        Serial.println("IR_send");
        JsonDoc.clear();
        deserializeJson(JsonDoc, IR_value);
        String protocol = JsonDoc["protocol"]; //get protocol
        String value = JsonDoc["value"];       //get value IR
        uint16_t nbit = JsonDoc["nbit"];       //get size IR value

        uint64_t IRvalue = stringToUint64(value);

        int arraySize = JsonDoc["state"].size(); // get array state
        uint8_t stateVal[arraySize];
        for (uint16_t i = 0; i < arraySize; i++)
        {
          stateVal[i] = JsonDoc["state"][i].as<uint8_t>();
        }

        uint16_t rawSize = JsonDoc["rawData"].size(); // get array state
        uint16_t rawData[rawSize];
        for (int i = 0; i < arraySize; i++)
        {
          stateVal[i] = JsonDoc["rawData"][i].as<uint16_t>();
        }

        char protocol_char[20];
        protocol.toCharArray(protocol_char, protocol.length() + 1);
        decode_type_t protocolType = strToDecodeType(protocol_char);
        // decode_type_t protocolType = decode_type_t::SONY;

        if (hasACState(protocolType))
        {
          Serial.println("Send AC");
          irsend.send(protocolType, stateVal, nbit / 8);
        }
        else if (protocol.equals("UNKNOWN"))
        {
          Serial.println("Send UNKNOW");
          irsend.sendRaw(rawData, rawSize, 38);
        }
        else
        {
          Serial.println("Send has Protocol");
          // irsend.send(protocolType, IRvalue, nbit);
          irsend.sendRaw(rawData, rawSize, 38);
        }

        control = false;
      }
      // //======================= end of control ================================

      // //======================= start learn signal ============================
      while (learn)
      {
        digitalWrite(ledB, LOW);
        tick();
        client.loop();
        if (irrecv.decode(&results))
        {
          JsonDoc.clear();
          JsonDoc["protocol"] = typeToString(results.decode_type);
          JsonDoc["nbit"] = results.bits;
          JsonDoc["value"] = uint64ToString(results.value, 10);

          JsonArray state = JsonDoc.createNestedArray("state"); // create array of state for Air conditioner
          for (int i = 0; i < (results.bits / 8); i++)
          {
            state.add(results.state[i]);
          }

          uint16_t size = getCorrectedRawLength(&results);
          uint16_t *rawData;
          rawData = resultToRawArray(&results);

          JsonArray raw = JsonDoc.createNestedArray("rawData");
          for (int i = 0; i < size; i++)
          {
            raw.add(rawData[i]);
          }

          String payload;
          serializeJson(JsonDoc, payload);
          char payload_char[2000];
          Serial.println(payload);
          payload.toCharArray(payload_char, payload.length() + 1);
          client.publish(topicLearn, payload_char, false);
          learn = false;
          digitalWrite(ledR, HIGH);
          digitalWrite(ledB, LOW);
          irrecv.disableIRIn(); // Stop Led receiver
        }
        delay(200);
      }
      digitalWrite(ledB, HIGH);
      digitalWrite(ledR, LOW);
    }
    else
    {
      delay(100);
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
    delay(500);
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

  for (uint16_t i = 0; i < length; i++)
  {
    data += (char)payload[i];
  }
  Serial.println(data);

  if (mtopic.equals(topicControl))
  {
    IR_value = data;
    control = true;
  }

  if (mtopic.equals(topicLearn))
  {
    Serial.println("learn");
    if (data.length() < 5)
    {
      learn = true;
      irrecv.enableIRIn(); // Start led receiver
    }
  }

  if (mtopic.equals(topicCancel))
  {
    learn = false;
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
      timer = millis();
      Serial.println(timer);
    }

    if (millis() - timer > longPressTime)
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