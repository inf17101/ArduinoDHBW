# Construction & implementation of a weather station

### Idea of a weather station

- Display of temperature and humidity values
- Display of the data on a website
- The possibility for a user to define threshold values
- Respond to particulate matter values
- Trigger events based on the sensor values

### Hardware
- Arduino Uno
- Temperature and humidity sensor DHT22
- Fine dust sensor SDS011
- LCD 1602 to display sensor data and let the users define threshold values
- ESP8266 (connected with the Arduino via UART) to display a website of the sensor values to wlan clients
- 4 pin 5V fan

### Concept development
Flow chart part 1: Start query
![Weather station development](./Presentation/images/Ablaufdiagramm2Start.jpg "Start query workflow")

Flow chart part 2: DHT22
![Weather station development](./Presentation/images/AD_TempFeucht.png "Workflow of DHT22")

Flow chart part 3: Particulate matter sensor
![Weather station development](./Presentation/images/AD_Feinstaub.png "Workflow of Particulate matter sensor")

