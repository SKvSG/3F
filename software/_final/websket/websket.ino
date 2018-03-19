#include <avr/io.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <stdio.h>

#define REQ_BUF_SZ   2000 // size of buffer used to capture HTTP requests

#define lcd Serial1  //pin 1 for lcd pin 0 for firmness head 1
#define head1 Serial2
#define head2 Serial3

//Serial3.
//altsoftserial  //recieve pin 20   //firmness head 3
#define MAX_LENGTH  64
#define MAX_SETTINGS  8
#define GROWER_SETTINGS  5
#define MAX_POST_LENGTH 8
/*/////////////////////////////////////////////////////////////////////////////////////////
* PIN ASSIGNMENTS
*//////////////////////////////////////////////////////////////////////////////////////////

//int FirmnessHead1RX = 0;
//int LCDTX = 1;
//int ?? = 2;
//int BTN1 = 3;
//int SDCS = 4;
//int ?? = 5;
int BTN1 = 6;
//int FirmnessHead3RX = 7;
//int LeftPad?? = 8;
//int WRESET = 9;
//int WCS = 10;
//int DOUT = 11;
//int DIN = 12;
//int DSCK = 13;
int MS3 = 14;
int MS1 = 15;
int STP = 16;
int DIR = 17;
int MS2 = 18;
int EN = 19;
int HEAD0EN = 20;
int HEAD1EN = 21;
int HEAD2EN = 22;
int BTN2 = 23;
//int ?? = 24;
//int ?? = 25;
//int FirmnessHead2RX = 26;
//int ?? = 27;
//int ?? = 28;
//int ?? = 29;
//int ?? = 30;
//int FirmnessHead2TX = 31;
//int ?? = 32;
//int ?? = 33;

/*/////////////////////////////////////////////////////////////////////////////////////////
* GLOBAL VARIABLES
*//////////////////////////////////////////////////////////////////////////////////////////
struct machineSettings
{
    int array[100];
    byte mac[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
 //   IPAddress ip(10, 34, 10, 131);
    int CPressure = 2;
    int FPressure = 10;
    int Calib = 25;
    int Rest = 0;
    int WUnits = 453.592; //conversion factor * gram = lbs
    int DUnits = 25.4; //conversion factor * inches = mm TODO: add rod conversion factor
} localSettings; 

struct lotSettings
{
    int changeCount = 25;
    char * growerName = "STEMILT";
    int currentLot = 0;
    int totalLot = 10;
} growerSettings; 

File settingsFile;
File growerFile;
static char h0str[7];  //must be one more than message length
static char h1str[7];  //must be one more than message length
static char h2str[7];  //must be one more than message length
int changecount = 25;  //TODO: add to grower struct

EthernetServer server(80);  // create a server at port 80
File webFile;
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
int request_index = 0;              // index into HTTP_req buffer
char* actions[] = { "ajax_ip", "ajax_subn", "ajax_gate", "ajax_changecount", "ajax_Grower", "ajax_Lot", 
                    "ajax_CPressure", "ajax_Remaining", "ajax_FPressure", "ajax_Rest", "ajax_WUnits", 
                    "ajax_DUnits", "ajax_Calib", "ajax_receiveip", "ajax_a0", "ajax_a1", "ajax_a2", "ajax_a3", 
                    "ajax_a4", "404"};
char* machineSets[] = { "mac", "IPAddress", "FPressure", "CPressure", "Calib", "Rest", "WUnits", "DUnits"}; 
char* growerSets[] = { "growerName", "currentLot", "totalLot", "changeCount"}; 
int animation_step = 0;
unsigned long previousMillis = 0;
char *str;  //must be one more than message length
int samples = 0;
int steps = 0;
int headcount = 2;
int head = 0;
bool new_lot = true;
// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found


/*/////////////////////////////////////////////////////////////////////////////////////////
* FUNCTION DECLARATIONS
*//////////////////////////////////////////////////////////////////////////////////////////
void initializePins();                        //set inputs and outputs
void initializeSD();                          //***check for SD card and needed files on SD (neds to check for more files)
//void initializeSerial();
//struct machineSettings loadSettings(struct machineSettings);                          //load settings set in the config file or sets to default value
void loadMachineSettings(struct machineSettings *, File);                          //load settings set in the config file or sets to default value
struct lotSettings loadGrowerSettings(struct lotSettings, File);                          //load settings set in the config file or sets to default value
void initializeNetwork(struct machineSettings);                     //start network adapter check connectivity
struct machinesettings initializeNetwork(struct machinesettings);                     //start network adapter check connectivity
void resetLCD();                              //clear text and symbols from the lcd
void serveclient(EthernetClient *client);      //recieve request from client
char * getclientdata(EthernetClient *client);
void processrequest(char * );
//void processrequest(char *, machinesettings)
const char * validaterequest(char *);
const char * processaction(char HTTP_req[REQ_BUF_SZ], EthernetClient *client); //***handle ajax requests TODO: REMOVE ETHERNETCLIENT RETURN STRING
//const char * processaction(char HTTP_req[REQ_BUF_SZ], machinesettings); //***handle ajax requests TODO: REMOVE ETHERNETCLIENT RETURN STRING
char StrContains(char *str, char *sfind);     //check request for string
void StrClear(char *str, char length);        //check if final line of request
char * getw();                                //request pressure reading
void testfirmness();                          //run the firmness testing procedure
void idleanim();
void sendheader(char *request,EthernetClient *client);
/*/////////////////////////////////////////////////////////////////////////////////////////
* MAIN
*//////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
    //initializeLCD();
    initializePins();
    initializeSerial();
    initializeSD();
    loadMachineSettings(&localSettings, settingsFile);
    growerSettings = loadGrowerSettings(growerSettings, growerFile);
    Serial.println(growerSettings.growerName);
    initializeNetwork(localSettings);
    delay(2000);
    resetLCD();
    lcd.write("Push Button to  Begin Test ->");
    digitalWrite(EN, HIGH);
    digitalWrite(HEAD1EN, LOW);
}

void loop()
{
    //server.statusreport();
    EthernetClient client = server.available();  // try to get client
    //Serial.print(int(client));
    if (client)   // serve client website
    {
        serveclient(&client);
        //break;
    }
//    else if (digitalRead(BTN1) == 0) // continue testing firmness
//    {
//        testfirmness();
//    }
//    
//    else { //idle animation
//        idleanim();
//    }
  //}
}

/*/////////////////////////////////////////////////////////////////////////////////////////
* FUNCTIONS
*//////////////////////////////////////////////////////////////////////////////////////////
void initializePins()
{
    pinMode(BTN1, INPUT_PULLUP);
    pinMode(BTN2, INPUT_PULLUP);
    pinMode(STP, OUTPUT);
    pinMode(DIR, OUTPUT); 
    pinMode(MS2, OUTPUT);
    pinMode(EN , OUTPUT);;
    pinMode(MS1 , OUTPUT);
    pinMode(MS3 , OUTPUT);
    pinMode(HEAD0EN , OUTPUT);
    pinMode(HEAD1EN , OUTPUT);
    pinMode(HEAD2EN , OUTPUT);
    head1.setRX(26);  //firmness head 1
    head1.setTX(31);  
    pinMode(9, OUTPUT);
    digitalWrite(9, LOW);    // begin reset the WIZ820io
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);  // de-select WIZ820io
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);   // de-select the SD Card
    digitalWrite(9, HIGH);   // end reset pulse
}

void initializeSerial()
{
    Serial.begin(9600);       // for debugging
    lcd.begin(9600);  //lcd and firmnesshead 0
    head1.begin(9600);
    head2.begin(9600); //firmness head 2
}

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

void resetLCD()
{
  lcd.write(0xFE);
  lcd.write(0x01);
  lcd.write(254); // move cursor to beginning of first line
  lcd.write(128);
}

void initializeLCD()
{
  lcd.write(0x7C);
  lcd.write(0x04);
  lcd.write(0x7C);
  lcd.write(0x06);
  lcd.write(0x7C);
  lcd.write(0x0A);
  lcd.print("3F Fast Firmness Finder");
  lcd.flush();
//  lcd.write(0x7C);
//  lcd.write(0x04);
//  lcd.write(0x7C);
//  lcd.write(0x06);
//  lcd.write(0x7C);
//  lcd.write(157);
  resetLCD();
}

void initializeSD()
{
    // initialize SD card
    lcd.print("Initializing SD card...");       
    delay(1250);
    if (!SD.begin(4)) {
        resetLCD();
        lcd.print("ERROR - SD card failed!");
        delay(1250);
        return;    // init failed
    }
    resetLCD();
    lcd.print("SUCCESS - SD card initialized.");
    Serial.println("SUCCESS - SD card initialized.");
    delay(1250);

    if (!SD.exists("DATALOG.TXT")) {
      resetLCD();
      lcd.print("ERROR - Can't find log file!");
      Serial.print("ERROR - Can't find log file!");
      delay(1250);
      
    }
    resetLCD();
    lcd.print("SUCCESS - Found log file.");
    Serial.println("SUCCESS - Log file found.");
    delay(1250);
}

void loadMachineSettings(struct machineSettings *localSettings, File settingsFile)
{
    char str[MAX_SETTINGS][MAX_LENGTH]; //longest field plus delimiter
    int i = 0;
    char *token;
    if (!SD.exists("settings.txt")) {
        resetLCD();
        lcd.print("ERROR - Can't find settings.txt file!");
        delay(1250);
//        return localSettings;  // can't find index file
    }
    resetLCD();
    Serial.println("SUCCESS - Found settings.txt file.");
    delay(1250);
    settingsFile = SD.open("settings.txt");
    
    while( i < MAX_SETTINGS) //read file into set of strings
    {
      size_t n = readField(&settingsFile, str[i], sizeof(str), "\n");
      i++;
    }
    int k = 0;
    while(k < MAX_SETTINGS) //turn strings into settings
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
              while( l < sizeof(localSettings->mac))
              {
                token = strtok(NULL, " ");
                localSettings->mac[l] =  strtol(token, (char **)NULL, 16) ;
                l++;
              }
              break;
              
            case 1 : //IPAddress
              //token = strtok(NULL, " ");
              break;

            case 2 : //FPressure
              token = strtok(NULL, " ");
              localSettings->FPressure = strtol(token, (char **)NULL, 10);
              break;
              
            case 3 : //CPressure
              token = strtok(NULL, " ");
              localSettings->CPressure = strtol(token, (char **)NULL, 10);
              break;

            case 4 : //Calib
              token = strtok(NULL, " ");
              localSettings->Calib = strtol(token, (char **)NULL, 10);
              break;
              
            case 5 : //Rest
              token = strtok(NULL, " ");
              localSettings->Rest = strtol(token, (char **)NULL, 10);
              break;
              
            case 6 : //WUnitst
              token = strtok(NULL, " ");
              localSettings->WUnits = strtol(token, (char **)NULL, 10);
              break;

            case 7 : //DUnits
              token = strtok(NULL, " ");
              localSettings->WUnits = strtol(token, (char **)NULL, 10);
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
    //return localSettings;
}; 

struct lotSettings loadGrowerSettings(struct lotSettings growerSetting, File growerFile)
{
    char str[GROWER_SETTINGS][MAX_LENGTH]; //longest field plus delimiter
    int i = 0;
    char *token;
    if (!SD.exists("growers.txt")) {
        resetLCD();
        lcd.print("ERROR - Can't find grower file!");
        Serial.println("ERROR - Can't find grower file!");
        delay(1250);
        return growerSetting;  // can't find index file
    }
    resetLCD();
    Serial.println("SUCCESS - Found Grower file.");
    growerFile = SD.open("growers.txt");
    
    while( i < GROWER_SETTINGS) //read file into set of strings
    {
      size_t n = readField(&growerFile, str[i], sizeof(str), "\n");
      i++;
    }
    int k = 0;
    while(k < GROWER_SETTINGS) //turn strings into settings
    {
      int j = 0;
      while (  j < GROWER_SETTINGS )  //findout action integer match use case
      {
        if ( strstr( str[k], growerSets[j] ) )
        {
          token = strtok(str[k], " ");
          Serial.print("Grower setting found: ");
          Serial.println(token);
          int l = 0;
          switch (j)
          {
            case 0 : //growerName
              token = strtok(NULL, " ");
              token = strtok(token, "\n");
              growerSetting.growerName = token;
              Serial.print(growerSetting.growerName);
              break;
              
            case 1 : //currentLot
              token = strtok(NULL, " ");
              growerSetting.currentLot = strtol(token, (char **)NULL, 10);
              //Serial.print(growerSetting.currentLot);
              break;

            case 2 : //totalLot
              token = strtok(NULL, " ");
              growerSetting.currentLot = strtol(token, (char **)NULL, 10);
              Serial.print(growerSetting.totalLot);
              break;
              
            case 3 : //changeCount
              while( l < growerSetting.currentLot) //problem is currentlot is not before changecount
              {
                token = strtok(NULL, " ");
                l++;
              }
              growerSetting.changeCount = strtol(token, (char **)NULL, 10);
              Serial.print(growerSetting.changeCount);
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
    return growerSetting;
}; 

size_t readField(File* file, char* str, size_t size, char* delim) 
{
  char ch;
  size_t n = 0;
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

void initializeNetwork(struct machineSettings localSettings)
{
    Ethernet.begin(localSettings.mac);  // initialize Ethernet device
    server.begin();           // start to listen for clients
    resetLCD();
    lcd.print("Server IP:      ");
    lcd.print(Ethernet.localIP());
    Serial.print("Server IP:      ");
    Serial.println(Ethernet.localIP());
}

char * getw()
{
//  static char h0str[7];  //must be one more than message length
  int h0DataLength = 0;
//  static char h1str[7];  //must be one more than message length
  int h1DataLength = 0;
//  static char h2str[7];  //must be one more than message length
  int h2DataLength = 0;

  StrClear(h0str, 7);
  StrClear(h1str, 7);
  StrClear(h2str, 7);
  
  digitalWrite(HEAD0EN, HIGH);
  digitalWrite(HEAD1EN, HIGH);
  digitalWrite(HEAD2EN, HIGH);
  delay(5);
  digitalWrite(HEAD0EN, LOW);
  digitalWrite(HEAD1EN, LOW);
  digitalWrite(HEAD2EN, LOW);
//  while ( !lcd.available() && !head1.available() && !head2.available() )
//  {
//    delay(1);
//  }
  
  if ( lcd.available() )
  {
      while(lcd.available())
      {
        char h0data = lcd.read();
//        Serial.print("->");
//        Serial.print(h0data, HEX);
//        Serial.print("<-");
        if( h0data == '\r' )
        {
          break;
        }
        if( h0data == '\0' )
        {
          break;
        }
        if( h0data != 0xFF )
        {
          h0str[h0DataLength] = h0data;
          h0DataLength++;
        }

      }
  }
//  while (!head1.available() )
//  {
//    delay(1);
//  }

  if ( head1.available() )
  {
      while(head1.available())
      {
        char h1data = head1.read();
//        Serial.print("->");
//        Serial.print(h1data, HEX);
//        Serial.print("<-");
        if( h1data == '\r' )
        {
          break;
        }
        if( h1data == '\0' )
        {
          break;
        }
        if( h1data != 0xFF )
        {
          h1str[h1DataLength] = h1data;
          h1DataLength++;
        }

      }
  }
//
//  while (!head2.available() )
//  {
//    delay(1);
//  }

  if ( head2.available() )
  {
      while( head2.available())
      {
        char h2data = head2.read();
//        Serial.print("->");
//        Serial.print(h2data, HEX);
//        Serial.print("<-");
        if( h2data == '\r' )
        {
          break;
        }
        if( h2data == '\0' )
        {
          break;
        }
        if( h2data != 0xFF )
        {
          h2str[h2DataLength] = h2data;
          h2DataLength++;
        }

      }
  }   
//  turn on all three digital pins
//  wait for serial data to hit buffer
//  store data into array


  return h2str;
}

void updatedata( char *str, File dataFile) //consider returning bool for success
{
  if (dataFile){
    dataFile.println( str );
    dataFile.close();
  }
  else{
    resetLCD();
    lcd.print("Error opening file");
  }
}
 
void processrequest(char * ,EthernetClient *client)
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
        while(webFile.available()) 
        {
          client->write(webFile.read()); // send web page to client
        }
        //Serial.println("file sent");
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
       
void validaterequest(char * HTTP_req, char * request)
{
  word i = 0;
  byte j = 0;
  boolean isRequest = false;
  while (HTTP_req[i] != '\n')
  {
    if (HTTP_req[i] == '/')
    {//start recording
      isRequest = true;
      if (HTTP_req[i+1] == ' ') //if current letter / and next blank send index
      {
        strcpy( request, "index.htm");
        j = 9;
      }
    }
    else if (isRequest && HTTP_req[i] == '?') //beginning of actions
    {
      request[j] = 0;
      i++;
      char postAction[MAX_POST_LENGTH] = {0};
      int m = 0;
      while (HTTP_req[i] != ' ')
      {
        if (HTTP_req[i] != '&')      //break up the string by ampersands and request those
        {
          postAction[m]= HTTP_req[i] ;
          m++;
        }
        else
        {
          Serial.println(postAction);
          //processPostAction(postAction);
          postAction[m] = 0;
          m = 0;
        } 
        i++;
      }
      Serial.println(postAction);
      //processPostAction(postAction);
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

void sendheader(char *request,EthernetClient *client)
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

const char * processaction(char HTTP_req[REQ_BUF_SZ], EthernetClient *client)  //return what the client should print
{
 //add contp and finap  and restt and the ability to find ?
  char *ret_string;
  int i = 0;

  while (strcmp(actions[i], HTTP_req) && i < (sizeof(actions)/sizeof(actions[0])) )  //findout action integer match use case
  {
    i++;
  }
  Serial.print("do this action:");
  Serial.println(actions[i]);
  switch (i){

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
      client->print("STEMILT");
      Serial.print(growerSettings.growerName);
      break;

    case 5 : //Lot
      client->print(growerSettings.currentLot);
      break;
            
    case 6 : //cpressure
      client->print(localSettings.CPressure);
      break;

    case 7 :
      client->print(growerSettings.totalLot-growerSettings.currentLot);
      break;

    case 8 : //fpressure
      client->print(localSettings.FPressure);
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

    case 12 : //calib
      client->print("12");
      break;
            
    case 13 :
      client->print("13");
      break;

    case 14 :
      client->print("14");
      break;

    case 15 : //a1
      client->print("15");
      break;
            
    case 16 : //a2
      client->print("16");
      break;

    case 17 :
      client->print("17");
      break;

    case 18 :
      client->print("18");
      break;

    case 19 :
      client->print("19");
      break;

    case 20 :
      client->print("20");
      break;

    default :
      Serial.println("404");
      client->write("HTTP/1.1 404 Not Found\n");
      client->write("Content-Type: text/html\n");
      client->write("Connnection: close\n");
      client->write("\n");
  }
}

void idleanim()
{
  unsigned long currentMillis = millis();
  if (currentMillis  - previousMillis >= 250 && ( animation_step == 0 ) ) 
  {
    previousMillis = currentMillis;
    animation_step++;
    lcd.write(254);
    lcd.write(128);
    lcd.write("Push Button to  Begin Test ->   ");
  }
  else if (currentMillis  - previousMillis >= 250 && ( animation_step == 1 )) 
  {
    previousMillis = currentMillis;
    animation_step++;
    lcd.write(254);
    lcd.write(204);
    lcd.write("->  ");
  }
  else if (currentMillis  - previousMillis >= 250 && ( animation_step == 2 )) 
  {
    previousMillis = currentMillis;
    animation_step++;
    lcd.write(254);
    lcd.write(205);
    lcd.write("-> ");
  }
  else if (currentMillis  - previousMillis >= 250 && ( animation_step == 3 )) 
  {
    previousMillis = currentMillis;
    animation_step = 0;
    lcd.write(254);
    lcd.write(206);
    lcd.write("->");
  }
}

void testfirmness()
{
    digitalWrite(DIR, HIGH);
    digitalWrite(EN, LOW);
    digitalWrite(MS1, LOW); //Pull MS1, and MS2 high to set logic to fullstep resolution
    digitalWrite(MS2, LOW);
    digitalWrite(MS3, LOW);

      if (samples <= changecount)    //move fruit under heads
      {
                          //load fruit onto turn table
        steps = 0;
        resetLCD();
        lcd.write("Loading Samples ");
        lcd.write("        ");
        if (samples >= 10)    //align the count
        {
          lcd.write(254);
          lcd.write(16);
        }
        lcd.print(samples);
        lcd.write("/");
        lcd.print(changecount);
        samples++;
        
        while(steps <= 100)  //turn the turntable to next cherry
        {
          steps++;
          digitalWrite(STP,HIGH); //Trigger one step forward
          delay(1);
          digitalWrite(STP,LOW); //Pull step pin low so it can be triggered again
          delay(1);
        }
      }
      else                  //start sampling
      {

        resetLCD();
        if (new_lot)                      //what to display on the screen
        {
          lcd.write("Ready To Sample New Lot"); //first sample of lot
        }
        else if ( (samples - (changecount / 3)) <= changecount)           //samples in lot limit
        {
          lcd.write("Next Sample"); //first sample of lot
        }
        else                              //last sample
        {
          lcd.write("Last Sample"); //first sample of lot
          samples = 0;
          new_lot = true;
        }
        
        steps = 0;
        while(steps <= 100)  
        {
          steps++;
          digitalWrite(STP,HIGH); //Trigger one step forward
          delay(1);
          digitalWrite(STP,LOW); //Pull step pin low so it can be triggered again
          delay(1);
        }

        //updatedata( getw(), dataFile );
        File dataFile = SD.open("DATALOG.TXT", FILE_WRITE);   //open data log file
        if (dataFile){
          str = getw();
          dataFile.println( str );
          dataFile.close();
        }
        else{
          resetLCD();
          lcd.print("Error opening file");
        }
        
        resetLCD();
        
        if (new_lot && samples > 0)
        {
          lcd.print("Getting first   sample");
          new_lot = false;
        }
//        else if (samples >= (samples - (changecount / 3)) <= changecount)
//        {
//          lcd.print("Getting last    sample");
//          
//        }
        else
        {
          lcd.print(h0str);
  //        lcd.write(254);
  //        lcd.write(16);
          lcd.write(", ");
          lcd.print(h1str);
  //        lcd.write(254);
  //        lcd.write(16);
          lcd.write(", ");
          lcd.print(h2str);
        }
        samples++;

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
      if (HTTP_req[req_index] == '\n')      // every line of text received from the client ends with \r\n
      {
         //last character on line of received text
         //starting new line with next character read
         currentLineIsBlank = true;
      } 
      else if (HTTP_req[req_index] != '\r') 
      {
        // a text character was received from client
        currentLineIsBlank = false;
      }

      req_index++;
    }// end if (client.available())
  } // end while (client.connected())
  // give the web browser time to receive the data
  delay(1);
  // close the connection:
  client->stop();
  //Serial.println("client disconnected");
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
