#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <SoftwareSerial.h>

#define slave Serial1  //pin 0, 1
SoftwareSerial mySerial(6, 7); // RX, TX (transmit only)
 /*Not all pins on the Leonardo and Micro support change interrupts,
 so only the following can be used for RX:
 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI).*/

int SLAVERX = 0;
int SLAVETX = 1;

int EN = 4;
int MS1 = 5;

int dir = 10;

int MS3 = 18;
int stp = 19;
int BTN2 = 21;
int BTN1 = 20;
int MS2 = 16;

//pins that need to be free 4 CS sd,9 reset,10 CS wiz,11 dout,12 din,13 sck
//use pin 8 for serial 3 lcs use, move ms1 to pin 6 

volatile bool foundflag = false;
volatile bool topflag = false;
bool button_state = 0;
int pressure000200 = 0;
int pressure005000 = 0;
int pressure025000 = 0;

void setup() {
//  pinMode(led0, OUTPUT);     

  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT); 
  pinMode(MS2, OUTPUT);
  pinMode(EN , OUTPUT);
  pinMode(MS1 , OUTPUT);
  pinMode(MS3 , OUTPUT);
  pinMode(BTN1, INPUT);
  pinMode(BTN2, INPUT);
  attachInterrupt(digitalPinToInterrupt(3), fruitfind, RISING);
//  attachInterrupt(digitalPinToInterrupt(17), topfind, RISING);
  
  Serial.begin(9600);
  slave.begin(9600);
  mySerial.begin(9600);
  digitalWrite(dir, HIGH);
  digitalWrite(EN, LOW);
  delay(1000);
  Serial.write("Push Button to  Begin Test ->");
}

void fruitfind()
{
  foundflag = true;
//  Serial.print("fruit found");
}

void topfind()
{
  topflag = true;
}


void StepFast()    // Moving forward at slow step mode.
{
  digitalWrite(dir, HIGH); //Pull direction pin low to move down
  digitalWrite(MS1, HIGH); //Pull MS1, and MS2 high to set logic to fullstep resolution
  digitalWrite(MS2, LOW);
  digitalWrite(MS3, LOW);
  while(!foundflag)  //add max steps
  {
    digitalWrite(stp,HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
}

int StepSlow()    // Moving forward at slow step mode.
{
  int count = 0;
  digitalWrite(dir, HIGH); //Pull direction pin low to move down
  digitalWrite(MS1, HIGH); //Pull MS1, and MS2 high to set logic to 1/8th microstep resolution
  digitalWrite(MS2, HIGH);
  digitalWrite(MS3, HIGH);
  while(!foundflag)  //add timeout
  {
    digitalWrite(stp,HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delay(1);
    count++;
  }
  return count;
}

int ReverseSteps()
{
  int count = 0;
  digitalWrite(dir, LOW); //Pull direction pin high to move in "reverse"
  digitalWrite(MS1, LOW); //Pull MS1, and MS2 high to set logic to fullstep resolution
  digitalWrite(MS2, LOW);
  digitalWrite(MS3, LOW);
  while(!topflag)  //
  {
    digitalWrite(stp,HIGH); //Trigger one step
    delay(1);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delay(1);
    count++;
  }
  return count;
}

void BackJump()
{
  int i = 0;
  digitalWrite(dir, LOW); //Pull direction pin high to move in "reverse"
  digitalWrite(MS1, LOW); //Pull MS1, and MS2 high to set logic to fullstep resolution
  digitalWrite(MS2, LOW);
  digitalWrite(MS3, LOW);
  while(i < 100 )  //Loop the stepping enough times for motion to be visible
  {
    digitalWrite(stp,HIGH); //Trigger one step
    delay(1);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delay(1);
    i++;
  }
}


void resetEDPins()
{
  digitalWrite(stp, LOW);
  digitalWrite(dir, LOW);
  digitalWrite(MS1, LOW);
  digitalWrite(MS2, LOW);
  digitalWrite(MS3, LOW);
  digitalWrite(EN, HIGH);
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

char * slavetomaster()
{
  static char slavestr[5];  //must be one more than message length
  int slaveDataLength = 0;
  StrClear(slavestr, 5);
  while(slave.available())
  {
    char slaveData = slave.read();
    //Serial.println(slaveData, HEX);
    if( slaveData == '\0' )
    {
      delay(1);
      slaveData = slave.read();
      //Serial.println(slaveData, HEX);
    }
    if( slaveData == '\n' )
    {
      break;
    }
    slavestr[slaveDataLength] = slaveData;
    slaveDataLength++;
  }
  return slavestr;
}

void errorcheck(char *p)
{
    if ( strcmp( "wset", p ) == 0) //if the reply is tare
    {
      Serial.print("->weight set at:");
    }

    else if ( strcmp( "tard", p ) == 0) 
    {
      Serial.println("->tared");
    }
    else if ( strcmp( "ok__", p ) == 0) 
    {
      Serial.println("->confirmed");
    }
    else if ( strcmp( "9999", p ) >= 0 &&  strcmp( p, "0000" ) >= 0 ) 
    {
      Serial.println(p);
    }
    else
    {
      Serial.print("error recieved:");
      Serial.println(p);
      //errorcheck ( slavetomaster() );
    }
    Serial.flush();
    return p;
}

void setpoint (char *p)
{
    slave.write("setw"); //tell weigh when to interrupt
    slave.write('\n');
    slave.flush();
    
    Serial.print("setw sent ");
    Serial.flush();
    
    //delay(1000);
   
    while( !slave.available() )
    {
      delay(1);
    }
    
    if( slave.available() )
    {
      delay(10);
      errorcheck ( slavetomaster() );
    }
    
    Serial.println(p);
    slave.write(p);
    slave.write('\n');
    slave.flush();
    
    while( !slave.available() )
    {
      delay(1);
    }
    
    if( slave.available() )
    {
      delay(10);
      errorcheck ( slavetomaster() );
    }
  
    foundflag = false;
}

char * getresult()
{
  char *p;
  int n;
  slave.write("getw"); //tell weigh when to interrupt
  slave.write('\n');
  slave.flush();
  //delay(1000);

    while( !slave.available() )
    {
      delay(1);
    }
    if( slave.available() )
    {
      delay(10);
      //errorcheck ( slavetomaster() );
    }
  
  p = slavetomaster();
   errorcheck( p );
   n = toString(p);
   return (n);
 // return slavetomaster(); //get touch pressure
}

int toString(char a[]) {
  int c, n;
  
  n = 0;
 
  for (c = 0; a[c] != '\0'; c++) {
    n = n * 10 + a[c] - '0';
  } 
  return n;
}


void loop() {

//improved getresult

  int downsteps = 0;
  float scale_weight = 0;
  char str[10];

  if (digitalRead(BTN1)&& (button_state == 0) )
  {
    button_state = 1;
    
    slave.write("tare"); //tell weigh scale to tare
    slave.write('\n');
    slave.flush();
    Serial.print("tare sent");
        
    while( !slave.available() )  //wait for response
    {
      delay(1);
    }
    
    if( slave.available() )
    {
      delay(10);
      errorcheck ( slavetomaster() );
    }
    
    digitalWrite(EN, LOW);//Pull enable pin low to allow motor control
    
////////////////////////////////////////////////////////////////////////////////////////////////
 //find fruit
   setpoint("0002"); //set detect pressure to 2 grams
   StepFast();     //go fast
   pressure000200 = getresult();
////////////////////////////////////////////////////////////////////////////////////////////////  
 //go to first pressure
   BackJump();
   setpoint("0020"); //set interrupt for 50 grams
   StepSlow();       //go slow
   pressure005000 = getresult();
////////////////////////////////////////////////////////////////////////////////////////////////  
 //got to second pressure
   setpoint("0230"); //set interrupt for 250 grams
   downsteps = StepSlow();
   pressure025000 = getresult();
//////////////////////////////////////////////////////////////////////////////////////////////// 
  //find height and dislay results
    BackJump();
    BackJump();
    BackJump();
   
   //stepheight = ReverseSteps();
   Serial.print("pressure zero ");
   Serial.print(pressure000200);
   Serial.print("pressure one ");
   Serial.print(pressure025000);
   Serial.print("minus pressure two ");
   Serial.print(pressure005000);
   Serial.print("divided by downsteps ");
   Serial.println(downsteps );
   
   scale_weight = (pressure025000-pressure005000)/(downsteps*0.01);
   button_state = 0;
   Serial.println(scale_weight);
   mySerial.println(scale_weight);
   mySerial.flush();
  
   resetEDPins(); 

 }
 
 else
 {
     // do nothing
 } 
}
