/* Model: Arduino UNO R3 - ATmega328P
   Software for monitoring temperature, humidity, RFID RC522 devices and providing joystick control
   Sketch uses 10230 bytes (31%) of program storage space. Maximum is 32256 bytes.
   Global variables use 580 bytes (28%) of dynamic memory, leaving 1468 bytes for local variables. Maximum is 2048 bytes.
*/

#include <stdarg.h>
#include <Arduino.h>
#include <stdlib.h>
// Temperature and humidity sensor
#include <SimpleDHT.h>
// I2C / TWI devices
#include <Wire.h>
// LCD screen library
#include <LiquidCrystal.h>
// Communicate with SPI devices
#include <SPI.h>
// RFID Library for MFRC522
#include <MFRC522.h>

// Define print buffer size
#define ARDBUFFER 16
// Define the pins for the RFID reader
#define RST_PIN 9
#define SS_PIN 10
// Preprocessor macro for the length of the display line and rows
#define LCD_LEN 16
#define LCD_ROWS 2
// Preprocessor macro for the pin where the temperature and humidity module is located
#define pinDHT11 2
// Delay for the sampling of temperature and time
#define DELAY 50
// Baud rate
#define BAUD_RATE 57600

// Define reader macros for Joystick pins
#define x_val analogRead(0)
#define y_val analogRead(1)
#define switch_val digitalRead(0)

// Global variables
int_fast8_t temperature;
uint_fast8_t humidity;
uint_fast8_t data[40];
uint_fast16_t passed_time;

/* Instantiate structures/classes
    SimpleDHT11 - temperature and humidity sensor
    LiquidCrystal - LCD screen structure
    MFRC522 - the RC522 reader
*/

SimpleDHT11 dht11;
LiquidCrystal lcd(3, 4, 5, 6, 7, 8);
MFRC522 mfrc522(SS_PIN, RST_PIN);

void print_hex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? "0" : "");
    Serial.print(buffer[i], HEX);
  }
}

// UID reader
const byte* read_uid(MFRC522* mfrc522) {
  byte uid[mfrc522->uid.size];
  for (byte i = 0; i < mfrc522->uid.size; ++i) {
    uid[i] = mfrc522->uid.uidByte[i];
  }
  return uid;
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
}

void loop() {
  // Get current time and convert it to long
  // If enough time has passed execute code block
  passed_time++;
  if ((passed_time) > DELAY) {
    // Update last update time
    passed_time=0;
    // Read temperature and humidity
    dht11.read(pinDHT11, &temperature, &humidity, data);

    // Write to LCD the temperature and humidity
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print("* C");
    lcd.setCursor(0, 1);
    lcd.print("Humidity: ");
    lcd.print(humidity);
    lcd.print("%");
    
    // Print time, temperature and humidity to serial
    serial_printf("{\"timestamp\":%l,\"temperature\":%d,\"humidity\":%d}", (uint_fast32_t)temperature, (uint_fast32_t)humidity);
    mfrc522.PCD_Init();
    // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++){
      key.keyByte[i] = 0xFF;
    }
    MFRC522::StatusCode status;

    // Look for new cards
    if (!mfrc522.PICC_IsNewCardPresent()) {
      return;
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial()) {
      return;
    }

    Serial.print("{\"UID\":\"");
    print_hex(read_uid(&mfrc522), MFRC522::MF_KEY_SIZE);
    Serial.print("\"}\n");
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
  // Print Joystick position
  serial_printf("{\"x\":%d,\"y\":%d,\"switch\":%d}", (uint_fast32_t)x_val, (uint_fast32_t)y_val, (uint_fast32_t)switch_val);
}
