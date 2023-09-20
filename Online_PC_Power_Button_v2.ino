#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ESP8266Ping.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

#define teleToken "6479367038:AAGnAfEmoE7bUxEsT5FsTanmRveFn9nB9hE"
#define devUser_id "1947379525"
#define device_pass "kerajinangentong"
IPAddress ip (192, 168, 1, 2);

String userID_List[8];
String waitingPass[8];
String waitingConfirmLongPress[8];
String urlRemote = "";
String urlSSH = "";
unsigned long ngrokCheck;


WiFiClientSecure client;
UniversalTelegramBot teleBot(teleToken, client);

#define def_ssid "BCT E2"
#define def_ssidPass "kosonginaja"

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
  urlRemote = ngrokURL("3389");
  urlSSH = ngrokURL("22");
}
void loop() {
  digitalWrite(relay_pin, HIGH);
  if (millis() - lastCheck > 1000)  {
    int numNewUpdate = teleBot.getUpdates(teleBot.last_message_received + 1);
    while(numNewUpdate) {
      Serial.println("try to handle number(s) of messages");
      handleNewMSG(numNewUpdate);
      Serial.println("try getting updates inside while loop");
      numNewUpdate = teleBot.getUpdates(teleBot.last_message_received + 1);
    }
    lastCheck = millis();  
  }
  if(WiFi.status() != WL_CONNECTED && millis() - lastConnect > 60000){
    Serial.println("Connection lost and try to connect");
    trySSID();
  }
  if(millis() - ngrokCheck >= 1800000){
    urlRemote = ngrokURL("3389");
    urlSSH = ngrokURL("22");
    ngrokCheck == millis();
  }
}
void connectWIFI(String ssid, String pass){
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to "+ WiFi.SSID()+"...");
    if(millis() - lastConnect >= 60000) break;
    delay(10000);
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
        Serial.println("Try Connect to "+ssidList[i]);
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
    Serial.println("Handling \t"+user_id+"("+name+") \tfrom "+chat_type);
    Serial.println("text : "+msg);
    if(msg == "/start" && !userAuth(user_id)){
      teleBot.sendMessage(user_id, "Give me the password");
      teleBot.sendMessage(devUser_id, user_id + "("+name+") trying to order the bot");
      Serial.println(user_id + "("+name+") trying to order the bot");
      addArr(waitingPass, sizeof(waitingPass) / sizeof(waitingPass[0]), user_id, "waitingPass");
    }
    if(msg == device_pass && !userAuth(user_id) && isAnyOnArray(waitingPass, sizeof(waitingPass) / sizeof(waitingPass[0]), user_id, "waitingPass")){
      //add user_id to userID_List
      addArr(userID_List, sizeof(userID_List)/ sizeof(userID_List[0]), user_id, "userID_List");
      // saveToWhitelist(user_id);
      Serial.println(user_id + " Added to userID_List/whitelist!!!");
      removeArr(waitingPass, sizeof(waitingPass) / sizeof(waitingPass[0]), user_id, "waitingPass");
      teleBot.sendMessage(user_id, "Now you have control to the bot");
      teleBot.sendMessage(devUser_id, "Now "+user_id+"("+name+") "+"have control to the bot");
    }
    if(msg == "/press" && userAuth(user_id)){
      broadcast("Someone try to pressing RL PC Power Button", user_id, name);
      teleBot.sendMessage(user_id, "You try to pressing RL PC Power Button");
      Serial.println("RL PC Power pressed");
      digitalWrite(relay_pin, LOW);
      delay(500);
      digitalWrite(relay_pin, HIGH);
    }
    if(msg == "/long_press" && userAuth(user_id)){
      Serial.println("Creating Keyboard Json for inline Keyboard");
      addArr(waitingConfirmLongPress, sizeof(waitingConfirmLongPress) / sizeof(waitingConfirmLongPress[0]), user_id, "waitingConfirmLongPress");
      Serial.println("send mesaage with inline keyboard");
      String keyboardJson = "[ [ { \"text\" : \"Yes\", \"callback_data\" : \"/yes_press_this_button_for_5S\" } ], [ { \"text\" : \"No\", \"callback_data\" : \"/no_don't_forcing_it\" } ] ]";
      teleBot.sendMessageWithInlineKeyboard(chat_id, "Choose from one of the following options", "", keyboardJson);
      broadcast("Someone try to forcing shutdown RL PC", user_id, name);
    }
    if(msg == "/yes_press_this_button_for_5S" && isAnyOnArray(waitingConfirmLongPress, sizeof(waitingConfirmLongPress) / sizeof(waitingConfirmLongPress[0]), user_id, "waitingConfirmLongPress")){
      broadcast("Someone forced shutdown RL PC", user_id, name);
      digitalWrite(relay_pin, LOW);
      delay(5000);
      digitalWrite(relay_pin, HIGH);
      teleBot.sendMessage(user_id, "The PC forced shutdown");
      removeArr(waitingConfirmLongPress, sizeof(waitingConfirmLongPress) / sizeof(waitingConfirmLongPress[0]), user_id, "waitingConfirmLongPress");
    }
    if(msg == "/no_don't_forcing_it" && isAnyOnArray(waitingConfirmLongPress, sizeof(waitingConfirmLongPress) / sizeof(waitingConfirmLongPress[0]), user_id, "waitingConfirmLongPress")){
      teleBot.sendMessage(user_id, "Forced shutdown undone");
      broadcast("Forced shutdown undone", user_id, name);
      removeArr(waitingConfirmLongPress, sizeof(waitingConfirmLongPress) / sizeof(waitingConfirmLongPress[0]), user_id, "waitingConfirmLongPress");
    }
    if(msg == "/ping" && userAuth(user_id)){
      if(Ping.ping(ip)){
        teleBot.sendMessage(user_id, "The PC is connected to" + WiFi.SSID());
      } else {
        teleBot.sendMessage(user_id, "The PC is not connected or power off");
      }
    }
    if(msg == "/remote" && userAuth(user_id)){
      teleBot.sendMessage(user_id, "Remote RL URL : "+urlRemote);
      Serial.println(user_id + " Mengambil url ngrok untuk 3389");
    }
    if(msg == "/ssh" && userAuth(user_id)){
      teleBot.sendMessage(user_id, "RL SSH URL : "+urlSSH);
      Serial.println(user_id + " Mengambil url ngrok untuk 22");
    }
    if(msg == "/refresh_ngrok" && userAuth(user_id)){
      urlRemote = ngrokURL("3389");
      urlSSH= ngrokURL("22");
      teleBot.sendMessage(user_id, "Ngrok URL refreshed");
    }
    // if(msg == "/user_list" && devUser_id){
    //   printUserIDListToSerial();
    // }
    if(msg == "/my_id"){
      Serial.println("your ID : "+user_id);
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
      Serial.println("Succes remove "+value+" from "+arrName);
      return true;
    }
  }
  Serial.println("Failed remove "+value+" from "+arrName);
  return false;
}
bool addArr(String array[], int size, String value, String arrName) {
  for (int i = 0; i < size; i++) {
    if (array[i] == value) {
      Serial.println("Failed add "+value+" to "+arrName);
      return false;
    }
  }
  for (int i = 0; i < size; i++) {
    if (array[i] == "") {
      array[i] = value;
      Serial.println("Succes add "+value+" to "+arrName);
      return true;
    }
  }
  return false;
}
bool isAnyOnArray(String array[], int size, String value, String arrName){
  for (int i = 0; i < size; i++) {
    if (array[i] == value) {
      Serial.println("Found "+value+" inside "+arrName);
      return true;
    }
  }
  Serial.println("Can't find "+value+" inside "+arrName);
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
  Serial.println("Broadcast send to whitelisted");
}
String ngrokURL(String port) {
  HTTPClient http; // Mendeklarasikan di sini, bukan di level global

  Serial.println("Try getting url for "+port);
  http.begin(client, "https://api.ngrok.com/tunnels");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("authorization", "Bearer 2LJSWpR99nzr96QSspKZPJHUwBt_78xmyb9xJunjTtorAWYgU");
  http.addHeader("ngrok-version", "2");

  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String response = http.getString();
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, response);

    JsonArray tunnels = doc["tunnels"].as<JsonArray>();

    for (JsonObject tunnel : tunnels) {
      if (tunnel["forwards_to"].as<String>() == ("localhost:" + port)) {
        String public_url = tunnel["public_url"].as<String>();
        public_url.replace("tcp://", "");  // Remove "tcp://" from the URL
        http.end();
        Serial.println("Get url succes");
        return public_url;
      }
    }
  } else {
    Serial.println("Error in HTTP request");
    http.end();
    Serial.println("Get url error");
    return "null";
  }
  http.end();
  Serial.println("Get url error/failed");
  return "";
}
