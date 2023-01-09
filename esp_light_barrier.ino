/*******************************************************************************************
* Features:
*   - WiFi connection/ reconnection
*   - MQTT connection/ reconnection
*   - Publish data to MQTT topic
* Created for a ESP8266/ NodeMCU project
* 
* 
* ******************************************************************************************
*/
 

/** Some libraries **/
/** -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- **/
#include <ArduinoJson.h>

/** WLAN **/
/** -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- **/
#include <ESP8266WiFi.h>
#include <Ticker.h>

#define WIFI_SSID "<WIFI SSID>"
#define WIFI_PASSWORD "<WIFI PASSWORD>"

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;



/** MQTT **/
/** -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- **/
#include <AsyncMqttClient.h>

#define MQTT_HOST IPAddress(192, 168, 2, 83)
#define MQTT_PORT 1883
static const char mqttUser[] = "lightbarrier";
static const char mqttPassword[] = "<MQTT PASSWORD>";

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;




/** -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- **/

static bool WIFI_STATUS = false;
static bool MQTT_STATUS = false;

static const char MQTT_PUBLISH_TOPIC[] = "esp8266/sensor/lightbarrier/arbeitszimmer/state";
static const char MQTT_SENSOR_NAME[] = "lightbarrier-arbeitszimmer";
static const char MQTT_NAME[] = "Light barrier Arbeitszimmer";


/**
 * SETUP
 */
void setup() {
  Serial.begin(9600);
  delay(3000);

  Serial.println("== [SETUP]: started ==");

 
  // Register MQTT Events/Callbacks 
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(mqttUser, mqttPassword);
  

  
  // Setup WiFi
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
    WiFi.onEvent([](WiFiEvent_t e) {
        Serial.printf("Event wifi -----> %d\n", e);

        switch(e) {
          case WIFI_EVENT_STAMODE_GOT_IP:
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());

            connectToMqtt();
            Serial.println("== [SETUP]: MQTT ready ==");

            break;
          case WIFI_EVENT_STAMODE_DISCONNECTED:
            Serial.println("WiFi lost connection");
            break;
        }

    });
  connectToWifi();
  Serial.println("== [SETUP]: WiFi ready ==");  

  Serial.println("== [SETUP]: completed ==");
}




/** -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- **/
int lastValue = -1;
int lastPublishedValue = -1;

/**
 * LOOP
 */
void loop() {
  int currentValue = analogRead(A0);
  if (lastValue == -1) {
    // Initialize in first loop
    lastValue = currentValue;
  }

  float changeValue = ((lastValue - currentValue) / currentValue) * 100;

  Serial.println(currentValue);
  Serial.println(changeValue);

  if (changeValue < 0) {
    changeValue *= -1;
  }

  if (changeValue > 20 && lastPublishedValue != 1) {
    publishToMqtt(MQTT_SENSOR_NAME, MQTT_NAME, MQTT_PUBLISH_TOPIC, 1);
    Serial.println("PERSON DETECTED !!!!!!!!!");
    lastPublishedValue = 1;
    delay(1000);
  }
  else if (lastPublishedValue != 0) {
    publishToMqtt(MQTT_SENSOR_NAME, MQTT_NAME, MQTT_PUBLISH_TOPIC, 0);
    lastPublishedValue = 0;
  }

  lastValue = currentValue;
  Serial.println("===============");
  delay(100);
  return;
}

/** -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- **/



/**
 * Publish data to mqtt
 */
void publishToMqtt(const char* sensorName, const char* mqttName, const char* topic, float value) {
  // Publish sensor data via MQTT
  StaticJsonDocument<400> payload;
  payload["sensor"] = sensorName;
  payload["name"] = mqttName;
  payload["time"] = 1351824120;
  payload["value"] = value;

  // Convert payload to a json String and then into a char array for MQTT transport
  String jsonString;
  serializeJson(payload, jsonString);
  char mqttPayload[jsonString.length()+1];
  jsonString.toCharArray(mqttPayload, jsonString.length()+1);

  // Debug print the MQTT json payload
  Serial.println(mqttPayload);

  // Publish
  uint16_t packetIdPub1 = mqttClient.publish(topic, 1, true, mqttPayload);
  Serial.println("[MQTT]: Publish sensor data via MQTT");

  if (MQTT_STATUS) {
    Serial.println("[MQTT]: STATUS OK");    
  } else {
    Serial.println("[MQTT]: STATUS NOT!! OK");
  }
}



/** -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- **/



/**
 * MQTT connect
 */
void connectToMqtt() {
  Serial.println("[MQTT]: Connecting to MQTT...");
  mqttClient.connect();
}

/**
 * MQTT is connected
 */
void onMqttConnect(bool sessionPresent) {
  Serial.println("[MQTT]: Connected to MQTT");
  MQTT_STATUS = true;

  // uint16_t packetIdSub = mqttClient.subscribe("esp8266/sensor/SENSOR_NAME/command/ping", 0);
  // Serial.print("Subscribing to COMMAND topic");
}

/**
 * 
 */
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT");
  String text;
  switch( reason) {
  case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:
     text = "TCP_DISCONNECTED"; 
     break; 
  case AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION:
     text = "MQTT_UNACCEPTABLE_PROTOCOL_VERSION"; 
     break; 
  case AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED:
     text = "MQTT_IDENTIFIER_REJECTED";  
     break;
  case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE: 
     text = "MQTT_SERVER_UNAVAILABLE"; 
     break;
  case AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS:
     text = "MQTT_MALFORMED_CREDENTIALS"; 
     break;
  case AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED:
     text = "MQTT_NOT_AUTHORIZED"; 
     break;
  }

  Serial.printf(" [%8u] Disconnected from the broker reason = %s\n", millis(), text.c_str() );

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
    MQTT_STATUS = false;
  }
}

/**
 * MQTT receive a Message
 */
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.println(payload);
}


/** -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- **/


/**
 * WiFi connect
 */
void connectToWifi() {
  Serial.println("[WiFi]: Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

/**
 * WiFi is connected
 */
void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("[WiFi]: Connected to Wi-Fi.");
  WIFI_STATUS = true;
  connectToMqtt();
}

/**
 * WiFi is disconnected
 */
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("[WiFi]: Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}
