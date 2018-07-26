/*/////////////////////////////////////////////////////////////////////////////////////////
  BRIDGE PROGRAM - SKVSG - 0.0.0
/////////////////////////////////////////////////////////////////////////////////////////*/

/*/////////////////////////////////////////////////////////////////////////////////////////
  LIBRARIES
/////////////////////////////////////////////////////////////////////////////////////////*/
#include <avr/io.h>
#include <SPI.h>
#include <Wire.h>
#include <Ethernet3.h>
#include <SD.h>
#include <stdio.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header
// I2C Address: It's either 0x27 or 0x3F

/*/////////////////////////////////////////////////////////////////////////////////////////
  MACROS
/////////////////////////////////////////////////////////////////////////////////////////*/
#define REQ_BUF_SZ   2000   // size of buffer used to capture HTTP requests
#define head1 Serial1
#define head2 Serial2
#define head3 Serial3
#define MAX_LENGTH 64
#define MAX_SETTINGS 8
#define GROWER_SETTINGS 4
#define MAX_POST_LENGTH 8l
#define LCD_COLS 20
#define LCD_ROWS 4

/*/////////////////////////////////////////////////////////////////////////////////////////
  PIN ASSIGNMENTS
/////////////////////////////////////////////////////////////////////////////////////////*/
const int FirmnessHead1RX = 0;
const int FirmnessHead1TX = 1;
const int MS2 = 2;
const int FTBTN = 3;
const int SDCS = 4;
const int EN = 5; //J13
const int LCDBTN1 = 6;
const int FirmnessHead3RX = 7;
const int FirmnessHead3TX  = 8;
const int WRESET = 9;
const int WCS = 10;
//const int DOUT = 11;
//const int DIN = 12;
//const int DSCK = 13;
const int MS3 = 14;
const int MS1 = 15;
const int STP = 16;
const int DIR = 17;
//const int SDA0 = 18; 
//const int SCL0 = 19;
const int HEAD1EN = 20;
const int HEAD2EN = 21;
const int HEAD3EN = 22;
const int LCDBTN2 = 23;
const int PHOTO1 = 24;
const int SOLO = 25;
const int FirmnessHead2RX = 26;
const int PHOTO2 = 27;    
const int FTDET = 28; 
//const int J13SCL1 = 29;
//const int J13SDA1 = 30;
const int FirmnessHead2TX = 31;
const int LED1 = 32;
const int LED2 = 33;

/*/////////////////////////////////////////////////////////////////////////////////////////
  DEVICES
/////////////////////////////////////////////////////////////////////////////////////////*/
hd44780_I2Cexp lcd;
EthernetServer server(80);  // create a server at port 80

/*/////////////////////////////////////////////////////////////////////////////////////////
  GLOBAL VARIABLES
/////////////////////////////////////////////////////////////////////////////////////////*/
struct machineSettings
{
  boolean lcd = 0;
  boolean network = 0;
  boolean head1 = 0;
  boolean head2 = 0;
  boolean head3 = 0;
  boolean ftpedal = 0;
  boolean serial = 0;
  boolean filesystem = 0;
  int h1Calib = 2185;
  int h2Calib = 2185;
  int h3Calib = 2185;
  int resttime = 0;
  
  int array[100];
  byte mac[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
  int pressure1 = 2;
  int pressure2 = 100;
  int pressure3 = 250;
  int Rest = 0;
  int WUnits = 453.592; //conversion factor * gram = lbs
  int DUnits = 25.4; //conversion factor * inches = mm TODO: add rod conversion factor
  int calibration_pressure = 100;
} localSettings;

struct lotSettings
{
  int changeCount = 25;
  char * growerName = "STEMILT\0";
  int currentLot = 0;
  int totalLot = 20;
} growerSettings;

struct lotSettings emptySettings = {0, "", 0, 0};

struct frame_vector
{
  int msg_direction = 0;
  int frame_index = 0;
  int pause_index = 0;
} frame1;

struct frame_vector frame2;
File settingsFile;
File growerFile;
static char h1str[7];  //must be one more than message length
static char h2str[7];  //must be one more than message length
static char h3str[7];  //must be one more than message length

int requestint = 1;
//IPAddress ip(192, 168, 0, 127);

File webFile;
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
int request_index = 0;              // index into HTTP_req buffer
const char* actions[] = { "ajax_ip", "ajax_subn", "ajax_gate", "ajax_changecount", "ajax_Grower", "ajax_Lot", //TODO : change to ajaxstrings
                    "ajax_pressure1", "ajax_Remaining", "ajax_pressure2", "ajax_Rest", "ajax_WUnits",
                    "ajax_DUnits", "ajax_pressure3", "ajax_receiveip", "ajax_a0", "ajax_graph", "ajax_table0",
                    "ajax_nexttable", "ajax_Calib", "404"
                    };
const char* poststrings[] = {"contp", "finap", "restt", "touchp", "cweigh"};
const char* machineSets[] = { "mac", "IPAddress", "Pressure1", "Pressure2", "Pressure3", "Rest", "WUnits", "DUnits"};
const char* growerSets[] = { "growerName", "currentLot", "totalLot", "changeCount"};
const char* filenames[] = { "growers.txt", "settings.txt", "DATALOG.TXT"};
int animation_step = 0;
unsigned long previousMillis = 0;
char *str;  //must be one more than message length
int samples = 0;
int steps = 0;
int head = 0;
boolean new_lot = true;

int mode = 0;
  //modes:
  //0-DEFAULT
  //1-SERIAL
  //2-SD
  //3-NETWORK
  //4-FOOTPEDAL  
  //5-HEAD1
  //6-HEAD2
  //7-HEAD3
  //8-MACHINE SETTINGS  ???
  //9-GROWER SETTINGS   ???
int submode = 0;
int oldmode = -1;
int oldsubmode = -1;
boolean button_pressed = 0;

/*/////////////////////////////////////////////////////////////////////////////////////////
  FUNCTION DECLARATIONS
/////////////////////////////////////////////////////////////////////////////////////////*/
void initializePins();                        //set inputs and outputs
boolean initializeLCD();
boolean initializeSerial();
boolean initializeFtPedal();
boolean initializeSD(char *filenames);
boolean initializeHead1();
boolean initializeHead2();
boolean initializeHead3();
struct machineSettings loadMachineSettings(struct machineSettings, File);                          //load settings set in the config file or sets to default value
struct lotSettings loadGrowerSettings(struct lotSettings, File, int);   //load settings set in the config file or sets to default value int value represents which
boolean initializeNetwork(struct machineSettings);                     //start network adapter check connectivity
void serveclient(EthernetClient *client);      //recieve request from client
char * getclientdata(EthernetClient *client);
void processrequest(char * );
//void processrequest(char *, machinesettings)
const char * validaterequest(char *);
void processaction(char HTTP_req[REQ_BUF_SZ], EthernetClient *client); //***handle ajax requests TODO: REMOVE ETHERNETCLIENT RETURN STRING
char StrContains(char *str, char *sfind);     //check request for string searches for the string sfind in the string str, returns 1 if string found, 0 if string not found
void StrClear(char *str, char length);        //check if final line of request
char * getw();                                //request pressure reading
void testfirmness();                          //run the firmness testing procedure
void idleanim();
void sendheader(char *request, EthernetClient *client);
void runmode();
void modeanim();
/*/////////////////////////////////////////////////////////////////////////////////////////
  MAIN
/////////////////////////////////////////////////////////////////////////////////////////*/
void setup()
{
  initializePins();
  localSettings.lcd  = initializeLCD();
  localSettings.serial = initializeSerial();
  localSettings.ftpedal = initializeFtPedal(); 
  localSettings.head1 = initializeHead1();
  localSettings.head2 = initializeHead2();
  localSettings.head3 = initializeHead3(); 
  displaydiaginfo(localSettings);
  localSettings.filesystem = initializeSD(); 
  displaydiaginfo(localSettings);
  localSettings.network = initializeNetwork(localSettings);
  displaydiaginfo(localSettings);
  localSettings = loadMachineSettings(localSettings, settingsFile);
  growerSettings = loadGrowerSettings(growerSettings, growerFile, 1);
  lcd.setCursor(0,2);
  lcd.print("                    ");   
  lcd.setCursor(0,3);
  lcd.print("                    ");
}

void loop()
{
  EthernetClient client = server.available();  // try to get client;
  if (client)   // serve client website
  {
      serveclient(&client);
  }
  else if (digitalRead(LCDBTN1) == 0 && (button_pressed == 0))    //buttons are active low 
  {
    button_pressed = 1;
    mode++;
    submode = 0;
    if (mode > 7)
    {      
      mode = 0;
    }
  }
  else if (digitalRead(LCDBTN2) == 0 && (button_pressed == 0))
  {
    button_pressed = 1;
    submode++;
  }
  else if (digitalRead(FTBTN) == 0 && (button_pressed == 0)) // continue testing firmness
  {
    button_pressed = 1;
    runmode();  //don't need to pass globals
  }
  else 
  { //lcd animation
      unsigned long currentMillis = millis();    
    if (currentMillis  - previousMillis >= 200 )
    {
      modeanim();
      displaydiaginfo(localSettings);
      if ((digitalRead(LCDBTN1) ==1) && (digitalRead(LCDBTN2) == 1) && (digitalRead(FTBTN) == 1))   
      {
        button_pressed = 0;
      }
       previousMillis = currentMillis; 
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
//int DOUT = 11;      //pinmode not required
//int DIN = 12;       //pinmode not required
//int DSCK = 13;      //pinmode not required
//int SDA0 = 18;      //pinmode not required
//int SCL0 = 19;      //pinmode not required
//int PHOTO1 = 24;    //pinmode not required  
//int PHOTO2 = 27;    //pinmode not required


  pinMode(LCDBTN1, INPUT_PULLUP);
  pinMode(LCDBTN2, INPUT_PULLUP);
  pinMode(FTBTN, INPUT_PULLUP);
  pinMode(FTDET, INPUT_PULLUP);
  pinMode(STP, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(EN , OUTPUT);;
  pinMode(MS1 , OUTPUT);
  pinMode(MS3 , OUTPUT);
  pinMode(HEAD1EN , OUTPUT);
  pinMode(HEAD2EN , OUTPUT);
  pinMode(HEAD3EN , OUTPUT);
  pinMode(SOLO, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  head2.setRX(26);  //firmness head 2
  head2.setTX(31);
  pinMode(WRESET, OUTPUT);
  digitalWrite(WRESET, LOW);    // begin reset the WIZ820io
  pinMode(WCS, OUTPUT);
  digitalWrite(WCS, HIGH);  // de-select WIZ820io
  pinMode(SDCS, OUTPUT);
  digitalWrite(SDCS, HIGH);   // de-select the SD Card
  digitalWrite(WRESET, HIGH);   // end reset pulse

  digitalWrite(EN, HIGH);
  digitalWrite(HEAD1EN, LOW);
  digitalWrite(HEAD2EN, LOW);
  digitalWrite(HEAD3EN, LOW);
  digitalWrite(FirmnessHead1RX, LOW);
  digitalWrite(FirmnessHead2RX, LOW);
  digitalWrite(FirmnessHead3RX, LOW);
}

boolean initializeFtPedal()
{
  return digitalRead(FTDET);
}

boolean initializeSerial() //TODO: Make this functional
{
  Serial.begin(9600);       // for debugging
  if(Serial.available())
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

boolean initializeHead1()
{ 
  int timeout = 0;
  char headId[4];
  lcd.setCursor(0,2);
  frame1 = idleanimotherone("Searching for Head 1",2,frame1);    
  head1.begin(9600);  //firmnesshead 1
  digitalWrite(HEAD1EN, HIGH);
  while(!head1.available() && (timeout <= 250))
  {
    delay(1);
    timeout++;
    lcd.setCursor(0,3);
    lcd.print(timeout);
  }
  digitalWrite(HEAD1EN, LOW);
  if(timeout < 250)
  {
    for(int i = 0; i < 4; i++)
    {
      headId[i] = char(head1.read());
      delay(1);
    }
    //what should go here calibration number?
    head1.write("ackn");
    lcd.setCursor(5,3);
    lcd.print(headId);
    Serial.print(headId);
    delay(1000);
    return 1;
  }
  else
  {
    return 0;
  }
}

boolean initializeHead2()
{ 
  int timeout = 0;
  char headId[4];
  lcd.setCursor(0,2);
  frame1 = idleanimotherone("Searching for Head 2",2,frame1);    
  head2.begin(9600);  //firmnesshead 2
  digitalWrite(HEAD2EN, HIGH);
  while(!head2.available() && (timeout <= 250))
  {
    delay(1);
    timeout++;
    lcd.setCursor(0,3);
    lcd.print(timeout);
  }
  digitalWrite(HEAD2EN, LOW);
  if(timeout < 250)
  {
    for(int i = 0; i < 4; i++)
    {
      headId[i] = char(head2.read());
      delay(1);
    }
    //what should go here calibration number?
    head2.write("ackn");
    lcd.setCursor(5,3);
    lcd.print(headId);
    Serial.print(headId);
    delay(1000);
    return 1;
  }
  else
  {
    return 0;
  }
}

boolean initializeHead3()
{ 
  int timeout = 0;
  char headId[4];
  lcd.setCursor(0,2);
  frame1 = idleanimotherone("Searching for Head 3",2,frame1);    
  head3.begin(9600);  //firmnesshead 3
  digitalWrite(HEAD3EN, HIGH);
  while(!head3.available() && (timeout <= 250))
  {
    delay(1);
    timeout++;
    lcd.setCursor(0,3);
    lcd.print(timeout);
  }
  digitalWrite(HEAD3EN, LOW);
  if(timeout < 250)
  {
    for(int i = 0; i < 4; i++)
    {
      headId[i] = char(head3.read());
      delay(1);
    }
    //what should go here calibration number?
    head3.write("ackn");
    lcd.setCursor(5,3);
    lcd.print(headId);
    Serial.print(headId);
    delay(1000);
    return 1;
  }
  else
  {
    return 0;
  }
}

boolean initializeLCD()
{
  int status;

  status = lcd.begin(LCD_COLS, LCD_ROWS);
  if(status) // non zero status means it was unsuccesful
  {
    Serial.print("LCD MISSING");
    return 0;
  }
  lcd.lineWrap();
  lcd.print("Fast Firmness Finder");
  return 1;
}

boolean initializeSD()
{
  // initialize SD card
  frame1 = idleanimotherone("Initializing SD card:",2,frame1);
  Serial.println("Initializing SD card:");
  
  if (!SD.begin(4)) {
    frame2 = idleanimotherone("ERROR - SD card fail",3,frame2);
    Serial.println("ERROR - SD card fail");
    delay(1250);
    return 0;    // init failed
  }
  frame2 = idleanimotherone("SUCCESS - SD found",3,frame2);
  Serial.println("SUCCESS - SD found  ");
  
  frame1 = idleanimotherone("Checking SD for Files",2,frame1);
  Serial.println("Checking SD for Files");
  int returnval = 1;
  for ( int i = 0; i < ( sizeof(filenames)/sizeof(filenames[0]) ); i++ )
  {
    if (!SD.exists(filenames[i]))
    {
      lcd.setCursor(0,3);
      lcd.print("ERROR: ");
      lcd.print(filenames[i]);
      Serial.print("ERROR: ");
      Serial.print(filenames[i]);
      delay(1250);
      returnval = 0;
    }
    else
    {
      lcd.setCursor(0,3);      
      lcd.print("FOUND: ");
      lcd.print(filenames[i]);
      Serial.print("FOUND: ");
      Serial.print(filenames[i]);
    }
  }
  return returnval;
}

boolean initializeNetwork(struct machineSettings localSettings)       //use new lcd functions
{
  //Ethernet.begin(localSettings.mac, ip);  // initialize Ethernet device
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("Getting DHCP IP");
  lcd.setCursor(0,3);
  lcd.print("                    ");
  Ethernet.begin(localSettings.mac);  // initialize Ethernet device
  server.begin();           // start to listen for clients
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("Visit to Configure: ");
  lcd.setCursor(0,3);
  lcd.print("                    ");
  lcd.setCursor(0,3);
  //if (strcmpy(Ethernet.localIP(), "0.0.0.0"))
  //{;
  //return 0;
  //}
  lcd.print(Ethernet.localIP());
  delay(2000);
  return 1;
}

struct machineSettings loadMachineSettings(struct machineSettings localSettings, File settingsFile)
{
  char str[MAX_SETTINGS][MAX_LENGTH]; //longest field plus delimiter
  int i = 0;
  char *token;
  if (!SD.exists("settings.txt")) {
    return localSettings;  // can't find index file
  }
  settingsFile = SD.open("settings.txt");

  while ( i < MAX_SETTINGS) //read file into set of strings
  {
    size_t n = readField(&settingsFile, str[i], sizeof(str), "\n");
    i++;
  }
  int k = 0;
  while (k < MAX_SETTINGS) //turn strings into settings
  {
    int j = 0;
    while (  j < MAX_SETTINGS )  //findout action integer match use case
    {
      if ( strstr( str[k], machineSets[j] ) )
      {
        token = strtok(str[k], " ");
        Serial.print("Setting found: ");
        Serial.println(token);
        int l = 0;
        switch (j)
        {
          case 0 : //mac
            while ( l < sizeof(localSettings.mac))
            {
              token = strtok(NULL, " ");
              localSettings.mac[l] =  strtol(token, (char **)NULL, 16) ;
              l++;
            }
            break;

          case 1 : //IPAddress
            //token = strtok(NULL, " ");
            break;

          case 2 : //Pressure1
            token = strtok(NULL, " ");
            localSettings.pressure1 = strtol(token, (char **)NULL, 10);
            break;

          case 3 : //Pressure2
            token = strtok(NULL, " ");
            localSettings.pressure2 = strtol(token, (char **)NULL, 10);
            break;

          case 4 : //Pressure3
            token = strtok(NULL, " ");
            localSettings.pressure3 = strtol(token, (char **)NULL, 10);
            break;

          case 5 : //Rest
            token = strtok(NULL, " ");
            localSettings.Rest = strtol(token, (char **)NULL, 10);
            break;

          case 6 : //WUnitst
            token = strtok(NULL, " ");
            localSettings.WUnits = strtol(token, (char **)NULL, 10);
            break;

          case 7 : //DUnits
            token = strtok(NULL, " ");
            localSettings.WUnits = strtol(token, (char **)NULL, 10);
            break;

          case 8 : //DUnits
            //    DUnits = int(strchr(str[k], ' '));
            break;

          default :
            Serial.print("error");
        }
        break;
      }
      j++;
    }
    k++;
  }
  settingsFile.close();
  return localSettings;
};

struct lotSettings loadGrowerSettings(struct lotSettings growerSetting, File growerFile, int growerNumber)
{
  char str[GROWER_SETTINGS][MAX_LENGTH]; //longest field plus delimiter
  int i = 0;
  char *token;
  if (!SD.exists("growers.txt")) {
    return growerSetting;  // can't find index file
  }
  growerFile = SD.open("growers.txt");
  int n = 0;
  while (growerNumber > 0)
  {
    while ( i < GROWER_SETTINGS) //read file into set of strings
    {
      n = readField(&growerFile, str[i], sizeof(str), "\n");
      Serial.print(n);
      i++;
    }
    i = 0;
    growerNumber--;
  }
  if (n == 0)
  {
    return emptySettings;
  }
  int k = 0;
  while (k < GROWER_SETTINGS) //turn strings into settings
  {
    int j = 0;
    while (  j < GROWER_SETTINGS )  //findout action integer match use case
    {
      if ( strstr( str[k], growerSets[j] ) )
      {
        token = strtok(str[k], " ");
        Serial.print(token);
        Serial.print(" : ");
        switch (j)
        {
          case 0 : //growerName             //TODO: FIX THIS
            token = strtok(NULL, " ");
            token = strtok(token, "\n");
            strncat(token, "\0", 1);
            Serial.println(token);
            growerSetting.growerName = token;
            //strcpy(growerSetting.growerName, token);

            Serial.println(growerSetting.growerName);
            break;

          case 1 : //currentLot
            token = strtok(NULL, " ");
            growerSetting.currentLot = strtol(token, (char **)NULL, 10);
            Serial.println(growerSetting.currentLot);
            break;

          case 2 : //totalLot
            token = strtok(NULL, " ");
            growerSetting.totalLot = strtol(token, (char **)NULL, 10);
            Serial.println(growerSetting.totalLot);
            break;

          case 3 : //changeCount
            token = strtok(NULL, " ");
            growerSetting.changeCount = strtol(token, (char **)NULL, 10);
            Serial.println(growerSetting.changeCount);
            break;

          default :
            Serial.println("error");
        }
        break;
      }
      j++;
    }
    k++;
  }
  settingsFile.close();
  return growerSetting;
};

/*/////////////////////////////////////////////////////////////////////////////////////////
  LCD FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////*/
void displaydiaginfo(struct machineSettings localSettings)  //MINOR BUG: Remove flicker from certain situations
{
  //lcd.print("SR SD NT FT H1 H2 H3");
  lcd.setCursor(0,1);
  lcd.print("** SD NT FT H1 H2 H3");    //MINOR BUG: This causes flicker on characters that get changed later but fixes left over character from changing modes
  lcd.setCursor(0,1);
  switch (mode)
  {
    case 0:   //DEFAULT mode shows all working systems
      if(localSettings.serial)
      {
        lcd.print("SR");
      }
      else
      {
        lcd.print("**");
      }
      lcd.setCursor(3,1);
      if(localSettings.filesystem)
      {
        lcd.print("SD");
      }
      else
      {
        lcd.print("**");
      }
      lcd.setCursor(6,1);
      if(localSettings.network)
      {
        lcd.print("NT");
      }
      else
      {
        lcd.print("**");
      }
      lcd.setCursor(9,1);
      if(localSettings.ftpedal)
      {
        lcd.print("FT");
      }
      else
      {
        lcd.print("**");
      }
      lcd.setCursor(12,1);
      if(localSettings.head1)
      {
        lcd.print("H1");
      }
      else
      {
        lcd.print("**");
      }
      lcd.setCursor(15,1);
      if(localSettings.head2)
      {
        lcd.print("H2");
      }
      else
      {
        lcd.print("**");
      }
      lcd.setCursor(18,1);
      if(localSettings.head3)
      {
        lcd.print("H3");
      }
      else
      {
        lcd.print("**");
      }
      break;
    case 1:       //Serial Mode
      if(animation_step % 2 == 0)
      {
        lcd.print("      ");
      }
      else
      {
        if(localSettings.serial)
        {
          lcd.print("SERIAL");
        }
        else
        {
          lcd.print("serial");
        }
      }
      lcd.print(" SDNTFTH1H2H3");   
      break;
    case 2:         //SD
      lcd.print("SR ");
      if(animation_step % 2 == 0)
      {
        lcd.print("      ");
      }
      else
      {
        if(localSettings.filesystem)
        {
          lcd.print("SDCARD");
        }
        else
        {
          lcd.print("sdcard");
        }
      }
      lcd.print(" NTFTH1H2H3 ");
      break;
    case 3:         //NT
      lcd.print("SRSD ");
      if(animation_step % 2 == 0)
      {
        lcd.print("      ");
      }
      else
      {
        if(localSettings.network)
        {
          lcd.print("NETWRK");
        }
        else
        {
          lcd.print("netwrk");
        }
      }
      lcd.print(" FTH1H2H3");      
      break;      
    case 4:         //NT
      lcd.print("SRSDNT ");
      if(animation_step % 2 == 0)
      {
        lcd.print("      ");
      }
      else
      {
        if(localSettings.network)
        {
          lcd.print("FTPEDL");
        }
        else
        {
          lcd.print("ftpedl");
        }
      }
      lcd.print(" H1H2H3");      
      break;
    case 5:         //HEAD 1
      lcd.print("SRSDNTFT ");
      if(animation_step % 2 == 0)
      {
        lcd.print("      ");
      }
      else
      {
        if(localSettings.head1)
        {
          lcd.print("HEAD 1");
        }
        else
        {
          lcd.print("head 1");
        }
      }
      lcd.print(" H2H3");      
      break;
    case 6:         //HEAD 2
      lcd.print("SRSDNTFTH1 ");
      if(animation_step % 2 == 0)
      {
        lcd.print("      ");
      }
      else
      {
        if(localSettings.head2)
        {
          lcd.print("HEAD 2");
        }
        else
        {
          lcd.print("head 2");
        }
      }
      lcd.print(" H3");      
      break;
    case 7:         //HEAD 3
      lcd.print("SRSDNTFTH1H2 ");
      if(animation_step % 2 == 0)
      {
        lcd.print("       ");
      }
      else
      {
        if(localSettings.head3)
        {
          lcd.print("HEAD 3 ");
        }
        else
        {
          lcd.print("head 3 ");
        }
      }      
      break;       
  }    
}

void idleanim()
{
  lcd.setCursor(animation_step,3);
  if (animation_step == LCD_COLS-1) 
  {
    lcd.setCursor(0,3);
    lcd.print("                    ");
  }
  else
  {
    lcd.print("->");
  }
}

int idleanimother(char * str, int line,int index) //some sort of delay or timing system or reversing system
{
  if (strlen(str) > 20 )    //doesn't fit on screen
  {
    lcd.setCursor(0,line);
    for(int pos = 0+index; pos <= 19+index; pos++)
    {
      lcd.print(str[pos]);
    }
    index++;
  }
  else    //fits on screen just diplay string
  {
    lcd.print(str);
  }
  if (index+20 > strlen(str))
  {
    index = 0;
  }
  return index;
}

struct frame_vector idleanimotherone(char * str, int line,struct frame_vector frame) //some sort of delay or timing system or reversing system
{
  if (strlen(str) > 20 )    //doesn't fit on screen
  {
    lcd.setCursor(0,line);
    if (!frame.msg_direction)
    {
      for(int pos = 0+frame.frame_index; pos <= 19+frame.frame_index; pos++)
      {
        lcd.print(str[pos]);
      }
      if((frame.frame_index == 0 || frame.frame_index == 20 || frame.frame_index == 40 || frame.frame_index == 60) && frame.pause_index < 10 )
      {
        frame.pause_index++;
      }
      else if(frame.frame_index+21 > strlen(str))
      {
        if (frame.pause_index < 10)
        {
         frame.pause_index++;
        }
        else
        {
          frame.pause_index = 0;
          frame.frame_index = 0;
          frame.msg_direction = true;
        }
      }
      else
      {
        frame.frame_index++;
        frame.pause_index = 0;
      }
    }
    else
    {
      for(int pos = strlen(str)-frame.frame_index-20; pos < strlen(str)-frame.frame_index; pos++)
      {
        lcd.print(str[pos]);
      }
      frame.frame_index ++;
      if (frame.frame_index > strlen(str)-19)
      {
        frame.frame_index = 0;
        frame.msg_direction = false;
      }
    }
  }
  else    //fits on screen just diplay string
  {
    lcd.setCursor(0,line);
    lcd.print("                    ");
    lcd.setCursor(0,line);
    lcd.print(str);
  }
  return frame;
}

void modeanim()
{

    char str[90];
    switch (mode)
    {
     case 0:    //DEFAULT
      switch (submode)
      {
        case 0:           //normal test
          frame1 = idleanimotherone("Push Pedal to Test, Push Button to view Grower Name",2,frame1);          
          idleanim();
          break;
        case 1:           //view grower setttings
          frame1 = idleanimotherone("Push Pedal to Restart Current Grower, Push Button to view Next Grower",2,frame1);
          strcpy(str, "The Current Grower: ");
          strcat(str, growerSettings.growerName);
          frame2 = idleanimotherone(str,3,frame2);          
          break;
        case 2:          //load next grower
          frame1 = idleanimotherone("Push Pedal to Select Grower, Push Button to view Next Grower",2,frame1);
          strcpy(str, "The Next Grower: ");
          //strcat(str, (char*)growerSettings.growerNameNext);          //get next growername
          frame2 = idleanimotherone(str,3,frame2);         
          break;
        case 3:         //back to 2
          submode = 2;
        default:
          submode = 0;
          break;
      }
      break;
     case 1:    //SERIAL
      switch (submode)
      {
        case 0:     //DEFAULT //available
          if(localSettings.serial)
          {
            frame1 = idleanimotherone("Serial Connected",2,frame1);
          }
          else
          {
            frame1 = idleanimotherone("Serial Not Connected Connect usb at 9600 baud ",2,frame1);
          }
          frame2 = idleanimotherone("Push Button to rescan serial connections",3,frame2);          
          break;
        case 1:     //rescan
          frame1 = idleanimotherone("Scanning....",2,frame1);
          initializeSerial();
          submode = 0;
          break;
        default:
          submode = 0;
          break;
      }
      break;

     case 2:    //SD
      switch (submode)
      {
        case 0:
          if (localSettings.filesystem)
          {
            frame1 = idleanimotherone("SD card and filesystem loaded successfully",2,frame1);      
          }
          else
          {
            frame1 = idleanimotherone("SD card and filesystem failed to load successfully",2,frame1);  
          }
          frame2 = idleanimotherone("Push Button 1 to Rescan SD Card and Filesystem",3,frame2);  
          break;
        case 1:           //rescan SD connections 
          frame2 = idleanimotherone("Rescanning SD...",2,frame2);
          initializeSD();
          submode = 0;
          break;     
        default:
          submode = 0;
          break;
      }   
      break;
     case 3:    //NETWORK
      switch (submode)
      {
        case 0:
          if (localSettings.network)
          {
            frame1 = idleanimotherone("Network Connected Successfully",2,frame1);      
          }
          else
          {
            frame1 = idleanimotherone("Network NOT Connected Successfully",2,frame1);  
          }
          frame2 = idleanimotherone("Push Button to Retry, Push Pedal to View More",3,frame2);  
          break;
        case 1:       //rescan network
          frame1 = idleanimotherone("Retrying Network...",2,frame1);
          initializeNetwork(localSettings);
          submode = 0;
          break;     
          //veiw current ip config
          //rescan network connections
        default:
          submode = 0;
          break;
      }
      break;
     case 4:    //FOOTPEDAL
      switch (submode)
      {
        case 0:     //DEFAULT //available
          if (localSettings.ftpedal)
          {
            frame1 = idleanimotherone("Footpedal is connected",2,frame1);      
          }
          else
          {
            frame1 = idleanimotherone("Footpedal is not connected",2,frame1);
          }
          frame2 = idleanimotherone("Push Button to Rescan Footpedal",3,frame2); 
          break;
        
        case 1:     //rescan
          frame1 = idleanimotherone("Rescanning footpedal...",2,frame1);
          initializeFtPedal();
          submode = 0;
          break;
          
        default:
          submode = 0;
          break;
      }  
      //Serial.write("Footpedal is not connected");   
      break;      
     case 5:    //HEAD1
      switch (submode)
      {
        case 0: //push to scan for head or push
          if(localSettings.head1)
          {
            frame1 = idleanimotherone("Head 1 found",2,frame1);
          }
          else
          {
            frame1 = idleanimotherone("Head 1 NOT found",2,frame1);
          }
          frame2 = idleanimotherone("Push Button to Rescan, Use Pedal to Start Calibration",3,frame2);
          break;
        case 1:
          frame1 = idleanimotherone("Rescanning Head 1...",2,frame1);
          initializeHead1();
          submode = 0;
          break; 
        default:
          submode = 0;
          break;
      }
      break;    
     case 6:    //HEAD2
      switch (submode)
      {
        case 0: //push to scan for head or push
          if(localSettings.head2)
          {
            frame1 = idleanimotherone("Head 2 found",2,frame1);
          }
          else
          {
            frame1 = idleanimotherone("Head 2 NOT found",2,frame1);
          }
          frame2 = idleanimotherone("Push Button to Rescan, Use Pedal to Start Calibration",3,frame2);
          break;
        case 1:
          frame1 = idleanimotherone("Rescanning Head 2...",2,frame1);
          initializeHead2();
          submode = 0;
          break; 
        default:
          submode = 0;
          break;
      }
      break;
     case 7:    //HEAD3
      switch (submode)
      {
        case 0: //push to scan for head or push
          if(localSettings.head3)
          {
            frame1 = idleanimotherone("Head 3 found",2,frame1);
          }
          else
          {
            frame1 = idleanimotherone("Head 3 NOT found",2,frame1);
          }
          frame2 = idleanimotherone("Push Button to Rescan, Use Pedal to Start Calibration",3,frame2);
          break;
        case 1:
          frame1 = idleanimotherone("Rescanning Head 3...",2,frame1);
          initializeHead3();
          submode = 0;
          break; 
        default:
          submode = 0;
          break;
      }
      break;
     case 8:    //NETWORK SETTINGS
      switch (submode)
      {
        case 0:
          break;
        case 1:
          break;
        case 2:
          break;
        default:
          submode = 0;
          break;
      }
      break;
     case 9:    //CALIBRATION SETTINGS HEAD1
        switch (submode)
        {
          case 0:
            frame1 = idleanimotherone("Use Foot Pedal to TEST Calibration, use Button change Calibration Factor",2,frame1);
            lcd.setCursor(0,3);
            lcd.print("    ");
            lcd.setCursor(0,3);            
            lcd.print(localSettings.h1Calib);      
            break;
          case 1:
          //get calib factor
            frame1 = idleanimotherone("Use Foot Pedal to DECREASE Calibration, use Button change Calibration Factor and return to Test",2,frame1);
            lcd.setCursor(0,3);
            lcd.print("    ");
            lcd.setCursor(0,3);            
            lcd.print(localSettings.h1Calib);  
            break;
          case 2:
            frame1 = idleanimotherone("Use Foot Pedal to INCREASE Calibration, use Button to return to Test",2,frame1);
            lcd.setCursor(0,3);
            lcd.print("    ");
            lcd.setCursor(0,3);            
            lcd.print(localSettings.h1Calib);   
            break;
          case 3:
            submode = 0;
            break;
          default:
            submode = 0;
            break;
        }
      break;
     case 10:    //CALIBRATION SETTINGS HEAD2
        switch (submode)
        {
          case 0:
            frame1 = idleanimotherone("Use Foot Pedal to TEST Calibration, use Button change Calibration Factor",2,frame1);
            lcd.setCursor(0,3);
            lcd.print("    ");
            lcd.setCursor(0,3);            
            lcd.print(localSettings.h2Calib);         
            break;
          case 1:
          //get calib factor
            frame1 = idleanimotherone("Use Foot Pedal to DECREASE Calibration, use Button change Calibration Factor and return to Test",2,frame1);
            lcd.setCursor(0,3);
            lcd.print("    ");
            lcd.setCursor(0,3);            
            lcd.print(localSettings.h2Calib);  
            break;
          case 2:
            frame1 = idleanimotherone("Use Foot Pedal to INCREASE Calibration, use Button to return to Test",2,frame1);
            lcd.setCursor(0,3);
            lcd.print("    ");
            lcd.setCursor(0,3);            
            lcd.print(localSettings.h2Calib);     
            break;
          case 3:
            submode = 0;
            break;
          default:
            submode = 0;
            break;
        }
        break;
      case 11:    //CALIBRATION SETTINGS HEAD3
        switch (submode)
        {
          case 0:
            frame1 = idleanimotherone("Use Foot Pedal to TEST Calibration, use Button change Calibration Factor",2,frame1);
            lcd.setCursor(0,3);
            lcd.print("    ");
            lcd.setCursor(0,3);            
            lcd.print(localSettings.h3Calib);       
            break;
          case 1:
          //get calib factor
            frame1 = idleanimotherone("Use Foot Pedal to DECREASE Calibration, use Button change Calibration Factor and return to Test",2,frame1);
            lcd.setCursor(0,3);
            lcd.print("    ");
            lcd.setCursor(0,3);            
            lcd.print(localSettings.h3Calib); 
            break;
          case 2:
            frame1 = idleanimotherone("Use Foot Pedal to INCREASE Calibration, use Button to return to Test",2,frame1);
            lcd.setCursor(0,3);
            lcd.print("    ");
            lcd.setCursor(0,3);            
            lcd.print(localSettings.h3Calib);    
            break;
          case 3:
            submode = 0;
            break;
          default:
            submode = 0;
            break;
        }
        break;
    }
    animation_step++;
    if (animation_step == LCD_COLS) 
    {
      animation_step = 0;
    }    
}

/*/////////////////////////////////////////////////////////////////////////////////////////
  WEBSERVER FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////*/
char StrContains(char *str, char *sfind)
{
  char found = 0;
  char index = 0;
  char len;

  len = strlen(str);

  if (strlen(sfind) > len) {
    return 0;
  }
  while (index < len) {
    if (str[index] == sfind[found]) {
      found++;
      if (strlen(sfind) == found) {
        return 1;
      }
    }
    else {
      found = 0;
    }
    index++;
  }
  return 0;
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, int length)
{
  for (int i = 0; i < length; i++) {
    str[i] = 0;
  }
}

int readField(File* file, char* str, size_t size, char* delim)
{
  char ch;
  int n = 0;
  while ((n + 1) < size && file->read(&ch, 1) == 1) {
    // Delete CR.
    if (ch == '\r') {
      continue;
    }
    str[n++] = ch;
    if (strchr(delim, ch)) {
      break;
    }
  }
  str[n] = '\0';
  return n;
}

void processrequest(char * , EthernetClient *client)
{
  // open requested web page file
  char request[12] = {0};
  Serial.print("validate request:");
  validaterequest( HTTP_req, request );
  Serial.println(request);

  if (SD.exists( request))       //check for requested file
  {
    Serial.print("open this file:");
    Serial.println(request);
    sendheader(request, client);    //send header depending on file type
    webFile = SD.open(request);

    if (webFile)
    {
      while (webFile.available())
      {
        client->write(webFile.read()); // send web page to client
      }
      webFile.close();
    }
  }

  else if (StrContains(request, "ajax"))   //serve ajax call
  {
    processaction(request, client);
  }
  else
  { //send 404
    Serial.println("404");
    client->write("HTTP/1.1 404 Not Found\n");
    client->write("Content-Type: text/html\n");
    client->write("Connnection: close\n");
    client->write("\n");
  }
}

void updatedata( char *str, File dataFile) //consider returning bool for success
{
  if (dataFile) {
    dataFile.println( str );
    dataFile.close();
  }
  else {
//    resetLCD();
//    lcd.print("Error opening file");
    Serial.println("Error opening file");
  }
}

void serveclient(EthernetClient *client)
{
  char * request;
  boolean currentLineIsBlank = true;
  boolean FileRequest = false;
  int req_index = 0;

  while (client->connected())
  {
    if (client->available())
    {
      getclientdata(client); //fills HTTP buffer
      if ((HTTP_req[req_index] == '\n') && currentLineIsBlank) //is this the end of data?  //use hex values?
      {
        processrequest(HTTP_req, client);        //serve request
        currentLineIsBlank = false;
        req_index = 0;
        request_index = 0;
        StrClear(HTTP_req, REQ_BUF_SZ);
        break;
      }
      if (HTTP_req[req_index] == '\n')      //last character is a /n /n
      {
        currentLineIsBlank = true;
      }
      else if (HTTP_req[req_index] != '\r')         // a text character was received from client
      {
        currentLineIsBlank = false;
      }
      req_index++;
    }// end if (client.available())
  } // end while (client.connected())
  delay(1);  // give the web browser time to receive the data
  client->stop();  // close the connection:
}

char * getclientdata(EthernetClient *client)
{
  char c = client->read(); // read 1 byte (character) from client
  // buffer first part of HTTP request in HTTP_req array (string)
  // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)

  if (request_index < (REQ_BUF_SZ - 1))
  {
    HTTP_req[request_index] = c;          // save HTTP request character
    request_index++;
  }
  return HTTP_req;
}

void validaterequest(char * HTTP_req, char * request)   //TODO : add ajaxhandler to improve efficiency
{
  word i = 0;
  byte j = 0;
  boolean isRequest = false;
  while (HTTP_req[i] != '\n')
  {
    if (HTTP_req[i] == '/')
    { //start recording
      isRequest = true;
      if (HTTP_req[i + 1] == ' ') //if current letter / and next blank send index
      {
        strcpy( request, "index.htm");
        j = 9;
      }
    }
    else if (isRequest && HTTP_req[i] == '?') //beginning of post action
    {
      request[j] = 0;
      i++;
      char postAction[MAX_POST_LENGTH] = {0};
      int m = 0;
      while (HTTP_req[i] != ' ')
      {
        if (HTTP_req[i] != '&')      //break up the string by ampersands and request those
        {
          postAction[m] = HTTP_req[i] ;
          m++;
        }
        else
        {
          Serial.println(postAction);
          processPostAction(postAction);
          postAction[m] = 0;
          m = 0;
        }
        i++;
      }
      Serial.println(postAction);
      processPostAction(postAction);
      postAction[m] = 0;
      m = 0;
      break;
    }
    else if (isRequest && HTTP_req[i] == ' ') //end of file name
    {
      request[j] = 0;
      break;
    }
    else if (isRequest)
    { //get filename/action details
      request[j] = HTTP_req[i];
      j++;
    }
    i++;
  }
}

void processPostAction(char *postrequest)
{
  char* token;
  int i = 0;
  token = strtok(postrequest, "=");
  token = strtok(NULL, "=");
  if (token == NULL)
  {
    return;
  }
  while (strcmp(poststrings[i], postrequest) && i < (sizeof(poststrings) / sizeof(poststrings[0])) ) //findout action integer match use case
  {
    i++;
  }
  Serial.print("do this action:");
  Serial.println(poststrings[i]); //request
  Serial.println(token);
  switch (i)
  {
    case 0: //contp aka CPressure
      localSettings.pressure2 = strtol(token, (char **)NULL, 10);
      setPressures2();
      break;
    case 1: //final pressure aka FPressure
      localSettings.pressure3 = strtol(token, (char **)NULL, 10);
      setPressures3();
      break;
    case 2: //rest time aka rest
      localSettings.Rest = strtol(token, (char **)NULL, 10);
      setRest();
      break;
    case 3: //touchp
      localSettings.pressure1= strtol(token, (char **)NULL, 10);
      setPressures1();
      break;
    case 4: //calibration pressure
      localSettings.calibration_pressure = strtol(token, (char **)NULL, 10);
      setCalibrationPressure();
      break;  
  }
}

void sendheader(char *request, EthernetClient *client)
{
  if (StrContains(request, ".htm")) //TODO fix this so that index is read correctly
  {
    client->write("HTTP/1.1 200 OK\n");
    client->write("Content-Type: text/html\n");
    client->write("Connnection: close\n");
    client->write("\n");
  }
  else if (StrContains(request, ".js"))
  {
    client->write("HTTP/1.1 200 OK\n");
    client->write("Content-Type: text/js\n");
    client->write("Connnection: close\n");
    client->write("\n");
  }
  else if (StrContains(request, ".css"))
  {
    client->write("HTTP/1.1 200 OK\n");
    client->write("Content-Type: text/css\n");
    client->write("Connnection: close\n");
    client->write("\n");
  }
  else if (StrContains(request, ".png")
           || StrContains(request, ".ico")
           || StrContains(request, ".mp4"))
  {
    client->write("HTTP/1.1 200 OK\n");
    client->write("\n");
  }
}

void processaction(char HTTP_req[REQ_BUF_SZ], EthernetClient *client)
{
  int i = 0;

  while (strcmp(actions[i], HTTP_req) && i < (sizeof(actions) / sizeof(actions[0])) ) //findout action integer match use case
  {
    i++;
  }
  Serial.print("do this action:");
  Serial.println(actions[i]);
  switch (i) {

    case 0 : //ip
      client->print( Ethernet.localIP());
      break;

    case 1 : //subn
      client->print( Ethernet.subnetMask());
      break;

    case 2 : //gate
      client->print( Ethernet.gatewayIP());
      break;

    case 3 :  //changecount
      client->print(growerSettings.changeCount);
      break;

    case 4 :  //Grower
      client->print(growerSettings.growerName);
      break;

    case 5 : //Lot
      client->print(growerSettings.currentLot);
      break;

    case 6 : //pressure1
      client->print(localSettings.pressure1);
      break;

    case 7 : //remaining
      client->print(growerSettings.totalLot - growerSettings.currentLot);
      break;

    case 8 : //pressure2
      client->print(localSettings.pressure2);
      break;

    case 9 : //rest
      client->print(localSettings.Rest);
      break;

    case 10 : //wunits
      client->print("10");
      break;

    case 11 : //dunits
      client->print("11");
      break;

    case 12 : //pressure3
      client->print(localSettings.pressure3);
      break;

    case 13 : //receiveip
      client->print("13");
      break;

    case 14 : //a0
      client->print("14");
      break;

    case 15 : //graph
      client->print("15");
      break;

    case 16 : //table0 reset table TODO : FIX THIS SO THAT IT DISPLAYS AVAILABLE PAGES
      requestint = 0;
      //current index
      client->print("#");
      client->print(' ');
      client->print("Grower");
      client->print(' ');
      client->print("Variety");
      client->print(' ');
      client->print("Time");
      client->print(' ');
      break;

    case 17 : //nexttable
      requestint++;   //requestint not a usefull because they dont reset
      {
        Serial.print(requestint);
        struct lotSettings nextGrower = loadGrowerSettings(growerSettings, growerFile, requestint);
        if (strlen(nextGrower.growerName))
        {
          client->print(nextGrower.currentLot);
          client->print(' ');
          client->print(nextGrower.growerName);
          client->print(' ');
          client->print(nextGrower.totalLot);
          client->print(' ');
          client->print(nextGrower.changeCount);
        }
        else
        {
          client->print("");
          client->print(' ');
          client->print("");
          client->print(' ');
          client->print("");
          client->print(' ');
          client->print("");
        }
      }
      break;

//    case 18 : //table1
//      requestint = 0;
//      //
//      client->print("0");
//      client->print(' ');
//      client->print(requestint);
//      client->print(' ');
//      client->print("16");
//      client->print(' ');
//      client->print("24");
//      client->print(' ');
//      break;

    case 18 :     //ajax calib
      client->print(localSettings.calibration_pressure);
      break;

    default :
      Serial.println("404");
      client->write("HTTP/1.1 404 Not Found\n");
      client->write("Content-Type: text/html\n");
      client->write("Connnection: close\n");
      client->write("\n");
  }
}

/*/////////////////////////////////////////////////////////////////////////////////////////
  COMMUNICATION FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////*/

char * headtobridge()
{
  static char headstr[5];  //must be one more than message length
  int headDataLength = 0;
  StrClear(headstr, 5);
  while(head3.available())
  {
    char headData = head3.read();
    //Serial.println(slaveData, HEX);
    if( headData == '\0' )
    {
      delay(1);
      headData = head3.read();
      //Serial.println(slaveData, HEX);
    }
    if( headData == '\n' )
    {
      break;
    }
    headstr[headDataLength] = headData;
    headDataLength++;
  }
  return headstr;
}

/*/////////////////////////////////////////////////////////////////////////////////////////
  FRUIT TESTING FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////*/

void runmode()        //Statemachine TODO: Add height and pressure calibration (cal+ cal- commands) settings for head, add option to view current network settings,
{
    switch (mode)
    {
      case 0:                 //DEFAULT  //testfruit:
        switch(submode)
        {
          case 0:             //normal test
            testfirmness();
            break;
          case 1:
            //restart current grower
            break;
          case 2:             //select next grower
            break;
          case 3:                    //back to 2
            //do nothing
            break;
          default:
            break;
        }
          break;
        
      case 1:    //SD
        switch(submode)
        {
          case 0:
            //do nothing
            break;
          case 1:
            //do nothing          
            break;            
          default:
            break;
        }
          break;
      case 2:  //testfruit:
        switch(submode)
        {
          case 0:
            //do nothing
            break;
          default:
            //do nothing
            break;
        }
          break;
      
      case 3:         //network
        switch(submode)
        {
          case 0: //more info
            mode = 8;
            break;
          default:
            //do nothing
            break;
        }
          break;
      
      case 4:    //FOOTPEDAL
        switch(submode)
        {
          case 0:
            //do nothing
            break;
          case 1:
            //do nothing          
            break;
          default:
            //do nothing          
            break;
        }
          break;          
        
      case 5:          //head1
        switch(submode)
        {
          case 0:
            mode = 9;
            submode = 0;
            break;
          case 1:
            //do nothing          
            break;            
          default:
            //do nothing
            break;
        }
          break;
      case 6:         //head2
        switch(submode)
        {
          case 0:
            mode = 10;
            submode = 0;
            break;
          case 1:
            //do nothing          
            break;            
          default:
            break;
        }
          break;
      
      case 7:          //head3
        switch(submode)
        {
          case 0:
            mode = 11;
            submode = 0;
            break;
          case 1:
            //do nothing          
            break;              
          default:
            //do nothing          
            break;
        }
          break;
        
      case 8:             //network mode
        switch(submode)
        {
          case 0:
            //do nothing          
            break;
          case 1:
            //do nothing          
            break;  
          default:
            //do nothing          
            break;
        }
          break;
          
      case 9:
        switch(submode)   //head 1 calibration mode
        {
          case 0:
            //start calibration
            break;
          case 1:
            //decrease calibration         
            break;
          case 2:
            //increase calibration         
            break;                 
          default:
            //do nothing          
            break;
        }
        break;
      case 10:
        switch(submode)   //head 2 calibration mode
        {
          case 0:
            //start calibration         
            break;
          case 1:
            //decrease calibration         
            break;
          case 2:
            //increase calibration         
            break;                 
          default:
            //do nothing          
            break;            
        }
        break;
      case 11:
        switch(submode)   //head 3 calibration mode
        {
          case 0:
            //start calibration
            calibrateHead3();
            break;
          case 1:
            localSettings.h3Calib--; 
            //decrease calibration         
            break;
          case 2:
            localSettings.h3Calib++; 
            //increase calibration         
            break;                 
          default:
            //do nothing          
            break;            
        }
        break;          
    }
    

    //load turn table by slow stepping 3/4 of entire rotation
    //step turntable to find first fruit
    //back up till fruit is under head 3
    //test all fruit on turn table
    //move one slot forward and repeat
    //eject all fruit
    
    //lcd.setCursor(0,0);
    //lcd.print("  Testing Firmness  ");
    //if (fruit == 0)
    //{
      //lcd.setCursor(0,1);  
      //lcd.print("Finding First Fruit ");
      //findposition();
      //lcd.setCursor(0,1);
      //lcd.print("Aligning Table Heads");
      //stepbacktohead();
      //lcd.setCursor(0,1)
      //lcd.print("Start Firmness Test ");
    //}

    //lcd.setCursor(0,1);  
    //lcd.print("  H1     H2     H3  ");
    //lcd.print("XXX.XX XXX.XX XXX.XX");
    //lcd.print("XXX.XX XXX.XX XXX.XX");

    //if (fruit == 24)
    //{
    //  kickfruit();
    //  fruit--;
    //}


}

char * getw()             //TODO: synchronize heads
{
  //  static char h0str[7];  //must be one more than message length
  int h1DataLength = 0;
  //  static char h1str[7];  //must be one more than message length
  int h2DataLength = 0;
  //  static char h2str[7];  //must be one more than message length
  int h3DataLength = 0;

  StrClear(h1str, 7);
  StrClear(h2str, 7);
  StrClear(h3str, 7);

//  if(localSettings.head1)
// {
//    int timeout = 0;
//    char headId[4];
//    
//    digitalWrite(HEAD1EN, HIGH);
//    while(!head1.available() && (timeout <= 250))
//    {
//      delay(1);
//      timeout++;
//      lcd.setCursor(0,3);
//      lcd.print(timeout);
//    }
//    digitalWrite(HEAD1EN, LOW);
//    if(timeout < 250)
//    {
//      for(int i = 0; i < 4; i++)
//      {
//        headId[i] = char(head1.read());
//        delay(1);
//      }
//      //what should go here calibration number?
//      head1.write("test");
//      while(!head1.available())
//      {
//        delay(1);
//      }
//      while (head1.available())
//      {
//        char h1data = head1.read();
//                Serial.print("->");
//                Serial.print(h1data);
//                Serial.print("<-");
//        if ( h1data == '\r' )
//        {
//          break;
//        }
//        if ( h1data == '\0' )
//        {
//          break;
//        }
//        if ( h1data != 0xFF )
//        {
//          h1str[h1DataLength] = h1data;
//          h1DataLength++;
//        }
//  
//      }
//    }
//  }  
//  if(localSettings.head2)
//  {
//    int timeout = 0;
//    char headId[4];
//    
//    digitalWrite(HEAD2EN, HIGH);
//    while(!head2.available() && (timeout <= 250))
//    {
//      delay(1);
//      timeout++;
//      lcd.setCursor(0,3);
//      lcd.print(timeout);
//    }
//    digitalWrite(HEAD2EN, LOW);
//    if(timeout < 250)
//    {
//      for(int i = 0; i < 4; i++)
//      {
//        headId[i] = char(head2.read());
//        delay(1);
//      }
//      //what should go here calibration number?
//      head2.write("test");
//      while(!head2.available())
//      {
//        delay(1);
//      }
//      while (head2.available())
//      {
//        char h2data = head2.read();
//        delay(1);
//                Serial.print("->");
//                Serial.print(h2data);
//                Serial.print("<-");
//        if ( h2data == '\r' )
//        {
//          break;
//        }
//        if ( h2data == '\0' )
//        {
//          break;
//        }
//        if ( h2data != 0xFF )
//        {
//          h2str[h2DataLength] = h2data;
//          h2DataLength++;
//        }
//  
//      }
//    }
//  }  
  if(localSettings.head3)
  {
    int timeout = 0;
    char headId[4];
    
    digitalWrite(HEAD3EN, HIGH);
    delay(10);
    digitalWrite(HEAD3EN, LOW);
    while(!head3.available() && (timeout <= 250))
    {
      delay(1);
      timeout++;
      lcd.setCursor(0,3);
      lcd.print(timeout);
    }

    if(timeout < 250)
    {
      int i = 0;
      while (head3.available())
      {
//        for(int i = 0; i < 4; i++)
//        {

          headId[i] = char(head3.read());
          
          Serial.print(headId[i]);
          delay(1);
          i++;
          //head3.flush();
//        }
      }
      //what should go here calibration number?
      delay(1);
      head3.write("test");
      timeout = 0;
      while(!head3.available() && (timeout < 5000))
      {
        delay(1);
        timeout++;
        lcd.setCursor(0,3);
        lcd.print(timeout);        
      }
      while (head3.available())
      {
        char h3data = head3.read();
        delay(1);
                Serial.print("->");
                Serial.print(h3data);
                Serial.print("<-");
        if ( h3data == '\r' )
        {
          break;
        }
        if ( h3data == '\0' )
        {
          break;
        }
        if ( h3data != 0xFF )
        {
          h3str[h3DataLength] = h3data;
          h3DataLength++;
        }
  
      }
      head3.flush();
      Serial.print("here");
    }
  }  
  return h3str;
}


//additional commands
//sts1
//sts2
//sts3

char * setPressures1()  //TODO: Confim function works, likely issue with head
{

  int timeout = 0;
  char headId[4];
  StrClear(h3str, 7);
  int h3DataLength = 0;
  
  digitalWrite(HEAD3EN, HIGH);
  while(!head3.available() && (timeout <= 250))
  {
    delay(1);
    timeout++;
    lcd.setCursor(0,3);
    lcd.print(timeout);
  }
  digitalWrite(HEAD3EN, LOW);
  if(timeout < 250)
  {
    for(int i = 0; i < 4; i++)
    {
      headId[i] = char(head3.read());
      delay(1);
    }
    //what should go here calibration number?
    head3.write("stw1");

    while(!head3.available())
    {
      delay(1);
    }
    while (head3.available())
    {
      char h3data = head3.read();
      delay(1);
      Serial.print(h3data);
      if ( h3data == '\r' )
      {
        break;
      }
      if ( h3data == '\0' )
      {
        break;
      }
      if ( h3data != 0xFF )
      {
        h3str[h3DataLength] = h3data;
        h3DataLength++;
      }

    }
  }
  digitalWrite(HEAD3EN, HIGH);
  timeout = 0;
  while(!head3.available() && (timeout <= 250))
  {
    delay(1);
    timeout++;
    lcd.setCursor(0,3);
    lcd.print(timeout);
  }
  digitalWrite(HEAD3EN, LOW);
  if(timeout < 250)
  {
    for(int i = 0; i < 4; i++)
    {
      headId[i] = char(head3.read());
      delay(1);
    }
  head3.write(localSettings.pressure1);
  head3.write('\n');

  while(!head3.available())
  {
    delay(1);
  }
  while (head3.available())
  {
    char h3data = head3.read();
            Serial.print(h3data);
    if ( h3data == '\r' )
    {
      break;
    }
    if ( h3data == '\0' )
    {
      break;
    }
    if ( h3data != 0xFF )
    {
      h3str[h3DataLength] = h3data;
      h3DataLength++;
    }

  }
  }
  return h3str;
} 

char * setPressures2()  //TODO: POPULATE FUNCTION
{
  
}

char * setPressures3()  //TODO: POPULATE FUNCTION
{
  
}

char * setRest()  //TODO: POPULATE FUNCTION
{
  
}

char * setCalibrationPressure()  //TODO: Confim function works, likely issue with head
{

  int timeout = 0;
  char headId[4];
  StrClear(h3str, 7);
  int h3DataLength = 0;
  
  digitalWrite(HEAD3EN, HIGH);
  while(!head3.available() && (timeout <= 250))
  {
    delay(1);
    timeout++;
    lcd.setCursor(0,3);
    lcd.print(timeout);
  }
  digitalWrite(HEAD3EN, LOW);
  if(timeout < 250)
  {
    for(int i = 0; i < 4; i++)
    {
      headId[i] = char(head3.read());
      delay(1);
    }
    //what should go here calibration number?
    head3.write("setc");
    head3.write("0100");
    while(!head3.available())
    {
      delay(1);
    }
    while (head3.available())
    {
      char h3data = head3.read();
              Serial.print("->");
              Serial.print(h3data);
              Serial.print("<-");
      if ( h3data == '\r' )
      {
        break;
      }
      if ( h3data == '\0' )
      {
        break;
      }
      if ( h3data != 0xFF )
      {
        h3str[h3DataLength] = h3data;
        h3DataLength++;
      }

    }
  }
  return h3str;
}  


char * calibrateHead3()  //TODO: Confim function works, likely issue with head
{
  int timeout = 0;
  char headId[4];
  StrClear(h3str, 7);
  int h3DataLength = 0;
  
  digitalWrite(HEAD3EN, HIGH);
  while(!head3.available() && (timeout <= 250))
  {
    delay(1);
    timeout++;
    lcd.setCursor(0,3);
    lcd.print(timeout);
  }
  digitalWrite(HEAD3EN, LOW);
  if(timeout < 250)
  {
    for(int i = 0; i < 4; i++)
    {
      headId[i] = char(head3.read());
      delay(1);
    }
    //what should go here calibration number?
    head3.write("cali");
    while(!head3.available())
    {
      delay(1);
    }
    while (head3.available())
    {
      char h3data = head3.read();
              Serial.print("->");
              Serial.print(h3data);
              Serial.print("<-");
      if ( h3data == '\r' )
      {
        break;
      }
      if ( h3data == '\0' )
      {
        break;
      }
      if ( h3data != 0xFF )
      {
        h3str[h3DataLength] = h3data;
        h3DataLength++;
      }

    }
  }
  return h3str;
}  

void testfirmness()   //TODO: UPDATE FUNCTION TO TESTING PROCEDURE USING LOT SETTINGS PARAMETERS
{
  digitalWrite(DIR, LOW);
  digitalWrite(EN, LOW);  //???????
  digitalWrite(MS1, LOW); //Pull MS1, and MS2 high to set logic to fullstep resolution
  digitalWrite(MS2, LOW);
  digitalWrite(MS3, LOW);

//  if (samples <= changecount)    //move fruit under heads
//  {
//    //load fruit onto turn table
//    steps = 0;
//    resetLCD();
//    lcd.write("Loading Samples ");
//    lcd.write("        ");
//    Serial.println("Loading Samples ");
//    if (samples >= 10)    //align the count
//    {
//      lcd.write(254);
//      lcd.write(16);
//    }
//    lcd.print(samples);
//    lcd.write("/");
//    lcd.print(changecount);
//    
//    Serial.print(samples);
//    Serial.print("/");
//    Serial.print(samples);
//    
//    samples++;
//
//    while (analogRead(PRES) <= 900) //turn the turntable to next cherry
//    //USE THE PHOTO RESISTOR
//    {
//      steps++;
//      digitalWrite(STP, HIGH); //Trigger one step forward
//      delay(1);
//      digitalWrite(STP, LOW); //Pull step pin low so it can be triggered again
//      delay(1);
//    }
//    //step back
//
//  }
//  else                  //start sampling
//  {

    //resetLCD();
    Serial.println();
////    if (new_lot)                      //what to display on the screen
////    {
////      //lcd.write("Ready To Sample New Lot"); //first sample of lot
////      Serial.print("Ready To Sample New Lot");
////    }
//    else if ( (samples - (changecount / 3)) <= changecount)           //samples in lot limit  //TODO: REMOVE CHANGECOUNT VARIABLE
//    {
//      //lcd.write("Next Sample"); //first sample of lot
//      Serial.print("Next Sample");
//    }
//    else                              //last sample
//    {
//      //lcd.write("Last Sample"); //first sample of lot
//      Serial.print("Last Sample");
//      samples = 0;
//      new_lot = true;
//    }

//    steps = 0;
//    while (steps <= 100 && !new_lot)
//    {
//      steps++;
//      digitalWrite(STP, HIGH); //Trigger one step forward
//      delay(1);
//      digitalWrite(STP, LOW); //Pull step pin low so it can be triggered again
//      delay(1);
//    }

    //updatedata( getw(), dataFile );
    File dataFile = SD.open("DATALOG.TXT", FILE_WRITE);   //open data log file
    if (dataFile) {
      str = getw();
      dataFile.println( str );
      dataFile.close();
      Serial.print(str);
    }
    else {
      //resetLCD();
      //lcd.print("Error opening file");
      Serial.println();
      Serial.print("Error opening file");
    }

    //resetLCD();

//    if (new_lot && samples > 0)
//    {
//      //lcd.print("Getting first   sample");
//      Serial.print("Getting first   sample");
//      new_lot = false;
//    }
    //        else if (samples >= (samples - (changecount / 3)) <= changecount)
    //        {
    //          lcd.print("Getting last    sample");
    //
    //        }
//    else
//    {
      //lcd.setCursor(0,2);
      //lcd.print(h1str);
//      Serial.print(h1str);
//      Serial.write(", ");
//      lcd.setCursor(0,2);
//      lcd.print("                    ");
//      lcd.setCursor(0,2);
//      lcd.print(h2str);
//      Serial.print(h2str);
    lcd.setCursor(0,3);
      lcd.print("                    ");
      lcd.setCursor(0,3);
      lcd.print(h3str);    
      Serial.write(", ");
      Serial.print(h3str);
      delay(5000);

//    }
//    samples++;

  //}
}

void findposition()   //TODO: Confirm Distances
{
  digitalWrite(DIR, LOW);
  digitalWrite(EN, LOW);
  digitalWrite(MS1, LOW);
  digitalWrite(MS2, HIGH);
  digitalWrite(MS3, LOW);
  //load fruit onto turn table
  Serial.write("Finding First Sample");
  int steps = 0;
  while ( analogRead(PHOTO1) <= 900 && steps <= 10000) //turn the turntable to next cherry
  {
    steps++;
    digitalWrite(STP, HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(STP, LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  delay(100);                       //why is this delay here
  digitalWrite(DIR, HIGH);
  steps = 0;
  while ( steps <= 1000 )  //reverse table certain distance TODO: distance should depend on number/postions of heads
  {
    steps++;
    digitalWrite(STP, HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(STP, LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  digitalWrite(DIR, LOW);
}

void kickfruit()
{
  digitalWrite(DIR, LOW);
  digitalWrite(EN, LOW);
  digitalWrite(MS1, LOW); //Pull MS1, and MS2 high to set logic to fullstep resolution
  digitalWrite(MS2, LOW);
  digitalWrite(MS3, LOW);
  //load fruit onto turn table
  steps = 0;
  //resetLCD();
  //lcd.write("Kicking all Samples");
  Serial.write("Finding First Sample");
  //Serial.println(analogRead(PRES)) 
  while ( analogRead(PHOTO1) <= 900 ) //turn the turntable to next cherry
  {
    //steps++;
    digitalWrite(STP, HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(STP, LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  delay(250);           //settling time
  digitalWrite(DIR, HIGH);
  //digitalWrite(MS2, HIGH);
  //digitalWrite(MS3, HIGH);
    while (analogRead(PHOTO1) >= 900) //turn the turntable to next cherry
    //USE THE PHOTO RESISTOR
    {    
      digitalWrite(SOLO, 1);
      delay(300);
      digitalWrite(SOLO, 0);
      delay(100);
    }
  //reverse table certain distance
}

