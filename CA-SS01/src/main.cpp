#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

#define ledR 16
#define ledB 5
#define btn_config 4
#define mc35 0
#define btn_allow 14
#define alarm 12
#define alarmLed 13

uint32_t timer = 0;
uint16_t longPressTime = 6000;

boolean buttonActive = false;
boolean sensorVal = false;
boolean sensorLoop = false;
boolean initialSensor = false;
boolean allowAlarm = false;
boolean buttonValue = false;
boolean door = false;
boolean doorLoop = false;

const char *mqtt_server = "chika.gq";
const int mqtt_port = 2502;
const char *mqtt_user = "chika";
const char *mqtt_pass = "2502";

const char *doorState = "CA-Security";
const char *doorCommand = "CA-Security/control";

Ticker ticker;
WiFiClient esp;
PubSubClient client(esp);

void tick();
void tick2();
void exitSmartConfig();
boolean startSmartConfig();
void longPress();
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();
boolean checkSensor();
void checkButton();
void tickAlarm();

void setup()
{
  Serial.begin(115200);

  WiFi.setAutoConnect(true);   // auto connect when start
  WiFi.setAutoReconnect(true); // auto reconnect the old WiFi when leaving internet

  pinMode(ledR, OUTPUT);            // led red set on
  pinMode(ledB, OUTPUT);            // led blue set on
  pinMode(btn_config, INPUT);       // btn_config is ready
  pinMode(mc35, INPUT_PULLUP);      // sensor is ready
  pinMode(btn_allow, INPUT_PULLUP); // button allow alarm ready
  pinMode(alarm, OUTPUT);
  pinMode(alarmLed, OUTPUT);

  ticker.attach(1, tick2); // initial led show up

  Serial.println("Waiting for Internet");
  uint16_t i = 0;
  digitalWrite(alarm, LOW);
  digitalWrite(alarmLed, LOW);
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
    digitalWrite(ledB, HIGH);
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
    digitalWrite(ledB, HIGH);
    digitalWrite(ledR, LOW);
    if (client.connected())
    {
      client.loop();
      // do something here
      door = checkSensor();
      checkButton();
      if (allowAlarm)
      {
        digitalWrite(alarmLed, HIGH); //turn on led
        if (door || doorLoop)
        {
          doorLoop = true;
          digitalWrite(alarm, HIGH);
          delay(200);
        }
      }
      else
      {
        doorLoop = false;
        ticker.detach();
        digitalWrite(alarm, LOW);
        digitalWrite(alarmLed, LOW); //turn off led
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
    delay(500);
  }

  delay(100);
}

void tickAlarm()
{
  boolean state = digitalRead(alarm);
  digitalWrite(alarm, !state);
}

boolean checkSensor()
{
  sensorVal = digitalRead(mc35);
  if (!initialSensor)
  {
    if (sensorVal)
    {
      sensorLoop = true;
      client.publish(doorState, "OPEN");
      return true;
    }
    else
    {
      sensorLoop = false;
      client.publish(doorState, "CLOSE");
      return false;
    }
    initialSensor = true;
  }
  if (sensorVal && !sensorLoop)
  {
    client.publish(doorState, "OPEN");
    sensorLoop = true;
    return true;
  }
  else if (!sensorVal && sensorLoop)
  {
    client.publish(doorState, "CLOSE");
    sensorLoop = false;
    return false;
  }
}

void checkButton()
{
  buttonValue = digitalRead(btn_allow);
  //Serial.println(buttonValue);
  if (!buttonValue)
  {
    while (!buttonValue)
    {
      buttonValue = digitalRead(btn_allow);
      delay(100);
    }
    allowAlarm = !allowAlarm;
  }
}

void reconnect()
{
  Serial.println("Attempting MQTT connection ...");
  String clientId = "ESP8266Client-testX";
  clientId += String(random(0xffff), HEX);
  if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
  {
    Serial.println("connected");
    client.subscribe(doorCommand);
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

  if(topic == doorCommand){
    if(data == "ON")  allowAlarm = true;
    else  allowAlarm = false;
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
  Serial.println(digitalRead(btn_config));
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