#include "Arduino.h"
#include "MqttService.h"

MqttService::MqttService(PubSubClient* mqttClient) {
  this->mqttClient = mqttClient;
  randomSeed(micros());
}

//void MqttService::setup(char* server){
//  this->setup(server, 1883);
//}
//
//void MqttService::setup(char* server, uint16_t port){
//   mqttClient->setServer(server, port);
//  // reconnect();
//}

void MqttService::sendCommand(const char* topic,const char* value) {
  mqttClient->publish(topic, value);
  Serial.print("MQTT topic :");
  Serial.print(topic);
  Serial.print(" - value: ");
  Serial.println(value);
}

void MqttService::tick() {
    if (!mqttClient->connected()) {
    reconnect();
  }
  mqttClient->loop();  
}

void MqttService::reconnect() {
  // Loop until we're reconnected
  while (!mqttClient->connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "SwitchClient-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttClient->connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //mqttClient->publish("outTopic", "hello world");
      // ... and resubscribe
      for(int x = 0; x < 8; x++) {
        char powerTopic[14] = "powerState/"; // + String(x,DEC);
        itoa(x,powerTopic+11, 10);
        Serial.print("subscribed to ");
        Serial.println(powerTopic);
        mqttClient->subscribe(powerTopic);
        //mqttClient->subscribe(stringToChars("dimmerState/" + String(x,DEC)));
      }      
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient->state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
