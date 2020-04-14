#pragma message "Original implementation in Smash-Dash-3000/code/step3-playerControls"

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

// The pin that the start button switch is attached to
#define START_BUTTON_PIN 30
// The pin for start button LED
#define START_BUTTON_LED_PIN 31

// Player 1
#define BUTTON_PIN_ONE 26
#define BUTTON_LED_PIN_ONE 27
// Player 2
#define BUTTON_PIN_TWO 22
#define BUTTON_LED_PIN_TWO 23

// #endregion Constant Variables

// #region Global Variables

// Create an array of `CRGB` instances.
// Each item in the array specifies the color for a given LED on the strip.
// Each item in the array holds a red, green, and blue value.
CRGB leds[NUM_LEDS];

// The last known state of the start button.
// 0 for not pressed, 1 for pressed.
byte lastStartButtonState = 0;

// The last known state of each controller.
byte lastButtonStateOne = 0;
byte lastButtonStateTwo = 0;

// In the game each player moves from the start to the end of the LED strip.
float playerPositionOne = 0;
float playerPositionTwo = 0;

// These variables store the color to display for each player
// 1: Red
const CRGB playerColorOne = CRGB(200, 0, 0);
// 2: Green
const CRGB playerColorTwo = CRGB(9, 227, 67);

// #endregion Global Variables

// #region Functions

void setup()
{
  Serial.begin(9600);

  // Turn on main power relay
  pinMode(POWER_RELAY_PIN, OUTPUT);
  digitalWrite(POWER_RELAY_PIN, LOW);

  // Ensure the correct pin modes for start button are set
  pinMode(START_BUTTON_PIN, INPUT);
  pinMode(START_BUTTON_LED_PIN, OUTPUT);

  // Ensure the correct pin modes are set for player buttons
  pinMode(BUTTON_PIN_ONE, INPUT);
  pinMode(BUTTON_LED_PIN_ONE, OUTPUT);

  pinMode(BUTTON_PIN_TWO, INPUT);
  pinMode(BUTTON_LED_PIN_TWO, OUTPUT);

  // Initialize the FastLED instance.
  // Use the `NEOPIXEL` protocol for WS2812B strings.
  FastLED.addLeds<NEOPIXEL, LED_STRING_DATA_PIN>(leds, NUM_LEDS);

  // Clear any existing state on the LED strip
  FastLED.clear();

  // Limit maximum brightness.
  // Depends on power supply, LED strip length, LED density, power cable, etc.
  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MILLIAMPS);
}

void loop()
{
  // Check for start button press
  byte startButtonState = digitalRead(START_BUTTON_PIN);

  if (startButtonState != lastStartButtonState)
  {
    lastStartButtonState = startButtonState;
  }

  digitalWrite(START_BUTTON_LED_PIN, startButtonState * LED_ON_POWER);

  // Check for player button press
  byte buttonStateOne = digitalRead(BUTTON_PIN_ONE);
  byte buttonStateTwo = digitalRead(BUTTON_PIN_TWO);

  // Check player 1 button
  if (buttonStateOne != lastButtonStateOne)
  {
    lastButtonStateOne = buttonStateOne;

    // Button has moved from LOW to HIGH
    if (buttonStateOne == HIGH)
    {
      playerPositionOne++;

      Serial.print('playerPositionOne=\t');
      Serial.println(playerPositionOne);
    }
  }

  // Check player 2 button
  if (buttonStateTwo != lastButtonStateTwo)
  {
    lastButtonStateTwo = buttonStateTwo;
    if (buttonStateTwo == HIGH)
    {
      playerPositionTwo++;
    }
  }

  // Update controller lights
  digitalWrite(BUTTON_LED_PIN_ONE, buttonStateOne * LED_ON_POWER);
  digitalWrite(BUTTON_LED_PIN_TWO, buttonStateTwo * LED_ON_POWER);

  // Update light strip based on player positions
  for (int lightIndex = 0; lightIndex < NUM_LEDS; lightIndex++)
  {
    byte red = 0;
    byte green = 0;
    byte blue = 0;

    // Player 1
    float distanceToPlayer = fabs(playerPositionOne - (float)lightIndex);
    if (distanceToPlayer == 0)
    {
      red += playerColorOne.red;
      green += playerColorOne.green;
      blue += playerColorOne.blue;
    }

    // Player 2
    distanceToPlayer = fabs(playerPositionTwo - (float)lightIndex);
    if (distanceToPlayer == 0)
    {
      red += playerColorTwo.red;
      green += playerColorTwo.green;
      blue += playerColorTwo.blue;
    }

    // Clamp light max value
    if (red > LED_ON_POWER)
    {
      red = LED_ON_POWER;
    }
    if (green > LED_ON_POWER)
    {
      green = LED_ON_POWER;
    }
    if (blue > LED_ON_POWER)
    {
      blue = LED_ON_POWER;
    }

    leds[lightIndex].red = red;
    leds[lightIndex].green = green;
    leds[lightIndex].blue = blue;
  }

  // Send 'leds' data from arduino to led strip.
  FastLED.show();
}

// #endregion
