#pragma message "Original implementation in Smash-Dash-3000/code/step1-turnOnLights"

// #region Imports

#include <Arduino.h>

// #region FastLED Import

// Add this before including `FastLED.h` to quiet pragma messages
#define FASTLED_INTERNAL

// Use this to import the library from project subdirectory.
// Note: This fails with VSCode Arduino compilation.
//#include "libraries/FastLED-3.3.3/FastLED.h"

// Use this import to include the global FastLED library
//   located at `~/Documents/Arduino/libraries/FastLED-3.3.3/`.
#include <FastLED.h>

// #endregion FastLED Import

// #endregion Imports

// #region Constant Variables

// Use `#define` for constant variables to leverage compiler optimization
#define NUM_LEDS 300
#define LED_STRING_DATA_PIN 12

// Note: Milliamp value determined experimentally.
// - 3500mA works well if the 15A power supply is connected directly to one
//   end of the LED strip.
// - 1850mA works well if the LED strip is powered through breadboard and
//   jumper wires
#define LED_MAX_MILLIAMPS 1850

// Maximum brightness displayed by any LED
#define LED_ON_POWER 200

// This is the signal pin for the main power relay
#define POWER_RELAY_PIN 32

// #endregion Constant Variables

// #region Global Variables

// Create an array of `CRGB` instances.
// Each item in the array specifies the color for a given LED on the strip.
// Each item in the array holds a red, green, and blue value.
CRGB leds[NUM_LEDS];

// #endregion Global Variables

// #region Functions

void setup()
{
  // Turn on main power relay
  pinMode(POWER_RELAY_PIN, OUTPUT);
  digitalWrite(POWER_RELAY_PIN, LOW);

  // Initialize the FastLED instance.
  // Use the `NEOPIXEL` protocol for WS2812B strings.
  FastLED.addLeds<NEOPIXEL, LED_STRING_DATA_PIN>(leds, NUM_LEDS);

  // Clear any existing state on the LED strip
  FastLED.clear();

  // Limit maximum brightness.
  // Depends on power supply, LED strip length, LED density, power cable, etc.
  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MILLIAMPS);

  // Set LEDs to alternating primary colors
  for (int lightIndex = 0; lightIndex < NUM_LEDS; lightIndex++)
  {
    byte lightRemainder = lightIndex % 3;

    leds[lightIndex].red = (lightRemainder == 0 ? 1 : 0) * LED_ON_POWER;
    leds[lightIndex].green = (lightRemainder == 1 ? 1 : 0) * LED_ON_POWER;
    leds[lightIndex].blue = (lightRemainder == 2 ? 1 : 0) * LED_ON_POWER;
  }

  // Send 'leds' data from arduino to led strip.
  FastLED.show();
}

void loop()
{
  delay(1000);
}

// #endregion
