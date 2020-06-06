#ifndef MqttService_h
#define MqttService_h
#include "Arduino.h"
#include <PubSubClient.h>

class MqttService {
  public:
    MqttService(PubSubClient* mqttClient);
    void sendCommand(const char* topic, const char* value);
    void tick();
  //  void callback(char* topic, byte* payload, unsigned int length);
//    void setup(char* server, uint16_t port);
//    void setup(char* server);
  private:
    PubSubClient* mqttClient;
    void reconnect();
//    const char* &stringToChars(String value);
};
#endif
