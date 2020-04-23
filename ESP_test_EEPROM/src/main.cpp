#include <Arduino.h>
#include <EEPROM.h>

int value = 0;

typedef struct
{
  int id;
  char productId[100];
  uint64_t RF_Chanel;
} device __attribute__((packed));

device CA_device[3];
device devicetest[3];

String CA1 = "2b92934f-7a41-4ce1-944d-d33ed6d97e13taolao";
String CA2 = "4a0bfbfe-efff-4bae-927c-c76f24d3343dtaolao";
String CA3 = "ebb2464e-ba53-4f22-aa61-c76f24d3343dtaolao2";

void setup()
{
  Serial.begin(115200);

  // CA_device[0].id = 1;
  // CA1.toCharArray(CA_device[0].productId, CA1.length() + 1);
  // CA_device[0].RF_Chanel = 1002502019001;
  // Serial.println(CA_device[0].productId);

  // Serial.printf("size of CA_device[0] = %d" , sizeof(CA_device[0]));
  // Serial.printf("size of devicetest[0] = %d" , sizeof(devicetest[0]));

  // CA_device[1].id = 2;
  // CA2.toCharArray(CA_device[1].productId, CA2.length() + 1);
  // CA_device[1].RF_Chanel = 1002502019002;
  // Serial.println(CA_device[1].productId);

  // CA_device[2].id = 3;
  // CA3.toCharArray(CA_device[2].productId, CA3.length() + 1);
  // CA_device[2].RF_Chanel = 1002502019003;
  // Serial.println(CA_device[2].productId);

  // EEPROM.begin(4095);
  // int address = 1;
  // for (int i = 0; i < 3; i++)
  // {
  //   EEPROM.put(address, CA_device[i]);
  //   delay(200);
  //   address += sizeof(CA_device[i]);
  // }
  // EEPROM.commit();
  // EEPROM.end();

  // delay(200);

  int address = 1;
  EEPROM.begin(4095);
  for (int i = 0; i < 3; i++)
  {
    EEPROM.get(address, devicetest[i]);
    delay(200);
    address += sizeof(CA_device[i]);
  }
  EEPROM.end();
}

void loop()
{
  // Serial.println("begin");
  Serial.println(devicetest[0].id);
  Serial.println(devicetest[0].productId);
  Serial.println(devicetest[1].id);
  Serial.println(devicetest[1].productId);
  Serial.println(devicetest[2].id);
  Serial.println(devicetest[2].productId);
  // Serial.printf("Device RF chanel: %d", devicetest[0].RF_Chanel);
  Serial.println();
  delay(1000);
}