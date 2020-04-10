#include <Arduino.h>
#include <MQ135.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <SPI.h>
#include <RF24.h>

#define MQ135_pin A2
#define dht_pin A1
#define dht_type DHT11
#define ledR 3
#define ledG 5
#define ledB 6
#define CE 9
#define CSN 10

RF24 radio(CE, CSN);
MQ135 mq135_sensor = MQ135(MQ135_pin);
DHT dht(dht_pin, dht_type);

const uint64_t address = 1002502019005;
// const byte address[6] = "00008";

float sensorValue[3];
uint16_t timer = 0;
uint16_t error = 0.5;

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

void setup()
{
  Serial.begin(9600);
  dht.begin();
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);
  //=================RF=====================
  SPI.begin();
  radio.begin();
  radio.setRetries(15, 15);
  radio.setPALevel(RF24_PA_MAX);
  radio.openWritingPipe(address);
  //========================================
}

void loop()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  String output;
  output += F("temperature : ");
  output += t;
  output += F("\t");
  output += F("humidity :");
  output += h;

  Serial.println(output);

  float rzero = mq135_sensor.getRZero();                       // Hệ số trung bình
  float correctedRZero = mq135_sensor.getCorrectedRZero(t, h); // Hệ số từ nhiệt độ độ ẩm
  float resistance = mq135_sensor.getResistance();             // khoảng cách tối đa
  float ppm = mq135_sensor.getPPM();                           // lấy giá trị ppm thường
  float correctedPPM = mq135_sensor.getCorrectedPPM(t, h);     // lấy giá trị ppm sau khi tính với nhiệt độ độ ẩm

  // Serial.print("MQ135 RZero: ");
  // Serial.print(rzero);
  // Serial.print("\t Corrected RZero: ");
  // Serial.print(correctedRZero);
  // Serial.print("\t Resistance: ");
  // Serial.print(resistance);
  // Serial.print("\t PPM: ");
  // Serial.print(ppm);
  // Serial.print("\t Corrected PPM: ");
  // Serial.print(correctedPPM);
  // Serial.println("ppm");
  // Serial.println();

  sensorValue[0] = t;
  sensorValue[1] = h;
  sensorValue[2] = correctedPPM;

  timer++;
  if(timer > 50){
    Serial.println("send ...");
    radio.write(sensorValue, sizeof(sensorValue));
    timer = 0;
    delay(100);
  }


  if (correctedPPM < 1.5)
  {
    if (!digitalRead(ledB))
    {
      showBlue();
      if(correctedPPM < 1.5 + error)        radio.write(sensorValue, sizeof(sensorValue));
    }
  }
  else if (correctedPPM >= 1.5 && correctedPPM < 4.2)
  {
    if (!digitalRead(ledG))
    {
      radio.write(sensorValue, sizeof(sensorValue));
      if (!digitalRead(ledR))
        showBlue2Green();
      else if (!digitalRead(ledB))
        showRed2Green();
    }
  }
  else if (correctedPPM >= 4.2)
  {
    if (!digitalRead(ledR))
    {
      radio.write(sensorValue, sizeof(sensorValue));
      showRed();
    }
  }

  delay(100);
}
