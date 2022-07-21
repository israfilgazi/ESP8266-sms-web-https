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
//#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
/*============================*/
#include <SoftwareSerial.h>
/*ADD YOUR PASSWORD BELOW*/
String ssid     = "";
String password = "";
String apiurl   = "";
String apikey   = "";
String content  = "";
String st       = "";
int statusCode  = 0;
int value;
/*eprom------------*/
void apirequst(void);
void defalut(void);
bool testWifi(void);
void createWebServer(void);
void setupAP(void);
void scanAP(void);
void readeprom(void);

String sendsms(String num,String msg);
void  gsmsetup(void);
bool  servermode = false;
float f = 0.00f;
uint8_t KEY_RST   = D1; // declare LED pin on NodeMCU Dev Kit
uint8_t LED_PIN   = D0; // declare LED pin on NodeMCU Dev Kit

SoftwareSerial gsm(D5,D6);//  Gnd, D5-TX, D6-RX
ESP8266WebServer server(80);

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
    WiFi.mode(WIFI_AP);
    WiFi.begin(ssid, password);// Connect to Wi-Fi
    servermode = false;
    if (digitalRead(KEY_RST) == LOW || !testWifi()){
        setupAP();
        scanAP();
        createWebServer();
        servermode = true;   
    } 
//===========================
  if(!servermode){
    gsmsetup(); 
    WiFi.mode(WIFI_STA);
    WiFi.persistent(true);
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
  }
}

void loop(){
    if(digitalRead(KEY_RST) == LOW && !servermode){ 
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
    
    if(servermode){
        server.handleClient(); //this is required for handling the incoming requests
        //Serial.println("handleClient");
    }
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
void defalut() {
    digitalWrite(LED_PIN, LOW);
    Serial.println("factory defalut");
    WiFi.disconnect();
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
         Serial.printf("[HTTPS] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
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
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void setupAP(void)
{
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  
  delay(100);
    
  IPAddress local_IP(192,168,1,1);
  IPAddress gateway(192,168,1,10);
  IPAddress subnet(255,255,255,0);

  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");
  Serial.println(WiFi.softAP("websms") ? "Ready" : "Failed!");
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.softAPIP());
  Serial.println("Initializing_softap_for_wifi credentials_modification");
  Serial.println("over");
}
void scanAP(){
  delay(100);
  int n = WiFi.scanNetworks();
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);
    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100); 
}

void createWebServer()
{  
    server.on("/", []() {
      //Serial.println(WiFi.localIP());
      //Serial.println(WiFi.softAPIP());
      //IPAddress ip = WiFi.softAPIP();
      //String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html><head><meta name='viewport' content='width=device-width, initial-scale=1'><style>";
      content += ".w-100{width:100%;display: inline-block;} body{font-family: Arial, Helvetica, sans-serif;} form{border: 3px solid #f1f1f1;}";
      content += "input[type=text], input[type=password] {width: 100%;padding: 12px 20px;margin: 8px 0;display: inline-block;border: 1px solid #ccc;box-sizing: border-box;}";
      content += "input[type=submit] {background-color: #04AA6D;color: white;padding: 14px 20px;margin: 8px 0;border: none;cursor: pointer;width: 100%;}";
      content += "</style></head><body class='w-100'>Welcome to Wifi Credentials Update page";
      content += "<form action=\"/scan\" method=\"post\"><input type=\"submit\" value=\"scan\"></form>";
      content += "<form action=\"/restart\" method=\"post\"><input type=\"submit\" value=\"Restart\"></form>";
      //content += ipStr;
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><p><label>SSID:   </label><input name='ssid' value='"+ssid+"' length=32></p><p><label>Password: </label><input name='pass' value='"+password+"' length=32></p><p><label>ApiUrl:  </label><input name='url' value='"+apiurl+"' length=64></p><p><label>Apikey:   </label><input name='key' value='"+apikey+"' length=32><input type='submit'></p></form>";
      content += "</body></html>";
      server.send(200, "text/html", content);
    });
    
    server.on("/scan", []() {
      scanAP();
      //IPAddress ip = WiFi.softAPIP();
      //String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      //content = "<!DOCTYPE HTML>\r\n<html>go back";
      //server.send(200, "text/html", content);
      server.sendHeader("Location", String("/"), true);
      server.send ( 302, "text/plain", "");
    });
    
    server.on("/restart", []() {
        server.sendHeader("Location", String("/"), true);
        server.send ( 302, "text/plain", "");
        delay(5000);
        ESP.reset();
    });
    
    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      String qpurl = server.arg("url");
      String qpkey = server.arg("key");
      if (qsid.length() > 0 && qpurl.length() > 0 && qpkey.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 512; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println("writing eeprom.. ssid:"+qsid+",password:"+qpass+",apiurl:"+qpurl+",apikey:"+qpkey);
        
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
        }
        
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
        }

        for (int i = 0; i < qpkey.length(); ++i)
        {
          EEPROM.write(64 + i, qpkey[i]);
        }
        
        for (int i = 0; i < qpurl.length(); ++i)
        {
          EEPROM.write(96 + i, qpurl[i]);
        }
        qsid = ""; qpass = ""; qpurl = ""; qpkey = "";
        EEPROM.commit();
        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        //server.sendHeader("Access-Control-Allow-Origin", "*");
        //server.send(statusCode, "application/json", content);
        readeprom();
        server.sendHeader("Location", String("/"), true);
        server.send ( 302, "text/plain", "");
      }else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(statusCode, "application/json", content);
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
    });
  
  server.begin();// Start server
  Serial.print("Start server:");
}
