
/* Arduino-Phone OS
 * 
 * This sketch lays out a few screens for a phone based on the Adafruit
 * FONA shield, their TFT screen, and their Metro 328 Arduino device.
 * 
 * 
 * 
 * 
 * 
 * 
 * Brian Gentry
 * Licensed via MIT license.
 */
#include <SPI.h>
//#include <Wire.h>      // this is needed even tho we aren't using it
//#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Adafruit_STMPE610.h>
#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"

// Pins for FONA use.
#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4
#define FONA_RI 6

// For the Adafruit TFT shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10

// For the touchscreen part of display use pin 8
#define STMPE_CS 8

// Screen identifiers.
#define SCR_MAIN 1
#define SCR_PHONE 2
#define SCR_RADIO 4
#define SCR_FUT1 8
#define SCR_FUT2 16

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

// Text box where info goes.
#define TEXT_X 10
#define TEXT_Y 10
#define TEXT_W 220
#define TEXT_H 30
#define TEXT_TSIZE 2
#define TEXT_TCOLOR ILI9341_MAGENTA

// Status line location (for status messages).
#define STATUS_X 10
#define STATUS_Y 45

// Length of text allowed in primary text field.
#define TEXT_LEN 20

// FM Radio settings.
#define FM_FREQ_MIN 870
#define FM_FREQ_MAX 1090

// Serial communication with FONA device.
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);

// Create objects for interfacing iwth Fona and TFT.
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// The STMPE610 uses hardware SPI on the shield, and #8
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// Current radio station.
uint16_t currentStation = 0;

// Tracking the last button pressed.
byte lastButton = 0;

// Tracking the screen currently shown.
byte currentScreen = 0;

// The data (phone #) we store in the textfield
char textfield[TEXT_LEN + 1] = "";
uint8_t textfield_i = 0;

// Struct for holding button elements.
typedef struct {
  int16_t x;
  int16_t y;
  uint16_t w;
  uint16_t h;
  uint16_t outlinecolor;
  uint16_t fillcolor;
  uint16_t textcolor;
  char label [9];
  uint8_t textsize;
} button;

// Main menu buttons.
const button mainbuttons[4] PROGMEM = {
  {40, 80, 60, 60, ILI9341_WHITE, ILI9341_PURPLE, ILI9341_WHITE, "Phone", 1},
  {120, 80, 60, 60, ILI9341_WHITE, ILI9341_DARKCYAN, ILI9341_WHITE, "Radio", 1},
  {40, 160, 60, 60, ILI9341_WHITE, ILI9341_DARKCYAN, ILI9341_WHITE, "Future 1", 1},
  {120, 160, 60, 60, ILI9341_WHITE, ILI9341_PURPLE, ILI9341_WHITE, "Future 2", 1}
};

// Phone UI buttons.
const button phonebuttons[15] PROGMEM = {
  {10, 80, 60, 30, ILI9341_WHITE, ILI9341_DARKGREEN, ILI9341_WHITE, "Pickup", 1},
  {90, 80, 60, 30, ILI9341_WHITE, ILI9341_DARKGREY, ILI9341_WHITE, "Clear", 1},
  {170, 80, 60, 30, ILI9341_WHITE, ILI9341_RED, ILI9341_WHITE, "Hangup", 1},
  {10, 130, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "1", 1},
  {90, 130, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "2 ABC", 1},
  {170, 130, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "3 DEF", 1},
  {10, 180, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "4 GHI", 1},
  {90, 180, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "5 JKL", 1},
  {170, 180, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "6 MNO", 1},
  {10, 230, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "7 PQRS", 1},
  {90, 230, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "8 TUV", 1},
  {170, 230, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "9 WXYZ", 1},
  {10, 280, 60, 30, ILI9341_WHITE, ILI9341_ORANGE, ILI9341_WHITE, "*", 1},
  {90, 280, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "0", 1},
  {170, 280, 60, 30, ILI9341_WHITE, ILI9341_ORANGE, ILI9341_WHITE, "#", 1},
};

const button radiobuttons[11] PROGMEM = {
  {10, 80, 60, 30, ILI9341_WHITE, ILI9341_DARKGREEN, ILI9341_WHITE, "On", 1},
  {90, 80, 60, 30, ILI9341_WHITE, ILI9341_DARKGREY, ILI9341_WHITE, "Off", 1},
  {170, 80, 60, 30, ILI9341_WHITE, ILI9341_RED, ILI9341_WHITE, "Back", 1},
  {10, 130, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "+ 1", 1},
  {90, 130, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "+ 10", 1},
  {170, 130, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "+ 100", 1},
  {10, 230, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "- 1", 1},
  {90, 230, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "- 10", 1},
  {170, 230, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "- 100", 1},
  {10, 280, 60, 30, ILI9341_WHITE, ILI9341_DARKGREEN, ILI9341_WHITE, "Vol +", 1},
  {170, 280, 60, 30, ILI9341_WHITE, ILI9341_RED, ILI9341_WHITE, "Vol -", 1},
};

// Print something in the mini status bar with either flashstring
void status(const __FlashStringHelper *msg) {
  tft.fillRect(STATUS_X, STATUS_Y, 240, 8, ILI9341_BLACK);
  tft.setCursor(STATUS_X, STATUS_Y);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.print(msg);
}

void deleteChar()
{
  textfield[textfield_i] = 0;
  if (textfield > 0) {
    textfield_i--;
    textfield[textfield_i] = ' ';
  }
}

void appendChar(char c)
{
  if (textfield_i < TEXT_LEN) {
    textfield[textfield_i] = c;
    textfield_i++;
    textfield[textfield_i] = 0; // zero terminate
  }
}

bool tuneRadio(uint16_t station)
{
  bool retval = false;
  
  // Check to see if we are in allowable range.
  if (station >= FM_FREQ_MIN && station <= FM_FREQ_MAX)
  {
    // Try to change station.
    retval = fona.tuneFMradio(station);
  }

  // Return result.
  return retval;
}

void drawTextField()
{
  Serial.println(textfield);
  tft.setCursor(TEXT_X + 2, TEXT_Y + 10);
  tft.setTextColor(TEXT_TCOLOR, ILI9341_BLACK);
  tft.setTextSize(TEXT_TSIZE);
  tft.print(textfield);
}

void drawButton(button btn, bool pressed) {

  // Temp variables for colors.
  uint16_t fill, outline, text;

  Serial.print("Label: "); Serial.println(btn.label);

  // Check to see if we are drawing 'pressed' state.
  if (pressed) {

    // Reverse colors.
    fill    = btn.textcolor;
    outline = btn.outlinecolor;
    text    = btn.fillcolor;
  }

  else {

    // Normal colors.
    fill    = btn.fillcolor;
    outline = btn.outlinecolor;
    text    = btn.textcolor;
  }

  // Corner radius.
  uint8_t r = min(btn.w, btn.h) / 4;

  // Draw each button.
  tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, r, fill);
  tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, r, outline);

  // Print the label on the button
  tft.setCursor(btn.x + (btn.w / 2) - (strlen(btn.label) * 3 * btn.textsize), btn.y + (btn.h / 2) - (4 * btn.textsize));
  tft.setTextColor(text);
  tft.setTextSize(btn.textsize, btn.textsize);
  tft.print(btn.label);
}

void setCurrentScreen(byte screen) {

  // Check to see if we are already on that screen.
  if (currentScreen != screen) {

    // Replace current screen.
    currentScreen = (~currentScreen) & screen;
  }
}

void drawMainUI() {

  // Button reference.
  button btn;

  // Clear the UI
  tft.fillScreen(ILI9341_BLACK);

  for (uint8_t cnt = 0; cnt < 4; cnt++) {

    // Get reference to struct.
    memcpy_P (&btn, &mainbuttons[cnt], sizeof btn);

    // Draw the button.
    drawButton(btn, false);
  }

  // Set the currently active screen to main.
  setCurrentScreen(SCR_MAIN);
}

void drawPhoneUI() {

  // Button reference.
  button btn;

  // Clear the UI
  tft.fillScreen(ILI9341_BLACK);

  for (uint8_t cnt = 0; cnt < 15; cnt++) {

    // Get reference to struct.
    memcpy_P (&btn, &phonebuttons[cnt], sizeof btn);

    // Draw the button.
    drawButton(btn, false);
  }

  // Set current screen to Phone.
  setCurrentScreen(SCR_PHONE);
}

void drawRadioUI() {

  // Button reference.
  button btn;

  // Clear the UI
  tft.fillScreen(ILI9341_BLACK);

  for (uint8_t cnt = 0; cnt < 15; cnt++) {

    // Get reference to struct.
    memcpy_P (&btn, &radiobuttons[cnt], sizeof btn);

    // Draw the button.
    drawButton(btn, false);
  }

  // Set current screen to Radio.
  setCurrentScreen(SCR_RADIO);
}

void handleMainUI(TS_Point p) {

  // Button reference.
  button btn;

  // Go thru all the buttons, checking if they were pressed.
  for (uint8_t b = 0; b < 4; b++) {

    // Get reference to struct.
    memcpy_P (&btn, &mainbuttons[b], sizeof btn);

    //DEBUG: Serial.print("Point x, y: ("); Serial.print(p.x); Serial.print(", ");
    //DEBUG: Serial.print(p.y); Serial.println(")");
    //DEBUG: Serial.print("Button x, w, y, h: ("); Serial.print(btn.x); Serial.print(", ");
    //DEBUG: Serial.print(btn.y); Serial.print(", ");
    //DEBUG: Serial.print(btn.w); Serial.print(", ");
    //DEBUG: Serial.print(btn.h); Serial.println(")");

    if (((p.x >= btn.x) && (p.x < (int16_t) (btn.x + btn.w)) && (p.y >= btn.y) && (p.y < (int16_t) (btn.y + btn.h)))) {

      //DEBUG: Serial.print("Pressing: "); Serial.println(b);
      drawButton(btn, true);
      lastButton = b;

      // Action based on which button was pressed.
      switch (b) {

        // Phone UI.
        case 0:

          // Show phone UI.
          drawPhoneUI();
          break;

        // Radio UI.
        case 1:

          // Show radio UI.
          drawRadioUI();
          break;
      }
    }
  }
}

void handlePhoneUI(TS_Point p) {

  button btn;
  
  // Go thru all the buttons, checking if they were pressed.
  for (uint8_t b = 0; b < 15; b++) {

    // Get reference to struct.
    memcpy_P (&btn, &phonebuttons[b], sizeof btn);

    // Check to see if we have touched this button.
    if (((p.x >= btn.x) && (p.x < (int16_t) (btn.x + btn.w)) && (p.y >= btn.y) && (p.y < (int16_t) (btn.y + btn.h)))) {

      //Serial.print("Pressing: "); Serial.println(b);
      drawButton(btn, true);
      lastButton = b;

      switch (b) {

        // Pickup/Call button.
        case 0:

          // Check for an incoming call.  Will return true if a call is incoming.
          if (digitalRead(FONA_RI) == LOW) {
            status(F("Answering"));
            fona.pickUp();
          }

          else {
            status(F("Calling"));
            fona.callPhone(textfield);
          }
          break;

        // Clear character
        case 1:
          // Delete a character.
          deleteChar();
          drawTextField();
          break;

        // Hangup phone.
        case 2:
          status(F("Hanging up"));
          fona.hangUp();

          // For now, this also sends things back to the main menu.
          drawMainUI();
          break;
          
        // All other buttons are numbers.
        default:
          appendChar(btn.label[0]);
          fona.playDTMF(btn.label[0]);

          // update the current text field
          drawTextField();
      }
    }

    else {

      // Draw the button.
      drawButton(btn, false);
    }
  }
}

void handleRadioUI(TS_Point p) {

  button btn;
  int8_t currentVol;
  
  // Go thru all the buttons, checking if they were pressed.
  for (uint8_t b = 0; b < 11; b++) {

    // Get reference to struct.
    memcpy_P (&btn, &radiobuttons[b], sizeof btn);

    // Check to see if we have touched this button.
    if (((p.x >= btn.x) && (p.x < (int16_t) (btn.x + btn.w)) && (p.y >= btn.y) && (p.y < (int16_t) (btn.y + btn.h)))) {

      //Serial.print("Pressing: "); Serial.println(b);
      drawButton(btn, true);
      lastButton = b;

      switch (b) {

        // Turn radio on.
        case 0:

          fona.FMradio(true, FONA_HEADSETAUDIO);
          break;

        // Turn radio off.
        case 1:
          
          fona.FMradio(false, FONA_HEADSETAUDIO);
          break;

        // Go back to main menu.
        case 2:
          
          drawMainUI();
          break;

        // Frequency up 1.
        case 3:

          if (tuneRadio(currentStation + 1)) {

          }
          break;

        // Frequency up 10.
        case 4:
          
          if (tuneRadio(currentStation + 10)) {

          }
          break;

        // Frequency up 100.
        case 5:
          
          if (tuneRadio(currentStation + 100)) {

          }
          break;

        // Frequency down 1.
        case 6:
          
          if (tuneRadio(currentStation - 1)) {

          }
          break;
        
        // Frequency down 10.
        case 7:
          
          if (tuneRadio(currentStation - 10)) {

          }
          break;
        
        // Frequency down 100.
        case 8:
          
          if (tuneRadio(currentStation - 100)) {

          }
          break;

        // Volume up.
        case 9:
          
          // Get current volume.
          currentVol = fona.getFMVolume();
          
          // Try to change volume.
          fona.setFMVolume(currentVol++);
          break;

        // Volume down.
        case 10:
          
          // Get current volume.
          currentVol = fona.getFMVolume();

          // Try to change volume.
          fona.setFMVolume(currentVol--);
          break;

        default:
          break;
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("Arduin-o-Phone!"));

  // clear the screen
  tft.begin();

  if (!ts.begin()) {
    Serial.println(F("Couldn't start touchscreen controller"));
    while (1);
  }

  // Create label to show phone number.
  tft.drawRect(TEXT_X, TEXT_Y, TEXT_W, TEXT_H, ILI9341_WHITE);

  Serial.println(F("Touchscreen started"));
  drawMainUI();
  /*
    status(F("Checking for FONA..."));
    // Check FONA is there
    fonaSS.begin(4800);

    // See if the FONA is responding
    if (! fona.begin(fonaSS)) {
      status(F("Couldn't find FONA :("));
      while (1);
    }
    status(F("FONA is OK!"));

    // Check we connect to the network
    while (fona.getNetworkStatus() != 1) {
      status(F("Looking for service..."));
      delay(5000);
    }
    status(F("Connected to network!"));

    // set to external mic & headphone
    fona.setAudio(FONA_EXTAUDIO);
  */
}


void loop(void) {

  // Check to see if the user touched the screen.
  if (ts.touched()) {

    // Get the point where the user touched.
    TS_Point p = ts.getPoint();
    p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

    //DEBUG: Serial.print("("); Serial.print(p.x); Serial.print(", ");
    //DEBUG: Serial.print(p.y); Serial.print(", ");
    //DEBUG: Serial.print(p.z); Serial.println(") ");

    // Check to see which screen is active.
    switch (currentScreen) {

      case SCR_MAIN:

        // Call screen handler.
        handleMainUI(p);
        break;

      case SCR_PHONE:

        // Call screen handler.
        handlePhoneUI(p);
        break;

      case SCR_RADIO:

        // Call screen handler.
        handleRadioUI(p);
        break;

      default:
        break;
    }
  }
/*
    else {
      // When the user lifts their finger, need to redraw the button that was just touched.
      if (phonebuttons[lastButton].isPressed()) {
        phonebuttons[lastButton].press(false);
        phonebuttons[lastButton].drawButton();
      }
    }

  */
  // UI touch debouncing
  delay(100);
}
