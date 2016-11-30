/*
  Web Server

 A simple web server that shows the value of the analog input pins.
 using an Arduino Wiznet Ethernet shield.

 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Analog inputs attached to pins A0 through A5 (optional)

 created 18 Dec 2009
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe

 */

#include <IRLibSendBase.h>    // First include the send base
//Now include only the protocols you wish to actually use.
//The lowest numbered protocol should be first but remainder 
//can be any order.
#include <IRLib_P01_NEC.h>    
#include <IRLib_P02_Sony.h>   
#include <IRLibCombo.h>     // After all protocols, include this

#include <SPI.h>
#include <Ethernet.h>

IRsend mySender;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 150, 117);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }


  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}


static char currentLine[256];
static uint8_t myBuffer[512];
static char url[256];
void loop() {
  int i=0;
  int index = 0;
  int urlFound = 0;
  memset(currentLine, 0, sizeof(currentLine));
  memset(url, 0, sizeof(url));
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    //Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        int results = client.read(myBuffer, sizeof(myBuffer));
        //Serial.print("results...");
        if(results == 0)results = -1;
        //Serial.println(results);
        if(results == -1)break;
        for(int idx=0;idx<results;idx++){
          char c = (char)(myBuffer[idx]);
          //char c = client.read();
          if(c != '\r'){
            //Serial.write(c);
            // if you've gotten to the end of the line (received a newline
            // character) and the line is blank, the http request has ended,
            // so you can send a reply
            if (c == '\n' && currentLineIsBlank) {
              //Serial.println("Sending Response...");
              char ** temp;
              urlFound = 2;
              temp = parseRequest(url,&urlFound);
              urlFound = performCommand(temp);
              sendResponse(&client, urlFound);
              results = -1;
              break;
            }
            if (c == '\n')  {
              // you're starting a new line
              if(!urlFound){
                urlFound = 1;
                currentLine[index] = 0;
                strcpy(url, currentLine);
              }
              index = 0;
              currentLineIsBlank = true;
            }else{
              if(!urlFound){
                currentLine[index++] = c;
                i++;
              }
              // you've gotten a character on the current line
              currentLineIsBlank = false;
            }
          }
        }
        if(results == -1)break;
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

static char * words[20];

char ** parseRequest(char * request, int * isPost){
  int idx = 0;
  int len = 0;
  char * requestEnd;
  //Serial.println(request);
  memset(words,0,sizeof(words));
  //First parse get vs post
  if(0 == strncmp(request, "POST", 4)){
    //We found a post requset
    *isPost = 1;
    request += 5;
  }else if(0 == strncmp(request, "GET", 3)){
    *isPost = 1;
    request += 4;
  }else{
    *(strchr(request,' ')) = 0; 
    
    return words;
  }
  //Now parse the url from the http request (FIXME, we just assume it's an http request)
  requestEnd = strchr(request,' ')+1; //HEre we find the end of the URL
  *requestEnd = 0; //Add null terminated byte to end the URL
  //len = strlen(request);
  if(request[0] == '/'){
    request++;
  }
  //words[idx++]=request;
  //Serial.println(request);
  while(1){
    char * next = strchr(request,'/');//strlen(request));
    if(next){
      *next = 0;
      words[idx++]=request;
      request = next+1;
    }else{
      words[idx++]=request;
      //words[idx]=0;
      break;
    }
  }
  return words;
}

//This routine just prints out the portions of the URLS -- one on each line
void printRequest(char * data[]){
  int i;
  for(i=0;data[i];i++){
    Serial.print("Line ");
    Serial.print(i);
    if(data[i]){
      Serial.println(data[i]);
    }
  }
}

int performCommand(char * urls[]){
  if(!strcmp(urls[0],"remote")){
    return performRemote(urls[1], atol(urls[2]), atoi(urls[3]));
  }
  return -1;
}

//This executes the IR writing.
int performRemote(char * protocol, long code, int bitcount){
  /*Serial.print("Writing to ");
  Serial.print(protocol);
  Serial.print(" ");
  Serial.print(code);
  Serial.print(" ");
  Serial.println(bitcount);*/
  if(!strcmp("SONY", protocol)){
    noInterrupts();
    mySender.send(SONY, code, bitcount);
    interrupts();
  }else if(!strcmp("NEC", protocol)){
    noInterrupts();
    mySender.send(NEC, code, bitcount);
    interrupts();
  }else{
    return -1;
  }
  return 0;
}

void sendResponse(EthernetClient * client, int results){
  // send a standard http response header
  client->println("HTTP/1.1 200 OK");
  client->println("Content-Type: text/html");
  client->println("Connection: close");  // the connection will be closed after completion of the response
  //client->println("Refresh: 2");  // refresh the page automatically every 5 sec
  client->println();
  client->println("<!DOCTYPE HTML>");
  client->println("<html>");
  // output the value of each analog input pin
  /*for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
    int sensorReading = analogRead(analogChannel);
    client->print("analog input ");
    client->print(analogChannel);
    client->print(" is ");
    client->print(sensorReading);
    client->println("<br />");
  }*/
  client->print("{results=");
  client->print(results);
  client->print("}");
  
  client->println("</html>");
}
