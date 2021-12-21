# AZenvy-on-Home-Assistant

The integration the AZ-Envy sensor in Home Assistant. The AZ-Envy sensor is based on a ESP8266 Arduino board with 3 sensors. The sensors are MQ-2 Gas sensor MT-30 temperature and humidity.
The software for the progamming the AZenvy i found the website in there blog.



Intergration of the AZenvy sensor in Home Assistant.

requirements
-Home Assistant installed
-MQQT Mosquitto broker intergration installed in Home Assistant.

In Arduino IDE

 instal the ESP8266 family with the board administrator http://arduino.esp8266.com/stable/package_esp8266com_index.json
 
 install the ESP8266_MQTT_Conf.h ESP8266_MQTT_Helper.h in the library of arduino.ide
 
 Open the AZenvy.ino file compile upload the file (maybe there has be installed extra files from library).
 
 After the upload the sensor has is own accespoint (see also the azenvy.jpg)
 You can configure this with your own settings.
 then SAVE en Restart the sensor.
 
 You have to make changes to youre configuration.yaml file in Home Assistant.
 
