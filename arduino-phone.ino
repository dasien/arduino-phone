
/* Arduino-Phone OS
 * 
 * This sketch lays out a few screens for a phone based on the Adafruit
 * FONA shield, their TFT screen, and their Metro 328 Arduino device.
 * 
 * This version uses the Adafruit capacitive touchscreen rather than the 
 * resisitive one.  If you want to use the resisitive screen, you will need 
 * to swap out the libraries and change the values for mapping touches to screen
 * coordinations (MINX, MAXX, etc) and perhaps the "map" overload used.
 * 
 * I pulled what was needed from the great Adafruit_GFX library as it was lighter
 * weight to just create a struct than an array of button classes from that lib.
 * This helped to get the project to fit in 2k of RAM.
 * 
 * Copyright 2019 Brian Gentry
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
 
//#include <Wire.h>      // this is needed even tho we aren't using it
#include <SPI.h>
#include <SoftwareSerial.h>
#include "Adafruit_FT6206.h" 
#include "Adafruit_ILI9341.h"
#include "Adafruit_FONA.h"

// Pins for FONA use.
#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4
#define FONA_RI 6

// For the Adafruit TFT shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10
#define TFT_BL 5

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 0
#define TS_MINY 0
#define TS_MAXX 240
#define TS_MAXY 320

// Screen identifiers.
#define SCR_MAIN 1
#define SCR_PHONE 2
#define SCR_RADIO 4
#define SCR_SMS 8
#define SCR_STATUS 16

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

// Array size constants.
#define MAIN_BTN_ARRAY_SIZE 4
#define PHONE_BTN_ARRAY_SIZE 15
#define RADIO_BTN_ARRAY_SIZE 11
#define STATUS_BTN_ARRAY_SIZE 2

// Serial communication with FONA device.
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);

// Create objects for interfacing iwth Fona and TFT.
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// This is for the capacitive touch screen.
Adafruit_FT6206 ts = Adafruit_FT6206();
 
// Current radio station.
uint16_t currentStation = 870;

// Tracking the last button pressed.
byte lastButton = 0;
bool redrawButton = false;

// Tracking the screen currently shown.
byte currentScreen = 0;

// The data (phone #) we store in the textfield
char textfield[TEXT_LEN + 1] = "";
uint8_t textfield_i = 0;

// Struct for holding button properties.
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
const button mainbuttons[MAIN_BTN_ARRAY_SIZE] PROGMEM = {
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
  {10, 180, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "- 1", 1},
  {90, 180, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "- 10", 1},
  {170, 180, 60, 30, ILI9341_WHITE, ILI9341_BLUE, ILI9341_WHITE, "- 100", 1},
  {10, 230, 60, 30, ILI9341_WHITE, ILI9341_DARKGREEN, ILI9341_WHITE, "Vol +", 1},
  {170, 230, 60, 30, ILI9341_WHITE, ILI9341_RED, ILI9341_WHITE, "Vol -", 1},
};

const button statusbuttons[2] PROGMEM = {
  {10, 280, 60, 30, ILI9341_WHITE, ILI9341_ORANGE, ILI9341_WHITE, "Reset", 1},
  {170, 280, 60, 30, ILI9341_WHITE, ILI9341_DARKGREEN, ILI9341_WHITE, "Ok", 1},
};

// For software reset.
void (* reset)(void) = 0;

void status(const __FlashStringHelper *msg) {
  
  tft.fillRect(STATUS_X, STATUS_Y, 240, 8, ILI9341_BLACK);
  tft.setCursor(STATUS_X, STATUS_Y);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.print(msg);
}

/*
*   Removes a character from a textfield
*/
void deleteChar() {
  textfield[textfield_i] = 0;
  
  if (textfield > 0) {
    textfield_i--;
    textfield[textfield_i] = ' ';
  }
}

/*
*   Appends a character to a textfield
*   
*   @param c - The character to be added
*/
void appendChar(char c) {
  
  if (textfield_i < TEXT_LEN) {
    textfield[textfield_i] = c;
    textfield_i++;
    textfield[textfield_i] = 0; // zero terminate
  }
}


/*
*   Clears all text from textfield
*/
void clearTextField() {

  textfield_i = 0;
  memset(textfield, 0, sizeof(textfield));
}


/*
*   Draws a textfield on the screen
*/
void drawTextField() {

  Serial.println(textfield);
  tft.setCursor(TEXT_X + 2, TEXT_Y + 10);
  tft.setTextColor(TEXT_TCOLOR, ILI9341_BLACK);
  tft.setTextSize(TEXT_TSIZE);
  tft.print(textfield);
}

/*
*   Draws a button on the screen.  
*   
*   @param btn - The button struct with size, color, etc
*   @param pressed - Whether the button should be drawn in 'pressed' state
*/
void drawButton(button btn, bool pressed) {

  // Temp variables for colors.
  uint16_t fill, outline, text;

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

/*
*   Keeps track of the current screen being shown.  This is used by the main loop to determine  
*   which screen handler to invoke in response to a touch.
*   
*   @param screen - The screen flag indentifier
*/
void setCurrentScreen(byte screen) {

  // Check to see if we are already on that screen.
  if (currentScreen != screen) {

    // Replace current screen.
    currentScreen = (~currentScreen) & screen;
  }
}

/*
*   Changes the radio station.  
*   
*   @param station - The station frequency to tune to, as an integer
*/
void tuneRadio(uint16_t station)
{
  // Check to see if we are in allowable range.
  if (station >= FM_FREQ_MIN && station <= FM_FREQ_MAX)
  {
    //  Update current station.
    currentStation = station;
    
    // Try to change station.
    fona.tuneFMradio(currentStation);

    // Clear the current text field.
    memset(textfield, 0, sizeof(textfield));

    // Set text.
    sprintf(textfield,"%i", currentStation);

    // Display text.
    drawTextField();
  }
}

/*
*   Draws screens which contain buttons in a generic way.  
*   
*   @param screen - The screen to set as current
*   @param maxcnt - The number of buttons which will be drawn (should be the length 
*                   of the array of buttons for this screen)
*   @param buttons - The button array to be used in drawing
*/
void drawButtonUI(byte screen, uint8_t maxcnt, button buttons[]) {

  // Button reference.
  button btn;

  // Set the currently active screen to main.
  setCurrentScreen(screen);

  // Clear the UI
  tft.fillScreen(ILI9341_BLACK);

  // Create label to show text.
  tft.drawRect(TEXT_X, TEXT_Y, TEXT_W, TEXT_H, ILI9341_WHITE);

  for (uint8_t cnt = 0; cnt < maxcnt; cnt++) {

    // Get reference to struct.
    memcpy_P (&btn, &buttons[cnt], sizeof btn);

    // Draw the button.
    drawButton(btn, false);
  }
}

void drawStatusUI() {

  // battery
  uint16_t vbat;
  if (! fona.getBattPercent(&vbat)) {
    Serial.println(F("Failed to read Batt"));
  } else {
    Serial.print(F("VPct = ")); Serial.print(vbat); Serial.println(F("%"));
  }

  // network
  uint8_t n = fona.getNetworkStatus();
  Serial.print(F("Network status "));
  Serial.print(n);
  Serial.print(F(": "));
  if (n == 0) Serial.println(F("Not registered"));
  if (n == 1) Serial.println(F("Registered (home)"));
  if (n == 2) Serial.println(F("Not registered (searching)"));
  if (n == 3) Serial.println(F("Denied"));
  if (n == 4) Serial.println(F("Unknown"));
  if (n == 5) Serial.println(F("Registered roaming"));

  // SMS
  int8_t smsnum = fona.getNumSMS();
  if (smsnum < 0) {
    Serial.println(F("Could not read # SMS"));
  } 
  
  else {
    Serial.print(smsnum);
    Serial.println(F(" SMS's on SIM card!"));
  } 

  // Time
  char buffer[23];
  fona.getTime(buffer, 23);  // make sure replybuffer is at least 23 bytes!
  Serial.print(F("Time = ")); Serial.println(buffer);

  uint8_t v = fona.getVolume();
  Serial.print("Current Volume = ");
  Serial.println(v);

}

/*
*   Handles all screen touches for the main screen.  
*   
*   @param p - The point struct for the screen location which was touched
*/
void handleMainUI(TS_Point p) {

  // Button reference.
  button btn;

  // Go thru all the buttons, checking if they were pressed.
  for (uint8_t b = 0; b < MAIN_BTN_ARRAY_SIZE; b++) {

    // Get reference to struct.
    memcpy_P (&btn, &mainbuttons[b], sizeof btn);

    // Check to see if we have touched this button.
    if (((p.x >= btn.x) && (p.x < (int16_t) (btn.x + btn.w)) && (p.y >= btn.y) && (p.y < (int16_t) (btn.y + btn.h)))) {

      // Draw the button as pressed.
      drawButton(btn, true);

      // Hang on to this button id.
      lastButton = b;

      // Action based on which button was pressed.
      switch (b) {

        // Phone UI.
        case 0:

          // Show phone UI.
          drawButtonUI(SCR_PHONE, PHONE_BTN_ARRAY_SIZE, phonebuttons);
          break;

        // Radio UI.
        case 1:

          // Show radio UI.
          drawButtonUI(SCR_RADIO, RADIO_BTN_ARRAY_SIZE, radiobuttons);
          break;

        case 2:
          drawStatusUI();
      }

      // Since we found the button, stop looping.
      b = MAIN_BTN_ARRAY_SIZE;
    }
  }
}

/*
*   Handles all screen touches for the phone screen.  
*   
*   @param p - The point struct for the screen location which was touched
*/
void handlePhoneUI(TS_Point p) {

  button btn;
  button lastBtn;
  
  // Go thru all the buttons, checking if they were pressed.
  for (uint8_t b = 0; b < PHONE_BTN_ARRAY_SIZE; b++) {

    // Get reference to struct.
    memcpy_P (&btn, &phonebuttons[b], sizeof btn);

    // Check to see if we have touched this button.
    if (((p.x >= btn.x) && (p.x < (int16_t) (btn.x + btn.w)) && (p.y >= btn.y) && (p.y < (int16_t) (btn.y + btn.h)))) {

      // Get reference to last button.
      memcpy_P (&lastBtn, &phonebuttons[lastButton], sizeof lastBtn);

      // Undraw last button (in case it was after the current button in the list).
      drawButton(lastBtn, false);

      // Draw the button as pressed.
      drawButton(btn, true);

      // Hang on to this button id.
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

          // Clear text field.
          clearTextField();
          
          // For now, this also sends things back to the main menu.
          drawButtonUI(SCR_MAIN, MAIN_BTN_ARRAY_SIZE, mainbuttons);
          break;
          
        // All other buttons are numbers.
        default:
          appendChar(btn.label[0]);
          fona.playDTMF(btn.label[0]);

          // update the current text field
          drawTextField();
          break;
      }

      // Since we found the button, stop looping.
      b = PHONE_BTN_ARRAY_SIZE;
    }

    else {

      // Draw the button.
      drawButton(btn, false);
    }
  }
}

/*
*   Handles all screen touches for the radio screen.  
*   
*   @param p - The point struct for the screen location which was touched
*/
void handleRadioUI(TS_Point p) {

  button btn;
  button lastBtn;
  int8_t currentVol;
  
  // Go thru all the buttons, checking if they were pressed.
  for (uint8_t b = 0; b < RADIO_BTN_ARRAY_SIZE; b++) {

    // Get reference to struct.
    memcpy_P (&btn, &radiobuttons[b], sizeof btn);

    // Check to see if we have touched this button.
    if (((p.x >= btn.x) && (p.x < (int16_t) (btn.x + btn.w)) && (p.y >= btn.y) && (p.y < (int16_t) (btn.y + btn.h)))) {

      // Get reference to last button.
      memcpy_P (&lastBtn, &radiobuttons[lastButton], sizeof lastBtn);

      // Undraw last button (in case it was after the current button in the list).
      drawButton(lastBtn, false);

      // Draw the button as pressed.
      drawButton(btn, true);

      // Hang on to this button id.
      lastButton = b;

      // Check to see what to do.
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
          drawButtonUI(SCR_MAIN, MAIN_BTN_ARRAY_SIZE, mainbuttons);
          break;

        // Frequency up 1.
        case 3:

          tuneRadio(currentStation + 1);
          break;

        // Frequency up 10.
        case 4:
          
          tuneRadio(currentStation + 10);
          break;

        // Frequency up 100.
        case 5:
          
          tuneRadio(currentStation + 100);
          break;

        // Frequency down 1.
        case 6:
          
          tuneRadio(currentStation - 1);
          break;
        
        // Frequency down 10.
        case 7:
          
          tuneRadio(currentStation - 10);
          break;
        
        // Frequency down 100.
        case 8:
          
          tuneRadio(currentStation - 100);
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

      // Since we found the button, stop looping.
      b = RADIO_BTN_ARRAY_SIZE;
    }

    else {

      // Draw the button.
      drawButton(btn, false);
    }
  }
}

/*
*   Handles all screen touches for the status screen.  
*   
*   @param p - The point struct for the screen location which was touched
*/
void handleStatusUI(TS_Point p) {
  
}

/*
*   Sets up phone hardware and draws main screen 
*/
void setup() {
  Serial.begin(9600);
  Serial.println(F("Arduin-o-Phone!"));

  // clear the screen
  tft.begin();

  if (!ts.begin()) {
    Serial.println(F("Couldn't start touchscreen controller"));
    while (1);
  }

  Serial.println(F("Touchscreen started"));

  drawButtonUI(SCR_MAIN, MAIN_BTN_ARRAY_SIZE, mainbuttons);
          
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

  // Set audio to headset.
  fona.setAudio(FONA_HEADSETAUDIO);
}

/*
*   The main loop registers and delegates the handling of screen touches to other methods.  
*   It also draws buttons after the user lifts their finger.
*/
void loop(void) {
  
  // Check to see if the user touched the screen.
  if (ts.touched()) {

    // Flag that we need to redraw button when user stops touching screen.
    redrawButton = true;
    
    // Get the point where the user touched.
    TS_Point p = ts.getPoint();

    // Map touch screen coordinates.
    p.x = map(p.x, TS_MINX, TS_MAXX, TS_MAXX, TS_MINX);
    p.y = map(p.y, TS_MINY, TS_MAXY, TS_MAXY, TS_MINY);

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

      case SCR_SMS:
        break;

      case SCR_STATUS:

        // Call screen handler.
        handleStatusUI(p);
        break;

      default:
        break;
    }
  }

  // When the user lifts their finger, need to redraw the button that was just touched.
  else {

    button lastBtn;

    // Check to see that we have not already drawn the button 'untouched'. (prevent flicker)
    if (redrawButton) {
      
      switch (currentScreen) {
  
        case SCR_MAIN:
          
          // Get reference to last button.
          memcpy_P (&lastBtn, &mainbuttons[lastButton], sizeof lastBtn);
          break;
  
        case SCR_PHONE:
  
          // Get reference to last button.
          memcpy_P (&lastBtn, &phonebuttons[lastButton], sizeof lastBtn);
          break;
  
        case SCR_RADIO:
  
          // Get reference to last button.
          memcpy_P (&lastBtn, &radiobuttons[lastButton], sizeof lastBtn);
          break;
  
        case SCR_SMS:
          break;
  
        case SCR_STATUS:
  
          // Get reference to last button.
          memcpy_P (&lastBtn, &statusbuttons[lastButton], sizeof lastBtn);
          break;
  
        default:
          break;
        }

        // Undraw last button (in case it was after the current button in the list).
        drawButton(lastBtn, false);
        redrawButton = false;
      }
    }
  // UI touch debouncing
  delay(100);
}
