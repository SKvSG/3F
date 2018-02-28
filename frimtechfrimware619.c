//http://www.atmel.com/images/doc7593.pdf

#include "HX711.h"

#define DOUT  1
#define CLK  0

HX711 scale(DOUT, CLK);
 
 float calibration_factor = 970; //-7050 worked for my 440lb max scale setup
 
int led0 = 6;// Pin 6  has the LED on Teensy++ 2.0
int dir = 38;
int stp = 39;
int MS2 = 40;
int EN = 41;
int MS1 = 42;
char user_input;
int x;
int y;
int state;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(CLK, OUTPUT);
  pinMode(DOUT, INPUT);
  pinMode(led0, OUTPUT);     
  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT); 
  pinMode(MS2, OUTPUT);
  pinMode(EN , OUTPUT);
  pinMode(MS1 , OUTPUT);
  resetEDPins();//Set step, direction, microstep and enable pins to default states
  Serial.begin(9600); //Open Serial connection for debugging
  // Serial.println("serial test");
  // Serial.println("Begin motor control");
  // Serial.println();
  // Print function list for user selection
  // Serial.println("Enter number for control option:");
  // Serial.println("1. Turn at default microstep mode.");
  // Serial.println("2. Reverse direction at default microstep mode.");
  // Serial.println("3. Turn at 1/8th microstep mode.");
  // Serial.println("4. Step forward and reverse directions.");
  // Serial.println();
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");

  scale.set_scale();
  scale.tare(); //Reset the scale to 0

  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
}


void StepForwardDefault()
{
  Serial.println("Moving forward at default step mode.");
  digitalWrite(dir, LOW); //Pull direction pin low to move "forward"
  for(x= 1; x<1000; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(stp,HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  Serial.println("Enter new option");
  Serial.println();
}

void ReverseStepDefault()
{
  Serial.println("Moving in reverse at default step mode.");
  digitalWrite(dir, HIGH); //Pull direction pin high to move in "reverse"
  for(x= 1; x<1000; x++)  //Loop the stepping enough times for motion to be visible
  {
    digitalWrite(stp,HIGH); //Trigger one step
    delay(1);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  Serial.println("Enter new option");
  Serial.println();
}

// 1/8th microstep foward mode function
void SmallStepMode()
{
  Serial.println("Stepping at 1/8th microstep mode.");
  digitalWrite(dir, LOW); //Pull direction pin low to move "forward"
  digitalWrite(MS1, HIGH); //Pull MS1, and MS2 high to set logic to 1/8th microstep resolution
  digitalWrite(MS2, HIGH);
  for(x= 1; x<1000; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(stp,HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  Serial.println("Enter new option");
  Serial.println();
}

//Forward/reverse stepping function
void ForwardBackwardStep()
{
  Serial.println("Alternate between stepping forward and reverse.");
  for(x= 1; x<500; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    //Read direction pin state and change it
    state=digitalRead(dir);
    if(state == HIGH)
    {
      digitalWrite(dir, LOW);
    }
    else if(state ==LOW)
    {
      digitalWrite(dir,HIGH);
    }

    for(y=1; y<1000; y++)
    {
      digitalWrite(stp,HIGH); //Trigger one step
      delay(1);
      digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
      delay(1);
    }
  }
  Serial.println("Enter new option:");
  Serial.println();
}


void resetEDPins()
{
  digitalWrite(stp, LOW);
  digitalWrite(dir, LOW);
  digitalWrite(MS1, LOW);
  digitalWrite(MS2, LOW);
  digitalWrite(EN, HIGH);
}


void loop() {
//  digitalWrite(led0, HIGH);   // turn the LED on (HIGH is the voltage level)
//  delay(200);               // wait for a second
//  digitalWrite(led0, LOW);    // turn the LED off by making the voltage LOW
//  delay(100); 
   scale.set_scale(calibration_factor);
  Serial.print("Reading: ");
  Serial.print(scale.get_units(), 1);
  Serial.print("grams"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
  Serial.print(" calibration_factor: ");
  Serial.print(calibration_factor);
  Serial.println();

  if(Serial.available())
  {
    char temp = Serial.read();
    if(temp == '+' || temp == 'a')
      calibration_factor += 10;
    else if(temp == '-' || temp == 'z')
      calibration_factor -= 10;
  }
  
  // Call set_scale() with no parameter.
// Call tare() with no parameter.

// Place a known weight on the scale and call get_units(10).
// Divide the result in step 3 to your known weight. You should get about the parameter you need to pass to set_scale.
// Adjust the parameter in step 4 until you get an accurate reading.
  
  

  
  // while(Serial.available()){
      // user_input = Serial.read(); //Read user input and trigger appropriate function
      // digitalWrite(EN, LOW); //Pull enable pin low to allow motor control
      // if (user_input =='1')
      // {
         // StepForwardDefault();
      // }
      // else if(user_input =='2')
      // {
        // ReverseStepDefault();
      // }
      // else if(user_input =='3')
      // {
        // SmallStepMode();
      // }
      // else if(user_input =='4')
      // {
        // ForwardBackwardStep();
      // }
      // else
      // {
        // Serial.println("Invalid option entered.");
      // }
      // resetEDPins();
  // }
}
  
