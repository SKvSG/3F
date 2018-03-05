#include <avr/io.h>
#include <SPI.h>
#include <Ethernet3.h>
#include <SD.h>
#include <stdio.h>

// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   2000

#define lcd Serial1  //pin 1 for lcd pin 0 for firmness head 1
#define head1 Serial2
#define head2 Serial3

//Serial3.
//altsoftserial  //recieve pin 20   //firmness head 3

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

static char h0str[7];  //must be one more than message length
static char h1str[7];  //must be one more than message length
static char h2str[7];  //must be one more than message length
int changecount = 25;  //TODO: add to grower struct
//byte mac[6] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
//IPAddress ip(10, 34, 10, 131);
EthernetServer server(80);  // create a server at port 80
File webFile;
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
int req_index = 0;              // index into HTTP_req buffer

char* request_list[] = {"GET / ","index.htm","about.htm","bar-grap.js",
                        "excanvas.js","export.htm","export.png","favicon.ico",
                        "download.png","grap_res.htm","grap_res.png","style.css",
                        "logo.mp4","lot_numb.htm","lot_numb.png","settings.htm",
                        "ajax_ip","ajax_subn","ajax_gate","changecount","Grower",
                        "Lot","Remaining","CPressure","FPressure","Rest","WUnits",
                        "DUnits","Calib","receiveip"};
                        
                        //***request list vs filelist should be seperated

//File dataFile;
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
struct machineSettings loadSettings(struct machineSettings);                          //load settings set in the config file or sets to default value
void initializeNetwork(struct machineSettings);                     //start network adapter check connectivity
//struct machinesettings initializeNetwork(struct machinesettings);                     //start network adapter check connectivity
void resetLCD();                              //clear text and symbols from the lcd
void serveclient(EthernetClient client);      //recieve request from client
char * getclientdata(EthernetClient client);
void processrequest(char * );
//void processrequest(char *, machinesettings)
char * validaterequest(char *);
const char * processaction(char HTTP_req[REQ_BUF_SZ], EthernetClient client); //***handle ajax requests TODO: REMOVE ETHERNETCLIENT RETURN STRING
//const char * processaction(char HTTP_req[REQ_BUF_SZ], machinesettings); //***handle ajax requests TODO: REMOVE ETHERNETCLIENT RETURN STRING
char StrContains(char *str, char *sfind);     //check request for string
void StrClear(char *str, char length);        //check if final line of request
char * getw();                                //request pressure reading
void testfirmness();                          //run the firmness testing procedure
void idleanim();
void sendheader(char *request,EthernetClient client);
/*/////////////////////////////////////////////////////////////////////////////////////////
* MAIN
*//////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
    //initializeLCD();
    initializePins();
    initializeSerial();
    initializeSD();
    localSettings = loadSettings(localSettings);
    initializeNetwork(localSettings);
    delay(2000);
    resetLCD();
    lcd.write("Push Button to  Begin Test ->");
    digitalWrite(EN, HIGH);
    digitalWrite(HEAD1EN, LOW);
}

void loop()
{

    EthernetClient client = server.available();  // try to get client
    

    if (client)   // serve client website
    {

        serveclient(client);
        Serial.println("served");
    }
//    else if (digitalRead(BTN1) == 0) // continue testing firmness
//    {
//        testfirmness();
//    }
//    
//    else { //idle animation
//        idleanim();
//    }
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
    Serial.print("SUCCESS - SD card initialized.");
    delay(1250);
    // check for index.htm file
    if (!SD.exists("index.htm")) {
        resetLCD();
        lcd.print("ERROR - Can't find index.htm file!");
        delay(1250);
        return;  // can't find index file
    }
    resetLCD();
    lcd.print("SUCCESS - Found index.htm file.");
    delay(1250);

    if (!SD.exists("DATALOG.TXT")) {
      resetLCD();
      lcd.print("ERROR - Can't find log file!");
      delay(1250);
      
    }
    resetLCD();
    lcd.print("SUCCESS - Found log file.");
    delay(1250);
}

struct machineSettings loadSettings(struct machineSettings localSettings)
{
    char buff[255];
   // FILE settingsFile;
    
    // check for index.htm file
    if (!SD.exists("settings.txt")) {
        resetLCD();
        lcd.print("ERROR - Can't find settings.txt file!");
        delay(1250);
        return localSettings;  // can't find index file
    }
    resetLCD();
    lcd.print("SUCCESS - Found settings.txt file.");
    delay(1250);
    File settingsFile = SD.open("settings.txt", FILE_READ);
    
  //  fgets(buff, 255, (FILE*)fp);
    // if StrContains(buff, "changecount"){
        // fscanf(fp, "%s", buff);
        ////machineSettings.changecount =  "%s" ;
        // }

    
        // int changecount = 25;
    // int array[100];
    // byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    // IPAddress ip(10, 34, 10, 131);
    // byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    // int CPressure = 2;
    // int FPressure = 10;
    // int Calib = 25;
    // int Rest = 0;
    // int WUnits = 453.592; //conversion factor * gram = lbs
    // int DUnits = 25.4; //conversion factor * inches = mm TODO: add rod conversion factor
}; 

void initializeNetwork(struct machineSettings localSettings)
{
//    pinMode(10, OUTPUT);
//    digitalWrite(10, HIGH);

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
 
void processrequest(char * ,EthernetClient client)
{
  // open requested web page file
  Serial.print("validate request:");
  char * request = validaterequest( HTTP_req ); //does file/command exist?
  Serial.println(request);


    if (SD.exists(request))       //check for requested file
    {
      Serial.print("open this file:");
      Serial.println(request);
      sendheader(request, client);    //send header depending on file type
      webFile = SD.open(request);
      if (webFile)
      {
        while(webFile.available()) 
        {
          client.write(webFile.read()); // send web page to client

        }
        Serial.println("file sent");
        webFile.close();
      }
    }
    
//    else if (StrContains(request, "ajax"))   //serve ajax call
//    {
//      client.println(processaction(HTTP_req, client));
//      //processaction(HTTP_req, client)
//    }
  else
  { //send 404
    Serial.print("404");
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/html");
    client.println("Connnection: close");
    client.println();
  }
}

       
char * validaterequest(char * HTTP_req)
{
  word i = 0;
  byte j = 0;
  char request[255] = {0};
  boolean isRequest = false;
  while (HTTP_req[i] != '\n')
  {
    if (HTTP_req[i] == '/')
    {//start recording
      isRequest = true;
      if (HTTP_req[i+1] == ' ') //if blank send index
      {
        return "index.htm";
      }
    }
    else if (isRequest && HTTP_req[i] == ' ')
    { //end of file name
      break;
    }
    else if (isRequest)
    { //get filename/action details
      request[j] = HTTP_req[i];
      j++;
    }
    i++;
  }
  if (request)
  {
    Serial.print(request);
    return request;
  }
  else
  {
    return false;
  }
//  //filename = http_req.substring(from)
//  //SD.exists(filename)
//  for (int request_count = 0; sizeof(request_list); request_count++)      //identify request
//  {
//    if (request(HTTP_req, request_list[request_count]))  
//    {
//      return true;
//    }
//    else if ( request_count == sizeof(request_list))
//    {
//      return false;
//    }
//  }
}

void sendheader(char *request,EthernetClient client)
{
  if (StrContains(request, ".htm")) //TODO fix this so that index is read correctly
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connnection: close");
    client.println();
  }
  else if (StrContains(request, ".js"))
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/js");
    client.println("Connnection: close");
    client.println();
  }
  else if (StrContains(request, ".css"))
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/css");
    client.println("Connnection: close");
    client.println();
  }
  else if (StrContains(request, ".png")
      || StrContains(request, ".ico")
      || StrContains(request, ".mp4"))
  {
    client.println("HTTP/1.1 200 OK");
    client.println();
  }
}

const char * processaction(char HTTP_req[REQ_BUF_SZ], EthernetClient client)  //return what the client should print
{
 // for command_list
  char *ret_string;
  
  if (StrContains(HTTP_req, "ip")) 
  {
    client.println(Ethernet.localIP());
  }
  else if (StrContains(HTTP_req, "subn")) 
  {
    client.println(Ethernet.subnetMask());
  }
  else if (StrContains(HTTP_req, "gate")) 
  {
    client.println(Ethernet.gatewayIP());
  }
  else if (StrContains(HTTP_req, "changecount")) 
  {
    return "25";
  }
  else if (StrContains(HTTP_req, "Grower")) 
  {
    return "25";
  }
  else if (StrContains(HTTP_req, "Lot")) 
  {
    return "25";
  }
  else if (StrContains(HTTP_req, "Remaining")) 
  {
    return "25";
  }
  else if (StrContains(HTTP_req, "CPressure")) 
  {
    return "25";
  }
  else if (StrContains(HTTP_req, "FPressure")) 
  {
    return "25";
  }
  else if (StrContains(HTTP_req, "Rest")) 
  {
    return "25";
  }
  else if (StrContains(HTTP_req, "WUnits")) 
  {
    return "25";
  }
  else if (StrContains(HTTP_req, "DUnits")) 
  {
    return "25";
  }
  else if (StrContains(HTTP_req, "Calib")) 
  {
    return "25";
  }
  else if (StrContains(HTTP_req, "receiveip")) 
  { //lotchangecount number
    client.println(Ethernet.localIP());
  }
  else if (StrContains(HTTP_req, "a0")) 
  { 
    return "25";
  }
  else if (StrContains(HTTP_req, "a1")) 
  {
    return "25";
  }
  else if (StrContains(HTTP_req, "a2")) 
  {
    return "25";
  }
  else if (StrContains(HTTP_req, "a3")) 
  {
    return "25";
  }
  else if (StrContains(HTTP_req, "a4")) 
  {
    return "25";
  }
  else
  {
    client.println("badcommand");
  }
  return ret_string;
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
      
void serveclient(EthernetClient client)
{
  char * request;
  boolean currentLineIsBlank = true;
  boolean FileRequest = false;

  int req_index = 0;


  while (client.connected()) 
  {
    if (client.available())
    {
      request = getclientdata(client); //fills HTTP buffer
      Serial.print( HTTP_req[req_index]);
      //Serial.print (HTTP_req);
      if ((HTTP_req[req_index] == '\n') && currentLineIsBlank) //is this the end of data?  //use hex values?
      {
       Serial.println("process this");
       Serial.print(request);
       processrequest(request, client);        //serve request
       //currentLineIsBlank = false;
       req_index = 0;

       StrClear(HTTP_req, REQ_BUF_SZ);
       break;
      }
      else if (HTTP_req[req_index] == '\n')      // every line of text received from the client ends with \r\n
      {
         //last character on line of received text
         //starting new line with next character read
         currentLineIsBlank = true;
      } 
      else if (HTTP_req[req_index] != '\r') 
      {
         //Serial.print("char");
        // a text character was received from client
        currentLineIsBlank = false;
      }

      req_index++;
    }
  } // end while (client.connected())
  // give the web browser time to receive the data
  delay(1);
  // close the connection:
  client.stop();
  Serial.println("client disconnected");
}

char * getclientdata(EthernetClient client)
{
    if (client.available()) // client data available to read
    {
      char c = client.read(); // read 1 byte (character) from client
                          // buffer first part of HTTP request in HTTP_req array (string)
                          // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)

      //Serial.print(c);

        if (req_index < (REQ_BUF_SZ - 1)) 
        {
  
          HTTP_req[req_index] = c;          // save HTTP request character
          req_index++;
        }
    return HTTP_req;
    }
}
