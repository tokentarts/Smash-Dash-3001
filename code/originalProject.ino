#pragma message "SUMMER'S SMASH DASH 3000 PROJECT FT. JJ BROWN"

#include "Arduino.h"
#include "SoftwareSerial.h"
// #include "libraries/DFRobotDFPlayerMini/DFRobotDFPlayerMini.h"
#include <DFRobotDFPlayerMini.h>

#define FASTLED_INTERNAL // add this before including FastLED.h
//#include "libraries/FastLED-3.3.3/FastLED.h"
#include <FastLED.h>

// #region Config

// Set this to false for performance improvements:
// - Fewer calls to `Serial` to log debugging values
// - Fewer diagnostic calls to connected peripherals
#define IS_DEVELOPMENT false
#define ENABLE_LOGS false

// ---- Sound ----

// const bool ENABLE_SOUND = false;

#define MP3_POWER_RELAY_PIN 32

// Value range (0~30)
#define MP3_VOLUME (IS_DEVELOPMENT ? 15 : 25)

#define MP3_START_SOUND 1
#define MP3_BOOP_SOUND 2

#define MIN_BOOP_DELAY 750

// ---- Start Controls ----
#define startButtonPin 30    //the pin that the start button is attached to
#define startButtonLedPin 31 // the pin for start button LED
//TO DO: connect LED light

// ---- Player Controls ----

// Player 1 - Red
#define buttonPin 26    // the pin that the pushbutton is attached to
#define buttonLedPin 27 // the pin for button LED

// Player 2 - green
#define buttonPinTwo 22
#define buttonLedPinTwo 23

// ---- Lights ----

// int lightPins[] = {13, 12};
// const int lightPinsLength = sizeof(lightPins) / sizeof(int);

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

// ---- Game ----

#define MIN_PLAYER_POSITION 0
#define MAX_PLAYER_POSITION NUM_LEDS

#define PLAYER_VELOCITY_PICKUP 0.1
#define MAX_PLAYER_VELOCITY (32 * PLAYER_VELOCITY_PICKUP)

#define FRICTION .96

#define ON_POWER (IS_DEVELOPMENT ? 200 : 240)

#define GRAVITY_START 210
#define GRAVITY_AMOUNT 0.022

// ---- State Machine ----

#define STATE_NOT_LOADED -1
#define STATE_START_MENU 0
#define STATE_RESET_GAME 1
#define STATE_321_COUNTDOWN 2
#define STATE_GAMEPLAY 3
#define STATE_FINISH_LINE 4

// #endregion

// #region Global Variables

// ---- Sound ----
int lastBoopTime = 0;

SoftwareSerial mySoftwareSerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// ---- Lights ----

CRGB leds[NUM_LEDS];

// ---- Game ----

unsigned int buttonPushCounter = 0; // counter for the number of button presses
byte playerButtonState = 0;         // current state of the button
byte lastPlayerButtonState = 0;     // previous state of the button
byte playerButtonStateTwo = 0;
byte lastPlayerButtonStateTwo = 0;

byte startButtonState = 0;                     // current state for start buttom
byte lastStartButtonState = 0;                 // previous state of start button
unsigned long lastStartButtonLedBlinkTime = 0; // previous state of led high/low

float playerPosition = 0;
float playerPositionTwo = 0;
float playerVelocity = 0;
float playerVelocityTwo = 0;

const CRGB playerColor = CRGB(200, 0, 0).fadeToBlackBy(IS_DEVELOPMENT ? 128 : 0);

// 2
const CRGB playerColorTwo = CRGB(9, 227, 67).fadeToBlackBy(IS_DEVELOPMENT ? 128 : 0);

CRGB *winnerColor;

// ---- State Machine ----
byte currentState = STATE_NOT_LOADED;

// #endregion

void setState(
    // input
    int newState)
{
  // logic
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
}

void transitionFromStartMenu()
{
  digitalWrite(startButtonLedPin, LOW);
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
  buttonPushCounter = 0;
  playerButtonState = 0;
  lastPlayerButtonState = 0;
  playerButtonStateTwo = 0;
  lastPlayerButtonStateTwo = 0;

  startButtonState = 0;
  lastStartButtonState = 0;

  playerPosition = 0;
  playerPositionTwo = 0;
  playerVelocity = 0;
  playerVelocityTwo = 0;

  // Stop all audio
  myDFPlayer.pause(); //pause the mp3

  setState(STATE_321_COUNTDOWN);
}

void halt(const String &s)
{
  if (ENABLE_LOGS)
  {
    Serial.println("...Program halted...");
    Serial.println(s);
  }

  while (true)
  {
    delay(0); // Code to compatible with ESP8266 watch dog.
  }
}

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

void setupAudio()
{
  // Turn on MP3 player power relay.
  pinMode(MP3_POWER_RELAY_PIN, OUTPUT);
  digitalWrite(MP3_POWER_RELAY_PIN, LOW);

  // Start MP3 player serial connection
  // - `9600` is the baud rate of the connection
  mySoftwareSerial.begin(9600);

  // Initialize MP3 player
  waitForMp3Connection();

  myDFPlayer.reset();
  myDFPlayer.setTimeOut(500);    //Set serial communictaion time out 500ms
  myDFPlayer.volume(MP3_VOLUME); //Set volume value (0~30).
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  myDFPlayer.enableDAC();

  if (ENABLE_LOGS)
  {
    //----Read imformation----
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

void setupUserInput()
{
  // Player Buttons

  // Red Button
  pinMode(buttonPin, INPUT);
  pinMode(buttonLedPin, OUTPUT);

  // Green Button
  pinMode(buttonPinTwo, INPUT);
  pinMode(buttonLedPinTwo, OUTPUT);
}

/** Sets pin mode for player controller. */
void setupStartButton()
{
  // Start Button
  pinMode(startButtonPin, INPUT);
  pinMode(startButtonLedPin, OUTPUT);
}

void setupLights()
{
  // // Initialize pin states
  // for (int index = 0; index < lightPinsLength; index++)
  // {
  //   pinMode(lightPins[index], OUTPUT);
  // }

  FastLED.addLeds<NEOPIXEL, LED_STRING_DATA_PIN>(leds, NUM_LEDS);

  FastLED.clear();

  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MILLIAMPS);
}

void setup()
{
  // Start serial connection over USB.
  // This is what shows up in the serial monitor (baud rate in monitor must match argument here).
  Serial.begin(115200);

  setupLights();

  // Waits for MP3 connection
  setupAudio();

  setupUserInput();

  setupStartButton();

  // Initialize player state
  setState(STATE_START_MENU);
}

/** @return Current value of start button LED pin. */
bool blinkStartButtonLed(const unsigned long now)
{
  bool startButtonLedState = digitalRead(startButtonLedPin);

  if (now - lastStartButtonLedBlinkTime > 500)
  {
    lastStartButtonLedBlinkTime = now;

    // Blink start button LED
    if (startButtonLedState == LOW)
    {
      // Turn LED on
      digitalWrite(startButtonLedPin, ON_POWER);
      return true;
    }
    else
    {
      // Turn LED off
      digitalWrite(startButtonLedPin, LOW);
      return false;
    }
  }

  return startButtonLedState;
}

void loopStateStartMenu(
    const unsigned long now)
{
  // Check for start button press
  int startButtonState = digitalRead(startButtonPin);

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
      setState(STATE_RESET_GAME);
      return;
    }
  }

  bool startButtonLedState = blinkStartButtonLed(now);

  byte brightness = startButtonLedState ? ON_POWER * 0.5 : 0;
  CRGB color = CRGB(brightness, brightness, brightness);

  for (int lightIndex = 0; lightIndex < NUM_LEDS; lightIndex++)
  {
    leds[lightIndex] = color;
  }
}

void loopStateFinishLine(
    const unsigned long now)
{
  blinkStartButtonLed(now);

  // Finish Line - - - blinks color of winner once then rgb colors
  // Render player position
  for (int lightIndex = 0; lightIndex < NUM_LEDS; lightIndex++)
  {
    leds[lightIndex] = *winnerColor;
  }
}

void playBoopSound(
    const unsigned long now)

{
  // Play boop sound
  if (now - lastBoopTime > MIN_BOOP_DELAY)
  {
    lastBoopTime = now;

    myDFPlayer.playMp3Folder(MP3_BOOP_SOUND);
  }
}

void loopStateGameplay(unsigned long now, byte playerButtonState, byte playerButtonStateTwo)
{
  // #region User Input: Player One
  if (playerButtonState != lastPlayerButtonState)
  {
    if (ENABLE_LOGS)
    {
      Serial.print("playerButtonState= ");
      Serial.println(playerButtonState);
    }

    // if the state has changed, increment the counter
    if (playerButtonState == HIGH)
    {
      // if the current state is HIGH then the button
      // wend from off to on:
      buttonPushCounter++;
      if (ENABLE_LOGS)
      {
        Serial.print("number of button pushes:  ");
        Serial.println(buttonPushCounter);
      }

      playerVelocity = playerVelocity + PLAYER_VELOCITY_PICKUP;

      // CLAMP PLAYER VELOCITY
      if (playerVelocity > MAX_PLAYER_VELOCITY)
      {
        playerVelocity = MAX_PLAYER_VELOCITY;
      }

      playBoopSound(now);
    }

    // save the current state as the last state,
    //for next time through the loop
    lastPlayerButtonState = playerButtonState;
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
  playerPosition = playerPosition + playerVelocity;
  playerPositionTwo = playerPositionTwo + playerVelocityTwo;

  // APPLY FRICTION
  float lastPlayerVelocity = playerVelocity;
  playerVelocity = playerVelocity * FRICTION;

  float lastPlayerVelocityTwo = playerVelocityTwo;
  playerVelocityTwo = playerVelocityTwo * FRICTION;

  // Apply gravity
  // Note: Only check start of gravity, since end is end of LED strip.
  if (playerPosition > GRAVITY_START)
  {
    playerVelocity -= GRAVITY_AMOUNT;
  }

  if (playerPositionTwo > GRAVITY_START)
  {
    playerVelocityTwo -= GRAVITY_AMOUNT;
  }

  if (ENABLE_LOGS && floor(lastPlayerVelocity * 10) != floor(playerVelocity * 10))
  {
    Serial.print("playerVelocity= ");
    Serial.print(playerVelocity);
    Serial.print("\n");
  }

  if (playerPosition >= MAX_PLAYER_POSITION)
  {
    playerPosition = MAX_PLAYER_POSITION;

    winnerColor = &playerColor;

    setState(STATE_FINISH_LINE);
    return;
  }

  if (ENABLE_LOGS && floor(lastPlayerVelocityTwo * 10) != floor(playerVelocityTwo * 10))
  {
    Serial.print("playerVelocityTwo= ");
    Serial.print(playerVelocityTwo);
    Serial.print("\n");
  }

  if (playerPositionTwo >= MAX_PLAYER_POSITION)
  {
    playerPositionTwo = MAX_PLAYER_POSITION;

    winnerColor = &playerColorTwo;

    setState(STATE_FINISH_LINE);
    return;
  }

  // Check for start button press
  int startButtonState = digitalRead(startButtonPin);

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
      setState(STATE_START_MENU);
      return;
    }
  }
}

/** Update player feedback based on player controller state. */
void renderPlayerButton(
    byte playerButtonState, byte playerButtonStateTwo)
{
  if (playerButtonState == LOW)
  {
    digitalWrite(buttonLedPin, LOW);
  }
  else
  {
    digitalWrite(buttonLedPin, ON_POWER);
  }

  if (playerButtonStateTwo == LOW)
  {
    digitalWrite(buttonLedPinTwo, LOW);
  }
  else
  {
    digitalWrite(buttonLedPinTwo, ON_POWER);
  }
}

/**
 * Render player position on first 0-4 LEDs.
 * These are the first LEDs added to breadboard, rainbow colors.
 */
void renderPlayerPosition1()
{
  // // Render player position
  // for (int lightIndex = 0; lightIndex < lightPinsLength; lightIndex++)
  // {
  //   float distanceToPlayer = fabs(playerPosition - (float)lightIndex);
  //   if (distanceToPlayer > MAX_DISTANCE_TO_PLAYER)
  //   {
  //     distanceToPlayer = MAX_DISTANCE_TO_PLAYER;
  //   }

  //   float lightPower = ((MAX_DISTANCE_TO_PLAYER - distanceToPlayer) / MAX_DISTANCE_TO_PLAYER) * ON_POWER;

  //   // if (ENABLE_LOGS && lightPower > 0) {
  //   //   Serial.print("Light status: index= ");
  //   //   Serial.print(lightIndex);
  //   //   Serial.print(" distanceToPlayer= ");
  //   //   Serial.print(distanceToPlayer);
  //   //   Serial.print(" power= ");
  //   //   Serial.print(lightPower);
  //   //   Serial.print("\n");
  //   // }

  //   digitalWrite(lightPins[lightIndex], lightPower);
  // }
}

/**
 * Render player position on LED strip.
 */
void renderPlayerPosition2()
{
  // Render player position
  for (int lightIndex = 0; lightIndex < NUM_LEDS; lightIndex++)
  {
    // Player One - Red
    float distanceToPlayer = fabs(playerPosition - (float)lightIndex);
    if (distanceToPlayer > MAX_DISTANCE_TO_PLAYER)
    {
      distanceToPlayer = MAX_DISTANCE_TO_PLAYER;
    }

    float lightPowerPercent = (MAX_DISTANCE_TO_PLAYER - distanceToPlayer) / MAX_DISTANCE_TO_PLAYER;

    byte red = playerColor.red * lightPowerPercent;
    byte green = playerColor.green * lightPowerPercent;
    byte blue = playerColor.blue * lightPowerPercent;

    // if (ENABLE_LOGS && lightPower > 0) {
    //   Serial.print("Light status: index= ");
    //   Serial.print(lightIndex);
    //   Serial.print(" distanceToPlayer= ");
    //   Serial.print(distanceToPlayer);
    //   Serial.print(" power= ");
    //   Serial.print(lightPower);
    //   Serial.print("\n");
    // }

    // Player Two - Green
    float distanceToPlayerTwo = fabs(playerPositionTwo - (float)lightIndex);

    if (distanceToPlayerTwo > MAX_DISTANCE_TO_PLAYER)
    {
      distanceToPlayerTwo = MAX_DISTANCE_TO_PLAYER;
    }

    float lightPowerPercentTwo = (MAX_DISTANCE_TO_PLAYER - distanceToPlayerTwo) / MAX_DISTANCE_TO_PLAYER;

    // Note: Use `+=` to add player two brighness to player one brightness.
    red += playerColorTwo.red * lightPowerPercentTwo;
    green += playerColorTwo.green * lightPowerPercentTwo;
    blue += playerColorTwo.blue * lightPowerPercentTwo;

    // Clamp light max value
    if (red > ON_POWER)
    {
      red = ON_POWER;
    }
    if (green > ON_POWER)
    {
      green = ON_POWER;
    }
    if (blue > ON_POWER)
    {
      blue = ON_POWER;
    }

    leds[lightIndex].red = red;
    leds[lightIndex].green = green;
    leds[lightIndex].blue = blue;
  }
}

void loop()
{
  unsigned long now = millis();

  if (currentState == STATE_GAMEPLAY)
  {
    // user input
    // read the pushbutton input pin:
    byte playerButtonState = digitalRead(buttonPin);
    byte playerButtonStateTwo = digitalRead(buttonPinTwo);

    loopStateGameplay(now, playerButtonState, playerButtonStateTwo);
    renderPlayerButton(playerButtonState, playerButtonStateTwo);
    // renderPlayerPosition1();
    renderPlayerPosition2();
  }
  else if (currentState == STATE_START_MENU)
  {
    loopStateStartMenu(now);
  }
  else if (currentState == STATE_FINISH_LINE)
  {
    loopStateFinishLine(now);
  }

  // Version 2: render player position

  // Send 'leds' data from arduino to led strip.
  // Should call once per `loop`.
  FastLED.show();

  // If MP3 player is connected, log info about it
  if (ENABLE_LOGS && myDFPlayer.available())
  {
    // Print the detail message from DFPlayer to handle different errors and states.
    printDetail(myDFPlayer.readType(), myDFPlayer.read());
  }
}

void printDetail(uint8_t type, int value)
{
  switch (type)
  {
  case TimeOut:
    Serial.println(F("Time Out!"));
    break;
  case WrongStack:
    Serial.println(F("Stack Wrong!"));
    break;
  case DFPlayerCardInserted:
    Serial.println(F("Card Inserted!"));
    break;
  case DFPlayerCardRemoved:
    Serial.println(F("Card Removed!"));
    break;
  case DFPlayerCardOnline:
    Serial.println(F("Card Online!"));
    break;
  case DFPlayerUSBInserted:
    Serial.println("USB Inserted!");
    break;
  case DFPlayerUSBRemoved:
    Serial.println("USB Removed!");
    break;
  case DFPlayerPlayFinished:
    Serial.print(F("Number:"));
    Serial.print(value);
    Serial.println(F(" Play Finished!"));
    break;
  case DFPlayerError:
    Serial.print(F("DFPlayerError:"));
    switch (value)
    {
    case Busy:
      Serial.println(F("Card not found"));
      break;
    case Sleeping:
      Serial.println(F("Sleeping"));
      break;
    case SerialWrongStack:
      Serial.println(F("Get Wrong Stack"));
      break;
    case CheckSumNotMatch:
      Serial.println(F("Check Sum Not Match"));
      break;
    case FileIndexOut:
      Serial.println(F("File Index Out of Bound"));
      break;
    case FileMismatch:
      Serial.println(F("Cannot Find File"));
      break;
    case Advertise:
      Serial.println(F("In Advertise"));
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
}

// add a new controller light
// log the time it takes to run a loop function put a ms statement at top & bottom log difference between times
// log the time between multiple calls to loop function (make a new variable lastNow)
// document times somewhere
// enable led strip, log times again, compare time differences
// look through loop function, identify areas that are taking a long time, form strategy to mitigate them
