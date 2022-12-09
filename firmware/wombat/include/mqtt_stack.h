#ifndef WOMBAT_MQTT_STACK_H
#define WOMBAT_MQTT_STACK_H

bool c1SetSystemTimeFromModem(void);

bool mqtt_login(void);
void mqtt_logout(void);
void mqtt_publish(String& topic, String& msg);

#endif //WOMBAT_MQTT_STACK_H
