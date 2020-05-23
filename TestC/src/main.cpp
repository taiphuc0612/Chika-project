#include <Arduino.h>

char *payload = "100010000101101111100100000011110000000000000000000000000100000010001000010110111110010000000000000000001000110000101100000000000000010100000000000000000000000000000000000000000000000000000011000000000000000011101011";
uint16_t payloadDakin216[439];
String byteAC[27];

void setupRaw();

void setup()
{
  Serial.begin(9600);
  setupRaw();
  for (int i = 0; i < 439; i++)
  {
    Serial.print(payloadDakin216[i]);
    Serial.print(" ");
  }
  Serial.println();
}

void loop()
{
  delay(2000);
}

void setupRaw()
{
  for (int i = 0; i < 27; i++)
  {
    for (int j = 8 * i; j < 8 * i + 8; j++)
    {
      byteAC[i] += payload[j];
    }
  }

  payloadDakin216[0] = 3460;
  payloadDakin216[1] = 1766;
  payloadDakin216[2] = 400;
  payloadDakin216[131] = 29628;
  payloadDakin216[132] = 3460;
  payloadDakin216[133] = 1766;
  payloadDakin216[134] = 400;

  int index = 3;

  while (*payload != NULL)
  {
    if (payloadDakin216[index] >= 25000)
      index += 4;
    if (*payload == '1')
    {

      payloadDakin216[index] = 1280;
      index++;
      payloadDakin216[index] = 440;
      index++;
    }
    else if (*payload == '0')
    {
      payloadDakin216[index] = 450;
      index++;
      payloadDakin216[index] = 420;
      index++;
    }
    payload++;
  }
}