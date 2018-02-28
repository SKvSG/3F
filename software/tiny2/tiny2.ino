#include "HX711.h"

#define DOUT  23
#define CLK  22

#define master Serial1

HX711 scale(DOUT, CLK);

float calibration_factor = 970; //-7050 worked for my 440lb max scale setup
float scale_weight = 0;
int led0 = 13;// Pin 6  has the LED on Teensy++ 2.0
char masterString[4];
char masterData;
byte dex = 0;

void setup() {   
  pinMode(CLK, OUTPUT);
  pinMode(DOUT, INPUT);
  pinMode(led0, OUTPUT);     
 // pinMode(ISR , OUTPUT);
  Serial.begin(9600); //Open Serial connection for debugging
  master.begin(9600);
  
//  Serial.println("Remove all weight from scale");
//  Serial.println("After readings begin, place known weight on scale");
//  Serial.println("Press + or a to increase calibration factor");
//  Serial.println("Press - or z to decrease calibration factor");

  scale.set_scale();
  scale.tare(); //Reset the scale to 0 */
  
  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
  scale.set_scale(calibration_factor);   
}
void loop() {


    //if(master.available())
    while(master.available())
    {
    //  char ackbit = master.read();
        
    while (dex <= 4)
    {
      masterData = master.read(); // Read a character
      Serial.write(masterData);
        masterString[dex] = masterData; // Store it
      dex++;
    }
  }
    
    
    
//      if(ackbit == 't')
//      {
//        scale.tare();
//        master.write('t');
//        Serial.print("tared");
//      }
//      else if (ackbit == 'e')
//      {
//      //retransmit last message?
//      }
//      else if (ackbit < 1000 && ackbit > 0 )
//      {
//        while (scale_weight >= ackbit)
//        {
//          scale_weight = scale.get_units(); // Get force
//        }
//        //start interrupt
//      }
//    }
//  else
//  {
//  digitalWrite(led0, HIGH);  
//    delay(1000);            
//    digitalWrite(led0, LOW);
//   // scale_weight = scale.get_units();
//    Serial.print(".");
//    delay(100);    
//  //wait for message
//  }
}

      //interrupt
    // send weight
    //recieve ok


