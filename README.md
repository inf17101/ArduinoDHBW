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

The following flowcharts describe the workflow, how the data is queried from sensors and how it is processed.

#### Flow chart part 1: Start query
![Weather station development](./Presentation/images/Ablaufdiagramm2Start.jpg "Start query workflow")

#### Flow chart part 2: DHT22
![Weather station development](./Presentation/images/AD_TempFeucht.PNG "Workflow of DHT22")

#### Flow chart part 3: Particulate matter sensor
![Weather station development](./Presentation/images/AD_Feinstaub.PNG "Workflow of Particulate matter sensor")

#### Flow chart part 4: ESP
![Weather station development](./Presentation/images/AD_ESP_Ausgabe.PNG "Workflow of ESP")

### Hardware structure

#### General structure
![Weather station development](./Presentation/images/TempLueftFeinstaub.png "Hardware structure")

#### ESP operation
![Weather station development](./Presentation/images/ESP_Betrieb.png "ESP operation")

#### Concept of data transfer
![Weather station development](./Presentation/images/AD_Workflow.png "Concept of data transfer")

### Data integrity check between Arduino and ESP

Implemented transmission security via CRC procedure:
G (x) = x^16 + x^15 + x^10 + x^3
- Arduino calculates CRC-16 (Xmodem) and sends Data + CRC
- ESP also calculates CRC from data
- ESP checks whether CRCs are the same

### Occurring challenges
- Connect ESP via UART ⇒ fine dust also sends
via UART
- All buttons of the LCD display are only activated via a pin A0
had read
- PWM signal for the fan
- Data synchronization for LCD display, user website
- Pin assignment due to the small number of pins

### Additional Notes

Please also see our German presentation inside the folder `Presentation` for more information.
