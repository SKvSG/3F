//#include <SoftSerial.h>
#include <string.h>
#include <stdio.h>
#include "HX711.h"
 
#define DOUT  2
#define CLK  3
#define W_INT 4

//SoftSerial master (1,0);  //rx, tx
HX711 scale(DOUT, CLK);
int i = 0;
byte byteRead;
float calibration_factor = 2185; //2003 for new load cell
float scale_weight = 0;
bool masterflag = 0;
bool weightflag = 0;
float master_weight = 20;

void setup() {
  pinMode(CLK, OUTPUT);
  pinMode(DOUT, INPUT);
  pinMode(W_INT, OUTPUT);
  Serial1.begin(9600);
  Serial.begin(9600);
  scale.set_scale();
  scale.tare(); //Reset the scale to 0 */
  long zero_factor = scale.read_average(); //Get a baseline reading
  scale.set_scale(calibration_factor);
}

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
      masterflag = 1;
      weightflag = 0;
      Serial1.write("ok__");
      Serial1.write('\n');
      Serial1.flush();

      //convert scale weight to ascii equivalent
      //20.00(float), to 0,0,2,0,(ascii)

      //scale_weight = master_weight;
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
    else if ( strcmp( "cal+", p ) == 0) 
    {
      calibration_factor++;
      Serial1.write("ok__");
      Serial1.write('\n');
      Serial1.flush();
      scale.set_scale(calibration_factor);
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

void loop() {
  // put your main code here, to run repeatedly:
  if( Serial1.available() && (masterflag == 0) )
  {
    errorcheck( mastertoslave() );
  }
  else
  {
    scale_weight = scale.get_units();
    Serial.println(scale_weight);
  }
  if( scale_weight >= master_weight && (masterflag == 1))
  {
    masterflag = 0;
    digitalWrite(W_INT, 1);
//    Serial1.write("found");
//    Serial1.write('\n');
//    Serial1.flush();
  }
  else
  {
    digitalWrite(W_INT, 0);
  }
}
