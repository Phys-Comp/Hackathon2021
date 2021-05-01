// Display pins
#define ILI9341_CS 0
#define ILI9341_DC 15
#define STMPE360_TOUCH 2

// Sensor settings
#define MESSUREMENT_INTERVAL 2500
#define PLOT_INTERVAL 15000
#define TOUCH_INTERVAL 1000
#define TEMPERATURE_OFFSET 2.0

// CO2 signal settings
#define NUMBER 200
#define PAGES 3
#define CO2_GREEN_LIMIT 800
#define CO2_YELLOW_LIMIT 1000

// Messurement limits
#define CO2_MIN 0.0
#define CO2_MAX 2000.0
#define HUMIDITY_MIN 0.0
#define HUMIDITY_MAX 100.0
#define TEMP_MIN -10.0
#define TEMP_MAX 30.0

// Display colors
#define BACKGROUND 0xF73C   // Background color (light gray)
#define CO2_GREEN 0x36E3    // Green CO2 signal color
#define CO2_YELLOW 0xFF20   // Yellow CO2 signal color
#define CO2_RED 0xF186      // Red CO2 signal color
