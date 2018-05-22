/**********************************************************************;
* Project           : Crypto Display
*
* Program name      : CryptoDisplay
*
* Author            : Victor MEUNIER
*
* Date created      : 20180501
*
* Purpose           : Display top 10 cryptos currencies on the MAX7219 display 
*                     using an ESP8266 (µc : NodeMCU 0.9)
*
* Revision History  : 
*
* Date        Author        Ref     Revision (Date in YYYYMMDD format) 
* 20180501    V.Meunier     v1.0    Creation of v1.0 for sharing 
* 20180504    V.Meunier     v1.1    New API + NTP query for time

**********************************************************************/


#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <ArduinoJson.h>

// Define the number of devices we have in the chain and the hardware interface
#define MAX_DEVICES     4

// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define CLK_PIN         D5  // or SCK
#define DATA_PIN        D7  // or MOSI
#define CS_PIN          D8  // or SS

#define LOOPDURATION    60000

#define BUF_SIZE        75

#define MAX_CURRENCIES  10

// NTP time stamp is in the first 48 bytes of the message
#define NTP_PACKET_SIZE 48

#define WORKING_HOUR_START      7
#define WORKING_MINUTES_START   30
#define WORKING_SECONDES_START  0

#define WORKING_HOUR_STOP       23
#define WORKING_MINUTES_STOP    30
#define WORKING_SECONDES_STOP   0

// HARDWARE SPI
MD_Parola P = MD_Parola(CS_PIN, MAX_DEVICES);

// SOFTWARE SPI
//MD_Parola P = MD_Parola(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Scrolling parameters
uint8_t scrollSpeed = 40;    // default frame delay value
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 2000; // in milliseconds

// Global message buffers shared by Serial and Scrolling functions

char curMessage[BUF_SIZE] = { "CryptoDisplay" };
struct currency{
  const char * symbol;
  const char * price_eur;
  const char * percentage_change_1_hour;
};
currency st_currencies[MAX_CURRENCIES];
String arrToDisplay[MAX_CURRENCIES] = {"CryptoDisplay"};

// Your network info
const char* ssid = "Network_SSID";
const char* password = "Network_PWD";


// API host and port
const char* host = "chasing-coins.com";
const int httpsPort = 443;


// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "008713570d4ab603e3435ba107ecc7b69b";

// Response from the API
//String currencyInfos;
char currencyInfos[3000];

// local port to listen for UDP packets
unsigned int localPort = 2390;

//buffer to hold incoming and outgoing packets
byte packetBuffer[NTP_PACKET_SIZE]; 

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

// Use WiFiClientSecure class to create TLS connection
WiFiClientSecure client;

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;


unsigned long currentMillis = 0;
unsigned long previousMillis = 0;

int bufferSize = 3000; // for ArduinoJSON

bool dataAvailable = false;

long hours = 0;
long minutes = 0;
long seconds = 0;


void setup() {
  Serial.begin(115200);
  
  P.begin(); // Start parola
  P.displayText(curMessage, scrollAlign, scrollSpeed, 0, scrollEffect, scrollEffect); // configure parola
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // USE TLS connexion
  Serial.print("Connecting..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  
}

void loop() {
  // We are in the working hours  
  if( (WORKING_HOUR_STOP >= WORKING_HOUR_START && (((hours*3600)+(minutes*60)+seconds) < ((WORKING_HOUR_STOP*3600) + (WORKING_MINUTES_STOP*60) + WORKING_SECONDES_STOP) && 
      ((hours*3600)+(minutes*60)+seconds) > ((WORKING_HOUR_START*3600) + (WORKING_MINUTES_START*60) + WORKING_SECONDES_START)))
      || 
      (WORKING_HOUR_STOP < WORKING_HOUR_START && (hours <= WORKING_HOUR_STOP) && (((hours*3600)+(minutes*60)+seconds) < ((WORKING_HOUR_STOP*3600) + (WORKING_MINUTES_STOP*60) + WORKING_SECONDES_STOP) && 
      ((hours*3600)+(minutes*60)+seconds) < ((WORKING_HOUR_START*3600) + (WORKING_MINUTES_START*60) + WORKING_SECONDES_START)))
      || 
      (WORKING_HOUR_STOP < WORKING_HOUR_START && (hours > WORKING_HOUR_STOP) && (((hours*3600)+(minutes*60)+seconds) > ((WORKING_HOUR_STOP*3600) + (WORKING_MINUTES_STOP*60) + WORKING_SECONDES_STOP) && 
      ((hours*3600)+(minutes*60)+seconds) > ((WORKING_HOUR_START*3600) + (WORKING_MINUTES_START*60) + WORKING_SECONDES_START)))
  ){
    // Every LOOPDURATION seconds
    currentMillis = millis();
    if(currentMillis - previousMillis >= LOOPDURATION) {
      previousMillis = currentMillis;    
  
      //get a random server from the pool
      WiFi.hostByName(ntpServerName, timeServerIP);
      
      // send an NTP packet to a time server
      sendNTPpacket(timeServerIP); 
      
      //char * currencyInfos;
      getCurrencyInfos();
  
      checkNTPresponse();

      // Parsing  
      /* Because parseObject writes inside the jsonBuffer, we can't
      call it again, it will fail. So we create it again, every time we
      need it.*/
      DynamicJsonBuffer jsonBuffer(bufferSize); // Don't put that outside the loop 
      JsonObject& root = jsonBuffer.parseObject(currencyInfos);
      
      // Test if parsing succeeds.
      if (!root.success()) {
        Serial.println("parseObject() failed");
        return;
      }
      
      for(int i = 0; i < MAX_CURRENCIES; i++){
        JsonObject& root_i = root[String(i+1)];
        st_currencies[i].symbol = root_i["symbol"];
        st_currencies[i].price_eur = root_i["price"];
        st_currencies[i].percentage_change_1_hour = root_i["change"]["hour"];
      }
      dataAvailable = true;
      
    }
    if(dataAvailable){
      P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
      for(int i = 0; i < MAX_CURRENCIES ; i++){
        arrToDisplay[i] = "";       
        arrToDisplay[i] += st_currencies[i].symbol;
        arrToDisplay[i] += " ";
        arrToDisplay[i] += st_currencies[i].percentage_change_1_hour;
        arrToDisplay[i] += "%";
        arrToDisplay[i] += " ";
        // Maximum length of 6 char fits perfectly on the display
        for(int j = 0; j < 6; j++){
          arrToDisplay[i] += st_currencies[i].price_eur[j];
        }
        Serial.println(arrToDisplay[i]);
        dataAvailable = false;
      }
    }
    // DISPLAY what's inside arrToDisplay
    displayArray();  
  }
  
  // No display and put to sleep
  
  else{
    Serial.println("Go to sleep, but ask time before");
    P.displayClear();
    P.displayReset();
    //get a random server from the pool
    WiFi.hostByName(ntpServerName, timeServerIP);
    
    // send an NTP packet to a time server
    sendNTPpacket(timeServerIP); 

    delay(10000);  

    checkNTPresponse();
  }
}   


void getCurrencyInfos(){
  Serial.print("Connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }

  // API request
  String url = "/api/v1/top-coins/"+String(MAX_CURRENCIES);
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");
  Serial.println("request sent");
  
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\r\n');
  client.readStringUntil('\r').toCharArray(currencyInfos, 3000);
 
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(currencyInfos);
  Serial.println("==========");
  
  Serial.println("closing connection \n"); 
}

void checkNTPresponse(){
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);

    //Convert to ETC time, summer time
    epoch += 7200;

    // print the hour, minute and second:
    Serial.print("The ETC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    hours = (epoch  % 86400L) / 3600;
    Serial.print(hours); // print the hour (86400 equals secs per day)
    minutes = (epoch % 3600) / 60;
    Serial.print(':');
    if ( minutes < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print(minutes); // print the minute (3600 equals secs per minute)
    seconds = epoch % 60;
    Serial.print(':');
    if ( seconds < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(seconds); // print the second
  }  
}


void displayArray() {
  for(int i = 0; i < MAX_CURRENCIES ; i++){  
    if (P.displayAnimate()) // if finished displaying
    {
      arrToDisplay[i].toCharArray(curMessage, BUF_SIZE); // copy the current data to the display buffer
      P.displayReset(); // reset to display again
    }
  } 
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address){
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

