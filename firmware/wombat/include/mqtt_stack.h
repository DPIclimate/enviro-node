#ifndef WOMBAT_MQTT_STACK_H
#define WOMBAT_MQTT_STACK_H

bool c1Ready(void);

bool mqInit(void);
bool mqConnect(void);
void mqDisconnect(void);
int mqPublish(const std::string topic, const std::string msg);

#endif //WOMBAT_MQTT_STACK_H
