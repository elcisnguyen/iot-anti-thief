//--------------------------------------------------------------------------
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
const char* mqtt_server = "broker.mqtt-dashboard.com";
//---------------------------------------------------
const int buzzer = D3;
const int relay = D2;
const int n = 2;
const int trig[n] = {D5, D7};
const int echo[n] = {D6, D8};
const int WAITING_TIME = 2;
const int GUEST_DISTANCE = 200;
const int THIEF_DISTANCE = 200;

bool turnOnPower = true;
bool turnOnBuzzer = false;
bool turnOnLight = false;

int startTime = 0;
int endTime = 0;
bool startCount = false;
//---------------------------------------------------
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
//---------------------------------------------------
//gets called when WiFiManager enters configuration mode - Ham co san
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup_WiFiManager(){
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // put your setup code here, to run once:
  Serial.begin(115200);
 
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  //reset settings - for testing
  // wm.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wm.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("SUCCESFULLY CONNECTED TO WIFI!");
}

//Nhan tin nhan tu node-red server
void callback(char* topic, byte* payload, unsigned int length) {
  // Switch on the LED if an 1 was received as first character
  // '1'/'2': powerOn/Off, '3'/'4': buzzerOn/Off, '5'/'6': lightOn/Off, '7'/'8'/'9': checkPower/Buzzer/Light.
  switch ((char)payload[0]) {
    case '1':
      turnOnPower = true;
      break;
    case '2':
      turnOnPower = false;
      break;
    case '3':
      turnOnBuzzer = true;  
      break;
    case '4':
      turnOnBuzzer = false;  
      break;
    case '5':
      turnOnLight = true;
      break;
    case '6':
      turnOnLight = false;
      break;
   case '7':
      if (turnOnPower)
        client.publish("elcis1210_send", "Device is on");
      else
        client.publish("elcis1210_send", "Device is off");
      break;
   case '8':
      if (turnOnBuzzer)
        client.publish("elcis1210_send", "Buzzer is on");
      else
        client.publish("elcis1210_send", "Buzzer is off");
      break;
   case '9':
      if (turnOnLight)
        client.publish("elcis1210_send", "Light is on");
      else
        client.publish("elcis1210_send", "Light is off");
      break;
  }
  //------------------------------------------------------------------
  turnOnBuzzer = turnOnPower && turnOnBuzzer;  
  turnOnLight = turnOnPower && turnOnLight;
}

//Ham co san
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("elcis1210_send", "hello world");
      // ... and resubscribe
      client.subscribe("elcis1210_receive");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//Setup cac chan esp INPUT/OUTPUT
void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(buzzer, OUTPUT);
  pinMode(relay, OUTPUT);
  for (int i = 0; i < n; i++){
    pinMode(trig[i], OUTPUT);
    pinMode(echo[i],INPUT);
  }
  //-----------------------------------------
  setup_WiFiManager();
  //-----------------------------------------
  Serial.begin(115200);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //------------------------------------------
  digitalWrite(BUILTIN_LED, HIGH);
}

//Lay khoang cach tu 2 ultrasonic sensors
float* getDistance(){
  float duration;
  float* distanceCm = new float[n];
  
  for (int i = 0; i < n; i++){
    digitalWrite(trig[i], LOW);
    delayMicroseconds(2);
    digitalWrite(trig[i], HIGH);
    delayMicroseconds(10);
    digitalWrite(trig[i], LOW);
  
    duration = pulseIn(echo[i], HIGH);
    distanceCm[i] = duration * 0.034 / 2;
  }

  Serial.print("Sensor 0: ");
  Serial.print(distanceCm[0]);
  Serial.print("Sensor 1: ");
  Serial.print(distanceCm[1]);
  Serial.println("---------------------");
  return distanceCm;
}

void guest_warning(float distance){
  endTime = millis();
    
    if (distance < GUEST_DISTANCE) {
        if (startCount == false) {
          startTime = millis();
          startCount = true;
        }

        // Calculate the gap between startTime and endTime
        int gaps = (endTime - startTime) / 1000;
        if(gaps >= WAITING_TIME) {
          client.publish("elcis1210_send", "There is a person standing in front of your door!");
          Serial.println("^^^^^^^^^^^^^^^^^^^^^^^^^^^");
          startCount = false;
        }
    } else {
      startCount = false;
    }
}

void thief_warning(float distance){
  if (distance < THIEF_DISTANCE){
      turnOnBuzzer = true;
      turnOnLight = true;
      client.publish("elcis1210_send", "THIEF IS IN YOUR HOUSE!!!!!");
    } 
}

//Ham xu ly chinh, loop to infinite
void loop() {
  if(turnOnPower){
    float* distance = getDistance();
    thief_warning(distance[0]);
    guest_warning(distance[1]);
    
    if (turnOnBuzzer)
      digitalWrite(buzzer, HIGH);
    else
      digitalWrite(buzzer, LOW);

    if (turnOnLight)
      digitalWrite(relay, HIGH);
    else
      digitalWrite(relay, LOW);
    
    delete[] distance;
  } else {
    digitalWrite(relay, LOW);
    digitalWrite(buzzer, LOW);
  }
  //------------------------------------------------------------
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
//
//  unsigned long now = millis();
//  if (now - lastMsg > 2000) {
//    lastMsg = now;
//    ++value;
//    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
//    Serial.print("Publish message: ");
//    Serial.println(msg);
//    client.publish("outTopic", msg);
//  }
  //------------------------------------------------------------
  delay(100);
}
