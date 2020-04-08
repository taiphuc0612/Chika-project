#include <Arduino.h>

#define sensor 3
#define buzzer 4

uint16_t delayTime = 3000;

void setup()
{
  Serial.begin(9600);
  pinMode(sensor, INPUT);
  pinMode(buzzer, OUTPUT);
}

void loop()
{
  delay(200);
  boolean value = digitalRead(sensor);

  if (value == 1)
  {
    digitalWrite(buzzer, HIGH);
    while (value == 1)
    {
      value = digitalRead(sensor);
      Serial.println(value);
      delay(200);
    }
    Serial.print("delay :");
    Serial.print(delayTime);
    Serial.println(" ms");
    delay(delayTime);
  }
  else
    digitalWrite(buzzer, LOW);
}