#pragma message "SUMMER'S SMASH DASH 3000 PROJECT FT. JJ BROWN\nOriginal implementation in Smash-Dash-3000/code/step6-final"

// #region Imports

#include "Arduino.h"

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

#include <SoftwareSerial.h>
// #include "libraries/DFRobotDFPlayerMini/DFRobotDFPlayerMini.h"
#include <DFRobotDFPlayerMini.h>

// #endregion Imports

// #region Constant Variables

// Use `#define` for constant variables to leverage compiler optimization

// Set this to false for performance improvements:
// - Fewer calls to `Serial` to log debugging values
// - Fewer diagnostic calls to connected peripherals
#define IS_DEVELOPMENT false
#define ENABLE_LOGS false

// This is the signal pin for the main power relay
#define POWER_RELAY_PIN 32

// ---- Sound ----

// Value range (0~30)
#define MP3_VOLUME (IS_DEVELOPMENT ? 15 : 28)

#define MP3_START_SOUND 1
#define MP3_BOOP_SOUND 2

#define MIN_BOOP_DELAY 300

// ---- Start Controls ----

// The pin that the start button switch is attached to
#define START_BUTTON_PIN 30
// The pin for start button LED
#define START_BUTTON_LED_PIN 31

// ---- Player Controls ----

// Player 1
#define BUTTON_PIN_ONE 26
#define BUTTON_LED_PIN_ONE 27
// Player 2
#define BUTTON_PIN_TWO 22
#define BUTTON_LED_PIN_TWO 23

// ---- Lights ----

#define NUM_LEDS 300
#define LED_STRING_DATA_PIN 12

// Render player position
#define MAX_DISTANCE_TO_PLAYER 2.0

// Note: Milliamp value determined experimentally.
// - 3500mA works well if the 15A power supply is connected directly to one
//   end of the LED strip.
// - 1850mA works well if the LED strip is powered through breadboard and
//   jumper wires
#define LED_MAX_MILLIAMPS 1850

// Maximum brightness displayed by any LED
#define LED_MAX_POWER (IS_DEVELOPMENT ? 200 : 240)

// ---- Game ----

#define MIN_PLAYER_POSITION 0
#define MAX_PLAYER_POSITION NUM_LEDS

// Magic number: Tune this based on your own gameplay
#define PLAYER_VELOCITY_PICKUP 0.11

// Maximum player speed is ~32 presses of controller button
#define MAX_PLAYER_VELOCITY (32 * PLAYER_VELOCITY_PICKUP)

// Magic number: Tune this based on your own gameplay
#define FRICTION .96

// The index of the LED on the strip where gravity should start being applied
#define GRAVITY_START 210

// Magic number: Tune this based on your own gameplay
#define GRAVITY_AMOUNT 0.022

// ---- State Machine ----

#define STATE_NOT_LOADED -1
#define STATE_START_MENU 0
#define STATE_RESET_GAME 1
#define STATE_321_COUNTDOWN 2
#define STATE_GAMEPLAY 3
#define STATE_FINISH_LINE 4

// #endregion Constant Variables

// #region Global Variables

// ---- Sound ----

// The last time (in milliseconds) that a sound was played.
// The DFPlayer can only play a single sound at once, so rapid calls to start
//   playing a sound result in never hearing more than the first few seconds of
//   the audio clip.
unsigned long lastBoopTime = 0;

// The SoftwareSerial is used to communicate with the DFPlayer.
// Argument order is (RX Pin, TX Pin)
SoftwareSerial mySoftwareSerial(10, 11);
DFRobotDFPlayerMini myDFPlayer;

// ---- Lights ----

// Create an array of `CRGB` instances.
// Each item in the array specifies the color for a given LED on the strip.
// Each item in the array holds a red, green, and blue value.
CRGB leds[NUM_LEDS];

// ---- Game ----

// The current state for start button
// 0 for not pressed, 1 for pressed.
byte lastStartButtonState = 0;
// The previous state of the start button.
byte startButtonState = 0;

// The current state of each controller.
byte playerButtonStateOne = 0;
byte playerButtonStateTwo = 0;

byte lastPlayerButtonStateOne = 0;
byte lastPlayerButtonStateTwo = 0;

unsigned long lastStartButtonLedBlinkTime = 0;

// In the game each player moves from the start to the end of the LED strip.
float playerPositionOne = 0;
float playerPositionTwo = 0;

// When a player controller is pressed, that player velocity is increased.
// Player velocity is applied to player position once per `loop()` call.
float playerVelocityOne = 0;
float playerVelocityTwo = 0;

// These variables store the color to display for each player
// 1: Red
const CRGB playerColorOne = CRGB(200, 0, 0).fadeToBlackBy(IS_DEVELOPMENT ? 128 : 0);
// 2: Green
const CRGB playerColorTwo = CRGB(9, 227, 67).fadeToBlackBy(IS_DEVELOPMENT ? 128 : 0);

byte winnerIndex = 0;
CRGB *winnerColor;
unsigned int finishLineAnimationStep = 0;
unsigned long finishLineLastAnimationStepTime = 0;

// ---- State Machine ----

byte currentState = STATE_NOT_LOADED;

// #endregion Global Variables

// #region State Machine Functions

void setState(int newState)
{
  int lastState = currentState;
  if (ENABLE_LOGS)
  {
    Serial.print("setState: newState= ");
    Serial.print(newState);
    Serial.print(" lastState= ");
    Serial.print(lastState);
    Serial.print("\n");
  }

  currentState = newState;

  // Transition between states

  // Transition FROM
  if (lastState == STATE_START_MENU)
  {
    transitionFromStartMenu();
  }

  // Transition TO
  if (currentState == STATE_321_COUNTDOWN)
  {
    transitionTo321Countdown();
  }
  else if (currentState == STATE_RESET_GAME)
  {
    transitionToResetGame();
  }
  else if (currentState == STATE_GAMEPLAY)
  {
    transitionToGameplay();
  }
  else if (currentState == STATE_FINISH_LINE)
  {
    transitionToFinishLine();
  }
}

void transitionToFinishLine()
{
  // Turn off all controller lights, leave winner on
  if (winnerIndex == 0)
  {
    digitalWrite(BUTTON_LED_PIN_ONE, LED_MAX_POWER);
    digitalWrite(BUTTON_LED_PIN_TWO, LOW);
  }
  else if (winnerIndex == 1)
  {
    digitalWrite(BUTTON_LED_PIN_ONE, LOW);
    digitalWrite(BUTTON_LED_PIN_TWO, LED_MAX_POWER);
  }
}

void transitionFromStartMenu()
{
  digitalWrite(START_BUTTON_LED_PIN, LOW);
}

/** Auto transition to STATE_GAMEPLAY */
void transitionTo321Countdown()
{
  // Play sound
  myDFPlayer.playMp3Folder(MP3_START_SOUND);

  // Wait for start sound to end
  delay(1000);

  setState(STATE_GAMEPLAY);
}

void transitionToGameplay()
{
  FastLED.clear();
}

/**
 * Reset all gameplay related variables.
 * Auto transition to STATE_START_MENU
 */
void transitionToResetGame()
{
  // Reset player state
  playerButtonStateOne = 0;
  lastPlayerButtonStateOne = 0;
  playerButtonStateTwo = 0;
  lastPlayerButtonStateTwo = 0;

  startButtonState = 0;
  lastStartButtonState = 0;

  playerPositionOne = 0;
  playerPositionTwo = 0;
  playerVelocityOne = 0;
  playerVelocityTwo = 0;

  winnerIndex = 0;
  winnerColor = nullptr;
  finishLineAnimationStep = 0;
  finishLineLastAnimationStepTime = 0;

  // Stop all audio
  myDFPlayer.pause();

  setState(STATE_321_COUNTDOWN);
}

// #endregion State Machine Functions

// #region Sound Functions

/**
 * This function will repeatedly attempt to establish a serial connection with
 * the DFPlayer. A single attempt infrequently fails. This guarantees a
 * consistent user experience.
 */
void waitForMp3Connection()
{
  if (ENABLE_LOGS)
    Serial.print("\nInitializing DFPlayer ... (May take 3~5 seconds)\n");

  int connectionTryCount = 0;

  while (true)
  {
    connectionTryCount++;

    if (ENABLE_LOGS)
    {
      Serial.print("Attempting connection to MP3 player... connectionTryCount= ");
      Serial.print(connectionTryCount);
      Serial.print("\n");
    }

    bool isMp3PlayerConnected = myDFPlayer.begin(mySoftwareSerial);

    if (!isMp3PlayerConnected)
    { //Use softwareSerial to communicate with mp3.
      Serial.println("Unable to begin, connection to MP3 player failed:");
      Serial.println("1.Please recheck the connection!");
      Serial.println("2.Please insert the SD card!");

      Serial.println("Retrying...");
      delay(1000);
    }
    else
    {
      break;
    }
  }

  if (ENABLE_LOGS)
    Serial.println("DFPlayer Mini online.");
}

void playBoopSound(const unsigned long now)
{
  if (now - lastBoopTime > MIN_BOOP_DELAY)
  {
    lastBoopTime = now;

    myDFPlayer.playMp3Folder(MP3_BOOP_SOUND);
  }
}

// #endregion Sound Functions

// #region Setup Functions

void setupLights()
{
  FastLED.addLeds<NEOPIXEL, LED_STRING_DATA_PIN>(leds, NUM_LEDS);

  FastLED.clear();

  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MILLIAMPS);
}

void setupAudio()
{
  // Start MP3 player serial connection
  // - `9600` is the baud rate of the connection
  mySoftwareSerial.begin(9600);

  // Initialize MP3 player
  waitForMp3Connection();

  myDFPlayer.reset();
  myDFPlayer.setTimeOut(500);
  myDFPlayer.volume(MP3_VOLUME);
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  myDFPlayer.enableDAC();

  if (ENABLE_LOGS)
  {
    Serial.print("mp3 state = ");
    Serial.println(myDFPlayer.readState());

    Serial.print("current volume = ");
    Serial.println(myDFPlayer.readVolume());

    Serial.print("EQ setting = ");
    Serial.println(myDFPlayer.readEQ());

    Serial.print("all file counts in SD card = ");
    Serial.println(myDFPlayer.readFileCounts());

    Serial.print("current play file number = ");
    Serial.println(myDFPlayer.readCurrentFileNumber());
  }
}

/** Sets pin mode for player controller. */
void setupStartButton()
{
  // Ensure the correct pin modes for start button are set
  pinMode(START_BUTTON_PIN, INPUT);
  pinMode(START_BUTTON_LED_PIN, OUTPUT);
}

void setupUserInput()
{
  // Ensure the correct pin modes are set for player buttons
  pinMode(BUTTON_PIN_ONE, INPUT);
  pinMode(BUTTON_LED_PIN_ONE, OUTPUT);

  pinMode(BUTTON_PIN_TWO, INPUT);
  pinMode(BUTTON_LED_PIN_TWO, OUTPUT);
}

void setup()
{
  if (ENABLE_LOGS)
  {
    // Start serial connection over USB.
    // This is what shows up in the serial monitor (baud rate in monitor must
    // match argument here).
    Serial.begin(9600);
  }

  // Turn on main power relay
  pinMode(POWER_RELAY_PIN, OUTPUT);
  digitalWrite(POWER_RELAY_PIN, LOW);

  setupLights();

  // Waits for MP3 connection
  setupAudio();

  setupStartButton();

  setupUserInput();

  // Initialize player state
  setState(STATE_START_MENU);
}

// #endregion Setup Functions

// #region Miscellaneous Functions

/** @return Current value of start button LED pin. */
bool blinkStartButtonLed(const unsigned long now)
{
  bool startButtonLedState = digitalRead(START_BUTTON_LED_PIN);

  if (now - lastStartButtonLedBlinkTime > 500)
  {
    lastStartButtonLedBlinkTime = now;

    // Blink start button LED
    if (startButtonLedState == LOW)
    {
      // Turn LED on
      digitalWrite(START_BUTTON_LED_PIN, LED_MAX_POWER);
      return true;
    }
    else
    {
      // Turn LED off
      digitalWrite(START_BUTTON_LED_PIN, LOW);
      return false;
    }
  }

  return startButtonLedState;
}

void setWinner(byte newWinnerIndex)
{
  winnerIndex = newWinnerIndex;

  if (winnerIndex == 0)
  {
    winnerColor = &playerColorOne;
  }
  else if (winnerIndex == 1)
  {
    winnerColor = &playerColorTwo;
  }
  else
  {
    // If unknown winner, transition to reset
    if (ENABLE_LOGS)
    {
      Serial.print("Unknown winnerIndex:");
      Serial.println(winnerIndex);
    }
    setState(STATE_RESET_GAME);
    return;
  }

  setState(STATE_FINISH_LINE);
}

bool checkStartButtonState()
{
  // Check for start button press
  int startButtonState = digitalRead(START_BUTTON_PIN);

  if (startButtonState != lastStartButtonState)
  {
    lastStartButtonState = startButtonState;

    if (ENABLE_LOGS)
    {
      Serial.print("startButtonState= ");
      Serial.println(startButtonState);
    }

    if (startButtonState == HIGH)
    {
      return true;
    }
  }

  return false;
}

// #endregion Miscellaneous Functions

// #region State Machine Loop Functions

void loopStateStartMenu(
    const unsigned long now)
{
  if (checkStartButtonState())
  {
    setState(STATE_RESET_GAME);
    return;
  }

  bool startButtonLedState = blinkStartButtonLed(now);

  // Set all LED strip to same state as start button LED
  byte brightness = startButtonLedState ? LED_MAX_POWER * 0.5 : 0;
  CRGB color = CRGB(brightness, brightness, brightness);

  for (int lightIndex = 0; lightIndex < NUM_LEDS; lightIndex++)
  {
    leds[lightIndex] = color;
  }
}

void loopStateFinishLine(
    const unsigned long now)
{
  if (checkStartButtonState())
  {
    setState(STATE_START_MENU);
    return;
  }

  // Indicate how to reset game to user
  blinkStartButtonLed(now);

  // Finish Line animation: drip the color of the winner from end of strip to
  // start. Leave non-winner colors on strip for bragging rights.
  if (
      now - finishLineLastAnimationStepTime > 15 &&
      finishLineAnimationStep < NUM_LEDS)
  {
    finishLineLastAnimationStepTime = now;

    unsigned int currentLedIndex = NUM_LEDS - finishLineAnimationStep;

    if (leds[currentLedIndex].red == 0 &&
        leds[currentLedIndex].green == 0 &&
        leds[currentLedIndex].blue == 0)
    {
      leds[currentLedIndex] = *winnerColor;
    }

    finishLineAnimationStep++;
  }
}

void loopStateGameplay(unsigned long now, byte playerButtonStateOne, byte playerButtonStateTwo)
{
  // #region User Input: Player One
  if (playerButtonStateOne != lastPlayerButtonStateOne)
  {
    if (ENABLE_LOGS)
    {
      Serial.print("playerButtonStateOne= ");
      Serial.println(playerButtonStateOne);
    }

    // if the state has changed, increment the counter
    if (playerButtonStateOne == HIGH)
    {
      // if the current state is HIGH then the button
      // wend from off to on:
      playerVelocityOne = playerVelocityOne + PLAYER_VELOCITY_PICKUP;

      // CLAMP PLAYER VELOCITY
      if (playerVelocityOne > MAX_PLAYER_VELOCITY)
      {
        playerVelocityOne = MAX_PLAYER_VELOCITY;
      }

      playBoopSound(now);
    }

    // save the current state as the last state,
    //for next time through the loop
    lastPlayerButtonStateOne = playerButtonStateOne;
  }
  // #endregion

  // #region User Input: Player Two
  if (playerButtonStateTwo != lastPlayerButtonStateTwo)
  {
    if (ENABLE_LOGS)
    {
      Serial.print("playerButtonStateTwo= ");
      Serial.println(playerButtonStateTwo);
    }

    // if the state has changed, increment the counter
    if (playerButtonStateTwo == HIGH)
    {
      playerVelocityTwo = playerVelocityTwo + PLAYER_VELOCITY_PICKUP;

      // CLAMP PLAYER VELOCITY
      if (playerVelocityTwo > MAX_PLAYER_VELOCITY)
      {
        playerVelocityTwo = MAX_PLAYER_VELOCITY;
      }

      playBoopSound(now);
    }

    // save the current state as the last state,
    //for next time through the loop
    lastPlayerButtonStateTwo = playerButtonStateTwo;
  }
  // #endregion

  // Update player position
  playerPositionOne = playerPositionOne + playerVelocityOne;
  playerPositionTwo = playerPositionTwo + playerVelocityTwo;

  // APPLY FRICTION
  float lastPlayerVelocity = playerVelocityOne;
  playerVelocityOne = playerVelocityOne * FRICTION;

  float lastPlayerVelocityTwo = playerVelocityTwo;
  playerVelocityTwo = playerVelocityTwo * FRICTION;

  // Apply gravity
  // Note: Only check start of gravity, since end is end of LED strip.
  if (playerPositionOne > GRAVITY_START)
  {
    playerVelocityOne -= GRAVITY_AMOUNT;
  }

  if (playerPositionTwo > GRAVITY_START)
  {
    playerVelocityTwo -= GRAVITY_AMOUNT;
  }

  if (ENABLE_LOGS && floor(lastPlayerVelocity * 10) != floor(playerVelocityOne * 10))
  {
    Serial.print("playerVelocityOne= ");
    Serial.println(playerVelocityOne);
  }

  if (playerPositionOne >= MAX_PLAYER_POSITION)
  {
    playerPositionOne = MAX_PLAYER_POSITION;
    setWinner(0);
    return;
  }

  if (ENABLE_LOGS && floor(lastPlayerVelocityTwo * 10) != floor(playerVelocityTwo * 10))
  {
    Serial.print("playerVelocityTwo= ");
    Serial.println(playerVelocityTwo);
  }

  if (playerPositionTwo >= MAX_PLAYER_POSITION)
  {
    playerPositionTwo = MAX_PLAYER_POSITION;
    setWinner(1);
    return;
  }
}

// #endregion State Machine Loop Functions

// #region Render Functions

/** Update player feedback based on player controller state. */
void renderPlayerButtons(
    byte playerButtonStateOne, byte playerButtonStateTwo)
{
  if (playerButtonStateOne == LOW)
  {
    digitalWrite(BUTTON_LED_PIN_ONE, LOW);
  }
  else
  {
    digitalWrite(BUTTON_LED_PIN_ONE, LED_MAX_POWER);
  }

  if (playerButtonStateTwo == LOW)
  {
    digitalWrite(BUTTON_LED_PIN_TWO, LOW);
  }
  else
  {
    digitalWrite(BUTTON_LED_PIN_TWO, LED_MAX_POWER);
  }
}

/** Render player positions on LED strip. */
void renderPlayerPositions()
{
  for (int lightIndex = 0; lightIndex < NUM_LEDS; lightIndex++)
  {
    // Player 1
    float distanceToPlayer = fabs(playerPositionOne - (float)lightIndex);
    if (distanceToPlayer > MAX_DISTANCE_TO_PLAYER)
    {
      distanceToPlayer = MAX_DISTANCE_TO_PLAYER;
    }

    float lightPowerPercent = (MAX_DISTANCE_TO_PLAYER - distanceToPlayer) / MAX_DISTANCE_TO_PLAYER;

    byte red = playerColorOne.red * lightPowerPercent;
    byte green = playerColorOne.green * lightPowerPercent;
    byte blue = playerColorOne.blue * lightPowerPercent;

    // if (ENABLE_LOGS && lightPower > 0) {
    //   Serial.print("Light status: index= ");
    //   Serial.print(lightIndex);
    //   Serial.print(" distanceToPlayer= ");
    //   Serial.print(distanceToPlayer);
    //   Serial.print(" power= ");
    //   Serial.print(lightPower);
    //   Serial.print("\n");
    // }

    // Player 2
    float distanceToPlayerTwo = fabs(playerPositionTwo - (float)lightIndex);

    if (distanceToPlayerTwo > MAX_DISTANCE_TO_PLAYER)
    {
      distanceToPlayerTwo = MAX_DISTANCE_TO_PLAYER;
    }

    float lightPowerPercentTwo = (MAX_DISTANCE_TO_PLAYER - distanceToPlayerTwo) / MAX_DISTANCE_TO_PLAYER;

    // Note: Use `+=` to add player two brightness to player one brightness.
    red += playerColorTwo.red * lightPowerPercentTwo;
    green += playerColorTwo.green * lightPowerPercentTwo;
    blue += playerColorTwo.blue * lightPowerPercentTwo;

    // Clamp light max value
    if (red > LED_MAX_POWER)
    {
      red = LED_MAX_POWER;
    }
    if (green > LED_MAX_POWER)
    {
      green = LED_MAX_POWER;
    }
    if (blue > LED_MAX_POWER)
    {
      blue = LED_MAX_POWER;
    }

    leds[lightIndex].red = red;
    leds[lightIndex].green = green;
    leds[lightIndex].blue = blue;
  }
}

// #endregion Render Functions

void loop()
{
  unsigned long now = millis();

  if (currentState == STATE_GAMEPLAY)
  {
    // Check for player button press
    byte playerButtonStateOne = digitalRead(BUTTON_PIN_ONE);
    byte playerButtonStateTwo = digitalRead(BUTTON_PIN_TWO);

    // Update game state
    loopStateGameplay(now, playerButtonStateOne, playerButtonStateTwo);

    // Display game state to user
    renderPlayerButtons(playerButtonStateOne, playerButtonStateTwo);
    renderPlayerPositions();
  }
  else if (currentState == STATE_START_MENU)
  {
    loopStateStartMenu(now);
  }
  else if (currentState == STATE_FINISH_LINE)
  {
    loopStateFinishLine(now);
  }

  // Send 'leds' data from arduino to led strip.
  // Should call once per `loop`.
  FastLED.show();
}
