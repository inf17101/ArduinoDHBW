#include "DHT.h"
#include <SDS011.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <Crc16.h>
#define ButtonDelay 5 // button delay 10 microsek.

#define espRx 2 // esp rx pin
#define espTx 13 // esp tx pin
#define espBAUDR 9600 // esp baud rate 9600
#define DHTPIN 12     // pin to dht
#define PWMPIN 3 // pwm pin to control pc fan
#define DHTTYPE DHT22   // DHT 22 (AM2302) temperature and humidity sensor
#define D1 0 // rx sds sensor
#define D2 1 // tx sds sensor
#define TRUE 1
#define FALSE 0
#define A0 0


SoftwareSerial esp_serial(espRx, espTx); //esp serial
DHT dht(DHTPIN, DHTTYPE); // dht22 object
SDS011 sds;            // sds object
Crc16 crc;            // crc16 object
LiquidCrystal lcd(8, 9, 4, 5, 6, 7); // lcd object
float measured_params[4] = {0}; // container for all data of sensors
uint8_t setFanSpeed = 0; //set fan speed
int fanSpeed; // current fan speed

// button of lcd displays
enum Button {
  UP,DOWN,LEFT,RIGHT,SELECT
};

const int numOfScreens = 4; // number of user input screens in menu
int currentScreen = 0;
unsigned int EnterExitMenu = FALSE; // user has entered menu
int reading;
uint8_t buttonState; // current button state
uint8_t lastButtonState = FALSE; // last button state
unsigned int lastDebounceTime = 0; // debounce time to ensure button was pressed correctly

float parameters[numOfScreens] = {50.0f, 25.0f, 12.0f, 24.0f}; //default threshold values, decides at which value action is performed
float max_params[numOfScreens][2] = {{0.0, 100.0}, {0,60}, {0, 100}, {0, 100}}; // number of max and min values of user settings
String SCREENS[numOfScreens][2] = {{"Max. Humidity", "percent"}, {"Max. Temperatur", "degC"}, {"Max. P25", ""}, {"Max. P10", ""}}; //screens to display on lcd, (menu screen, einheit)
int input; // input - UP,DOWN, LEFT, ...

//user defined functions
void readTemperatureAndHumidity();
void readSDSValues();
void checkThresholdValues();
void setSpeedOfFan();
int sendDataToESP();
void updateDataWithCRC16(char* data, int len);
void usermenu(void);
int detectLCDButton(int* pressed_button);
void inputAction(int* input);
void printMenuScreen(void);

void setup() 
{
    lcd.begin(16, 2); // initialize lcd display 2 rows,16 columns
    Serial.begin(9600); //set baud rate and begin serial of arduino
    esp_serial.begin(espBAUDR); // esp serial
    Wire.begin();
    dht.begin(); // Temp-Humidity sensor
    sds.begin(D1, D2); //RX, TX sds
    analogWrite(PWMPIN, 0); // set pwm fan to 0 at beginning

    cli();//stop interrupts
    //set timer1 interrupt at approx. every 4 seconds
    TCCR1A = 0;// set entire TCCR1A register to 0
    TCCR1B = 0;// same for TCCR1B
    TCNT1  = 0;//initialize counter value to 0
    // set compare match register for 1hz increments
    OCR1A = 65535;// = ((16*10^6) / 1024)*4.19 - 1 (must be <65536)
    // turn on CTC mode
    TCCR1B |= (1 << WGM12);
    // Set CS10 and CS12 bits for 1024 prescaler
    TCCR1B |= (1 << CS12) | (1 << CS10);  
    // enable timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);

    sei();//allow interrupts

    pinMode(A3, OUTPUT); // LED for humidity
    digitalWrite(A3, LOW); // disable LED

    pinMode(A2, OUTPUT); // LED for p2.5,p10
    digitalWrite(A2, LOW); // disable LED
}

void loop() 
{
   usermenu();
   readTemperatureAndHumidity();
   readSDSValues();
   checkThresholdValues();
   setSpeedOfFan();
}

ISR(TIMER1_COMPA_vect)
{  // send Data to ESP after every 4 seconds
   //triggered through timer interrupt
   sendDataToESP();
}

void readTemperatureAndHumidity()
{
    static unsigned long counter = 0;
    float tmp_values[2] = {0};
    if(counter < 10)
    {
      counter++;
      return;
    }
    counter = 0;
    if(!dht.readTempAndHumidity(tmp_values)){ // returns 0 if values has been written into array
        measured_params[0] = tmp_values[0]; // humidity
        measured_params[1] = tmp_values[1]; // temperature
  }
}

void readSDSValues()
{
  int error;
  float tmp_params[2] = {0};
  error = sds.read(&tmp_params[2], &tmp_params[3]);
  if (!error) {
    measured_params[2] = tmp_params[2];
    measured_params[3] = tmp_params[3];
  }
}

void checkThresholdValues()
{
 // check the threshold value and perform action
  //humidity value = measured_params[0]
  if(measured_params[0] >= parameters[0])
  {
    digitalWrite(A3, HIGH); //Humidity LED on
  }else
  {
    digitalWrite(A3, LOW); //Humidity LED off
  }
  
  if(measured_params[1] >= parameters[1]) // temperature value = measured_params[1]
  {
    setFanSpeed = 9; // enable to max
  }else
  {
    setFanSpeed = 0; // turn off fan
  }
  
  if(measured_params[2] >= parameters[2] || measured_params[3] >= parameters[3]) // p2.5 and p10 values
  {
    digitalWrite(A2, HIGH); // p2.5,p10 LED on
  }else
  {
    digitalWrite(A2, LOW); // p2.5,p10 LED off
  }
}

void setSpeedOfFan()
{
        fanSpeed = map(setFanSpeed, 0, 9, 0, 255); // map speed of fan steps (0-9) to pwm out (0-255)
        if(setFanSpeed == 0)
        {
          setFanSpeed = 0;
          digitalWrite(PWMPIN, LOW);
        }
        analogWrite(PWMPIN, fanSpeed);
}

int sendDataToESP()
{
  static char data[30];
  byte* buffer_with_crc;
  sprintf(&data[0], "<%d.%02d,%d.%02d,%d.%02d,%d.%02d>", (int) measured_params[0], (int)(measured_params[0]*100)%100, (int) measured_params[1], (int)(measured_params[1]*100)%100, (int) measured_params[2], (int)(measured_params[2]*100)%100, (int) measured_params[3], (int)(measured_params[3]*100)%100);
  size_t len_data = strlen(data);

  buffer_with_crc = (byte*) malloc((len_data+3) * sizeof(byte)); // reserve buffer for data + crc
  updateDataWithCRC16(&buffer_with_crc[0], (byte*) &data[0], len_data); // calculate crc and concat data with crc16 (XModem)
  
  esp_serial.print((char*) buffer_with_crc);
  free(buffer_with_crc);
}


void updateDataWithCRC16(byte* dest_buffer, byte* data, int len)
{
    Crc16 crc; // crc object of library Crc16.h
    byte crc_value[3]; // reserve byte array for crc
    crc.clearCrc(); //clear crc buffer
    unsigned short value;
    value = crc.XModemCrc(data,0,len); // function wants uint8_t = byte as argument for data

    crc_value[0] = (byte) (value >> 8); // upper values of crc16 into buffer
    crc_value[1] = (byte) (value & 0x00FF); // lower value of crc16 into buffer
    crc_value[2] = '\0';
    sprintf((char*) dest_buffer, "%s%s", &data[0], &crc_value[0]); // concat data with crc16 into dest_buffer
    dest_buffer[len + 2] = 0; // set null terminator
}

int detectLCDButton(int* adc_key_in)
{
    // detect which button was pressed
    if (*adc_key_in < 30) 
    {
      return RIGHT;
    }else if ( (*adc_key_in > 120) && (*adc_key_in < 151) )
    {
      return UP;
    }else if ( (*adc_key_in > 250) && (*adc_key_in < 361) )
    {
      return DOWN;
    }else if ( (*adc_key_in > 450) && (*adc_key_in < 536) )
    {
      return LEFT;
    }else if ( (*adc_key_in > 700) && (*adc_key_in < 761) )
    {
      return SELECT;
    }else
    {
      return -1;
    }
}

void inputAction(int *input)
{
  //performs input action depending on the pressed button of method detectLCDButton
  //LEFT, RIGHT - change input screens
  // UP, DOWN - increment, decrement threshold values
  //SELECT - start or exit usermenu

  // this method debounce the pressed button of lcd display
    if (buttonState != lastButtonState) // proof if button state has changed
    {
      lastDebounceTime = millis(); // notify last time
    }

  if( (millis() - lastDebounceTime) > ButtonDelay) // go on only if delay has reached global button delay time
  {
    if(reading != buttonState) // check if buttonState has been changed
    {
      buttonState = reading;
    }

    if(buttonState == TRUE) // check if button was pressed
    {
      switch(*input)
      {
        case LEFT: if(!EnterExitMenu){return;} if(currentScreen == 0) { currentScreen = numOfScreens -1; } else { currentScreen--; } printMenuScreen(); break;
        case RIGHT: if(!EnterExitMenu){return;} if(currentScreen == (numOfScreens - 1)) { currentScreen = 0; } else { currentScreen++; } printMenuScreen(); break;
        case UP:  if(!EnterExitMenu){return;} if(parameters[currentScreen] < max_params[currentScreen][1]){ parameters[currentScreen] += 0.5f; } printMenuScreen(); break ;
        case DOWN: if(!EnterExitMenu){return;} if(parameters[currentScreen] > max_params[currentScreen][0]) { parameters[currentScreen] -= 0.5f; } printMenuScreen(); break;
        case SELECT: if(EnterExitMenu == TRUE) { EnterExitMenu = FALSE; lcd.clear(); } else { printMenuScreen(); EnterExitMenu = TRUE;} break;
      }
    }
  }
  lastButtonState = (reading<1023) ? TRUE : FALSE; // overwrite last button state
}

void printMenuScreen(void)
{
  lcd.clear();
  lcd.print(SCREENS[currentScreen][0]); // display description of parameter
  lcd.setCursor(0,1); // go to the next line
  lcd.print(parameters[currentScreen]); // display current threshold value
  lcd.print(" ");
  lcd.print(SCREENS[currentScreen][1]); // display unit
}

void usermenu(void)
{
    //run user menu
  input = analogRead(A0); // read value of current a0 pin
  reading = (input<1023) ? TRUE : FALSE; // digital state of button pressed (true) or not pressed (false)
  input = detectLCDButton(&input);
  inputAction(&input);
}
