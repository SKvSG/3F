#include <avr/io.h>
#include <SPI.h>
#include <Ethernet3.h>
#include <SD.h>

// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   1024

#define lcd Serial1  //pin 1 for lcd pin 0 for firmness head 1
//#define head2 Serial2
//#define head3 Serial3
//Serial2.setRX(26);  //firmness head 2
//Serial3.
//altsoftserial  //recieve pin 20   //firmness head 3

/*/////////////////////////////////////////////////////////////////////////////////////////
* PIN ASSIGNMENTS
*//////////////////////////////////////////////////////////////////////////////////////////
//int FirmnessHead1RX = 0;
//int LCDTX = 1;
//int ?? = 2;
int BTN1 = 3;
//int SDCS = 4;
//int ?? = 5;
//int ?? = 6;
//int FirmnessHead3RX = 7;
//int LeftPad?? = 8;
//int WRESET = 9;
//int WCS = 10;
//int DOUT = 11;
//int DIN = 12;
//int DSCK = 13;
//int DSCK = 14;
int MS1 = 15;
int STP = 16;
int DIR = 17;
int MS2 = 18;
int EN = 19;
int HEAD1EN = 20;
//int ?? = 21;
//int ?? = 22;
//int ?? = 23;
//int ?? = 24;
//int ?? = 25;
//int FirmnessHead2RX = 26;
//int ?? = 27;
//int ?? = 28;
//int ?? = 29;
//int ?? = 30;
//int ?? = 31;
//int ?? = 32;
//int ?? = 33;

/*/////////////////////////////////////////////////////////////////////////////////////////
* GLOBAL VARIABLES
*//////////////////////////////////////////////////////////////////////////////////////////
struct machineSettings
{
    int changecount = 25;
    int array[100];
    byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    IPAddress ip(10, 34, 10, 131);
    byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    int CPressure = 2;
    int FPressure = 10;
    int Calib = 25;
    int Rest = 0;
    int WUnits = 453.592; //conversion factor * gram = lbs
    int DUnits = 25.4; //conversion factor * inches = mm TODO: add rod conversion factor
}; 

EthernetServer server(80);
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
int animation_step = 0;
unsigned long previousMillis = 0;
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
void initializepins();                        //set inputs and outputs
void initializeSD();                          //***check for SD card and needed files on SD (neds to check for more files)
struct loadSettings(struct);                          //load settings set in the config file or sets to default value
void initializeNetwork();                     //start network adapter check connectivity
//machinesettings initializeNetwork(machinesettings);                     //start network adapter check connectivity
void resetLCD();                              //clear text and symbols from the lcd
void serveclient(EthernetClient client);      //recieve request from client
char * getclientdata(EthernetClient client);
void processrequest(char * );
//void processrequest(char *, machinesettings)
boolean validaterequest(char *);
const char * processaction(char HTTP_req[REQ_BUF_SZ], EthernetClient client); //***handle ajax requests TODO: REMOVE ETHERNETCLIENT RETURN STRING
//const char * processaction(char HTTP_req[REQ_BUF_SZ], machinesettings); //***handle ajax requests TODO: REMOVE ETHERNETCLIENT RETURN STRING
char StrContains(char *str, char *sfind);     //check request for string
void StrClear(char *str, char length);        //check if final line of request
char * getw();                                //request pressure reading
void testfirmness();                          //run the firmness testing procedure
void idleanim();                              //when idle animate lcd

/*/////////////////////////////////////////////////////////////////////////////////////////
* MAIN
*//////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
    Serial.begin(9600);       //*** for debugging (could use more speed)
    lcd.begin(9600);
    resetLCD();
    initializeSD();
    initializeNetwork();
    resetLCD();
}

void loop()
{
    EthernetClient client = server.available();  // try to get client

    if (client) 
    {                             // serve client website
      serveclient(client);
    }
    
    else if (digitalRead(BTN1))   // continue testing
    {
      testfirmness();
    }
    
    else                        //idle add second animation with more instructions
    { 
      idleanim();
    }
}

/*/////////////////////////////////////////////////////////////////////////////////////////
* FUNCTIONS
*//////////////////////////////////////////////////////////////////////////////////////////
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
    delay(1250);
}

struct loadSettings()
{
    char buff[255];
    
    // check for index.htm file
    if (!SD.exists("settings.txt")) {
        resetLCD();
        lcd.print("ERROR - Can't find settings.txt file!");
        delay(1250);
        return;  // can't find index file
    }
    resetLCD();
    lcd.print("SUCCESS - Found settings.txt file.");
    delay(1250);
    fp = fopen("settings.txt", "r")
    
    fgets(fp, "%s", buff);
    if StrContains(buff, "changecount"){
        fscanf(fp, "%s", buff);
        //machineSettings.changecount =  "%s" ;
        }

    
/*         int changecount = 25;
    int array[100];
    byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    IPAddress ip(10, 34, 10, 131);
    byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    int CPressure = 2;
    int FPressure = 10;
    int Calib = 25;
    int Rest = 0;
    int WUnits = 453.592; //conversion factor * gram = lbs
    int DUnits = 25.4; //conversion factor * inches = mm TODO: add rod conversion factor */
}; 
    
}

void initializeNetwork()
{
    Ethernet.begin(mac, ip);  // initialize Ethernet device
    server.begin();           // start to listen for clients
    resetLCD();
    lcd.print("Server IP:      ");
    lcd.print(Ethernet.localIP());
    delay(1250);
}

void initializepins()
{
    pinMode(BTN1, INPUT);
    pinMode(STP, OUTPUT);
    pinMode(DIR, OUTPUT); 
    pinMode(MS2, OUTPUT);
    pinMode(EN , OUTPUT);;
    pinMode(MS1 , OUTPUT);
    pinMode(HEAD1EN , OUTPUT);
}

void resetLCD()
{
  lcd.write(254); // move cursor to beginning of first line
  lcd.write(128);
  lcd.write("                "); // clear display
  lcd.write("                ");
}

void serveclient(EthernetClient client)
{
  // Serial.print("got client");
  boolean currentLineIsBlank = true;
  boolean FileRequest = false;
  char * request;
  while (client.connected()) 
  {
  
    request = getclientdata(client); //fills HTTP buffer  TODO: BUFF REMAINS EMPTY
    Serial.print(">>");
    Serial.println(request);
    Serial.print((int)req_index);
    Serial.println("<<");
    if (HTTP_req[req_index] == '\n' && currentLineIsBlank) //is this the end of data?
    {
     Serial.print("process this");
     Serial.print(HTTP_req);
     processrequest(HTTP_req, client);        //serve request
    }
    else if (HTTP_req[req_index] == '\n')      // every line of text received from the client ends with \r\n
    {
      Serial.println("got new line");
       //last character on line of received text
       //starting new line with next character read
       currentLineIsBlank = true;
    } 
    else if (HTTP_req[req_index] != '\r') 
    {
      // a text character was received from client
      currentLineIsBlank = false;
    }
  } // end while (client.connected())
  delay(1);      // give the web browser time to receive the data
  client.stop(); // close the connection
}

char * getclientdata(EthernetClient client)
{
    if (client.available()) // client data available to read
    {
      char c = client.read(); // read 1 byte (character) from client
                          // buffer first part of HTTP request in HTTP_req array (string)
                          // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
      if (req_index < (REQ_BUF_SZ - 1)) 
      {
        HTTP_req[req_index] = c;          // save HTTP request character
        req_index++;
      }
      //Serial.print(c);
    }
    return HTTP_req;
}

void processrequest(char * ,EthernetClient client)
{
  // open requested web page file
  boolean request = validaterequest( HTTP_req ); //does file/command exist?
  Serial.print("validate request:");
  Serial.print(request);

  if (request) //serve request
  {
    boolean FileRequest =  sendheader(HTTP_req, client);    //identify request, send header depending on file type  

    if (StrContains(HTTP_req, "GET / "))                  //if home page edge case TODO: remove this case
    {
      webFile = SD.open("index.htm");
      while(webFile.available()) 
      {
          client.write(webFile.read()); // send web page to client
      }
      webFile.close();
    }
    
    else if (FileRequest)       //send requested file
    {
      //webFile = SD.open(request_list[request_count]);
      Serial.print(HTTP_req);
      webFile = SD.open(HTTP_req);
      while(webFile.available()) 
      {
        client.write(webFile.read()); // send web page to client
      }
      webFile.close();
    }
    
    else if (StrContains(HTTP_req, "ajax"))   //serve ajax call
    {
      client.println(processaction(HTTP_req, client));
      //processaction(HTTP_req, client)
    }
    req_index = 0;
    StrClear(HTTP_req, REQ_BUF_SZ);
  }
  else
  { //send 404
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/html");
    client.println("Connnection: close");
    client.println();
  }
  // reset buffer index and all buffer elements to 0
  req_index = 0;
  StrClear(HTTP_req, REQ_BUF_SZ);
}

boolean validaterequest(char * HTTP_req)
{    
  for (int request_count = 0; sizeof(request_list); request_count++)      //identify request
  {
    if (StrContains(HTTP_req, request_list[request_count]))  
    {
      return true;
    }
    else if ( request_count == sizeof(request_list))
    {
      return false;
    }
  }
}

boolean sendheader(char *request,EthernetClient client)
{
  if (StrContains(request, ".htm"))//change from strcontain to request count
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connnection: close");
    client.println();
    return true;
  }
  else if (StrContains(request, "GET / "))
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connnection: close");
    client.println();
    return true;
  }
  else if (StrContains(request, ".js"))
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/js");
    client.println("Connnection: close");
    client.println();
    return true;
  }
  else if (StrContains(request, ".css"))
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/css");
    client.println("Connnection: close");
    client.println();
    return true;
  }
  else if (StrContains(request, ".png")
      || StrContains(request, ".ico")
      || StrContains(request, ".mp4"))
  {
    client.println("HTTP/1.1 200 OK");
    client.println();
    return true;
  }
  else
  {
    return false;
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
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

char * getw()
{
  static char h0str[7];  //must be one more than message length
  int h0DataLength = 0;
//  while (head < headcount, headcount++)
  StrClear(h0str, 7);
  digitalWrite(HEAD1EN, HIGH);
  while (!lcd.available() )
  {
    delay(1);
  }
  if ( lcd.available() )
  {
      digitalWrite(HEAD1EN, LOW);
      while(1)
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
       // h0str[h0DataLength] = h0data;
        //h0DataLength++;
      }   
  }
//  else if ( head1.available() )
//  {
//      while(head1.available())
//    {
//      h1str[h1DataLength] = head1.read();
//      h1DataLength++;
//    }
//  }
//  else if ( head2.available() )
//  {
//      while(head2.available())
//    {
//      h2str[h2DataLength] = head2.read();
//      h2DataLength++;
//    }
//  }
//  turn on all three digital pins
//  wait for serial data to hit buffer
//  store data into array
return h0str;
}

void testfirmness()
{
  digitalWrite(DIR, HIGH); //Pull direction pin low to move down
  digitalWrite(MS1, LOW); //Pull MS1, and MS2 high to set logic to fullstep resolution
  digitalWrite(MS2, LOW);

  if (samples <= 2)  //ifdef samples for how many samples before testing should begin
  {
    samples++;
    steps = 0;
    resetLCD();
    lcd.write("Loading Samples            ");
    while(steps <= 100)  
    {
      steps++;
      digitalWrite(STP,HIGH); //Trigger one step forward
      delay(1);
      digitalWrite(STP,LOW); //Pull step pin low so it can be triggered again
      delay(1);
    }
  }
  else
  {
    steps = 0;
    resetLCD();
    if (new_lot)
    {
      lcd.write("Ready To Sample    ");
      new_lot = false;
    }
    else if (samples <= 32)
    {
      resetLCD();
      lcd.write("Pres Xxx Xxx Xxx");
      lcd.write("Size Xxx Xxx Xxx");
    }
    else
    {
      resetLCD();
      lcd.write("Pres Xxx Xxx Xxx");
      lcd.write("Size Xxx Xxx Xxx");
      samples = 0;
    }
    samples++;
    while(steps <= 100)  
    {
      steps++;
      digitalWrite(STP,HIGH); //Trigger one step forward
      delay(1);
      digitalWrite(STP,LOW); //Pull step pin low so it can be triggered again
      delay(1);
    }
    Serial.println( getw() );
    //updatearray( size, firmness );
    //updatefile( size, firmness );
    delay(1250);
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

