/* In this product - the address (channel) to communicate is define as <the HC code> (3 degits)
+ <company code> (7 degits) + <product code> (3 degits). So we have the following list product code:      
CA-SWR: 1002502019001 (13)
CA-SWR2: 1002502019002 (13)
CA-SWR3: 1002502019003 (13)
  Product code: 740a8d1e-c649-475e-a270-c5d9a44b40a8
  RF channel (3 button): 83878226022002
  Button topic: 774f2306-51ad-4bf1-ba9e-0ddee9bd2375
                5124ba3a-7a45-472b-8468-6f2a041733ac
                a0087ff7-3613-442f-b6c5-0d5d2f0f1a30
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <RF24.h>
#include <SPI.h>
using namespace std;

int CE = 9;
int CSN = 10;
RF24 radio(CE, CSN);                    //nRF24L01 (CE,CSN) connections PIN
const uint64_t address = 1002502019003; //Changeable

const int control[3] = {2, 3, 4};
const int button[3] = {5, 6, 7};
const int led_state[3] = {A3, A2, A1};

int deviceState[3];

float state_Device_sendtoHC[3];
float message_controlfromHC[3];

void checkDevicesState()
{

  for (int i = 0; i < 3; i++)
  {
    deviceState[i] = EEPROM.read(i);
    if (deviceState[i])
    {
      digitalWrite(control[i], HIGH);
      digitalWrite(led_state[i], HIGH);
    }
  }
}

int isButton_Click(int GPIO_to_read)
{
  int out = 0;
  while (digitalRead(GPIO_to_read) == 1)
  {
    delay(50);
    out = 1;
  }
  return out;
}

void setup()
{
  SPI.begin();
  Serial.begin(9600);
  Serial.println("\nCA-SWR3 say hello to your home <3 ! ");
  for (int i = 0; i < 3; i++)
  {
    pinMode(button[i], INPUT);
    pinMode(control[i], OUTPUT);
    pinMode(led_state[i], OUTPUT);
  }
  pinMode(CSN, OUTPUT);

  checkDevicesState();
  radio.begin();
  radio.setRetries(15, 15);
  radio.setPALevel(RF24_PA_MAX);
}

void loop()
{
  radio.openReadingPipe(1, address);
  radio.startListening();

  if (radio.available())
  {
    memset(&message_controlfromHC, ' ', sizeof(message_controlfromHC));
    radio.read(&message_controlfromHC, sizeof(message_controlfromHC));

    // first message (message_controlfromHC[0]) is index button (1 - 2 -3)
    // second message is controled state of button
    int buttonIndex = ((int)message_controlfromHC[0]) - 1;
    boolean controlState = (boolean)message_controlfromHC[1];


    digitalWrite(control[buttonIndex], controlState);
    digitalWrite(led_state[buttonIndex], controlState);
    deviceState[buttonIndex] = controlState;
  }

  for (int i = 0; i < 3; i++)
  {
    boolean check_Button = isButton_Click(button[i]);
    if (check_Button)
    {
      radio.stopListening();
      deviceState[i] = !deviceState[i];
      digitalWrite(control[i], deviceState[i]);
      digitalWrite(led_state[i], deviceState[i]);
      state_Device_sendtoHC[0] = (float)(i + 1);        // button index (1 - 2 - 3)
      state_Device_sendtoHC[1] = (float)deviceState[i]; // state button (true - false)
      EEPROM.update(i, deviceState[i]);

      radio.openWritingPipe(address);
      radio.write(&state_Device_sendtoHC, sizeof(state_Device_sendtoHC));
    }
  }
}