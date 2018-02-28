#include <SoftSerial.h>
#include <string.h>
#include <stdio.h>
#include "HX711.h"

#define DOUT  2
#define CLK  3
#define W_INT 4

SoftSerial master (1,0);  //rx, tx
HX711 scale(DOUT, CLK);
int i = 0;
byte byteRead;
float calibration_factor = 970; //-7050 worked for my 440lb max scale setup
float scale_weight = 0;
bool masterflag = 0;
bool weightflag = 0;
float master_weight = 20;

void setup() {
  pinMode(CLK, OUTPUT);
  pinMode(DOUT, INPUT);
  pinMode(W_INT, OUTPUT);
  master.begin(9600);
  scale.set_scale();
  scale.tare(); //Reset the scale to 0 */
  long zero_factor = scale.read_average(); //Get a baseline reading
  scale.set_scale(calibration_factor);
}

char *  mastertoslave()
{
  static char masterstr[5];  //must be one more than message length
  int masterDataLength = 0;
  
  while(master.available())
  {
    char masterData = master.read();
    if( masterData == '\n' )
    {
      break;
    }
  masterstr[masterDataLength] = masterData;
  masterDataLength++;
  }
  return masterstr;
}

void errorcheck(char *p)
{
  char number_array[4];

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
      master.write("ok__");
      master.write('\n');
      //master.flush();

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

      master.write(number_array[3]);
      master.write(number_array[2]);
      master.write(number_array[1]);
      master.write(number_array[0]);
      master.write('\n');
      //master.flush();
   //   master.write(p);
    }
    
    else if ( strcmp( "tare", p ) == 0) //if the reply is tare
    {
      scale.tare();
      digitalWrite(W_INT, 0);
      master.write("tard");
      master.write('\n');
      //master.flush();
    }
    else if ( strcmp( "setw", p ) == 0)
    {
      weightflag = 1;
      master.write("wset");
      master.write('\n');
      //master.flush();
    }
    else if ( strcmp( "cal+", p ) == 0) 
    {
      calibration_factor++;
      master.write("ok__");
      master.write('\n');
      //master.flush();
      scale.set_scale(calibration_factor);
    }
    else if ( strcmp( "cal-", p ) == 0) 
    {
      calibration_factor--;
      master.write("ok__");
      master.write('\n');
      //master.flush();
      scale.set_scale(calibration_factor);
    }
    else
    {
      master.write("no__");
      master.write('\n');
      //master.flush();
    }
}

void loop() {
  // put your main code here, to run repeatedly:
  if(master.available() && (masterflag == 0) )
  {
    errorcheck( mastertoslave() );

  }
  else
  {
    //noInterrupts();
    scale_weight = scale.get_units();
    //interrupts();
  }
  if( scale_weight >= master_weight && (masterflag == 1))
  {
    masterflag = 0;
    digitalWrite(W_INT, 1);
  }
  else
  {
    digitalWrite(W_INT, 0);
  }
}
