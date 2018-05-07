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

**********************************************************************/


#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
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

// You network info
const char* ssid = "Network_SSID";
const char* password = "your_pwd";

// API host and port
const char* host = "api.coinmarketcap.com";
const int httpsPort = 443;

// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "008713570d4ab603e3435ba107ecc7b69b";

// Response from the API
String currencyInfos;

// Use WiFiClientSecure class to create TLS connection
WiFiClientSecure client;

unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
//const long interval = 60000;

int bufferSize = 10000; // for ArduinoJSON

bool dataAvailable = false;


void setup() {
  Serial.begin(115200);
  
  P.begin(); // Start parola
  P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect); // configure parola
  
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

}

void loop() {

  // Every LOOPDURATION seconds
  currentMillis = millis();
  if(currentMillis - previousMillis >= LOOPDURATION) {
    previousMillis = currentMillis;   
    
    getCurrencyInfos();
    
    // Parsing  
    /* Because parseObject writes inside the jsonBuffer, we can't
    call it again, it will fail. So we create it again, every time we
    need it.*/
    DynamicJsonBuffer jsonBuffer(bufferSize); // Don't put that outside the loop 

    JsonArray& root = jsonBuffer.parseArray(currencyInfos);
    
    // Test if parsing succeeds.
    if (!root.success()) {
      Serial.println("parseObject() failed");
      return;
    }

    JsonArray& root_ = root;
    
    for(int i = 0; i < MAX_CURRENCIES ; i++){
      JsonObject& root_i = root_[i];
      st_currencies[i].symbol = root_[i]["symbol"];
      st_currencies[i].price_eur = root_[i]["price_eur"];
      st_currencies[i].percentage_change_1_hour = root_[i]["percent_change_1h"];
    }
    dataAvailable = true;
  }
  if(dataAvailable){
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
  String url = "/v1/ticker/?convert=EUR&limit="+String(MAX_CURRENCIES);
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");
  
  Serial.println("request sent");
  while (client.connected()) {
    currencyInfos = client.readStringUntil('\n');
    if (currencyInfos == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  currencyInfos = client.readStringUntil(']');
  currencyInfos += ']';
  
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(currencyInfos);
  Serial.println("==========");
  
  Serial.println("closing connection \n");
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


