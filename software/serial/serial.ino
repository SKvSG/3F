#include <avr/io.h>
#include <avr/interrupt.h>
//#include <SPI.h>

#define slave Serial1  //pin 0, 1
#define lcd Serial3  //pin 8

int SLAVERX = 0;
int SLAVETX = 1;
//int ?? = 2;
//int ?? = 3;
//int SDCS = 4;
int EN = 5;
int MS1 = 6;
//int LCDRX = 7;
//int LCDTX = 8;
//int WRESET = 9;
//int WCS = 10;
//int DOUT = 11;
//int DIN = 12;
//int DSCK = 13;
//int led0 = 13;// Pin 6  has the LED on Teensy++ 2.0
//int ?? = 14;
int dir = 15;
//int ?? = 16;
//int ?? = 17;
//int ?? = 18;
int MS3 = 19;
int stp = 20;
int BTN2 = 21;
int BTN1 = 22;
int MS2 = 23;

//pins that need to be free 4 CS sd,9 reset,10 CS wiz,11 dout,12 din,13 sck
//use pin 8 for serial 3 lcs use, move ms1 to pin 6 

volatile bool foundflag = false;
volatile bool topflag = false;
bool button_state = 0;


void setup() {
//  pinMode(led0, OUTPUT);     

//reset sd and wiz
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);    // begin reset the WIZ820io
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);  // de-select WIZ820io
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);   // de-select the SD Card
  digitalWrite(9, HIGH);   // end reset pulse

  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT); 
  pinMode(MS2, OUTPUT);
  pinMode(EN , OUTPUT);
  pinMode(MS1 , OUTPUT);
  pinMode(MS3 , OUTPUT);
  pinMode(BTN1, INPUT);
  pinMode(BTN2, INPUT);
  attachInterrupt(digitalPinToInterrupt(18), fruitfind, RISING);
  attachInterrupt(digitalPinToInterrupt(17), topfind, RISING);
  
  Serial.begin(9600);
  slave.begin(9600);
  lcd.begin(9600);
  resetLCD();
  digitalWrite(dir, HIGH);
  digitalWrite(EN, LOW);
    delay(5000); //this is here just so I can see the sd card show up
  delay(5000);
  lcd.write("Push Button to  Begin Test ->");

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
  while(!foundflag)  
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
  while(!foundflag)  
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
  while(!topflag)  //Loop the stepping enough times for motion to be visible
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

void resetLCD()
{
  lcd.write(254); // move cursor to beginning of first line
  lcd.write(128);
  lcd.write("                "); // clear display
  lcd.write("                ");
}

char * slavetomaster()
{
  static char slavestr[5];  //must be one more than message length
  int slaveDataLength = 0;
  while(slave.available())
  {
    slavestr[slaveDataLength] = slave.read();
    slaveDataLength++;
  }
  return slavestr;
}

void errorcheck(char *p)
{
    if ( strcmp( "wset", p ) == 0) //if the reply is tare
    {
      Serial.print("weight set at:");
    }

    else if ( strcmp( "tard", p ) == 0) //if the reply is tare
    {
      Serial.println("tared");
    }
    else if ( strcmp( "ok__", p ) == 0) //if the reply is tare
    {
      Serial.println("weightset");
    }
    else
    {
      Serial.print("error recieved:");
      Serial.println(p);
    }

}

void setpoint (char *p)
{
    slave.print("setw"); //tell weigh when to interrupt
    Serial.print("setw sent ");
    
    delay(1000);
    errorcheck( slavetomaster() );


    slave.print(p);

    Serial.print(p);
    delay(1000);
    errorcheck( slavetomaster() );
  
    foundflag = false;
}

char * getresult()
{
  slave.print("getw"); //tell weigh when to interrupt
  delay(1000);
  errorcheck( slavetomaster() );
  return slavetomaster(); //get touch pressure
}


void loop() {

//improved getresult

  int downsteps = 0;
  int scale_weight = 0;
  char str[10];


  if (digitalRead(BTN1)&& (button_state == 0) )
  {
    button_state = 1;
    resetLCD();
    
    slave.print("tare"); //tell weigh scale to tare
    Serial.println("tare sent");
    delay(1000);
    if( slave.available() )
    {
      errorcheck ( slavetomaster() );
    }   
  
    lcd.print("Searching"); 
    digitalWrite(EN, LOW);//Pull enable pin low to allow motor control
    
////////////////////////////////////////////////////////////////////////////////////////////////
 //find fruit
   setpoint("0002"); //set detect pressure to 2 grams
   StepFast();     //go fast
   resetLCD();
   lcd.print("Fruit Found At: "); 
   //pressure000200 = getresult();
   lcd.print(  getresult() );
 
 // (step backwards?)
////////////////////////////////////////////////////////////////////////////////////////////////  
 //go to first pressure
   BackJump();
   setpoint("0020"); //set interrupt for 50 grams
   StepSlow();       //go slow
   resetLCD();
   lcd.print("Initial Pressure At: "); 
  // pressure005000 = getresult();
   lcd.print(  getresult() );

////////////////////////////////////////////////////////////////////////////////////////////////  
 //got to second pressure
 
   setpoint("0230"); //set interrupt for 250 grams
   downsteps = StepSlow();
   resetLCD();
   lcd.print("Final Pressure At: "); 
 //  pressure035000 = getresult();
   lcd.print( getresult() );
//////////////////////////////////////////////////////////////////////////////////////////////// 
  //find height and dislay results
     BackJump();
        BackJump();
           BackJump();
   
   //stepheight = ReverseSteps();
 
   scale_weight = (250-50)/(downsteps*0.01);
       
   sprintf(str, "%d.%02d", (int)scale_weight, (int(scale_weight*100-((int)scale_weight*100))));
       
   resetLCD();
   lcd.write(str);
 
   resetEDPins(); 

   delay(5000);
   button_state = 0;
 }
 
 else
 {
  // digitalWrite(led0, HIGH);   // turn the LED on (HIGH is the voltage level)
   lcd.write(254);
   lcd.write(128);
   lcd.write("Push Button to  Begin Test ->   ");
   delay(125);
  // digitalWrite(led0, LOW);    // turn the LED off by making the voltage LOW
   lcd.write(254);
   lcd.write(204);
   lcd.write("->  ");
   delay(125);
  // digitalWrite(led0, HIGH);   // turn the LED on (HIGH is the voltage level)
   lcd.write(254);
   lcd.write(205);
   lcd.write("-> ");
   delay(125);
  // digitalWrite(led0, LOW);    // turn the LED off by making the voltage LOW
   lcd.write(254);
   lcd.write(206);
   lcd.write("->");
   delay(125);
 } 
}
