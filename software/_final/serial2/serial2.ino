/*/////////////////////////////////////////////////////////////////////////////////////////
  MICROSTEP PROGRAM - SKVSG - 0.0.0
/////////////////////////////////////////////////////////////////////////////////////////*/

/*/////////////////////////////////////////////////////////////////////////////////////////
  LIBRARIES
/////////////////////////////////////////////////////////////////////////////////////////*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <SoftwareSerial.h>

/*/////////////////////////////////////////////////////////////////////////////////////////
  MACROS
/////////////////////////////////////////////////////////////////////////////////////////*/
#define slave Serial1  //pin 0, 1

/*/////////////////////////////////////////////////////////////////////////////////////////
  PIN ASSIGNMENTS
/////////////////////////////////////////////////////////////////////////////////////////*/
 /*Not all pins on the Leonardo and Micro support change interrupts,
 so only the following can be used for RX:
 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI).*/
const int SLAVERX = 0;
const int SLAVETX = 1;
const int EN = 4;
const int MS1 = 5;
const int dir = 10;
const int MS3 = 18;
const int stp = 19;
const int BTN2 = 21;
const int BTN1 = 20;
const int MS2 = 16;

/*/////////////////////////////////////////////////////////////////////////////////////////
  DEVICES
/////////////////////////////////////////////////////////////////////////////////////////*/
SoftwareSerial mySerial(8, 7); // RX, TX (transmit only)

/*/////////////////////////////////////////////////////////////////////////////////////////
  GLOBAL VARIABLES
/////////////////////////////////////////////////////////////////////////////////////////*/
volatile bool foundflag = false;
volatile bool topflag = false;
bool button_state = 0;
int pressure000200 = 0;
int pressure005000 = 0;
int pressure025000 = 0;
volatile bool msgflag = false;
int setpressure = 0;
bool calibrationflag = 0;
bool weightflag = 0;
bool restflag = 0;
int speedflag = 0;
int calibrationfactor = 2185;
char * weighttarget[3];
int speedtarget[3];
bool calibrationsetflag = 0;
bool weightsetflag = 0;
bool speedsetflag = 0;
bool restsetflag = 0;
int restlength = 0;

/*/////////////////////////////////////////////////////////////////////////////////////////
  FUNCTION DECLARATIONS
/////////////////////////////////////////////////////////////////////////////////////////*/
void initializePins();
void fruitfind();
void topfind();
float StepToWeight(int multiplier);
float StepToDistanace(int speed);
int StepSuperFast();
void StepFast();
int StepSlow();
int StepSuperSlow();
int StepUltraSlow();
int ReverseSteps();
void BackJump();
void BackJump(int steps);
void resetEDPins();
void StrClear(char *str, char length);
char * slavetomaster();
void errorcheck(char *p);
void setpoint (char *p);
char * getresult();
int toString(char a[]);
void testfirmness();
void calib();
void bridgeerrorcheck(char *p);
char * bridgetomaster();
/*/////////////////////////////////////////////////////////////////////////////////////////
  MAIN
/////////////////////////////////////////////////////////////////////////////////////////*/
void setup() {
  initializePins();
  initializeSerial();   
  delay(1000);
  Serial.write("Device Ready");
}

void loop() {
 
 if (digitalRead(BTN1))
 {
    //read the incoming data
    delay(1);
    mySerial.write("0050");        ///unique ID and weight?  //calibration factor?
    mySerial.flush();
    delay(1);
            
    while( !mySerial.available() )  //wait for response
    {
      delay(1);
    }
    
    if( mySerial.available() )
    {
      delay(10);
      bridgeerrorcheck ( bridgetomaster() );  //interpret command
    }
 } 
}

/*/////////////////////////////////////////////////////////////////////////////////////////
  FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////*/

/*/////////////////////////////////////////////////////////////////////////////////////////
  INITIALIZATION FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////*/
void initializePins()
{
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
  //attachInterrupt(digitalPinToInterrupt(9), msgIncoming, RISING);
  digitalWrite(dir, LOW);
  digitalWrite(EN, LOW);
}

void initializeSerial()
{
  Serial.begin(9600);
  slave.begin(9600);
  mySerial.begin(9600);
}

/*/////////////////////////////////////////////////////////////////////////////////////////
  INTERRUPT FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////*/
void fruitfind()
{
  foundflag = true;
//  Serial.print("fruit found");
}

void topfind()
{
  topflag = true;
}

void msgIncoming()
{
  msgflag = true;
}
/*/////////////////////////////////////////////////////////////////////////////////////////
  STEPPER FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////*/
float StepToWeight(int multiplier)
{
  digitalWrite(MS1, multiplier/8);
  digitalWrite(MS2, multiplier/4);
  digitalWrite(MS3, multiplier/2);
  //000-full step(0), 001-halfstep(1), 010-quarterstep(2), 011-eightstep(3), 100-UNSUPPORTED(4), 101-UNSUPPORTED(5), 110-UNSUPPORTED(6), 111-SIXTEENTH(7)
}

float StepToDistanace(int speed)
{
  
}

int StepSuperFast()    // Moving forward at slow step mode.
{
  int count = 0;
  digitalWrite(dir, HIGH); //Pull direction pin low to move down
  digitalWrite(MS1, LOW); //Pull MS1, and MS2 high to set logic to fullstep resolution
  digitalWrite(MS2, LOW);
  digitalWrite(MS3, LOW);
  while(!foundflag)  //add max steps
  {
    digitalWrite(stp,HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delay(1);
    count++;
  }
  return count;
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
  return count/8;                                   //TODO: make sure that I am returning a float
}

int StepSuperSlow()    // Moving forward at slow step mode.
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
    delay(10);
    count++;
  }
  return count/8;                                   //TODO: make sure that I am returning a float
}

int StepUltraSlow()    // Moving forward at slow step mode.
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
    delay(50);
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

void BackJump(int steps)
{
  int i = 0;
  digitalWrite(dir, LOW); //Pull direction pin high to move in "reverse"
  digitalWrite(MS1, LOW); //Pull MS1, and MS2 high to set logic to fullstep resolution
  digitalWrite(MS2, LOW);
  digitalWrite(MS3, LOW);
  while(i < steps )  //Loop the stepping enough times for motion to be visible
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

/*/////////////////////////////////////////////////////////////////////////////////////////
  SERIAL FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////*/
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

char * bridgetomaster()
{
  static char bridgestr[5];  //must be one more than message length
  int bridgeDataLength = 0;
  StrClear(bridgestr, 5);
//  mySerial.println(scale_weight);
 // mySerial.flush();
  while(mySerial.available())
  {
    char bridgeData = mySerial.read();
    if( bridgeData == '\0' )
    {
      delay(1);
      bridgeData = mySerial.read();
      //Serial.println(slaveData, HEX);
    }
    if( bridgeData == '\n' )
    {
      break;
    }
    bridgestr[bridgeDataLength] = bridgeData;
    bridgeDataLength++;
  }
  return bridgestr;
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

void bridgeerrorcheck(char *p)
{
    char number_array[4];

    if ( calibrationflag == 1 )
    {
      //getnumber and turn it into weight
      //convert sent number to decimal
      //convert measure to ascii
      //convert goal weight to decimal equivalent (not robust)
      //1,1,1,1(ascii) to 1111 (decimal)

      calibrationfactor = (p[3]-48)+(p[2]-48)*10+(p[1]-48)*100+(p[0]-48)*1000;
      calibrationsetflag = 1;           //flag to set calibration point
      calibrationflag = 0;
      mySerial.write("ok__");
      mySerial.write('\n');
      mySerial.flush();
    }
    else if ( restflag == 1 )
    {
      //getnumber and turn it into weight
      //convert sent number to decimal
      //convert measure to ascii
      //convert goal weight to decimal equivalent (not robust)
      //1,1,1,1(ascii) to 1111 (decimal)

      restlength = (p[3]-48)+(p[2]-48)*10+(p[1]-48)*100+(p[0]-48)*1000;
      restsetflag = 1;           //flag to set calibration point
      restflag = 0;
      mySerial.write("ok__");
      mySerial.write('\n');
      mySerial.flush();
    }    
    else if ( weightflag > 0 )
    {
      //getnumber and turn it into weight
      //convert sent number to decimal
      //convert measure to ascii
      //convert goal weight to decimal equivalent (not robust)
      //1,1,1,1(ascii) to 1111 (decimal)

      weighttarget[weightflag] = (p[0])+(p[1])*10+(p[2])*100+(p[3])*1000;
      //weighttarget[weightflag] = p
      weightsetflag = 1;           //flag to set weight point
      Serial.print("weight set for");
      //setpoint(p);
      //Serial.println(int(weighttarget[weightflag]));
      Serial.print(p[3], HEX);
      Serial.print(p[2], HEX);
      Serial.print(p[1], HEX);
      Serial.print(p[0], HEX);      
      weightflag = 0;
      mySerial.write("ok__");
      mySerial.write('\n');
      mySerial.flush();
    }
    else if ( speedflag < 0 )
    {
      //getnumber and turn it into weight
      //convert sent number to decimal
      //convert measure to ascii
      //convert goal weight to decimal equivalent (not robust)
      //1,1,1,1(ascii) to 1111 (decimal)

      speedtarget[speedflag] = (p[3]-48)+(p[2]-48)*10+(p[1]-48)*100+(p[0]-48)*1000;
      speedsetflag = 1;           //flag to set weight point
      speedflag = 0;
      mySerial.write("ok__");
      mySerial.write('\n');
      mySerial.flush();
    }  
    else if ( strcmp( "test", p ) == 0) //if the reply is tare
    {
      if (button_state == 0 )
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
        
        testfirmness();
        //calib();
      
       resetEDPins(); 
    
     }
    }
    else if ( strcmp( "cali", p ) == 0) //this one starts the test
    {
      Serial.print("begin calibration");
      calib();
    }
    else if ( strcmp( "setc", p ) == 0) 
    {
      Serial.print("setc");
      calibrationflag = 1;
    }
    else if ( strcmp( "stw1", p ) == 0) 
    {
      Serial.print("set weight 1");
      mySerial.write("set1");
      mySerial.write('\n');
      mySerial.flush();
      weightflag = 1;
    }
    else if ( strcmp( "stw2", p ) == 0)
    {
      Serial.print("set weight 2");
      weightflag = 2;
    } 
    else if ( strcmp( "stw3", p ) == 0)
    {
      Serial.print("set weight 3");
      weightflag = 3;
    }
    else if ( strcmp( "sts1", p ) == 0)
    {
      Serial.print("set speed 1");
      speedflag = 1;
    } 
    else if ( strcmp( "sts2", p ) == 0) 
    {
      Serial.print("set speed 2");
      speedflag = 2;
    } 
    else if ( strcmp( "sts3", p ) == 0)
    {
      Serial.print("set speed 3");
      speedflag = 3;
    }
    else if ( strcmp( "rest", p ) == 0)
    {
      Serial.print("set rest");
      restflag = 1;
    }      
    else if ( strcmp( "ackn", p ) == 0) //do nothing reply
    {
      Serial.print("akn");
    }    
    else if ( strcmp( "9999", p ) >= 0 &&  strcmp( p, "0000" ) >= 0 ) 
    {
      Serial.println(p);
    }
    else
    {
      Serial.print("error recieved:");
      Serial.println(p);
    }
    Serial.flush();
    //return p;
}

void setcalibraion (char *p)
{
    slave.write("cali"); //tell weigh when to interrupt
    slave.write('\n');
    slave.flush();
    
    Serial.print("cali sent ");
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

int toString(char a[]) 
{
  int c, n;
  
  n = 0;
 
  for (c = 0; a[c] != '\0'; c++) {
    n = n * 10 + a[c] - '0';
  } 
  return n;
}

/*/////////////////////////////////////////////////////////////////////////////////////////
  FIRMNESS FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////*/
void testfirmness()
{
  float scale_weight = 0;
  int downsteps = 0;
  digitalWrite(EN, LOW);//Pull enable pin low to allow motor control
    
////////////////////////////////////////////////////////////////////////////////////////////////
 //find fruit
   setpoint("0007"); //set detect pressure to 2 grams
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
   
   scale_weight = (pressure025000-pressure005000)/(downsteps*0.00125);
   button_state = 0;                  //return control and accept communications
   Serial.println(scale_weight);
   mySerial.println(scale_weight);
   mySerial.flush();
}

void testfirmnessfast()
{
  float scale_weight = 0;
  int downsteps = 0;
  digitalWrite(EN, LOW);//Pull enable pin low to allow motor control
    
////////////////////////////////////////////////////////////////////////////////////////////////
 //find fruit
   setpoint("0002"); //set detect pressure to 2 grams
   StepSuperFast();     //go fast
   pressure000200 = getresult();
 //got to second pressure
   setpoint("0230"); //set interrupt for 250 grams
   downsteps = StepSlow();
   pressure025000 = getresult();
//////////////////////////////////////////////////////////////////////////////////////////////// 
  //find height and dislay results
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
   
   scale_weight = (pressure025000-pressure005000)/(downsteps*0.000625);
   button_state = 0;                  //return control and accept communications
   Serial.println(scale_weight);
   mySerial.println(scale_weight);
   mySerial.flush();
}

void calib()
{
   BackJump();
   setpoint("0020"); //set interrupt for 50 grams
   StepSuperSlow();
   pressure000200 = getresult();
   Serial.print(pressure000200);
   delay(1000);
   setpoint("0100"); //set interrupt for 50 grams
   StepUltraSlow();
   pressure000200 = getresult();
   Serial.print(pressure000200);
   delay(10000);
   BackJump();
   mySerial.println(pressure000200);
   mySerial.flush();
   button_state = 0;
}

