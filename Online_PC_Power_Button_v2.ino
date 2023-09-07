#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <EEPROM.h>
#include <ESP8266Ping.h>

#define teleToken "6589969405:AAFtzJMenzHnxfef0BiyI_0g1sfHbCczCJk"
#define devUser_id "1947379525"
#define device_pass "kerajinangentong"

String userID_List[7];
String waitingPass[7];
String waitingConfirmLongPress[7];

WiFiClientSecure client;
UniversalTelegramBot teleBot(teleToken, client);

#define def_ssid "Rizky Lamp"
#define def_ssidPass "#gemini#"

String ssidList[1] = {"DBali"};
String ssidPassList[1] = {"dbali5386"};
unsigned long lastConnect, lastCheck;
int numNewUpdate = 0, numWhitelist = 0;

#define relay_pin D2

void setup() {
  Serial.begin(9600);
  client.setInsecure();
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, HIGH);
  WiFi.mode(WIFI_STA);
  trySSID();
  EEPROM.begin(512);
  loadWhitelist();
}
void loop() {
  digitalWrite(relay_pin, HIGH);
  if (millis() - lastCheck > 1000)  {
    int numNewUpdate = teleBot.getUpdates(teleBot.last_message_received + 1);
    while(numNewUpdate) {
      // Serial.println("try to handle number(s) of messages");
      handleNewMSG(numNewUpdate);
      // Serial.println("try getting updates inside while loop");
      numNewUpdate = teleBot.getUpdates(teleBot.last_message_received + 1);
    }
    lastCheck = millis();  
  }
  if(WiFi.status() != WL_CONNECTED && millis() - lastConnect > 60000){
    // Serial.println("Connection lost and try to connect");
    trySSID();
  }
}
void connectWIFI(String ssid, String pass){
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    // Serial.println("Connecting to "+ WiFi.SSID()+"...");
    if(millis() - lastConnect >= 60000) break;
    delay(10000);
  }
  if(WiFi.status() == WL_CONNECTED){
    // Serial.println("Connected to: "+WiFi.SSID());
    // Serial.println("Router (Gateway) IP address: " + WiFi.gatewayIP().toString());
    // Serial.println("Device IP address: "+WiFi.localIP().toString());
  } else if (WiFi.status() != WL_CONNECTED) {
    // Serial.println("Failed to connect "+ ssid );
  }
  lastConnect = millis();
}
void trySSID(){
  do{
    connectWIFI(def_ssid, def_ssidPass);
    if(WiFi.status() != WL_CONNECTED){
      for(int i = 0; i < sizeof(ssidList) / sizeof(ssidList[0]); i++){
        // Serial.println("Try Connect to "+ssidList[i]);
        connectWIFI(ssidList[i], ssidPassList[i]);
      }
    }
    if(WiFi.status() != WL_CONNECTED) delay(450000);
  } while (WiFi.status() != WL_CONNECTED);
}
void handleNewMSG(int numNewUpdate){
  for(int a = 0; a < numNewUpdate ; a++){
    String user_id = String(teleBot.messages[a].from_id);
    String name = String(teleBot.messages[a].from_name);
    String chat_id = String(teleBot.messages[a].chat_id);
    String chat_type = String(teleBot.messages[a].type);
    String msg = String(teleBot.messages[a].text);
    // Serial.println("Handling \t"+user_id+"("+name+") \tfrom "+chat_type);
    // Serial.println("text : "+msg);
    if(msg == "/start" && !userAuth(user_id)){
      teleBot.sendMessage(user_id, "Give me the password");
      teleBot.sendMessage(devUser_id, user_id + "("+name+") trying to order the bot");
      // Serial.println(user_id + "("+name+") trying to order the bot");
      addArr(waitingPass, sizeof(waitingPass) / sizeof(waitingPass[0]), user_id, "waitingPass");
    }
    if(msg == device_pass && !userAuth(user_id)){
      //add user_id to userID_List
      addArr(userID_List, sizeof(userID_List)/ sizeof(userID_List[0]), user_id, "userID_List");
      saveToWhitelist(user_id);
      // Serial.println(user_id + " Added to userID_List/whitelist!!!");
      removeArr(waitingPass, sizeof(waitingPass) / sizeof(waitingPass[0]), user_id, "waitingPass");
      teleBot.sendMessage(user_id, "Now you have control to the bot");
      teleBot.sendMessage(devUser_id, "Now "+user_id+"("+name+") "+"have control to the bot");
    }
    if(msg == "/rl_press" && userAuth(user_id)){
      broadcast("Someone try to pressing RL PC Power Button", user_id, name);
      teleBot.sendMessage(user_id, "You try to pressing RL PC Power Button");
      // Serial.println("RL PC Power pressed");
      digitalWrite(relay_pin, LOW);
      delay(500);
      digitalWrite(relay_pin, HIGH);
    }
    if(msg == "/rl_long_press" && userAuth(user_id)){
      // Serial.println("Creating Keyboard Json for inline Keyboard");
      String keyboardJson = "{\"inline_keyboard\":[[{\"text\":\"Yes\",\"callback_data\":\"/yes_press_this_button_for_5S\"},{\"text\":\"No\",\"callback_data\":\"/no_dont_frocing_it\"}]]}"; 
      addArr(waitingConfirmLongPress, sizeof(waitingConfirmLongPress) / sizeof(waitingConfirmLongPress[0]), user_id, "waitingConfirmLongPress");
      // Serial.println("send mesaage with inline keyboard");
      teleBot.sendMessageWithInlineKeyboard(chat_id, "Are you sure to do this thing?", "", keyboardJson);
      broadcast("Someone try to forcing shutdown RL PC", user_id, name);
    }
    if(msg == "/yes_press_this_button_for_5S" && isAnyOnArray(waitingConfirmLongPress, sizeof(waitingConfirmLongPress) / sizeof(waitingConfirmLongPress[0]), user_id, "waitingConfirmLongPress")){
      digitalWrite(relay_pin, LOW);
      teleBot.sendMessage(user_id, "The PC forced shutdown");
      broadcast("Someone forced shutdown RL PC", user_id, name);
      removeArr(waitingConfirmLongPress, sizeof(waitingConfirmLongPress) / sizeof(waitingConfirmLongPress[0]), user_id, "waitingConfirmLongPress");
      delay(5000);
      digitalWrite(relay_pin, HIGH);
    }
    if(msg == "/no_don't_frocing_it" && isAnyOnArray(waitingConfirmLongPress, sizeof(waitingConfirmLongPress) / sizeof(waitingConfirmLongPress[0]), user_id, "waitingConfirmLongPress")){
      teleBot.sendMessage(user_id, "Forced shutdown undone");
      broadcast("Forced shutdown undone", user_id, name);
      removeArr(waitingConfirmLongPress, sizeof(waitingConfirmLongPress) / sizeof(waitingConfirmLongPress[0]), user_id, "waitingConfirmLongPress");
    }
    if(msg == "/rl_ping" && userAuth(user_id)){
      if(Ping.Ping(192.168.1.2)){
        teleBot.sendMessage(user_id, "The PC is connected to" + WiFi.SSID());
      } else {
        teleBot.sendMessage(user_id, "The PC is not connected or power off");
      }
    }
    if(msg == "/rl_remote" && userAuth(user_id)){
     
    }
  }
}
bool userAuth(String user_id){
  for(int i = 0; i < sizeof(userID_List) / sizeof(userID_List[0]) ; i++){
    if(user_id == userID_List[i]) return true;
  }
  if(user_id == devUser_id) return true; 
  return false;
}
bool removeArr(String array[], int size, String value, String arrName) {
  for (int i = 0; i < size; i++) {
    if (array[i] == value) {
      array[i] = "";
      // Serial.println("Succes remove "+value+" from "+arrName);
      return true;
    }
  }
  // Serial.println("Failed remove "+value+" from "+arrName);
  return false;
}
bool addArr(String array[], int size, String value, String arrName) {
  for (int i = 0; i < size; i++) {
    if (array[i] == value) {
      // Serial.println("Failed add "+value+" to "+arrName);
      return false;
    }
  }
  for (int i = 0; i < size; i++) {
    if (array[i] == "") {
      array[i] = value;
      // Serial.println("Succes add "+value+" to "+arrName);
      return true;
    }
  }
  return false;
}
bool isAnyOnArray(String array[], int size, String value, String arrName){
  for (int i = 0; i < size; i++) {
    if (array[i] == value) {
      // Serial.println("Found "+value+" inside "+arrName);
      return true;
    }
  }
  // Serial.println("Can't find "+value+" inside "+arrName);
  return false;
}
void broadcast(String msg, String trig_User_id, String trigByName){
  if(trig_User_id != String(devUser_id)){
    teleBot.sendMessage(devUser_id,"\""+msg+"\" : triggred by "+ trig_User_id+"("+trigByName+")");
  }
  for(int i=0; i< sizeof(userID_List) / sizeof(userID_List[0]); i++){
    if(trig_User_id != userID_List[i] && userID_List[i] != ""){
      teleBot.sendMessage(userID_List[i], msg);
    }
  }
  // Serial.println("Broadcast send to whitelisted");
}
void loadWhitelist(){
  for (int i = 0; i < 7; i++) {
    userID_List[i] = readEEPROM(i * 15);  // Misalkan setiap string maksimal 15 karakter
    if (userID_List[i] != "") {
      numWhitelist++;
    }
  }
}
void saveToWhitelist(String user_id){
  if(numWhitelist == 0){
    userID_List[numWhitelist]=user_id;
    writeEEPROM(numWhitelist * 15, user_id);
    numWhitelist++;
  } else {
    // Serial.println("Array sudah penuh!");
  }
}
String readEEPROM(int addr){
  byte length = EEPROM.read(addr);
  String value = "";
  for(int i = 0; i < length;i++){
    value += (char)EEPROM.read(addr + 1 + i);
  }
  return value;
}
void writeEEPROM(int addr, String value){
  byte length = value.length();
  EEPROM.write(addr, length);
  for(int i = 0; i<length;i++){
    EEPROM.write(addr + 1 +i, value[i]);
  }
  EEPROM.commit();
}