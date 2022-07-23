/*D0 GPIO16
D1 GPIO5
D2 GPIO4
D3 GPIO0
D4 GPIO2
D5 GPIO14
D6 GPIO12
D7 GPIO13
D8 GPIO15
D9/RX GPIO3
D10/TX GPIO1
D11/SD2 GPIO9
D12/SD3 GPIO10
time  04/01/01,00:05:15+32"

*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>
/*==========================*/
#include <WiFiManager.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
/*============================*/
#include <SoftwareSerial.h>
/*ADD YOUR PASSWORD BELOW*/
//const char *password = "";
String ssid     = "";
String password = "";
String apiurl   = "";
String apikey   = "";
int value;
/*eprom------------*/
void apirequst(void);
void defalut(void);
bool testWifi(void);
void readeprom(void);
void writeprom(void);

String sendsms(String num,String msg);
void  gsmsetup(void);
bool  servermode = false;
float f = 0.00f;
uint8_t KEY_RST   = D1; // declare LED pin on NodeMCU Dev Kit
uint8_t LED_PIN   = D0; // declare LED pin on NodeMCU Dev Kit

SoftwareSerial gsm(D5,D6);//  Gnd, D5-TX, D6-RX
WiFiManager wifimanager;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    pinMode(KEY_RST, INPUT_PULLUP);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(LED_PIN, LOW);
    Serial.begin(9600);
    gsm.begin(9600); 
    EEPROM.begin(512); 
    delay(500);
    readeprom();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);// Connect to Wi-Fi
    servermode = false;
    if (digitalRead(KEY_RST) == LOW || !testWifi()){
        digitalWrite(LED_PIN, LOW);
        WiFi.mode(WIFI_AP); 
        const char *url = apiurl.c_str();
        const char *key = apikey.c_str();
        WiFiManagerParameter custom_text_box("apiurl", "Api url",url, 50);
        WiFiManagerParameter custom_text_box_num("apikey", "api number count(7)", key, 7);    
        // Add custom parameter
        wifimanager.addParameter(&custom_text_box);
        wifimanager.addParameter(&custom_text_box_num);
        bool res;
        res = wifimanager.autoConnect("websms",""); // password protected ap   
        if(!res)Serial.println("Failed to connect");// ESP.restart();
        else Serial.println("connected...yeey :)"); //if you get here you have connected to the WiFi    
        digitalWrite(LED_BUILTIN, HIGH);
        WiFi.persistent(true);
        WiFi.setAutoConnect(true);
        WiFi.setAutoReconnect(true);
        
        ssid = WiFi.SSID();
        password = WiFi.psk();
        apiurl = custom_text_box.getValue();
        apikey = custom_text_box_num.getValue();
        Serial.println("READ EPROM ssid:"+ssid+",password:"+password+",apiurl:"+apiurl+",apikey:"+apikey);
        writeprom();
        servermode = true; 
        wifimanager.resetSettings();
        ESP.reset(); 
    } 
//===========================
  if(!servermode){
    gsmsetup(); 
    WiFi.mode(WIFI_STA);
  }
}

void loop(){
    if(digitalRead(KEY_RST) == LOW){ 
          defalut();          
    }
    if (!servermode && WiFi.status() == WL_CONNECTED){
          digitalWrite(LED_PIN, HIGH);
          apirequst();
          digitalWrite(LED_PIN, LOW);
          delay(1000);
    }
    if(!servermode && WiFi.status() != WL_CONNECTED){
      digitalWrite(LED_PIN, LOW);
      delay(500);
      digitalWrite(LED_PIN, HIGH);
      delay(500);
    }
}

void defalut() {
    digitalWrite(LED_PIN, LOW);
    Serial.println("factory defalut");
    WiFi.disconnect();
    wifimanager.resetSettings();
    for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.write(i, 0);
    }
    Serial.println("clearing eeprom");
    delay(500);
    WiFi.persistent(false);
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(false);
    ESP.reset();
    ESP.restart(); 
}


void apirequst() {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    digitalWrite(LED_BUILTIN, HIGH);  
    Serial.println(apiurl); 
    String fullurl = apiurl + "/hello/"+apikey;
          http.begin(client,fullurl);
          http.addHeader("Content-Type", "text/plain");
          int httpCode = http.GET();
          String data = http.getString();
          //Serial.println(httpCode);
          Serial.println(data);
          if(httpCode > 0) {
              if(httpCode == HTTP_CODE_OK && data.indexOf(",") > -1) {
                int idindex   = data.indexOf(",");
                String id     = data.substring(0,idindex);
                int numindex  = data.indexOf(",",10);
                String num    = data.substring(idindex+1,numindex);
                int msgindex  = data.indexOf(",",20);
                String msg    = data.substring(msgindex+1);
                if(id !="invalid" && num.length() > 10 && msg.length() > 10){
                    digitalWrite(LED_PIN, LOW);
                    String msgresult = sendsms(num,msg);
                    ///==============
                    String time = String(gsm.print("AT+CCLK?\r"));
                    ///==============
                    HTTPClient result;
                    String fullurl = apiurl + "/"+msgresult+"/"+id+"/"+time;
                    result.begin(client,fullurl);
                    result.addHeader("Content-Type", "text/plain");
                    int httpCodes = result.GET();
                    if(httpCodes == HTTP_CODE_OK){
                    Serial.println(result.getString());
               }
               result.end();
               digitalWrite(LED_PIN, HIGH);
            }else{
               Serial.println("Message Not Sending");
               HTTPClient result;
               String fullurl = apiurl + "/error/"+id+"/";
               result.begin(client,fullurl);
               int httpCodes = result.GET();
               result.end();
           }
        }
     }else {
         digitalWrite(LED_PIN, LOW);
         delay(10);
         Serial.printf("[HTTPS] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
         digitalWrite(LED_PIN, HIGH);
         delay(10);
         digitalWrite(LED_PIN, LOW);
         delay(10);
         digitalWrite(LED_PIN, HIGH);
         digitalWrite(LED_PIN, LOW);
         delay(10);
     }
     http.end();
     delay(1500);
}


// For data transmission from Serial to Software Serial port & vice versa
void gsmsetup() {
  //Begin serial communication with Arduino and SIM800L
  Serial.println("Initializing...");
  delay(500);
  gsm.println("AT"); //Once the handshake test is successful, it will back to OK
  while (gsm.available()) {
    Serial.write(gsm.read());//Forward what Software Serial received to Serial Port
  } 
  gsm.println("AT+CSQ"); //Signal quality test, value range is 0-31 , 31 is the best
  while (gsm.available()) {
    Serial.write(gsm.read());//Forward what Software Serial received to Serial Port
  } 
  gsm.println("AT+CCID"); //Read SIM information to confirm whether the SIM is plugged
  while (gsm.available()) {
    Serial.write(gsm.read());//Forward what Software Serial received to Serial Port
  } 
  gsm.println("AT+CREG?"); //Check whether it has registered in the network
  while (gsm.available()) {
    Serial.write(gsm.read());//Forward what Software Serial received to Serial Port
  } 
    // Send attention command to check if all fine, it returns OK
  gsm.println("AT");
  while (gsm.available()) {
    Serial.write(gsm.read());//Forward what Software Serial received to Serial Port
  } 
  // Configuring module in TEXT mode
  gsm.println("AT+CMGF=1");
  while (gsm.available()) {
    Serial.write(gsm.read());//Forward what Software Serial received to Serial Port
  } 
}

String sendsms(String num = "",String msg = ""){  
  gsm.print("AT+CMGF=1\r");                   //Set the module to SMS mode
  delay(100);
  gsm.print("AT+CMGS=\""+num+"\"\r");  //Your phone number don't forget to include your country code, example +212123456789"
  delay(500);
  gsm.print(msg);       //This is the text to send to the phone number, don't make it too long or you have to modify the SoftwareSerial buffer
  delay(500);
  gsm.print((char)26);// (required according to the datasheet)
  delay(500);
  gsm.println();
  //Serial.println("message sending success... ,"+num+" ,"+msg);
  int i=0;
  while (gsm.available()){
    i++; 
    String readgsm =   gsm.readString();
    Serial.println(" sending "+readgsm  + " /r/n");
    delay(500);
  }
  
  if( i == 0){
     Serial.write("Message Not Sending "+i);
     return "error"; 
  }else{
    return "success";
  }
}

bool testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(500);
    Serial.print("*");
    c++;
  }
  Serial.println("Connect timed out, opening AP");
  return false;
}

void readeprom() {
    ssid = ""; password = ""; apiurl = ""; apikey = "";
    for (int i = 0; i < 32; ++i)
    {
      value = EEPROM.read(i);
      if(value != '\0')ssid += (char)(value);
    }
    for (int i = 32; i < 64; ++i)
    {
      value = EEPROM.read(i);
      if(value != '\0')password += (char)(value);
    }
    for (int i = 64; i < 96; ++i)
    {
      value = EEPROM.read(i);
      if(value != '\0')apikey += (char)(value);
    }
    for (int i = 96; i < 160; ++i)
    {
      value = EEPROM.read(i);
      if(value != '\0')apiurl += (char)(value);
    }   
    Serial.println("READ EPROM ssid:"+ssid+",password:"+password+",apiurl:"+apiurl+",apikey:"+apikey);
}

void writeprom()
{
   for (int i = 0; i < 512; ++i) {
          EEPROM.write(i, 0);
   }
   
   for (int i = 0; i < ssid.length(); ++i)
   {
      EEPROM.write(i, ssid[i]);
   }
   
   for (int i = 0; i < password.length(); ++i)
   {
      EEPROM.write(32 + i, password[i]);
   }
   
    for (int i = 0; i < apikey.length(); ++i)
    {
       EEPROM.write(64 + i, apikey[i]);
    }
    for (int i = 0; i < apiurl.length(); ++i)
    {
       EEPROM.write(96 + i, apiurl[i]);
    }
    EEPROM.commit();
    Serial.print("writing eeprom DONE:");
}
