#include "DFRobot_EC.h"
#include "DFRobot_PH.h"
#include "GravityTDS.h"
#include <EEPROM.h>
#include <Keypad.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// EC sensor pin
#define EC_PIN A8
// pH sensor pin
#define PH_PIN A9
// TDS sensor pin
#define TDS_PIN A10
// Temperature sensor pin
#define ONE_WIRE_BUS 33

// Define the keymap
const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Connect keypad ROW0, ROW1, ROW2, ROW3 to these Arduino pins
byte rowPins[ROWS] = {8, 7, 6, 5}; 

// Connect keypad COL0, COL1, COL2, COL3 to these Arduino pins
byte colPins[COLS] = {12, 11, 10, 9}; 

// Create the Keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

LiquidCrystal_I2C lcd(0x3F, 16, 2);  // set the LCD address to 0x3F for a 16 chars and 2 line display

float voltage, ecValue, phValue, tdsValue, temperature = 25;
DFRobot_EC ec;
DFRobot_PH ph;
GravityTDS gravityTds;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

enum State { SELECT_SENSOR, EC_SENSOR, PH_SENSOR, TDS_SENSOR, TEMP_SENSOR, ALL_SENSORS, CONFIRM_CLEAR_EEPROM };
State currentState = SELECT_SENSOR;

void setup()
{
  Serial.begin(115200);
  lcd.init();
  lcd.clear();        
  lcd.backlight();  
  ec.begin();
  ph.begin();
  gravityTds.setPin(TDS_PIN);
  gravityTds.setAref(5.0);  // reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);  // 1024 for 10bit ADC; 4096 for 12bit ADC
  gravityTds.begin();  // initialization
  sensors.begin();
  showMenu();
  pinMode(34, OUTPUT);
  digitalWrite(34, HIGH);
  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH);
  pinMode(44, OUTPUT);
  digitalWrite(44, HIGH);
}

void loop()
{
    char key = keypad.getKey();
    if (key) {
        if (currentState == SELECT_SENSOR) {
            if (key == '*') {
                currentState = CONFIRM_CLEAR_EEPROM;
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Clear EEPROM?");
                lcd.setCursor(0,1);
                lcd.print("A=Yes D=No");
            } else if (key == '1') {
                currentState = EC_SENSOR;
                Serial.println("EC Sensor selected");
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("EC Selected");
                delay(500);
            } else if (key == '2') {
                currentState = PH_SENSOR;
                Serial.println("pH Sensor selected");
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("pH Selected");
                delay(500);
            } else if (key == '3') {
                currentState = TDS_SENSOR;
                Serial.println("TDS Sensor selected");
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("TDS Selected");
                delay(500);
            } else if (key == '4') {
                currentState = TEMP_SENSOR;
                Serial.println("Temperature Sensor selected");
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Temp Selected");
                delay(500);
            } else if (key == '8') {
                currentState = ALL_SENSORS;
                Serial.println("All Sensors selected");
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("ALL Selected");
                delay(500);
            }
        } else if (currentState == CONFIRM_CLEAR_EEPROM) {
            if (key == 'A') {
                clearEEPROM();
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("EEPROM Cleared");
                Serial.println("EEPROM Cleared");
                delay(1000);
                currentState = SELECT_SENSOR;
                showMenu();
            } else if (key == 'D') {
                currentState = SELECT_SENSOR;
                showMenu();
            }
        } else if (currentState == EC_SENSOR) {
            switch (key) {
                case 'A':
                    Serial.println("Entering EC calibration mode");
                    Serial.println("enterec");
                    ec.calibration(voltage, temperature, "enterec");
                    break;
                case 'B':
                    Serial.println("Calibrating EC");
                    Serial.println("calec");
                    ec.calibration(voltage, temperature, "calec");
                    break;
                case 'C':
                    Serial.println("Exiting EC calibration mode");
                    Serial.println("exitec");
                    ec.calibration(voltage, temperature, "exitec");
                    break;
                case 'D':
                    currentState = SELECT_SENSOR;
                    showMenu();
                    break;
            }
        } else if (currentState == PH_SENSOR) {
            switch (key) {
                case 'A':
                    Serial.println("Entering pH calibration mode");
                    Serial.println("enterph");
                    ph.calibration(voltage, temperature, "enterph");
                    break;
                case 'B':
                    Serial.println("Calibrating pH");
                    Serial.println("calph");
                    ph.calibration(voltage, temperature, "calph");
                    break;
                case 'C':
                    Serial.println("Exiting pH calibration mode");
                    Serial.println("exitph");
                    ph.calibration(voltage, temperature, "exitph");
                    break;
                case 'D':
                    currentState = SELECT_SENSOR;
                    showMenu();
                    break;
            }
        } else if (currentState == TDS_SENSOR) {
            if (key == 'D') {
                currentState = SELECT_SENSOR;
                showMenu();
            }
        } else if (currentState == TEMP_SENSOR) {
            if (key == 'D') {
                currentState = SELECT_SENSOR;
                showMenu();
            }
        } else if (currentState == ALL_SENSORS) {
            if (key == 'D') {
                currentState = SELECT_SENSOR;
                showMenu();
            }
        }
    }

    // Read temperature from DS18B20 sensor
    sensors.requestTemperatures();
    temperature = sensors.getTempCByIndex(0);

    if (currentState == EC_SENSOR) {
        static unsigned long timepoint = millis();
        if (millis() - timepoint > 1000U) {  // time interval: 1s
            timepoint = millis();
            digitalWrite(34, LOW);
            delay(1000);
            voltage = analogRead(EC_PIN) / 1024.0 * 5000;   // read the voltage
            ecValue = ec.readEC(voltage, temperature);  // convert voltage to EC with temperature compensation
            Serial.print("temperature:");
            Serial.print(temperature, 1);
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Temperature:");
            lcd.print(temperature);
            Serial.print("^C  EC:");
            Serial.print(ecValue, 2);
            lcd.setCursor(0,1);
            lcd.print("EC : ");
            lcd.print(ecValue, 2);
            lcd.print(" ms/cm");
            Serial.println("ms/cm");
            
        }
        ec.calibration(voltage, temperature);  // calibration process by Serial CMD
    }

    if (currentState == PH_SENSOR) {
        static unsigned long timepoint = millis();
        if (millis() - timepoint > 1000U) {  // time interval: 1s
            timepoint = millis();
            digitalWrite(38, LOW);
            delay(1000);
            voltage = analogRead(PH_PIN) / 1024.0 * 5000;  // read the voltage
            phValue = ph.readPH(voltage, temperature);  // convert voltage to pH with temperature compensation
            Serial.print("temperature:");
            Serial.print(temperature, 1);
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Temperature:");
            lcd.print(temperature);
            Serial.print("^C  pH:");
            Serial.println(phValue, 2);
            lcd.setCursor(0,1);
            lcd.print("pH : ");
            lcd.print(phValue, 2);
        }
        ph.calibration(voltage, temperature);  // calibration process by Serial CMD
    }

    if (currentState == TDS_SENSOR) {
        static unsigned long timepoint = millis();
        if (millis() - timepoint > 1000U) {  // time interval: 1s
            timepoint = millis();
            digitalWrite(44, LOW);
            delay(1000);
            gravityTds.setTemperature(temperature);  // set the temperature and execute temperature compensation
            gravityTds.update();  // sample and calculate
            tdsValue = gravityTds.getTdsValue();  // then get the value
            Serial.print("temperature:");
            Serial.print(temperature, 1);
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Temperature:");
            lcd.print(temperature);
            Serial.print("^C  TDS:");
            Serial.print(tdsValue, 0);
            Serial.println("ppm");
            lcd.setCursor(0,1);
            lcd.print("TDS : ");
            lcd.print(tdsValue, 2);
            lcd.print(" ppm");
        }
    }

    if (currentState == TEMP_SENSOR) {
        static unsigned long timepoint = millis();
        if (millis() - timepoint > 1000U) {  // time interval: 1s
            timepoint = millis();
            Serial.print("Temperature: ");
            Serial.print(temperature, 1);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Temperature:");
            lcd.print(temperature);
            Serial.println(" °C");
        }
    }

    if (currentState == ALL_SENSORS) {
        lcd.clear();
        static unsigned long timepoint = millis();
        if (millis() - timepoint > 1000U) {  // time interval: 1s
            timepoint = millis();
            
            // EC Sensor Reading
            digitalWrite(34, LOW);
            delay(5000);
            voltage = analogRead(EC_PIN) / 1024.0 * 5000;   // read the voltage
            ecValue = ec.readEC(voltage, temperature);
            lcd.setCursor(0,0);
            lcd.print("EC:");
            lcd.print(ecValue);
            digitalWrite(34, HIGH);  // convert voltage to EC with temperature compensation

            // pH Sensor Reading
            digitalWrite(38, LOW);
            delay(5000);
            voltage = analogRead(PH_PIN) / 1024.0 * 5000;  // read the voltage
            phValue = ph.readPH(voltage, temperature);
            lcd.print(" PH :");
            lcd.print(phValue);
            digitalWrite(38, HIGH);  // convert voltage to pH with temperature compensation

            // TDS Sensor Reading
            digitalWrite(44, LOW);
            delay(5000);
            gravityTds.setTemperature(temperature);  // set the temperature and execute temperature compensation
            gravityTds.update();  // sample and calculate
            tdsValue = gravityTds.getTdsValue();
            lcd.setCursor(0,1);
            lcd.print("TDS:");
            lcd.print(tdsValue);
            lcd.print("Temp:");
            lcd.print(temperature);
            digitalWrite(44, HIGH);
            delay(5000);  // then get the value

            // Display All Readings
            Serial.print("temperature:");
            Serial.print(temperature, 1);
            Serial.print("^C  EC:");
            Serial.print(ecValue, 2);
            Serial.print("ms/cm  pH:");

            Serial.print(phValue, 2);

            Serial.print("  TDS:");
            Serial.print(tdsValue, 0);
            Serial.println("ppm");

        }
    }
}

void showMenu() {
  digitalWrite(34, HIGH);
  digitalWrite(38, HIGH);
  digitalWrite(44, HIGH);
  Serial.println("Select sensor:");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select Sensor");
  Serial.println("1. EC Sensor");
  Serial.println("2. pH Sensor");
  Serial.println("3. TDS Sensor");
  Serial.println("4. Temperature Sensor");
  Serial.println("8. All Sensors");
}

void clearEEPROM() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Clearing EEPROM");
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 255);
  }
  Serial.println("EEPROM cleared");
}
