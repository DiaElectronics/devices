#define MODEL_TYPE "DIAE_ESSENTIALS"

#define BUTTON_DELAY_MS 80

#define BUTTON1_PIN 0
#define BUTTON2_PIN 1
#define BUTTON3_PIN 2
#define BUTTON4_PIN 3
#define BUTTON5_PIN 4
#define BUTTON6_PIN 5
#define BUTTON7_PIN 6
#define BUTTON8_PIN 7

#define LED1_PIN 9
#define LED2_PIN 8
#define LED3_PIN 7
#define LED4_PIN 6
#define LED5_PIN 5
#define LED6_PIN 4
#define LED7_PIN 11
#define LED8_PIN 10

#define TOTAL_BUTTONS 8

int ledPins[TOTAL_BUTTONS];
int buttonPins[TOTAL_BUTTONS];
int buttonStates[TOTAL_BUTTONS];
long buttonStateChangeLast[TOTAL_BUTTONS];

byte lastButtonPressed = 0;
void TurnLight(int buttonNum, int isOn)
{
  if(isOn)
  {
    Serial.print("led on:");
    digitalWrite(ledPins[buttonNum], HIGH);
  }
  else
  {
    Serial.print("led off");
    digitalWrite(ledPins[buttonNum], LOW);
  }
  Serial.println(buttonNum+1);
  return;
}

void CheckCode()
{
  char temp;
  while(Serial.available())
  {
    temp=Serial.read();
    if(temp==0x23) //#
    {
      Serial.write("#");
      Serial.write(MODEL_TYPE);
      Serial.println();
    }
    else if(temp == 0x24) //$
    {
      byte res = lastButtonPressed +48;
      Serial.write(res);
      lastButtonPressed = 0;
    } 
    else if(temp=='+' || temp=='-')
    {
      int sig = 1;
      if(temp =='-')
      {
        sig = 0;
      }
      if(!Serial.available())
      {
        delay(20);
      }
      if(Serial.available())
      {
        temp=Serial.read();
      }
      else
      {
        temp='0';
      }
      
      if(temp=='0')
      {
        for(int i=0;i<TOTAL_BUTTONS;i++)
        {
          TurnLight(i,sig);
        }
      }
      else
      {
        TurnLight(temp-48-1,sig);
      }
    }
  }
}

void PrepareButtonPin(int buttonNumber, int PinNumber)
{
  if(TOTAL_BUTTONS>buttonNumber)
  {
    buttonPins[buttonNumber] = PinNumber;
  }
}

void PrepareLedPin(int buttonNumber, int pinNumber)
{
  if(TOTAL_BUTTONS>buttonNumber)
  {
    pinMode(pinNumber, OUTPUT);
    digitalWrite(pinNumber, HIGH);
    ledPins[buttonNumber] = pinNumber;
  }
}

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  PrepareButtonPin(0, BUTTON1_PIN);
  PrepareButtonPin(1, BUTTON2_PIN);
  PrepareButtonPin(2, BUTTON3_PIN);
  PrepareButtonPin(3, BUTTON4_PIN);
  PrepareButtonPin(4, BUTTON5_PIN);
  PrepareButtonPin(5, BUTTON6_PIN);
  PrepareButtonPin(6, BUTTON7_PIN);
  PrepareButtonPin(7, BUTTON8_PIN);

  PrepareLedPin(0, LED1_PIN);
  PrepareLedPin(1, LED2_PIN);
  PrepareLedPin(2, LED3_PIN);
  PrepareLedPin(3, LED4_PIN);
  PrepareLedPin(4, LED5_PIN);
  PrepareLedPin(5, LED6_PIN);
  PrepareLedPin(6, LED7_PIN);
  PrepareLedPin(7, LED8_PIN);
 
  for(int i=0;i<TOTAL_BUTTONS;i++)
  {
    buttonStateChangeLast[i] = millis();
    buttonStates[i] = 0;    
  }  
}

void ChangeState(byte buttonNumber)
{
  buttonStateChangeLast[buttonNumber] = millis();
  if(buttonStates[buttonNumber]>0)
  {
    buttonStates[buttonNumber] = 0;
    lastButtonPressed = buttonNumber+1;

    Serial.println(buttonNumber+1);
  }
  else
  {
    buttonStates[buttonNumber] = 1;
  }
}
void loop() 
{
  //if # returns device name, if $ returns last button
  CheckCode(); 

  int val = 0;
    
  for(byte i=0;i<TOTAL_BUTTONS;i++)
  {
    val = analogRead(buttonPins[i]);
    //Serial.println(val);
    if(val<20)
    {
      val = 1;
    }
    else
    {
      val = 0;
    }
    if(buttonStates[i]!=val)
    {
      if(millis()>buttonStateChangeLast[i]+80)
      {
        ChangeState(i);
      }
    }
  }
}
