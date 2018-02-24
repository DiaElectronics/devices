/*
commands supported
------------------
command 1:
+1/180000/1000/500|+2/4000|-3|-4|+5/6000/100/30;
relay 1, turn on for 180000 ms (180 sec) wih loop of 1000ms on and 500 ms off
relay 2, just turn on for 4000 ms (4 sec)
relays 3 and 4 are off,
relay 5 is on for 6 seconds, with cycles of 100ms on and 30 ms off
command 2:
#
returns the name of current device

command 3:
$
use it for ping. if device is not get pinged for few seconds it stops blinking,
so people can see its not working, by default it is returning 'K' which means ok 
(so pinging device can check that everything is fine)

command 4:
g
means get channel, returns channel

command 5:
s1
sets first channel for the device
*/

#include <EEPROM.h>

#define MODEL_TYPE "DIAE_ACTIVATOR_SATELLITE"

//NO COMMENTS FOR THIS MODULE :(
#define CMD_BUFFER_SIZE 256

#define RELAY1_PIN 12
#define RELAY2_PIN 11
#define RELAY3_PIN 10
#define RELAY4_PIN 9
#define RELAY5_PIN 8
#define RELAY6_PIN 7
#define RELAY7_PIN 6
#define RELAY8_PIN 5

#define RELAYSTATUS_PIN 13

#define TOTAL_RELAYS 8

int statusNeedToBlink = 4;

int isNum(char key)
{
  return (key>='0'&&key<='9');
}


char scan_num(char * buf, int * index, long * result)
{
  *result = 0;
  char key;
  while(*index<CMD_BUFFER_SIZE && isNum(key = buf[*index]))
  {
    (*result)*=10;
    (*result)+=key-'0';
    (*index)++;
  }
  
  if(*index>=CMD_BUFFER_SIZE)
  {
    return 0;
  }
  else
  {
    (*index)++;
  }
  return key;
}

// one activation server has multiple USB devices to manipulate different objects
char relayPins[TOTAL_RELAYS];

//These arrays are for physical control
char relayStates[TOTAL_RELAYS];
long relayLastStateStartedAt[TOTAL_RELAYS];


//These arrays are for logical arrays
char logicStates[TOTAL_RELAYS]; // logic state can be 1, but as soon as we need "blinking" feature its physical state will be changing from 0 to 1 back and forth
long logicLastStateStartedAt[TOTAL_RELAYS];
long logicTotalDelay[TOTAL_RELAYS];
long logicOnDelay[TOTAL_RELAYS]; //this is not supposed to work for status led
long logicOffDelay[TOTAL_RELAYS]; //not supposed to be used for status led


void TurnLogicRelay(char relayIndex, char isOn,long TotalTime, long OnTime, long OffTime)
{
  //Serial.print("rl:");
  //Serial.print((int)relayIndex);
  //Serial.print(";on:");
  //Serial.print((int)isOn);
  //Serial.print(";t:");
  //Serial.print(TotalTime);
  //Serial.print(";+t:");
  //Serial.print(OnTime);
  //Serial.print("-t:");
  //Serial.println(OffTime);
  
  TurnRelay(relayIndex, isOn);
  logicStates[relayIndex] = isOn;
  if(isOn)
  {
    logicTotalDelay[relayIndex] = TotalTime;
    logicOnDelay[relayIndex] = OnTime;
    logicOffDelay[relayIndex] = OffTime;
  }
  logicLastStateStartedAt[relayIndex]=millis();
}

//turn on or off relay physically, relayIndex must by from 0 to TOTAL_RELAYS-1; if TOTAL_RELAYS passed it means "status" pin
void TurnRelay(int relayIndex, char isOn)
{
  if(isOn)
  {
    Serial.print("+");
    digitalWrite(relayPins[relayIndex], HIGH);
  }
  else
  {
    Serial.print("-");
    digitalWrite(relayPins[relayIndex], LOW);
  }
  relayStates[relayIndex]=isOn;
  relayLastStateStartedAt[relayIndex]=millis();
  
  Serial.println(relayIndex+1);
  return;
}


void ParseBuf(char * buf)
{
  // line would be(no spaces at all):+1/180000/1000/300|+2/180000|-3|-4|+5/180000/100/30;
  int index = 0;
  while(index<CMD_BUFFER_SIZE)
  {
    char key = buf[index++];
    int sig = 1;
    if(key=='-')
    {
      sig = 0;
    }
    else if(key=='+')
    {
      sig = 1;
    }
    else
    {
      Serial.print(index);
      Serial.println(":err_2");
      return;
    }
    long relNum =0;

    key = scan_num(buf,&index,&relNum); //relay number
    relNum-=1;
    if(relNum <0 || relNum>=TOTAL_RELAYS)
    {
      Serial.print(index);
      Serial.println(":rl>N");
      return;
    }
    if(key == '|' || key == ';')
    {
      TurnLogicRelay((char)relNum,sig,0, 0, 0);
      if(key==';')
      {
        return;
      }
    }
    else if(key=='/')
    {
      long TotalTime = 0;
      key = scan_num(buf, &index, &TotalTime);
      if(key == '|' || key == ';')
      {
        TurnLogicRelay((char)relNum,sig,TotalTime, 0, 0);
        if(key==';')
        {
          return;
        }
      }
      else if(key=='/')
      {
        long onTime = 0;
        key = scan_num(buf, &index, &onTime);
        if(key!='/')
        {
          Serial.println("err_6");
          return;
        }

        long offTime = 0;
        key = scan_num(buf, &index, &offTime);
        TurnLogicRelay((char)relNum, sig, TotalTime, onTime, offTime);
        if(key==';')
        {
          return;
        }
        else if(key=='|')
        {
        }
        else 
        {
          Serial.println("err_7");
          return;
        }
      }
      else
      {
        Serial.println("err_5");
        return;
      }
      //additional parameters exist
    }
    else
    {
      Serial.print(index);
      Serial.println(";err_4");
      return;
    }
  }
}

int _channel = -1;
void setChannel(byte newChannel)
{
  EEPROM.write(0, newChannel);
  _channel = EEPROM.read(0);
}
void CheckCode()
{
  char temp;
  while(Serial.available())
  {
    statusNeedToBlink = 19;
    temp=Serial.read();
    if(temp==0x23) //#
    {
      Serial.write("#");
      Serial.write(MODEL_TYPE);
      Serial.println();
    }
    else if(temp=='g') 
    {
      Serial.write(_channel+48);
    }
    else if(temp=='s') 
    {
      delay(20);
      if(Serial.available())
      {
        char new_channel = Serial.read();
        if(!isNum(new_channel))
        {
          Serial.print("err, no channel set:");
          Serial.println(new_channel);
          return;
        }
        setChannel(new_channel-'0');
        Serial.write(_channel+48);
      }
      else
      {
        
      }
    }
    else if(temp == 0x24) //$
    {
      Serial.write('K');
    } 
    else if(temp=='+' || temp=='-')
    {
      char buf[CMD_BUFFER_SIZE]; //hope your arduino has enough memory to work with
      buf[0]=temp;
      char key = 0;
      int attempts = 0;
      int index = 1; //one symbol is already read
      
      while(key!=';' && attempts<20 && index<CMD_BUFFER_SIZE)
      {
        if(!Serial.available())
        {
          delay(20);
          attempts++;
        }
        //20ms is about 20 symbols. If we do not get anything in this period of time then everything looks bad
        if(!Serial.available())
        {
          Serial.println("err");
          return;
        }
        key = Serial.read();
        buf[index++]=key;
      } //end of reading loop

    
      
      if(attempts>=20||index>=sizeof(buf))
      {
        Serial.println("err_");
        return;
      }
      buf[index]=0;
      ParseBuf(buf);
      
    }
  }
}

void PrepareRelayPin(int relayNumber, int pinNumber)
{
  if(TOTAL_RELAYS>relayNumber)
  {
    pinMode(pinNumber, OUTPUT);
    digitalWrite(pinNumber, LOW);
    relayPins[relayNumber] = pinNumber;
  }
}


void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  PrepareRelayPin(0, RELAY1_PIN);
  PrepareRelayPin(1, RELAY2_PIN);
  PrepareRelayPin(2, RELAY3_PIN);
  PrepareRelayPin(3, RELAY4_PIN);
  PrepareRelayPin(4, RELAY5_PIN);
  PrepareRelayPin(5, RELAY6_PIN);
  PrepareRelayPin(6, RELAY7_PIN);
  PrepareRelayPin(7, RELAY8_PIN);
 
  for(int i=0;i<TOTAL_RELAYS;i++)
  {
    relayStates[i] = 0 ;
    relayLastStateStartedAt[i] = 0;

    logicStates[i]=0; // logic state can be 1, but as soon as we need "blinking" feature its physical state will be changing from 0 to 1 back and forth
    logicLastStateStartedAt[i]=0;
    logicTotalDelay[i]=0;
    logicOnDelay[i]=0; 
    logicOffDelay[i]=0; 
  }

  pinMode(RELAYSTATUS_PIN, OUTPUT);
  digitalWrite(RELAYSTATUS_PIN, LOW);
  _channel = EEPROM.read(0);
}

void CheckWorkingRelayStates()
{
  long curMillis = millis();
 
  for(int i=0;i<TOTAL_RELAYS;i++)
  {
    if(logicStates[i])
    {
      //that means relay[i] is turned on we need to handle it
      long turnedOffAt = logicLastStateStartedAt[i] + logicTotalDelay[i];
      
      if(curMillis>turnedOffAt)
      {
        TurnRelay(i,0);
        logicStates[i]=0;
        logicLastStateStartedAt[i]=curMillis;
      }
      else
      {
        if(logicOnDelay[i] && logicOffDelay[i])
        {
          if(relayStates[i])
          {
            long ChangedAt = logicOnDelay[i] + relayLastStateStartedAt[i];
            if(ChangedAt<=curMillis)
            {
              TurnRelay(i,0);
            }
          }
          else
          {
            //relay is turned off right now
            long ChangedAt = logicOffDelay[i] + relayLastStateStartedAt[i];
            if(ChangedAt<=curMillis)
            {
              TurnRelay(i,1);
            }
          }
        }
      }
    }
  }

}
char status_isOn = 0;
long lastStatusChanged = 0;

void BlinkStatus()
{
  if(statusNeedToBlink)
  {
    long changedAt = lastStatusChanged + 500;
    long curMillis = millis();
    if(curMillis>=changedAt)
    {
      lastStatusChanged = curMillis;
      if(status_isOn)
      {
        status_isOn = 0;
        digitalWrite(RELAYSTATUS_PIN, LOW);
      }
      else
      {
        status_isOn = 1;
        digitalWrite(RELAYSTATUS_PIN, HIGH);
      }
      statusNeedToBlink--;
      if(!statusNeedToBlink)
      {
        digitalWrite(RELAYSTATUS_PIN, LOW);
      }
    }
  }
}
void loop() 
{
  //if # returns device name, if $ returns last button
  CheckCode(); 
  delay(1);
  CheckWorkingRelayStates();
  BlinkStatus();
}
