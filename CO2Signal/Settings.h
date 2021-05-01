// WiFi settings
#define WIFI_ENABLE true
#define WIFI_SSID "<YOUR_SSID>"
#define WIFI_PASSWORD "<YOUR_PASSWORD>"

// Thingspeak settings
#define THINGSPEAK_HOST "api.thingspeak.com"
#define THINGSPEAK_API_KEY "<YOUR_API_KEY>"
#define THINGSPEAK_INTERVAL 15000

// CO2 signal limits
#define CO2_YELLOW_RISING 800.0
#define CO2_RED_RISING 1000.0
#define CO2_YELLOW_FALLING 500.0
#define CO2_GREEN_FALLING 700.0

// IAQ signal limits
#define IAQ_GREEN_LIMIT 50
#define IAQ_LIME_LIMIT 100
#define IAQ_YELLOW_LIMIT 150
#define IAQ_ORANGE_LIMIT 200
#define IAQ_RED_LIMIT 250
#define IAQ_MAGENTA_LIMIT 350

// I2C settings
#define I2C_CLOCK 100000
#define I2C_STRECH_LIMIT 200000

// Neopixel settings
#define NEOPIXEL_COUNT 2
#define NEOPIXEL_PIN 13
#define NEOPIXEL_BRIGHTNESS 255

// LCD settings
#define LCD_PAGE_INTERVAL 5000
#define LCD_UPDATE_INTERVAL 1000

// SCD30 settings
#define SDC60_AUTO_CALIBRATION false
#define SDC60_MESUREMENT_INTERVAL 1
#define SDC60_TEMPERATURE_OFFSET 2.0

// Calibration settings
#define CALIBRATION_ALTITUDE 0
#define CALIBRATION_CO2_VALUE 420
