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
bool mqtt_publish(String& topic, const char* msg, size_t msg_len);

/**
 * Publish a message from a file on the modem filesystem.
 *
 * @param topic the topic to publish to.
 * @param filename the filename on the modem filesystem containing the message to publish.
 * @return true if the message was published, otherwise false.
 */
bool mqtt_publish_file(const String& topic, const String& filename);

#endif //WOMBAT_MQTT_STACK_H
