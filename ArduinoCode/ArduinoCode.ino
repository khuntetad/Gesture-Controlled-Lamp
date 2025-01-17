#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_APDS9960.h>

// Define the LED strip configuration
#define DATA_PIN    9        // Pin connected to WS2811 strip
#define NUM_LEDS    60       // Number of LEDs
#define BRIGHTNESS  128      // Default brightness level
#define LED_TYPE    WS2811   // LED type
#define COLOR_ORDER RGB      // Color order

CRGB leds[NUM_LEDS];
// start with LEDs off
CRGB currentColor = CRGB::Black;

// Initialize gesture sensor
Adafruit_APDS9960 apds;

// variable for gesture-controlled brightness and color
// get the last gesture time
uint8_t brightness = BRIGHTNESS;
uint8_t colorIndex = 0; 
uint8_t previousGesture = 0; 
unsigned long lastGestureTime = 0; 

void setup() {
  Serial.begin(9600); // For Serial communication

  // Initialize LED strip
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);
  fill_solid(leds, NUM_LEDS, currentColor);
  FastLED.show();

  // Initialize Gesture Sensor
  Wire.begin();
  if (!apds.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor!");
    while (1);
  }
  apds.enableGesture(true);

  // Configure gesture sensitivity
  //apds.setGestureGain(APDS9960_GAIN_4X);  // Increase the gain for better sensitivity
  //apds.setGestureThreshold(10);           // Set an appropriate threshold for detecting gestures

  Serial.println("Setup complete! Use gestures or Serial input to control LEDs.");
}

void loop() {
  // Handle Serial input for finger count
  if (Serial.available() > 0) {
    int fingers = Serial.parseInt();

    while (Serial.available() > 0) {
      Serial.read();  // Clear any remaining data
    }

    if (fingers >= 0 && fingers <= 5) {
      CRGB chosenColor = getColorForFingers(fingers);

      if (chosenColor != currentColor) {
        currentColor = chosenColor;
        fill_solid(leds, NUM_LEDS, currentColor);
        FastLED.show();

        Serial.print("Color set for ");
        Serial.print(fingers);
        Serial.println(" fingers.");
      } else {
        Serial.println("Same color as current, no update made.");
      }
    } else {
      Serial.println("Invalid input. Please send a number between 0 and 5.");
    }
  }

  if (apds.gestureValid()) {
    uint8_t gesture = apds.readGesture();

    // use a timer to make sure we aren't rendering the same thing twice
    if (gesture != previousGesture && millis() - lastGestureTime > 200) {
      previousGesture = gesture;
      lastGestureTime = millis();

      switch (gesture) {
        // increase the brightness on right swipe and account for overflow
        case APDS9960_RIGHT:
          brightness += 75;
          if (brightness > 180) brightness = 255;
          updateLEDs();
          break;

        // reduce the brightness on left swipe and account for overflow
        case APDS9960_LEFT:
          brightness -= 75;
          if (brightness < 75) brightness = 0;
          updateLEDs();
          break;

        default:
          break;
      }
    }
  }
}

// make a basic helper that can update the Fast LED lights
// brightness and color index are what are important here
void updateLEDs() {
  FastLED.setBrightness(brightness);
  fill_solid(leds, NUM_LEDS, CHSV(colorIndex, 255, 255));
  FastLED.show();
}

// get the leds that we want to show
CRGB getColorForFingers(int count) {
  switch (count) {
    case 1:  return CRGB::Red;
    case 2:  return CRGB::Green;
    case 3:  return CRGB::Blue;
    case 4:  return CRGB::Yellow;
    case 5:  return CRGB::Purple;
    default: return CRGB::White;
  }
}
