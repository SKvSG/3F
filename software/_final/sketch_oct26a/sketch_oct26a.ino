/*/////////////////////////////////////////////////////////////////////////////////////////
  MICROADC PROGRAM - SKVSG - 0.0.0
/////////////////////////////////////////////////////////////////////////////////////////*/

/*/////////////////////////////////////////////////////////////////////////////////////////
  LIBRARIES
/////////////////////////////////////////////////////////////////////////////////////////*/
#include <string.h>
#include <stdio.h>
#include "HX711.h"

/*/////////////////////////////////////////////////////////////////////////////////////////
  PIN ASSIGNMENTS
/////////////////////////////////////////////////////////////////////////////////////////*/
const int DOUT = 2;
const int CLK = 3;
const int W_INT = 4;

/*/////////////////////////////////////////////////////////////////////////////////////////
  DEVICES
/////////////////////////////////////////////////////////////////////////////////////////*/
HX711 scale(DOUT, CLK);

/*/////////////////////////////////////////////////////////////////////////////////////////
  GLOBAL VARIABLES
/////////////////////////////////////////////////////////////////////////////////////////*/
int i = 0;
byte byteRead;
float calibration_factor = 2185; //2003 for new load cell
float scale_weight = 0;
bool masterflag = 0;
bool weightflag = 0;
bool calibrationflag = 0;
float master_weight = 20;

/*/////////////////////////////////////////////////////////////////////////////////////////
  FUNCTION DECLARATIONS
/////////////////////////////////////////////////////////////////////////////////////////*/
char *  mastertoslave();
void errorcheck(char *p);

/*/////////////////////////////////////////////////////////////////////////////////////////
  MAIN
/////////////////////////////////////////////////////////////////////////////////////////*/
void setup() {
  initializePins();
  initializeSerial();
  initializeScale();
}

void loop() {

  if( Serial1.available() && (masterflag == 0) )        //look for communications
  {
    errorcheck( mastertoslave() );
  }
  else                                                  //update weight reading
  {
    scale_weight = scale.get_units();
    Serial.println(scale_weight);
  }
  
  if( scale_weight >= master_weight && (masterflag == 1))   //Set output at weight
  {
    masterflag = 0;
    digitalWrite(W_INT, 1);
  }
  else
  {
    digitalWrite(W_INT, 0);
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
  pinMode(CLK, OUTPUT);
  pinMode(DOUT, INPUT);
  pinMode(W_INT, OUTPUT);
}

void initializeSerial()
{
  Serial1.begin(9600);
  Serial.begin(9600);
}

void initializeScale()
{
  scale.set_scale();
  scale.tare(); //Reset the scale to 0 */
  long zero_factor = scale.read_average(); //Get a baseline reading
  scale.set_scale(calibration_factor);
}

/*/////////////////////////////////////////////////////////////////////////////////////////
  SERIAL FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////*/
char *  mastertoslave()
{
  static char masterstr[5];  //must be one more than message length
  int masterDataLength = 0;
  
//  while(Serial1.available())
  while(1)
  {
    char masterData = Serial1.read();
//    Serial.println(' ');
//    Serial.print("->");
//    Serial.print(masterData, HEX);
//    Serial.print("<-");
    if( masterData == '\n' )
    {
//      Serial.print("break");
      break;
    }
    if( masterData == 0xFFFFFFFF )
    {
//      Serial.print("ignored");
    }
    else
    {
      masterstr[masterDataLength] = masterData;
      masterDataLength++;
    }
  }
//  Serial.print("this message");
//  Serial.print(masterstr);
  return masterstr;
}

void errorcheck(char *p)
{
  char number_array[4];
  Serial.write(p);

    if ( weightflag == 1 )
    {
      //getnumber and turn it into weight
      //convert sent number to decimal
      //convert measure to ascii
      //convert goal weight to decimal equivalent (not robust)
      //1,1,1,1(ascii) to 1111 (decimal)

      master_weight = (p[3]-48)+(p[2]-48)*10+(p[1]-48)*100+(p[0]-48)*1000;
      Serial.print(p[3], HEX);
      Serial.print(p[2], HEX);
      Serial.print(p[1], HEX);
      Serial.print(p[0], HEX);                  
      masterflag = 1;
      weightflag = 0;
      Serial1.write("ok__");
      Serial1.write('\n');
      Serial1.flush();

      //convert scale weight to ascii equivalent
      //20.00(float), to 0,0,2,0,(ascii)

      //scale_weight = master_weight;
    }
    else if (calibrationflag == 1)
    {
      calibration_factor = (p[3]-48)+(p[2]-48)*10+(p[1]-48)*100+(p[0]-48)*1000;
      Serial1.write("ok__");
      Serial1.write('\n');
      Serial1.flush();
      scale.set_scale(calibration_factor);
      calibrationflag = 0;
    }

     else if (strcmp( "getw", p ) == 0 )
     {
      number_array[3] = ( int(scale_weight) / 1000 ) + 48;
      number_array[2] = ( int(scale_weight) / 100 % 10 ) + 48;
      number_array[1] = ( int(scale_weight) / 10 % 10 ) + 48;
      number_array[0] = ( int(scale_weight) % 10 ) + 48;
   

      
      //delay(10);

      Serial1.write(number_array[3]);
      Serial1.write(number_array[2]);
      Serial1.write(number_array[1]);
      Serial1.write(number_array[0]);
      Serial1.write('\n');
      Serial1.flush();
   //   Serial.write(p);
    }
    
    else if ( strcmp( "tare", p ) == 0) //if the reply is tare
    {
      scale.tare();
      digitalWrite(W_INT, 0);
      Serial1.write("tard");
      Serial1.write('\n');
      Serial1.flush();
    }
    else if ( strcmp( "setw", p ) == 0)
    {
      weightflag = 1;
      Serial1.write("wset");
      Serial1.write('\n');
      Serial1.flush();
    }
    else if ( strcmp( "cali", p ) == 0) 
    {
      calibrationflag = 1;
      Serial1.write("ok__");
      Serial1.write('\n');
      Serial1.flush();
    }
    else if ( strcmp( "cal-", p ) == 0) 
    {
      calibration_factor--;
      Serial1.write("ok__");
      Serial1.write('\n');
      Serial1.flush();
      scale.set_scale(calibration_factor);
    }
    else
    {
      //Serial1.write(p);
      Serial1.write("no__");
      Serial1.write('\n');
      Serial1.flush();
    }
}

