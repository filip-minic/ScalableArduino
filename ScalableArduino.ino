/* Model: Arduino UNO R3 - ATmega328P
   Software for monitoring temperature,humidity, RFID RC522 devices and providing joystick control
   Sketch uses 16192 bytes (50%) of program storage space. Maximum is 32256 bytes.
   Global variables use 989 bytes (48%) of dynamic memory, leaving 1059 bytes for local variables. Maximum is 2048 bytes.
*/

#include <stdarg.h>
#include <Arduino.h>
#include <stdlib.h>
// Temperature and humidity sensor
#include <SimpleDHT.h>
// I2C / TWI devices
#include <Wire.h>
// Precise time module library
#include <DS3231.h>
// LCD screen library
#include <LiquidCrystal.h>
// Communicate with SPI devices
#include <SPI.h>
// RFID Library for MFRC522
#include <MFRC522.h>

#define ARDBUFFER 16
#define RST_PIN 9
#define SS_PIN 10
// Preprocessor macro for the length of the display line and rows
#define LCD_LEN 16
#define LCD_ROWS 2
// Preprocessor macro for the pin where the temperature and humidity module is located
#define pinDHT11 2
// Delay for the sampling of temperature and time
#define DELAY 1
// Baud rate
#define BAUD_RATE 9600

// Define reader macros for Joystick pins
#define x_val analogRead(0)
#define y_val analogRead(1)

// Global variables
uint_fast32_t last_update_time = 0;
int_fast8_t temperature;
uint_fast8_t humidity;
uint_fast8_t data[40];
uint_fast32_t current_time;

/* Instantiate structures/classes
    DS3231 - real time clock module
    RTCDateTime - datetime type for the clock
    SimpleDHT11 - temperature and humidity sensor
    LiquidCrystal - LCD screen structure
    MFRC522 - the RC522 reader
*/

DS3231 clock;
RTCDateTime dt;
SimpleDHT11 dht11;
LiquidCrystal lcd(3, 4, 5, 6, 7, 8);
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Helper function to dump some information from the RFID reader
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

// printf function for the serial interface
int serial_printf(char *str, ...) {
  int i, count = 0, j = 0, flag = 0;
  char temp[ARDBUFFER + 1];
  for (i = 0; str[i] != '\0'; i++)  if (str[i] == '%')  count++;

  va_list argv;
  va_start(argv, count);
  for (i = 0, j = 0; str[i] != '\0'; i++)
  {
    if (str[i] == '%')
    {
      temp[j] = '\0';
      Serial.print(temp);
      j = 0;
      temp[0] = '\0';

      switch (str[++i])
      {
        case 'd': Serial.print(va_arg(argv, uint_fast32_t));
          break;
        case 'l': Serial.print(va_arg(argv, long));
          break;
        case 'f': Serial.print(va_arg(argv, double));
          break;
        case 'c': Serial.print((char)va_arg(argv, uint_fast32_t));
          break;
        case 's': Serial.print(va_arg(argv, char *));
          break;
        default:  ;
      };
    }
    else {
      temp[j] = str[i];
      j = (j + 1) % ARDBUFFER;
      if (j == 0) {
        temp[ARDBUFFER] = '\0';
        Serial.print(temp);
        temp[0] = '\0';
      }
    }
  };
  Serial.println("}");
  return count + 1;
}

void setup() {
  // Start serial transfer
  Serial.begin(BAUD_RATE);
  // Init SPI bus
  SPI.begin();
  // Init MFRC522 card
  mfrc522.PCD_Init();

  // Setup the LCD length and rows
  lcd.begin(LCD_LEN, LCD_ROWS);
  // Start the clock module
  clock.begin();
  // Set current compiling time
  clock.setDateTime(__DATE__, __TIME__);
  // delay instantiate
  last_update_time = atol(clock.dateFormat("U", dt));
}

void loop() {
  dt = clock.getDateTime();
  current_time = atol(clock.dateFormat("U", dt));
  if ((current_time - last_update_time) > DELAY) {
    last_update_time = current_time;

    dht11.read(pinDHT11, &temperature, &humidity, data);

    // Write to LCD and get humidity
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print("* C");
    lcd.setCursor(0, 1);
    lcd.print("Humidity: ");
    lcd.print(humidity);
    lcd.print("%");

    serial_printf("{\"timestamp\":%l,\"temperature\":%d,\"humidity\":%d}", current_time, (uint_fast32_t)temperature, (uint_fast32_t)humidity);

    // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++)
      key.keyByte[i] = 0xFF;

    //some variables we need
    uint_fast8_t  block;
    uint_fast8_t  len;
    MFRC522::StatusCode status;

    // Look for new cards
    if (!mfrc522.PICC_IsNewCardPresent()) {
      return;
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial()) {
      return;
    }

    Serial.println(F("**Card Detected**"));

    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("Authentication failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }

    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522.uid)); //line 834
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("Authentication failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }
    Serial.print("{\"UID\":\"");
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.print("\",\"type\":\"");
    Serial.print(mfrc522.PICC_GetTypeName(mfrc522.PICC_GetType(mfrc522.uid.sak)));
    Serial.print("\"}");
    Serial.println(F("\n**End Reading**"));

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
  //serial_printf("{\"x\":%d,\"y\":%d}", (uint_fast32_t)x_val, (uint_fast32_t)y_val);
}
