#ifndef WOMBAT_MQTT_STACK_H
#define WOMBAT_MQTT_STACK_H

bool mqtt_login(void);
bool mqtt_logout(void);
bool mqtt_publish(String& topic, const char * const msg);

#endif //WOMBAT_MQTT_STACK_H
