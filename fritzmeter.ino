/*
  Fritzmeter
 
 This sketch connects to a Fritz!Box DSL router
 using an Arduino Ethernet shield. 
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Servos attached to pins 5 (receive rate) and 6 (transmit rate)
 
 Credits:
 Parts of the code are taken from the web client example code by David A. Mellis
 The XML parsing is adapted from Bob S. aka XTALKER's weather data XML extractor.
 
 created 20 Mar 2012
 last modified Mar 30 2012
 by A. Reischle
 www.reischle.net
 
 */
 
#include <SPI.h>
#include <Ethernet.h>
#include <Servo.h>

#define MAX_STRING_LEN  30

Servo srxservo;
Servo stxservo;



// MAC address for controller.
byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// IP address for remote Server
IPAddress server(192,168,1,1); // FritzBox

// most variables are global for convenience
String nsb; //newSentBytes
String nrb; //nweReceiveBytes
unsigned long nsbl; //bits per second send
unsigned long nrbl; //bits persecond received
int rxservo=0;
int txservo=0;

String xml="<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">\n<s:Body>\n<u:GetAddonInfos xmlns:u=\"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1\" />\n</s:Body>\n</s:Envelope>";


// Setup vars for serialEvent
char tagStr[MAX_STRING_LEN] = "";
char dataStr[MAX_STRING_LEN] = "";
char tmpStr[MAX_STRING_LEN] = "";
char endTag[3] = {'<', '/', '\0'};
int len;
// Flags to differentiate XML tags from document elements (ie. data)
boolean tagFlag = false;
boolean dataFlag = false;


// Initialize the Ethernet client library
// with the IP address and port of the server 

EthernetClient client;

void setup()
{
// start the serial library:
  Serial.begin(9600);
  
   
srxservo.attach(5);
stxservo.attach(6);
pinMode(13, OUTPUT);
}

void loop()
{
 if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    //for(;;)
    //  ;
    softReset();
  }
  // give the Ethernet shield a second to initialize:
  digitalWrite(13, HIGH);
  delay(1000);
  digitalWrite(13, LOW);
  
 // print your local IP address:
  Serial.print("My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print("."); 
  }
  Serial.println();

 // print your local Gateway address:
  Serial.print("My Gateway address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.gatewayIP()[thisByte], DEC);
    Serial.print("."); 
  }
  Serial.println();

 Serial.println("connecting...");
 digitalWrite(13, HIGH);
   // make the request to the server
   
   if (client.connect(server, 49000)) {
    Serial.println("connected");
    // Make a HTTP POST request:
    Serial.println("Sending POST request");
    client.println("POST /upnp/control/WANIPConn1 HTTP/1.1");
    client.println("HOST: 192.168.1.1:49000");
    client.println("SOAPACTION: \"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1#GetAddonInfos\"");
    client.println("Content-Type: text/xml; charset=\"utf-8\"");
    client.print("Content-Length: ");
    client.println(xml.length());
    client.println();
    client.println(xml);
    client.println();
    Serial.println("Post request sent");
    digitalWrite(13, LOW);
  } 
  else {
    // no connection to the server:
    Serial.println("connection failed in requesttraffic");
	//for(;;)
        //;
        softReset();
  }
   
   //wait for the server to answer
   
   while (!client.available())
  {
  ;
  }
  
  
  //get data
  while (client.available()) {
    //char c = client.read();
    //delay(1);
    //Serial.print(c);
	serialEvent();
   }
     
 client.flush();
 Serial.println();
 Serial.print("Disconnecting...");
 client.stop();  
 Serial.println("done.");
 client.flush();  
}

// Process each char from web
void serialEvent() {

   // Read a char
	char inChar = client.read();
   //Serial.print(inChar);
  
   if (inChar == '<') {
      addChar(inChar, tmpStr);
      tagFlag = true;
      dataFlag = false;

   } else if (inChar == '>') {
      addChar(inChar, tmpStr);

      if (tagFlag) {      
         strncpy(tagStr, tmpStr, strlen(tmpStr)+1);
      }

      // Clear tmp
      clearStr(tmpStr);

      tagFlag = false;
      dataFlag = true;      
      
   } else if (inChar != 10) {
      if (tagFlag) {
         // Add tag char to string
         addChar(inChar, tmpStr);

         // Check for </XML> end tag, ignore it
         if ( tagFlag && strcmp(tmpStr, endTag) == 0 ) {
            clearStr(tmpStr);
            tagFlag = false;
            dataFlag = false;
         }
      }
      
      if (dataFlag) {
         // Add data char to string
         addChar(inChar, dataStr);
      }
   }  
  
   // If a LF, process the line
   if (inChar == 10 ) {

/*
      Serial.print("tagStr: ");
      Serial.println(tagStr);
      Serial.print("dataStr: ");
      Serial.println(dataStr);
*/

      // Find specific tags and print data

	  if (matchTag("<NewByteSendRate>")) {
	      Serial.print("TX: ");
              nsbl = strtoul(dataStr,NULL,0);
              nsbl=nsbl*8; //mach bits draus
              Serial.println(nsbl);
              txservo=map(nsbl,0,733000,179,1);
              Serial.print("Servo TX value: ");
              Serial.println(txservo);
              stxservo.write(txservo);
      }
	  
	 if (matchTag("<NewByteReceiveRate>")) {
	      Serial.print("RX: ");
           // Serial.print(dataStr);
              nrbl = strtoul(dataStr,NULL,0);
              nrbl=nrbl*8;  //mach bits
              Serial.println(nrbl);
              rxservo=map(nrbl,0,6906000,1,179);
              Serial.print("Servo RX value: ");
              Serial.println(rxservo);
              srxservo.write(rxservo);
      }
           
      // Clear all strings
      clearStr(tmpStr);
      clearStr(tagStr);
      clearStr(dataStr);

      // Clear Flags
      tagFlag = false;
      dataFlag = false;
   }
}

/////////////////////
// Other Functions //
/////////////////////

// Function to clear a string
void clearStr (char* str) {
   int len = strlen(str);
   for (int c = 0; c < len; c++) {
      str[c] = 0;
   }
}

//Function to add a char to a string and check its length
void addChar (char ch, char* str) {
   char *tagMsg  = "<TRUNCATED_TAG>";
   char *dataMsg = "-TRUNCATED_DATA-";

   // Check the max size of the string to make sure it doesn't grow too
   // big.  If string is beyond MAX_STRING_LEN assume it is unimportant
   // and replace it with a warning message.
   if (strlen(str) > MAX_STRING_LEN - 2) {
      if (tagFlag) {
         clearStr(tagStr);
         strcpy(tagStr,tagMsg);
      }
      if (dataFlag) {
         clearStr(dataStr);
         strcpy(dataStr,dataMsg);
      }

      // Clear the temp buffer and flags to stop current processing
      clearStr(tmpStr);
      tagFlag = false;
      dataFlag = false;

   } else {
      // Add char to string
      str[strlen(str)] = ch;
   }
}

// Function to check the current tag for a specific string
boolean matchTag (char* searchTag) {
   if ( strcmp(tagStr, searchTag) == 0 ) {
      return true;
   } else {
      return false;
   }
}

// In case all goes wrong
void softReset(){
asm volatile ("  jmp 0");
} 

 
