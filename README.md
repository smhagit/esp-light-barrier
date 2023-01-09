# ESP8266 based light barrier
The light barrier is based on a photoresistor.

When the photoresistor detects a sudden drop in brightness of 20%, the state in MQTT is updated to `1`. A short delay of one second follows and the state is changed back to `0`.

The change in the MQTT topic is picked up by Home Assistant to set the room presence accordingly.
