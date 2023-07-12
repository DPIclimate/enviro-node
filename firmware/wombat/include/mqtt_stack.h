#ifndef WOMBAT_MQTT_STACK_H
#define WOMBAT_MQTT_STACK_H

bool mqtt_login(void);
bool mqtt_logout(void);
/**
 * Publish a message to the MQTT broker.
 *
 * @param topic the topic to publish to.
 * @param msg the message to send - this is expected to be a UTF-8 text message.
 * @param msg_len the length of the message in units of uint8_t.
 * @return true if the message was published, otherwise false.
 */
bool mqtt_publish(String& topic, const char * const msg, size_t msg_len);

#endif //WOMBAT_MQTT_STACK_H
