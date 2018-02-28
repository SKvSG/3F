#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

int EN = 4;
int MS1 = 5;

int dir = 10;

int MS3 = 18;
int stp = 15;
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

  
  Serial.begin(9600);

  digitalWrite(dir, HIGH);
  digitalWrite(EN, LOW);
  delay(1000);
  Serial.write("Push Button to  Begin Test ->");
}




void StepFast()    // Moving forward at slow step mode.
{
  digitalWrite(dir, HIGH); //Pull direction pin low to move down
  digitalWrite(MS1, HIGH); //Pull MS1, and MS2 high to set logic to fullstep resolution
  digitalWrite(MS2, LOW);
  digitalWrite(MS3, LOW);
  while(1)  //add max steps
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


void loop() {
    
    digitalWrite(EN, LOW);//Pull enable pin low to allow motor control
    

   StepFast();     //go fast

}
