#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <rgb_lcd.h>
#include <Ticker.h>
#include <bsec.h>
#include <SparkFun_SCD30_Arduino_Library.h>

#include "Icons.h"
#include "Settings.h"

// CO2 qualitiy state
typedef enum CO2State {
  GOOD = 0, MEDIUM = 1, BAD = 2
} CO2State;

// Library instances
rgb_lcd lcd;
Adafruit_NeoPixel neopixel = Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRBW + NEO_KHZ800);
Ticker timer;
Bsec bme680;
SCD30 scd30;

// Messurement values
uint16_t co2 = 0;
float temperature = 0;
float humidity = 0;
float iaq = 0;

// Messurement array
uint8_t counter = 0;
float co2Array[NUMBER] = {0};

// Regression states
boolean hyperbola = false;
boolean slope = true;

// Program states
CO2State state = GOOD;
uint8_t page = 0;
uint64_t lastLCDUpdate = 0;
uint64_t lastPageUpdate = 0;
uint64_t lastMessurement = 0;

// Program setup
void setup(){
  Serial.begin(115200);

  connectI2C();

  setupLCD();
  setupNeopixel();
  setupSCD30();
  setupBME680();
  delay(1000);
  
  for(int i = 0; i < 10; i++) {
    scd30.getCO2();
    delay(1000);
  }
}

// Program loop
void loop() {
  readSensors();
  
  runSignal();
}

// Connect to I2C
void connectI2C() {
  Serial.println("\nConnecting I2C...");
  Wire.begin();
  while(Wire.status() != I2C_OK) {
    Serial.print("Connection failed (Status: ");
    Serial.print(Wire.status());
    Serial.println(")!");
    delay(5000);
    Wire.begin();
  }
  Wire.setClock(I2C_CLOCK);
  Wire.setClockStretchLimit(I2C_STRECH_LIMIT);
  Serial.println("I2C connected!");
}

// Setup the LCD
void setupLCD() {
  lcd.begin(16, 2);
  lcd.createChar(GOOD, good);
  lcd.createChar(MEDIUM, medium);
  lcd.createChar(BAD, bad);
  lcd.createChar(UE_CHAR, ue);
}

// Setup the Neopixel
void setupNeopixel() {
  neopixel.begin();
  neopixel.setPixelColor(0, 0, 0, 0, 0);
  neopixel.setPixelColor(1, 0, 0, 0, 0);
  neopixel.show();
}

// Setup the SCD30
void setupSCD30() {
  Serial.println("\nConnecting SCD30...");
  while(!scd30.begin()) {
    Serial.println("Connection failed!");
    delay(5000);
  }
  scd30.reset();
  scd30.setAutoSelfCalibration(SDC60_AUTO_CALIBRATION);
  scd30.setMeasurementInterval(SDC60_MESUREMENT_INTERVAL);
  Serial.println("SCD30 connected!");
}

// Setup the BME680
void setupBME680() {
  Serial.println("\nConnecting BME680...");
  bme680.begin(BME680_I2C_ADDR_PRIMARY, Wire);
  
  while(bme680.status < BSEC_OK || bme680.bme680Status < BME680_OK) {
    Serial.println("Connection failed!");
    delay(5000);
    bme680.begin(BME680_I2C_ADDR_PRIMARY, Wire);
  }
  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT
  };
  bme680.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  bme680.run();
  timer.attach_ms(min(3000, SDC60_MESUREMENT_INTERVAL * 1000), runBME680);
  
  Serial.println("BME680 connected!");
}

// Run the BME680
void runBME680() {
  bme680.run();
}

// Read the sensor values
void readSensors() {
  if(millis() - lastMessurement < INTERVAL * 1000) {
    return;
  }
  co2 = scd30.getCO2();
  temperature = scd30.getTemperature() - SDC60_TEMPERATURE_OFFSET;
  humidity = scd30.getHumidity();
  iaq = bme680.staticIaq;

  insertArray(co2);

  Serial.printf("\nTemperature: %.2f °C\n", temperature);
  Serial.printf("Humidity: %.2f %%\n", humidity);
  Serial.printf("CO2: %d ppm\n", co2);
  Serial.printf("\nTemperature: %.2f °C\n", bme680.temperature);
  Serial.printf("Humidity: %.2f %%\n", bme680.humidity);
  Serial.printf("Pressure: %.2f hPa\n", bme680.pressure / 100.0F);
  Serial.printf("IAQ: %.2f (Accuracy: %d)\n", bme680.staticIaq, bme680.staticIaqAccuracy);
  Serial.printf("CO2: %.2f ppm\n", bme680.co2Equivalent);
  Serial.printf("VOC: %.2f ppm\n\n", bme680.breathVocEquivalent);
  lastMessurement = millis();
}

// Insert value into array
void insertArray(uint16_t co2Value) {
  if(counter < NUMBER) {
    co2Array[counter] = co2Value;
    counter++;
  }
  else {
    for(int i = 0 ; i < NUMBER - 1; i++) {
      co2Array[i] = co2Array[i + 1];
    }
    co2Array[NUMBER - 1] = co2Value;
  }
}

// Run the CO2 signal
void runSignal() {
  if((millis() - lastPageUpdate) >= LCD_PAGE_INTERVAL) {
    page = (page >= 3) ? 0 : page + 1;
    lastPageUpdate = millis();
  }
  if((millis() - lastLCDUpdate) >= LCD_UPDATE_INTERVAL) {
    writeLCD();
    writeSignal();
    prediction();
    lastLCDUpdate = millis();
  }
}

// Update the LCD display
void writeLCD() {
  lcd.setCursor(0, 0);
  String text = "";
  switch(page) {
    case 0:
      lcd.print("CO2:            ");
      text = String(co2) + " ppm";
      break;
    case 1:
      lcd.print("Temp:           ");
      text = String(temperature) + " C";
      break;
    case 2:
      lcd.print("Humid:          ");
      text = String(humidity) + " %";
      break;
    case 3:
      lcd.print("IAQ:            ");
      text = String(iaq);
      break;
    default:
      break;
  }
  lcd.setCursor(14 - text.length(), 0);
  lcd.print(text);
}

// Write the Neopixel signal
void writeSignal() {
  if((state == GOOD && co2 < CO2_YELLOW_RISING) || (state != GOOD && co2 < CO2_GREEN_FALLING)) {
    neopixel.setPixelColor(0, 0, NEOPIXEL_BRIGHTNESS, 0, 0);
    neopixel.setPixelColor(1, 0, NEOPIXEL_BRIGHTNESS, 0, 0);
    state = GOOD;
  }
  else if((state != BAD && co2 < CO2_RED_RISING) || (state == BAD && co2 < CO2_YELLOW_FALLING)) {
    neopixel.setPixelColor(0, NEOPIXEL_BRIGHTNESS, NEOPIXEL_BRIGHTNESS, 0, 0);
    neopixel.setPixelColor(1, NEOPIXEL_BRIGHTNESS, NEOPIXEL_BRIGHTNESS, 0, 0);
    state = MEDIUM;
  }
  else {
    neopixel.setPixelColor(0, NEOPIXEL_BRIGHTNESS, 0, 0, 0);
    neopixel.setPixelColor(1, NEOPIXEL_BRIGHTNESS, 0, 0, 0);
    state = BAD;
  }
  neopixel.show();
  
  lcd.setCursor(15, 0);
  lcd.write(state);
}

// Predict next signal state
void prediction() {
  lcd.setCursor(0, 1);
  
  if(co2 < CO2_GREEN_FALLING) {
    lcd.print("Super!          ");
    return;
  }
  calcSlope();
  
  if(slope) {
    if(state == BAD) {
      lcd.print("Bitte luften!!! ");
      lcd.setCursor(7, 1);
      lcd.write(UE_CHAR);
    }
    else {
      float linCoeff[2] = {0};
      linearRegression(linCoeff, INTERVAL, co2Array, counter, true);
      
      int32_t time = (CO2_RED_RISING - linCoeff[0]) / linCoeff[1] - counter * INTERVAL;
      if(time > 0) {
        uint16_t hours = time / 3600;
        uint16_t minutes = time / 60 - hours * 60;
        uint16_t seconds = time - minutes * 60 - hours * 3600;
        hours = (hours > 999) ? 999 : hours;
       
        if(hours == 0 && minutes == 0) {
          lcd.printf("Rot in: %ds       ", seconds);
        }
        else if(hours == 0) {
          lcd.printf("Rot in: %dmin       ", minutes);
        }
        else {
          lcd.printf("Rot in: %d:%02dh       ", hours, minutes);
        }
      }
    }
  }
  else {
    if(state == GOOD) {;
      lcd.print("Super!          ");
    }
    else if(hyperbola) {
      float hypCoeff[3] = {0};      
      if(counter >= 10) {
        hyperbolicRegression(hypCoeff, INTERVAL, &co2Array[counter - 10], 10);
      }
      else {
        hyperbolicRegression(hypCoeff, INTERVAL, co2Array, counter);
      }
      
      int32_t time = hypCoeff[2] / (CO2_GREEN_FALLING - hypCoeff[0]) - hypCoeff[1] - 10 * INTERVAL;
      if(time > 0) {
        uint16_t hours = time / 3600;
        uint16_t minutes = time / 60 - hours * 60;
        uint16_t seconds = time - minutes * 60 - hours * 3600;
        hours = (hours > 999) ? 999 : hours;
      
        if(hours == 0 && minutes == 0) {
          lcd.printf("Grun in: %ds       ", seconds);
        }
        else if(hours == 0) {
          lcd.printf("Grun in: %dmin       ", minutes);
        }
        else {
          lcd.printf("Grun in: %d:%02dh       ", hours, minutes);
        }
        lcd.setCursor(2, 1);
        lcd.write(UE_CHAR);
      }
    }
  }
}

// Calculate the current slope
void calcSlope() {
  float linCoeff[2] = {0};
  if(counter >= 8) {
    linearRegression(linCoeff, INTERVAL, &co2Array[counter - 8], 8, false);
  }
  else {
    linearRegression(linCoeff, INTERVAL, co2Array, counter, false);
  }
  
  if(linCoeff[1] < -4) {
    hyperbola = true;
    slope = false;
  }
  else if(linCoeff[1] < -1) {
    slope = false;
  }
  else {
    hyperbola = false;
    slope = true;
  }
}

// Linear regression
void linearRegression(float coefficients[], float interval, float y[], unsigned int n, boolean withWeight) {
  float sum1, sumX, sumY, sumXX, sumXY;
  sum1 = sumX = sumY = sumXX = sumXY = 0.0;
  
  for(int i = 0; i < n; i++) {
    float weight = withWeight ? i + 1 : 1;
    weight *= weight;
    float x = i * interval;

    sum1 += weight;
    sumX += x * weight;
    sumY += y[i] * weight;
    sumXX += x * x * weight;
    sumXY += x * y[i] * weight;
  }
  float denominator = sum1 * sumXX - sumX * sumX;
  
  if(denominator != 0) {
    float coefficient = (sum1 * sumXY - sumX * sumY) / denominator;
    float yOffset = (sumY * sumXX - sumX * sumXY) / denominator;
    
    coefficients[1] = coefficient;
    coefficients[0] = yOffset;
  }
}

// Hyperbolic regression
void hyperbolicRegression(float coefficients[], float interval, float y[], unsigned int n) {
  float sumX, sumY, sumXX, sumXY, sumYY;
  float minError = 1e38;
  
  for(float xOffset = 0; xOffset <= MAX_XOFFSET; xOffset += PRECISION) {
    sumX = sumY = sumXX = sumXY = sumYY = 0.0;
  
    for(int i = 0; i < n; i++) {
      float x = 1 / (i * interval + xOffset);
      sumX += x;
      sumY += y[i];
      sumXX += x * x;
      sumXY += x * y[i];
      sumYY += y[i] * y[i];
    }
    float denominator = n * sumXX - sumX * sumX;

    if(denominator != 0) {
      float coefficient = (n * sumXY - sumX * sumY) / denominator;
      float yOffset = (sumY * sumXX - sumX * sumXY) / denominator;
      
      float sx = coefficient * (sumXY - sumX * sumY / n);
      float sy = sumYY - sumY * sumY / n;
      float error = sqrt((sy - sx) / (n - 2.0));
      
      if(error < minError) {
        minError = error;
        coefficients[2] = coefficient;
        coefficients[1] = xOffset;
        coefficients[0] = yOffset;
      }
      else if(error - minError > 2) {
        break;
      }
    }
    yield();
  }
}
