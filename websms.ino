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
//#include <EEPROM.h>
#include <WiFiClientSecure.h>
/*==========================*/
#include <DNSServer.h>
#include <WiFiManager.h> 
#include <ArduinoJson.h>
/*============================*/
#include <SoftwareSerial.h>
/*ADD YOUR PASSWORD BELOW*/

String apiurl = "https://******.com/sms/api";
String apikey = "1234567";

SoftwareSerial gsm(D5,D6);//  Gnd, D5-TX, D6-RX
WiFiManager wifimanager;

uint8_t KEY_RST   = D1; // declare LED pin on NodeMCU Dev Kit
uint8_t LED_PIN   = D2; // declare LED pin on NodeMCU Dev Kit
uint8_t LED_Pine  = D3; // declare LED pin on NodeMCU Dev Kit

String sendsms(String num,String msg);
void   gsmsetup(void);

void setup() {
    pinMode(LED_PIN, OUTPUT);
    pinMode(KEY_RST, INPUT_PULLUP);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.begin(9600);
    gsm.begin(9600);     
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    WiFiManagerParameter custom_text_box("apiurl", "Api url", custom_text_box.getValue(), 50);
    WiFiManagerParameter custom_text_box_num("apikey", "api number count(7)", custom_text_box_num.getValue(), 7);    
    // Add custom parameter
    wifimanager.addParameter(&custom_text_box);
    wifimanager.addParameter(&custom_text_box_num);
    bool res;
    res = wifimanager.autoConnect("websms",""); // password protected ap   
    if(!res)Serial.println("Failed to connect");// ESP.restart();
    else Serial.println("connected...yeey :)"); //if you get here you have connected to the WiFi    
    digitalWrite(LED_BUILTIN, HIGH);
    apiurl = custom_text_box.getValue();
    apikey = custom_text_box_num.getValue();
    Serial.println("apiurl:"+apiurl);
    Serial.println("apikey:"+apikey);
//===========================
  gsmsetup();  
}

void loop(){
      if(digitalRead(KEY_RST) == LOW){ 
          digitalWrite(LED_BUILTIN, LOW);
          Serial.println("factory defalut");
          delay(1000);
          digitalWrite(LED_BUILTIN, HIGH); 
          WiFi.disconnect();
          wifimanager.resetSettings();
          ESP.reset();
          ESP.restart();
          digitalWrite(LED_BUILTIN, LOW);  
          delay(1000);
          setup();              
     }
     
     if (WiFi.status() == WL_CONNECTED) {
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
              Serial.printf("[HTTPS] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
          }
          http.end();
          digitalWrite(LED_BUILTIN, LOW);
          delay(1500);
     }else{
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
     }
     
     //while (gsm.available()) {          //Displays on the serial monitor if there's a communication from the module
         //Serial.write(gsm.read()); 
     //}
}

// For data transmission from Serial to Software Serial port & vice versa
void gsmsetup() {
  //Begin serial communication with Arduino and SIM800L
  Serial.println("Initializing...");
  delay(1000);
  gsm.println("AT"); //Once the handshake test is successful, it will back to OK
  updateSerial();
  gsm.println("AT+CSQ"); //Signal quality test, value range is 0-31 , 31 is the best
  updateSerial();
  gsm.println("AT+CCID"); //Read SIM information to confirm whether the SIM is plugged
  updateSerial();
  gsm.println("AT+CREG?"); //Check whether it has registered in the network
  updateSerial();
    // Send attention command to check if all fine, it returns OK
  gsm.println("AT");
  updateSerial();
  // Configuring module in TEXT mode
  gsm.println("AT+CMGF=1");
  updateSerial();
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

// For data transmission from Serial to Software Serial port & vice versa
void updateSerial() {
  delay(500);
  while (gsm.available()) {
    Serial.write(gsm.read());//Forward what Software Serial received to Serial Port
  } 
}
