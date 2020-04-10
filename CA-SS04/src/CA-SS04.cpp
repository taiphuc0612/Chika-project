// gia tri gui di  flame : true/false ; Gas 0/1/2


#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <EEPROM.h>

using namespace std;

#define pinFlame A0
#define pinGas A1
#define ledR 3
#define ledG 5
#define ledB 6
#define buzzer 4
#define CE 9
#define CSN 10

RF24 radio(CE, CSN);

const uint64_t address = 1002502019006;

float sensorValue[2];
float control_fromHC;
float flame, gas;
boolean allowWarning = true;

uint16_t timer = 0;

void showBlue()
{
  analogWrite(ledR, 0);
  analogWrite(ledG, 255);
  analogWrite(ledB, 0);
  for (int i = 0; i <= 255; i++)
  {
    analogWrite(ledB, i);
    analogWrite(ledG, 255 - i);
    delay(3);
  }
}

void showRed()
{
  analogWrite(ledR, 0);
  analogWrite(ledG, 255);
  analogWrite(ledB, 0);
  for (int i = 0; i <= 255; i++)
  {
    analogWrite(ledR, i);
    analogWrite(ledG, 255 - i);
    delay(3);
  }
}

void showRed2Green()
{
  analogWrite(ledR, 255);
  analogWrite(ledG, 0);
  analogWrite(ledB, 0);
  for (int i = 0; i <= 255; i++)
  {
    analogWrite(ledG, i);
    analogWrite(ledR, 255 - i);
    analogWrite(ledB, 0);
    delay(3);
  }
}

void showBlue2Green()
{
  analogWrite(ledR, 0);
  analogWrite(ledG, 0);
  analogWrite(ledB, 255);
  for (int i = 0; i <= 255; i++)
  {
    analogWrite(ledG, i);
    analogWrite(ledB, 255 - i);
    analogWrite(ledR, 0);
    delay(3);
  }
}

void recieveData(){
  radio.openReadingPipe(1,address);
  radio.startListening();
  if(radio.available()){
    radio.read(&control_fromHC,sizeof(control_fromHC));
    allowWarning = (boolean)control_fromHC;
    EEPROM.update(0, allowWarning);
    Serial.println(allowWarning);
  }
}

void sendData() {
  radio.stopListening();
  radio.openWritingPipe(address);
  radio.write(sensorValue, sizeof(sensorValue));
}

void warning(){
  if(allowWarning)  digitalWrite(buzzer, LOW);
  else digitalWrite(buzzer, HIGH);
}

// có lửa chuyển đỏ , nồng độ khí gas cao báo lun

void setup()
{
  Serial.begin(9600);
  //=================RF=====================
  SPI.begin();
  radio.begin();
  radio.setRetries(15, 15);
  radio.setPALevel(RF24_PA_MAX);
  //========================================
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);
  pinMode(buzzer, OUTPUT);
  allowWarning = EEPROM.read(0);
  Serial.println(allowWarning);
}

void loop()
{
  flame = !digitalRead(A0);
  gas = analogRead(A1);

  String output;
  output += F("Flame : ");
  output += flame;
  output += F("\t");
  output += F("gas :");
  output += gas;

  // Serial.println(output);

  recieveData();
  delay(100);

  sensorValue[0] = (float)flame;

  timer++;
  if (timer > 50)
  {
    Serial.println("send ...");
    sendData();
    timer = 0;
  }

  if (!flame)
  {
    if (!digitalRead(buzzer) && gas < 100)
    {
      sendData();
    }
    digitalWrite(buzzer, HIGH);
    //=========================================GAS==========================
    if (gas < 50)
    {
      if (!digitalRead(ledB))
      {
        sensorValue[1] = (float)0;
        sendData();
        digitalWrite(buzzer, HIGH);
        showBlue();
      }
    }
    else if (gas >= 50 && gas < 100)
    {
      sensorValue[1] = (float)1;
      if (!digitalRead(ledG))
      {
        sendData();
        digitalWrite(buzzer, HIGH);
        if (!digitalRead(ledR))
          showBlue2Green();
        else if (!digitalRead(ledB))
          showRed2Green();
      }
    }
    else if (gas >= 100)
    {
      sensorValue[1] = (float)2;
      if (!digitalRead(ledR))
      {
        sendData();
        showRed();
        warning();
      }
    }
    //=====================================================================================
  }
  else if (flame)
  {
    sendData();
    warning();
    if (digitalRead(ledB))
    {
      showBlue2Green();
      showRed();
    }
    else if (digitalRead(ledG))
    {
      showRed();
    }
  }

  delay(100);
}