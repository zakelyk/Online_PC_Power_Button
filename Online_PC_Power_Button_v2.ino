#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define teleToken "6589969405:AAFtzJMenzHnxfef0BiyI_0g1sfHbCczCJk"
#define dev_id "1947379525"
#define device_pass "kerajinangentong"

String userID_List[5];
String name_List[5];
String username_List[5];
String chatID_List[1];

WiFiClientSecure client;
UniversalTelegramBot teleBot(teleToken, client);

#define def_ssid "Rizky Lamp"
#define def_ssidPass "#gemini#"

String ssidList[1] = {"DBali"};
String ssidPassList[1] = {"dbali5386"};
unsigned long lastConnect, lastCheck;
int updateID = 0 , numNewUpdate = 0;

#define relay_pin D2

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  client.setInsecure();
  pinMode(relay_pin, HIGH);
  WiFi.mode(WIFI_STA);
  trySSID();
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(relay_pin, HIGH);
  if(updateID != 0){
    numNewUpdate = teleBot.getUpdates(updateID + 1); 
  } else if (updateID == 0){
    numNewUpdate = teleBot.getUpdates(); 
  }
  while (numNewUpdate){
    Serial.println("There new update from server");
    handleNewMSG(numNewUpdate);
  }

  if(WiFi.status() != WL_CONNECTED){
      trySSID();
  }
}

void connectWIFI(String ssid, String pass){
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print("Connecting to "+ WiFi.SSID());
    delay(500);
    Serial.print(".");
    delay(500);
    Serial.print(".");
    delay(400);
    Serial.println(".");
    if(millis() - lastConnect >= 60000) break;
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("Connected to: "+WiFi.SSID());
    Serial.println("Router (Gateway) IP address: " + WiFi.gatewayIP().toString());
    Serial.println("Device IP address: "+WiFi.localIP().toString());
  } else if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect "+ ssid );
  }
  lastConnect = millis();
}

void trySSID(){
  do{
    connectWIFI(def_ssid, def_ssidPass);
    if(WiFi.status() != WL_CONNECTED){
      for(int i = 0; i < sizeof(ssidList) / sizeof(ssidList[0]); i++){
        connectWIFI(ssidList[i], ssidPassList[i]);
      }
    }
    // if(WiFi.status() != WL_CONNECTED) delay(900000);
  } while (WiFi.status() != WL_CONNECTED);
}

void handleNewMSG(int numNewUpdate){
  for(int a = 0; a < numNewUpdate ; a++){
    String user_id = String(teleBot.messages[a].chat_id);
    String name = String(teleBot.messages[a].from_name);
    String username = String(teleBot.messages[a].);
  }
}


