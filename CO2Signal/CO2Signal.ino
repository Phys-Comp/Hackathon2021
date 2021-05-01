#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <rgb_lcd.h>
#include <Ticker.h>
#include <bsec.h>
#include <SparkFun_SCD30_Arduino_Library.h>

#include "Icons.h"
#include "Settings.h"

// Available signal states
typedef enum SignalState {
  CO2_GREEN, CO2_YELLOW, CO2_RED
} SignalState;

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

// Program states
SignalState state = CO2_GREEN;
uint8_t page = 0;
uint64_t lastLCDUpdate = 0;
uint64_t lastPageUpdate = 0;
uint64_t lastThingspeakUpdate = 0;

// Program setup
void setup(){
  Serial.begin(115200);

  connectWiFi();
  connectI2C();

  setupLCD();
  setupNeopixel();
  setupBME680();
  setupSCD30();
}

// Program loop
void loop() {
  checkCalibration();
  readSensors();
  
  runSignal();
  uploadThingspeak();
}

// Connect to WiFi
void connectWiFi() {
  #if WIFI_ENABLE
  Serial.print ("\nWiFi connecting to ");
  Serial.print(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println ("\nWiFi connected! IP Adress: " + WiFi.localIP().toString());
  #endif
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
  lcd.createChar(0, progress1);
  lcd.createChar(1, progress2);
  lcd.createChar(2, progress3);
  lcd.createChar(3, progress4);
  lcd.createChar(4, progress5);
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

// Check for calibration
void checkCalibration() {
  if(Serial.available() <= 0) {
    return;
  }
  String input = Serial.readStringUntil('\n');
  
  if(input == "calibrate") {
    Serial.println("\nStarting SCD30 calibration, please wait 10s...");
    delay(10000);
    
    scd30.setAltitudeCompensation(CALIBRATION_ALTITUDE);
    scd30.setForcedRecalibrationFactor(CALIBRATION_CO2_VALUE);
    Serial.println("Calibration finished!\n");
  }
}

// Read the sensor values
void readSensors() {
  if(!scd30.dataAvailable()) {
    return;
  }
  co2 = scd30.getCO2();
  temperature = scd30.getTemperature() - SDC60_TEMPERATURE_OFFSET;
  humidity = scd30.getHumidity();
  iaq = bme680.staticIaq;

  Serial.printf("\nTemperature: %.2f °C\n", temperature);
  Serial.printf("Humidity: %.2f %%\n", humidity);
  Serial.printf("CO2: %d ppm\n", co2);
  Serial.printf("\nTemperature: %.2f °C\n", bme680.temperature);
  Serial.printf("Humidity: %.2f %%\n", bme680.humidity);
  Serial.printf("Pressure: %.2f hPa\n", bme680.pressure / 100.0F);
  Serial.printf("IAQ: %.2f (Accuracy: %d)\n", bme680.staticIaq, bme680.staticIaqAccuracy);
  Serial.printf("CO2: %.2f ppm\n", bme680.co2Equivalent);
  Serial.printf("VOC: %.2f ppm\n\n", bme680.breathVocEquivalent);
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
  lcd.setCursor(16 - text.length(), 0);
  lcd.print(text);
  
  int progress = 16 * 5 * co2 / CO2_RED_RISING;
  int index = progress / 5;
  
  for(int x = 0; x < 16; x++) {
    lcd.setCursor(x, 1);
    
    if(x < index) {
      lcd.write(4);
    }
    else if(x == index) {
      lcd.write(progress - index * 5);
    }
    else {
      lcd.print(" ");
    }
  }
}

// Update the Neopixel signal
void writeSignal() {
  if((state == CO2_GREEN && co2 < CO2_YELLOW_RISING) || (state != CO2_GREEN && co2 < CO2_GREEN_FALLING)) {
    neopixel.setPixelColor(0, 0, NEOPIXEL_BRIGHTNESS, 0, 0);
    state = CO2_GREEN;
  }
  else if((state != CO2_RED && co2 < CO2_RED_RISING) || (state == CO2_RED && co2 < CO2_YELLOW_FALLING)) {
    neopixel.setPixelColor(0, NEOPIXEL_BRIGHTNESS, NEOPIXEL_BRIGHTNESS, 0, 0);
    state = CO2_YELLOW;
  }
  else {
    neopixel.setPixelColor(0, NEOPIXEL_BRIGHTNESS, 0, 0, 0);
    state = CO2_RED;
  }
  
  if(iaq <= IAQ_GREEN_LIMIT) {
    neopixel.setPixelColor(1, 0, NEOPIXEL_BRIGHTNESS, 0, 0);
  }
  else if(iaq <= IAQ_LIME_LIMIT) {
    neopixel.setPixelColor(1, NEOPIXEL_BRIGHTNESS/2, NEOPIXEL_BRIGHTNESS, 0, 0);
  }
  else if(iaq <= IAQ_YELLOW_LIMIT) {
    neopixel.setPixelColor(1, NEOPIXEL_BRIGHTNESS, NEOPIXEL_BRIGHTNESS, 0, 0);
  }
  else if(iaq <= IAQ_ORANGE_LIMIT) {
    neopixel.setPixelColor(1, NEOPIXEL_BRIGHTNESS, NEOPIXEL_BRIGHTNESS/2, 0, 0);
  }
  else if(iaq <= IAQ_RED_LIMIT) {
    neopixel.setPixelColor(1, NEOPIXEL_BRIGHTNESS, 0, 0, 0);
  }
  else if(iaq <= IAQ_MAGENTA_LIMIT) {
    neopixel.setPixelColor(1, NEOPIXEL_BRIGHTNESS, 0, NEOPIXEL_BRIGHTNESS/8, 0);
  }
  else {
    neopixel.setPixelColor(1, NEOPIXEL_BRIGHTNESS, 0, NEOPIXEL_BRIGHTNESS/4, 0);
  }
  neopixel.show();
}

// Upload values to Thingspeak
void uploadThingspeak() {
  if((millis() - lastThingspeakUpdate) < THINGSPEAK_INTERVAL) {
    return;
  }
  String cmd = "/update?api_key=" + String(THINGSPEAK_API_KEY);
  cmd += "&field1=" + String(co2);
  cmd += "&field2=" + String(humidity);
  cmd += "&field3=" + String(temperature);
  if(bme680.staticIaqAccuracy > 0) {
    cmd += "&field4=" + String(iaq);
  }
  cmd += "\n\r";
  httpGET(THINGSPEAK_HOST, cmd, 80);
  
  lastThingspeakUpdate = millis();
}

// Send HTTP GET request
String httpGET(String host, String cmd, int port) {
  #if WIFI_ENABLE
  WiFiClient client;
  
  String request = "GET http://"+ host + cmd + " HTTP/1.1\r\n";
  request += "Host: " + host + "\r\n";
  request += "Connection: close\r\n\r\n";
  String answer = "";
  
  int code = client.connect(host.c_str(), port);
  if(code) {
    client.print(request);
    
    if(client.available() > 0) {
      while(client.available()) {
        answer += client.readStringUntil('\r');
      }
    }
    client.stop();
  }
  else {
    Serial.println("\nNo connection to \"" + host + "\"!");
  }
  return answer;
  #endif
}
