void serveclient(EthernetClient client)
{
  // Serial.print("got client");
  boolean currentLineIsBlank = true;
  boolean FileRequest = false;
  while (client.connected()) 
  {
    getclientdata(client);
    if (HTTP_req[req_index] == \n && currentLineIsBlank)
    {
     processrequest(HTTP_req);
    }
    else if (HTTP_req[req_index] == \n)
    {
       //last character on line of received text
       //starting new line with next character read
       currentLineIsBlank = true;
    } 
    else if (c != '\r') 
    {
      // a text character was received from client
      currentLineIsBlank = false;
    }
      
      // every line of text received from the client ends with \r\n

    } // end if (client.available())
  } // end while (client.connected())
  delay(1);      // give the web browser time to receive the data
  client.stop(); // close the connection
}


void getclientdata(EthernetClient client)
{
    if (client.available()) 
    {

// client data available to read
      
      char c = client.read(); // read 1 byte (character) from client
                          // buffer first part of HTTP request in HTTP_req array (string)
                          // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
      if (req_index < (REQ_BUF_SZ - 1)) 
      {
        HTTP_req[req_index] = c;          // save HTTP request character
        req_index++;
      }

      Serial.print(c);
    }
}

void processrequest(EthernetClient client)
    // open requested web page file
    for (int request_count = 0; sizeof(request_list); request_count++)
    {
      if (StrContains(HTTP_req, request_list[request_count]))  ///make req function to determin request
      {
        if (StrContains(HTTP_req, ".htm")//change from strcontain to request count
        {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connnection: close");
          client.println();
          FileRequest = true;
        }
        else if (StrContains(HTTP_req, "GET / "))
        {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connnection: close");
          client.println();
          FileRequest = true;
        }
        else if (StrContains(HTTP_req, ".js"))
        {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/js");
          client.println("Connnection: close");
          client.println();
          FileRequest = true;
        }
        else if (StrContains(HTTP_req, ".css"))
        {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/css");
          client.println("Connnection: close");
          client.println();
          FileRequest = true;
        }
        else if (StrContains(HTTP_req, ".png")
              || StrContains(HTTP_req, ".ico")
              || StrContains(HTTP_req, ".mp4"))
        {
          client.println("HTTP/1.1 200 OK");
          client.println();
          FileRequest = true;
        }
//            else
//            {
//              client.println("HTTP/1.1 200 OK");
//              client.println("Content-Type: text/html");
//              client.println("Connnection: close");
//              client.println();
//            }
        Serial.println(request_list[request_count]);
        if (StrContains(HTTP_req, "GET / "))
        {
          webFile = SD.open("index.htm");
          while(webFile.available()) 
          {
              client.write(webFile.read()); // send web page to client
          }
          webFile.close();
          FileRequest = false;
          // break;
        }
        
        else if (FileRequest)
        {
          webFile = SD.open(request_list[request_count]);
          while(webFile.available()) 
          {
            client.write(webFile.read()); // send web page to client
          }
          webFile.close();
          FileRequest = false;
        }
        
        else if (StrContains(HTTP_req, "ajax"))
        {
          client.println(processrequest(HTTP_req, client));
        }
        req_index = 0;
        StrClear(HTTP_req, REQ_BUF_SZ);
        break;
      }
    } 
    // reset buffer index and all buffer elements to 0
    req_index = 0;
    StrClear(HTTP_req, REQ_BUF_SZ);
    break;
  }
