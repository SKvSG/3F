//#include <SoftwareSerial.h>

//solve communication issue


//http://www.atmel.com/images/doc7593.pdf
#include <avr/io.h>
#include <avr/interrupt.h>
//#include <AltSoftSerial.h>
#include <stdio.h>
#include <string.h>
#include <Wire.h>
#define lcd Serial1

//SoftwareSerial slave(4,25);
//AltSoftSerial slave;
 
float calibration_factor = 970; //-7050 worked for my 440lb max scale setup
float scale_weight = 0;
int step_counter;
int led0 = 6;// Pin 6  has the LED on Teensy++ 2.0
int dir = 26;
int stp = 21;
int MS2 = 24;
int EN = 8;
int MS1 = 7;
int BTN1 = 23;
int BTN2 = 22;
char user_input;
int x;
int y;
int state;
bool button_state = 0;
bool contact;
float scale_weight_1 = 0;
float scale_weight_2 = 0;
volatile float scale_weight_50 = 0;
volatile float scale_weight_350 = 0;
char slaveString1[5] = "tare";
char slaveString2[5] = "errr";
char slaveString3[5] = "setw";

bool foundflag = false;

bool topflag = false;

const byte numChars = 32;
char receivedChars[numChars];

boolean newData = false;


void setup() 
{                
  // initialize the digital pin as an output.
  pinMode(18, INPUT_PULLUP);
  pinMode(19, INPUT_PULLUP);
  pinMode(led0, OUTPUT);     
  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT); 
  pinMode(MS2, OUTPUT);
  pinMode(EN , OUTPUT);
  pinMode(MS1 , OUTPUT);
  pinMode(BTN1, INPUT);
  pinMode(BTN2, INPUT);
  resetEDPins();//Set step, direction, microstep and enable pins to default states
  Serial.begin(9600); //Open Serial connection for debugging
  lcd.begin(9600);
  Wire.begin();
  resetLCD();
  lcd.write("Push Button to  Begin Test ->");
  digitalWrite(dir, HIGH);
  digitalWrite(EN, LOW);

  


   //slave.print("tare"); //tell weigh scale to tare
  // Serial.write( slavetomaster() );
   //delay(1000);
  attachInterrupt(18, fruitfind, FALLING);
  attachInterrupt(19, topfind, FALLING);


}

void fruitfind()
{
  foundflag = true;
}

void topfind()
{
  topflag = true;
}

void StepFast()    // Moving forward at slow step mode.
{
  digitalWrite(dir, HIGH); //Pull direction pin low to move down
  digitalWrite(MS1, LOW); //Pull MS1, and MS2 high to set logic to fullstep resolution
  digitalWrite(MS2, LOW);
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

void resetEDPins()
{
  digitalWrite(stp, LOW);
  digitalWrite(dir, LOW);
  digitalWrite(MS1, LOW);
  digitalWrite(MS2, LOW);
  digitalWrite(EN, HIGH);
}

void resetLCD()
{
  lcd.write(254); // move cursor to beginning of first line
  lcd.write(128);
  lcd.write("                "); // clear display
  lcd.write("                ");
}

//char * slavetomaster()
//{
//  static char slavestr[20];
//  int slaveDataLength;
////delay(10);
//    while ( slave.available() < 3)
//    {
//      Serial.print(slave.available());
//    }
//
//  if ( slave.available() )
//  {
//
//    Serial.print(" in queue:");   
//    Serial.print(slave.available());
//    Serial.print(" actual chars ");  
//    
//    slaveDataLength = slave.readBytesUntil('\n', slavestr, slave.available() );
//        Serial.print(slaveDataLength);  
//        Serial.print (slavestr);
// 
//    
//    if (slaveDataLength == 0)
//    {
//      Serial.print("no message ");
//    }
//    else
//    {
//      slavestr[slaveDataLength] = '\0';
//    }
//    if ( strcmp(slavestr, "") == 0)  //this means we aren't reading right
//    {
//      Serial.print("empty message ");
//    }
//
//    if ( strcmp(slavestr, "\r") == 0)  //this means we aren't reading right
//    {
//      Serial.print("cr message ");
//    }
//    return slavestr;
//  }
//  Serial.print("errorssdsdsds");
//}
//
//void errorcheck(char *p)
//{
//  Serial.print(sizeof(p));
//        Serial.write("_message:");
//        Serial.write(p);   
//    if ( strcmp( "", p ) == 0) //if the reply is not ok
//    {
//     // resetLCD();
//    //  lcd.write("com error");
////        Serial.write("blankmessage:");
////        Serial.write(p);
////        Serial.write("_blankdifference_");
////        Serial.write(strcmp( "ok", p ));
////        Serial.write("-");
//
//      
//    }
//
//    else if ( strcmp( "no", p ) == 0) //if the reply is neg
//    {
//      lcd.write(" bad command");
//      while(1);
//    }
//   else if ( strcmp( "ok", p ) ) //if the reply is not ok
//    {
//      resetLCD();
//      lcd.write("com error");
//      //lcd.write("no error");     
//    }
//
//}
//
//void setpoint (char *p)
//{
//    slave.print("setw"); //tell weigh when to interrupt
//    errorcheck( slavetomaster() );
//
//    slave.print(p); //set interrupt for 2 grams
//    errorcheck( slavetomaster() );
//  
//    foundflag = false;
//}
//
//char * getresult()
//{
//  slave.print("getw"); //tell weigh when to interrupt
//  errorcheck( slavetomaster() );
//  return slavetomaster(); //get touch pressure
//}

void loop() {
  
  int stepheight;
  int downsteps;
  char *pressure000200;
  char *pressure005000;
  char *pressure035000;
  char str[10];
  char *p;
 
      Wire.requestFrom(8, 1 ); //slave 1 request 1 byte
      Serial.print(Wire.available());
      delay(100);
    while(Wire.available()){
      Wire.write(1);
      char c = Wire.read();
      Serial.print("..");
      Serial.print(c);
    }

//  if (digitalRead(BTN1)&& (button_state == 0) )
//  {
//    button_state = 1;
//    resetLCD();
//    
//    slave.print("tare"); //tell weigh scale to tare
//
//
//   
//    //delay(200);
//    p = slavetomaster();
//    //errorcheck( p ); //read reply (doesn't really work) and error check it  
//
//    
//    lcd.print("Searching"); 
//    digitalWrite(EN, LOW);//Pull enable pin low to allow motor control
//   

  
    
//////////////////////////////////////////////////////////////////////////////////////////////////
//  //find fruit
//    setpoint("000200"); //set detect pressure to 2 grams
//    StepFast();     //go fast
//    resetLCD();
//    lcd.print("Fruit Found At: "); 
//    pressure000200 = getresult();
//    lcd.print(  pressure000200 );
//
//  // (step backwards?)
//////////////////////////////////////////////////////////////////////////////////////////////////  
//  //go to first pressure
//
//    setpoint("005000"); //set interrupt for 50 grams
//    StepSlow();       //go slow
//    resetLCD();
//    lcd.print("Initial Pressure At: "); 
//    pressure005000 = getresult();
//    lcd.print(  pressure005000 );
//  
//////////////////////////////////////////////////////////////////////////////////////////////////  
//  //got to second pressure
//  
//    setpoint("25000"); //set interrupt for 250 grams
//    downsteps = StepSlow();
//    resetLCD();
//    lcd.print("Final Pressure At: "); 
//    pressure035000 = getresult();
//    lcd.print( pressure035000 );
// //////////////////////////////////////////////////////////////////////////////////////////////// 
//   //find height and dislay results
//   
//    stepheight = ReverseSteps();
//  
//    scale_weight = (pressure005000-pressure035000)/(downsteps*0.02);
//        
//    sprintf(str, "%d.%02d", (int)scale_weight, (int(scale_weight*100-((int)scale_weight*100))));
//        
//    resetLCD();
//    lcd.write(str);
//  
//    resetEDPins();

//    delay(250);
//    button_state = 0;
//  }
//  
//  else
//  {
//    digitalWrite(led0, HIGH);   // turn the LED on (HIGH is the voltage level)
//    lcd.write(254);
//    lcd.write(128);
//    lcd.write("Push Button to  Begin Test ->   ");
//    delay(125);
//    digitalWrite(led0, LOW);    // turn the LED off by making the voltage LOW
//    lcd.write(254);
//    lcd.write(204);
//    lcd.write("->  ");
//    delay(125);
//    digitalWrite(led0, HIGH);   // turn the LED on (HIGH is the voltage level)
//    lcd.write(254);
//    lcd.write(205);
//    lcd.write("-> ");
//    delay(125);
//    digitalWrite(led0, LOW);    // turn the LED off by making the voltage LOW
//    lcd.write(254);
//    lcd.write(206);
//    lcd.write("->");
//    delay(125);
//  } 
}
  
   
// if(Serial.available())
// {
//   char temp = Serial.read();
//   if(temp == '+' || temp == 'a')
//     calibration_factor += 1;
//   else if(temp == '-' || temp == 'z')
//     calibration_factor -= 1;
//   scale.set_scale(calibration_factor);
// }
//  
  // Call set_scale() with no parameter.
// Call tare() with no parameter.

// Place a known weight on the scale and call get_units(10).
// Divide the result in step 3 to your known weight. You should get about the parameter you need to pass to set_scale.
// Adjust the parameter in step 4 until you get an accurate reading.


