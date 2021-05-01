#include <Adafruit_ILI9341.h>
#include <Adafruit_GFX.h>
#include <Adafruit_STMPE610.h>
#include <Wire.h>
#include <SparkFun_SCD30_Arduino_Library.h>

#include "Icons.h"
#include "Settings.h"

// Represents a display page
typedef struct Page {
  String title;
  float minValue;
  float maxValue;
  float *values;
} Page;

// TFT display
Adafruit_ILI9341 display = Adafruit_ILI9341(ILI9341_CS, ILI9341_DC);
Adafruit_STMPE610 touchscreen = Adafruit_STMPE610(STMPE360_TOUCH);

// CO2 sensor (SCD30)
SCD30 scd30;

// Current messurement values
unsigned int co2Value;                                              // CO2 messurement value
float humidValue;                                                   // Humidity messurement value
float tempValue;                                                    // Temperature messurement value

// Counter for messurement values
unsigned int counter = 0;

// Messurement arrays
float co2[NUMBER] = {0};                                            // Array of CO2 messurement values
float humidity[NUMBER] = {0};                                       // Array of humidity messurement values
float temperature[NUMBER] = {0};                                    // Array of temperature messurement values

const Page pages[PAGES] = {                                         // Array of display pages
  {"CO2 [ppm]", CO2_MIN, CO2_MAX, co2},
  {"Humidity [%]", HUMIDITY_MIN, HUMIDITY_MAX, humidity},
  {"Temperature [C]", TEMP_MIN, TEMP_MAX, temperature}
};
unsigned int currentPage = 0;                                       // Index of the current display page

unsigned long lastValueUpdate = 0;                                  // Last messurement update
unsigned long lastPlotUpdate = 0;                                   // Last plot diagram update
unsigned long lastTouchUpdate = 0;                                  // Last touchscreen update

void setup() {
  Serial.begin(115200);
  
  // Connectning the SCD30
  Serial.println("\nConnecting SCD30...");
  Wire.begin();                                                     // Starting I2C connection
  
  while(!scd30.begin()) {                                           // Starting the sensor
    Serial.println("Connection failed!");                           // Printing sensor error
    delay(5000);
  }
  scd30.reset();                                                    // Resetting CO2 sensor
  scd30.setAutoSelfCalibration(false);                              // Disabling the auto self calibration
  scd30.setMeasurementInterval(1);                                  // Setting the messurement interval
  Wire.setClock(100000);                                            // Setting I2C clock
  Wire.setClockStretchLimit(200000);                                // Setting I2C clock stretch limit
  Serial.println("SCD30 connected!");
  
  delay(1000);
  scd30.getCO2();                                                   // Calibrating CO2 sensor
  delay(1500);
  
  // Starting the display
  touchscreen.begin();                                              // Starting the touchscreen
  display.begin();                                                  // Staring the TFT display
  display.fillScreen(ILI9341_WHITE);                                // Clearing the display
  display.setTextColor(ILI9341_BLACK);                              // Setting the text color
  display.setTextWrap(false);

  // Drawing the upper section
  display.fillRoundRect(2, 5, 236, 95, 10, BACKGROUND);             // Drawing the upper section

  // Drawing the plot
  Page page = pages[currentPage];
  display.fillRoundRect(2, 110, 236, 208, 10, BACKGROUND);          // Drawing the lower section
  display.setTextSize(2);
  display.setCursor(10, 120);
  display.println(page.title);                                      // Drawing the plot title
  drawPlot(page.minValue, page.maxValue);                           // Drawing the plot diagramm
}

void loop() {
  Page page = pages[currentPage];

  if((millis() - lastValueUpdate) >= MESSUREMENT_INTERVAL) {
    // Reading messurement values
    co2Value = scd30.getCO2();                                      // Reading the CO2 value
    humidValue = scd30.getHumidity();                               // Reading the humidity
    tempValue = scd30.getTemperature() - TEMPERATURE_OFFSET;        // Reading the temperature
  
    // Drawing the CO2 smiley
    if(co2Value <= CO2_GREEN_LIMIT) {
      display.fillCircle(42, 52, 32, CO2_GREEN);                    // Green CO2 smiley
      display.drawBitmap(29, 59, happy, 28, 11, ILI9341_BLACK);     // Drawing smiley mouth (happy)
    }
    else if(co2Value <= CO2_YELLOW_LIMIT) {
      display.fillCircle(42, 52, 32, CO2_YELLOW);                   // Yellow CO2 smiley
      display.drawBitmap(29, 59, medium, 28, 11, ILI9341_BLACK);    // Drawing smiley mouth (medium)
    }
    else {
      display.fillCircle(42, 52, 32, CO2_RED);                      // Red CO2 smiley
      display.drawBitmap(29, 59, sad, 28, 11, ILI9341_BLACK);       // Drawing smiley mouth (sad)
    }  
    display.fillCircle(32, 43, 4, ILI9341_BLACK);                   // Drawing smiley eyes
    display.fillCircle(53, 43, 4, ILI9341_BLACK);
  
    // Drawing the current messurement values
    display.setTextSize(2);
    display.setCursor(85, 25);
    display.fillRect(84, 20, 154, 22, BACKGROUND);                  // Overriding CO2 value
    display.print("CO2: " + String(co2Value) + "ppm");              // Drawing new CO2 value
    display.setCursor(85, 45);
    display.fillRect(84, 42, 154, 22, BACKGROUND);                  // Overriding humidity value
    display.print("Humid: " + String(humidValue, 1) + "%");         // Drawing new humidity value
    display.setCursor(85, 65);
    display.fillRect(84, 64, 154, 22, BACKGROUND);                  // Overriding temperature value
    display.print("Temp: " + String(tempValue, 1) + "C");           // Drawing new temperature value

    lastValueUpdate = millis();                                     // Updating last messurement time
  }

  if((millis() - lastPlotUpdate) >= PLOT_INTERVAL) {
    // Storing the messurement values
    if(counter < NUMBER) {                                          // Less than 200 messurement values
      co2[counter] = co2Value;                                      // Appending CO2 value to list
      humidity[counter] = humidValue;                               // Appending humidity value to list
      temperature[counter] = tempValue;                             // Appending temperature value to list
      counter++;                                                    // Counting counter up
    }
    else {                                                          // 200 or more messurement values
      for(int i = 0 ; i < NUMBER-1; i++) {                          // Moving all list values one to the left
        co2[i] = co2[i+1];
        humidity[i] = humidity[i+1];
        temperature[i] = temperature[i+1];
      }
      co2[NUMBER-1] = co2Value;                                     // Appending CO2 value to list
      humidity[NUMBER-1] = humidValue;                              // Appending humidity value to list
      temperature[NUMBER-1] = tempValue;                            // Appending temperature value to list
    }
  
    // Plotting messurement values
    display.fillRect(30, 150, 200, 160, BACKGROUND);                // Overriding plot values
    plot(page.values, counter, page.minValue, page.maxValue);       // Plotting messurement values
    
    lastPlotUpdate = millis();                                      // Updating last plotting time
  }
  
  if (touchscreen.touched() && (millis() - lastTouchUpdate) >= TOUCH_INTERVAL) {
    // Updating current page
    currentPage = (currentPage >= PAGES-1) ? 0 : currentPage + 1;   // Updating page index
    page = pages[currentPage];
    
    // Drawing the plot
    display.setTextSize(2);
    display.setCursor(10, 120);
    display.fillRoundRect(2, 110, 236, 208, 10, BACKGROUND);        // Drawing the lower section
    display.println(page.title);                                    // Drawing the plot title
    drawPlot(page.minValue, page.maxValue);                         // Drawing the plot diagramm    
    plot(page.values, counter, page.minValue, page.maxValue);       // Plotting messurement values

    lastTouchUpdate = millis();                                     // Updating last touchscreen time
  }
}

// Draws a plot diagramm
void drawPlot(float min, float max) {
  // Drawing x-axis (time)
  display.drawFastHLine(30, 310, 204, ILI9341_BLACK);               // Drawing line for x-axis (time)
  display.drawLine(234, 310, 231, 313, ILI9341_BLACK);              // Drawing upper line for x-axis arrowhead
  display.drawLine(234, 310, 231, 307, ILI9341_BLACK);              // Drawing lower line for x-axis arrowhead
  
  // Drawing y-axis (messurement value)
  display.drawFastVLine(29, 146, 164, ILI9341_BLACK);               // Drawing line for y-axis (messurement value)
  display.drawLine(29, 146, 26, 149, ILI9341_BLACK);                // Drawing left line for y-axis arrowhead
  display.drawLine(29, 146, 32, 149, ILI9341_BLACK);                // Drawing right line for y-axis arrowhead

  // Drawing y-scale
  display.setTextSize(1);
  display.setCursor(2, 146);
  display.printf("%4.0f", max);                                     // Scale value for 100%
  display.setCursor(2, 186);
  display.printf("%4.0f", min + 0.75*(max - min));                  // Scale value for 75%
  display.drawFastHLine(27, 190, 3, ILI9341_BLACK);                 // Line for scale value 75%
  display.setCursor(2, 226);
  display.printf("%4.0f", min + 0.5*(max - min));                   // Scale value for 50%
  display.drawFastHLine(27, 230, 3, ILI9341_BLACK);                 // Line for scale value 50%
  display.setCursor(2, 266);
  display.printf("%4.0f", min + 0.25*(max - min));                  // Scale value for 25%
  display.drawFastHLine(27, 270, 3, ILI9341_BLACK);                 // Line for scale value 25%
  display.setCursor(2, 306);
  display.printf("%4.0f", min);                                     // Scale value for 0%
  display.drawFastHLine(27, 310, 3, ILI9341_BLACK);                 // Line for scale value 0%
}

// Plots the messurement values
void plot(float values[], int count, float min, float max) {
  // Drawing the graph
  int height = display.height() - 170;                              // Maximum display height for messurement values
  for(int x = 1; x < count; x++) {                                  // Iterating over messurement values    
    // Scaling messurement values
    int y0 = height + (160 - scale(values[x-1], min, max, 0, 160)); // Scaling last value for display
    int y1 = height + (160 - scale(values[x], min, max, 0, 160));   // Scaling next value for display
    display.drawLine(29 + x, y0, 30 + x, y1, ILI9341_BLACK);        // Drawing line from last to next value
  }
}

// Scales a value of an interval to a new interval
int scale(float value, float fromMin, float fromMax, float toMin, float toMax) {
  value = constrain(value, fromMin, fromMax);                       // Limiting value to original interval
  return (int) (toMin + (value - fromMin) * (toMax - toMin) / (fromMax - fromMin));
}
