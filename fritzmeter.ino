/*
  Fritzmeter
 
 This sketch connects to a Fritz!Box DSL router
 using an Arduino Ethernet shield.
 Other UPNP capable routers might work as well but are untested.
 Please let me know if you have tried any other routers. 
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Servos attached to pins 5 (receive rate) and 6 (transmit rate)
 
 Credits:
 Parts of the code are taken from the web client example code by David A. Mellis
 The XML parsing is adapted from Bob S. aka XTALKER's weather data XML extractor.
 
 created 20 Mar 2012
 
 last modified Dec 29 2012 - Release version
 by A. Reischle
 www.reischle.net
 
 */
 
#include <SPI.h>
#include <Ethernet.h>
#include <Servo.h>


#define MAX_STRING_LEN  35


Servo srxservo;
Servo stxservo;


// MAC address for controller.
byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// most variables are global for convenience
String nsb; //newSentBytes
String nrb; //nweReceiveBytes
String xml; //Buffer for xml request string
unsigned long nsbl; //bits per second send
unsigned long nrbl; //bits persecond received
unsigned long mxrx; //max bits per second receive
unsigned long mxtx; //max bits per second transmit


int rxservo=0;
int txservo=0;


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
 
 Serial.println("Fritzmeter 20121228/ARe");
 Serial.println("Starting setup..."); 
   
// Attach servos to pins 5 and 6
srxservo.attach(5);
stxservo.attach(6);


if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    softReset();
  }


  // give the Ethernet shield a second to initialize:
  delay(1000);
  
 // print local IP address:
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
  

 // print local Gateway address:
  Serial.print("My Gateway address: ");
  Serial.println(Ethernet.gatewayIP());

// read maximum bandwidth values
// Call the xml Processor to obtain maximum useable bandwidth
netread(1); 
Serial.print("DSL maxTXspeed:");
  Serial.println(mxtx);
  Serial.print("DSL maxRXspeed:");
  Serial.println(mxrx);

  
 Serial.println(" Setup done.");
}

void loop()
{
 
//Not too hasty 
 delay(1000);
   
// Call the xml Processor to obtain currently used bandwidth
  netread(2);  
  
  //Display Send Speed
  Serial.print("The main loop sees sendspeed:");
  Serial.println(nsbl);
  txservo=map(nsbl,0,mxtx,179,1);
  Serial.print("Servo TX value: ");
  Serial.println(txservo);
  stxservo.write(txservo);


  //Display Receive Speed
  Serial.print("The main loop sees readspeed:");
  Serial.println(nrbl);
  rxservo=map(nrbl,0,mxrx,1,179);
  Serial.print("Servo RX value: ");
  Serial.println(rxservo);
  srxservo.write(rxservo);

  

}




/////////////////////
//  Functions      //
// Trying to keep  //
// the main loop   //
// clean           //
/////////////////////



/////////////////////
//  Serial Event   //
//read individual  //
//chars from the   //
//ethernet session //
//and assemble     //
//xml tags         //
/////////////////////

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
              nsbl=nsbl*8; //convert bytes to bits
      }
	  
	 if (matchTag("<NewByteReceiveRate>")) {
	      Serial.print("RX: ");
           // Serial.print(dataStr);
              nrbl = strtoul(dataStr,NULL,0);
              nrbl=nrbl*8;  //mach bits
      }
      

	 if (matchTag("<NewLayer1UpstreamMaxBitRate>"))
            {
	    //  Serial.print("MaxTX: ");
              mxtx = strtoul(dataStr,NULL,0);
            //  mxtx=mxtx*8;  //mach bits
            //  Serial.println(mxtx);
             }

	 if (matchTag("<NewLayer1DownstreamMaxBitRate>"))
            {
	    //  Serial.print("MaxRX: ");
           // Serial.print(dataStr);
              mxrx = strtoul(dataStr,NULL,0);
            //  mxrx=mxrx*8;  //mach bits
            //  Serial.println(mxrx);
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
//  ClearStr       //
//seems we have to //
//do that manually //
/////////////////////

// Function to clear a string
void clearStr (char* str) {
   int len = strlen(str);
   for (int c = 0; c < len; c++) {
      str[c] = 0;
   }
}






/////////////////////
//   addChar       //
// assemble String //
//from characters  //
/////////////////////


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





/////////////////////
//   matchTag      //
/////////////////////

// Function to check the current tag for a specific string
boolean matchTag (char* searchTag) {
   if ( strcmp(tagStr, searchTag) == 0 ) {
      return true;
   } else {
      return false;
   }
}


/////////////////////
//   softReset     //
// just in case.   //
// Not recommended //
/////////////////////


// In case all goes wrong
void softReset(){
asm volatile ("  jmp 0");
} 







/////////////////////
//   netread       //
/////////////////////

// Do the network reading stuff

void netread(int fselector)
{
 Serial.println("connecting...");
   // make the request to the server
   
   if (client.connect(Ethernet.gatewayIP(), 49000)) {
    Serial.println("connected");

   
    // Make a HTTP POST request:
    Serial.println("Sending POST request");
    client.println("POST /upnp/control/WANIPConn1 HTTP/1.1");
    client.print("HOST: ");
    client.print(Ethernet.gatewayIP());
    client.println(":49000");
    if (fselector==2){
    client.println("SOAPACTION: \"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1#GetAddonInfos\"");
    }
    if (fselector==1){
    client.println("SOAPACTION: \"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1#GetCommonLinkProperties\"");
    }
    client.println("Content-Type: text/xml; charset=\"utf-8\"");
    client.print("Content-Length: ");
   // client.println(xml.length());
     
    // With fselector set to 1 we request the maximum possible DSL bandwidth
    if (fselector==1)
      {
 // Serial.print("fselector ist auf: ");
 // Serial.println(fselector); 
        // client.println("309");
      client.println("292");
      client.println();
      client.println(F("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">\n<s:Body>\n<u:GetCommonLinkProperties xmlns:u=\"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1\" />\n</s:Body>\n</s:Envelope>"));  
      }
    
    // With fselector 2 we read the currently consumed bandwidth
    if (fselector==2)
      {
 // Serial.print("fselector ist auf: ");
 // Serial.println(fselector); 
      client.println("282");
      client.println();
      client.println(F("<?xml version=\"1.0\" encodin=\"utf-8\"?>\n<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">\n<s:Body>\n<u:GetAddonInfos xmlns:u=\"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1\" />\n</s:Body>\n</s:Envelope>"));
      }
    
    //client.println(xml);
    client.println();
    Serial.println("Post request sent");

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
  // Serial.println("Waiting for server...");
    ;
  }
  
  
  //get data
  while (client.available()) {
	serialEvent();
   }
     
 client.flush();
 Serial.println();
 Serial.print("Disconnecting...");
 client.stop();  
 Serial.println("done.");
 client.flush();  
 }

