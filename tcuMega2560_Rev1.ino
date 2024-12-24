/*
  ===============================================================
  Transmission Control Unit (TCU) - Open Source Project
  ===============================================================
  
  Author: Mostafa Khalaf Nezhad
  Email: bestmosi@gmail.com

  ---------------------------------------------------------------
  Required Libraries:
  ---------------------------------------------------------------
  
  1. **Adafruit_GFX**
     - **Description:** A versatile graphics library for drawing shapes, text, and images on displays.
     - **Installation:**
       a. Open the Arduino IDE.
       b. Navigate to `Sketch` > `Include Library` > `Manage Libraries...`
       c. In the Library Manager, search for "Adafruit GFX".
       d. Click the `Install` button next to **Adafruit GFX Library** by Adafruit.

  2. **MCUFRIEND_kbv**
     - **Description:** Library for interfacing with various TFT LCD displays.
     - **Installation:**
       a. Open the Arduino IDE.
       b. Navigate to `Sketch` > `Include Library` > `Manage Libraries...`
       c. In the Library Manager, search for "MCUFRIEND_kbv".
       d. Click the `Install` button next to **MCUFRIEND_kbv** by Kai Engel.

  3. **Encoder**
     - **Description:** Library for reading rotary encoders, useful for user input controls.
     - **Installation:**
       a. Open the Arduino IDE.
       b. Navigate to `Sketch` > `Include Library` > `Manage Libraries...`
       c. In the Library Manager, search for "Encoder".
       d. Click the `Install` button next to **Encoder** by Paul Stoffregen.

  4. **EEPROM**
     - **Description:** Library for reading and writing to the Arduino's EEPROM (Electrically Erasable Programmable Read-Only Memory).
     - **Installation:**
       - **Note:** The EEPROM library comes pre-installed with the Arduino IDE. No additional installation is required.
  
  ---------------------------------------------------------------
  Alternative Installation Method (Manual):
  ---------------------------------------------------------------
  
  If you prefer to install the libraries manually or need a specific version, follow these steps:

  1. **Download the Libraries:**
     - **Adafruit_GFX:** [GitHub Repository](https://github.com/adafruit/Adafruit-GFX-Library)
     - **MCUFRIEND_kbv:** [GitHub Repository](https://github.com/prenticedavid/MCUFRIEND_kbv)
     - **Encoder:** [GitHub Repository](https://github.com/PaulStoffregen/Encoder)
  
  2. **Install the Libraries:**
     a. Download the ZIP file of each library from their respective GitHub repositories.
     b. Open the Arduino IDE.
     c. Navigate to `Sketch` > `Include Library` > `Add .ZIP Library...`
     d. Select the downloaded ZIP file and click `Choose`.
     e. Repeat the process for each library.

  ---------------------------------------------------------------
  Additional Resources:
  ---------------------------------------------------------------
  
  - **Arduino Library Manager Guide:** [Arduino Official Documentation](https://www.arduino.cc/en/Guide/Libraries)
  - **Adafruit GFX Documentation:** [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
  - **MCUFRIEND_kbv Documentation:** [MCUFRIEND_kbv GitHub](https://github.com/prenticedavid/MCUFRIEND_kbv)
  - **Encoder Library Documentation:** [Encoder GitHub](https://github.com/PaulStoffregen/Encoder)
  - **EEPROM Library Documentation:** [Arduino EEPROM](https://www.arduino.cc/en/Reference/EEPROM)
  
  ===============================================================
*/


#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <Encoder.h>
#include <EEPROM.h> 

// Define Colors
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF

// Define long press duration in milliseconds
#define LONG_PRESS_DURATION 1000

// EEPROM Address Definitions
#define EEPROM_CURRENT_PAGE 0
#define EEPROM_CURRENT_GEAR 1
#define EEPROM_SELECTED_CHECKBOX_START 2
#define EEPROM_OUTPUTS_START 7

// Initialize display
MCUFRIEND_kbv tft;

// Define rotary encoder pins
#define CLK 22
#define DT 23
#define SW 24

// Define gear change buttons pins
#define BUTTON_PLUS 33
#define BUTTON_MINUS 34

// Initialize rotary encoder
Encoder myEnc(CLK, DT);

// Define output pins 
const int outputPins[8] = {25, 26, 27, 28, 29, 30, 31, 32};

// Variables
int currentPage = 1;                 
int currentGear = 1;                 
int selectedCheckbox[5] = {0, 0, 0, 0, 0}; 
bool buttonPressed = false;          
bool buttonHeld = false;             
unsigned long buttonPressTime = 0;   
bool outputs[5][8] = {false};        

// Variables for gear change buttons
bool plusButtonPressed = false;
bool minusButtonPressed = false;

// Debounce timers
unsigned long lastPressPlus = 0;
unsigned long lastPressMinus = 0;
const unsigned long debounceDelay = 200;

// Message handling
bool showMessage = false;
String messageText = "";
unsigned long messageStartTime = 0;
const unsigned long messageDuration = 1000; // 1 second

// Function Prototypes
void loadSettings();
void saveSettings();
void drawPage(int page);
void drawGearIndicator();
void drawConfigurationPage(int page);
void drawCheckbox(int page, int checkboxIndex);
void updateGearNumber();
void updateCheckbox(int page, int checkboxIndex);
void toggleSelectedCheckbox();
void switchPage();
void handleRotaryEncoderButton();
void handleRotaryEncoderRotation();
void handleGearButtons();
void navigateRotation(bool clockwise);
void incrementGear();
void decrementGear();
void displayMessage(String msg);
void applyGearOutputs(); // Newly added function

// Setup function
void setup() {
  Serial.begin(9600);

  // Setup output pins (Pages 2-6 for Gears 1-5)
  for (int i = 0; i < 8; i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], LOW);
  }

  // Setup rotary encoder switch pin
  pinMode(SW, INPUT_PULLUP);

  // Setup gear change buttons
  pinMode(BUTTON_PLUS, INPUT_PULLUP);
  pinMode(BUTTON_MINUS, INPUT_PULLUP);

  // Initialize display
  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(1);
  tft.fillScreen(BLACK);

  // Load settings from EEPROM
  loadSettings();

  // Draw the loaded/current page
  drawPage(currentPage);
  
  // Apply outputs for the current gear
  applyGearOutputs();
}

// Main loop function
void loop() {
  // Handle rotary encoder button presses (short and long press)
  handleRotaryEncoderButton();

  // Handle rotary encoder rotation for page navigation and checkbox selection
  handleRotaryEncoderRotation();

  // Handle gear up/down buttons
  handleGearButtons();

  // Handle message display timing
  if (showMessage) {
    if (millis() - messageStartTime >= messageDuration) {
      showMessage = false;
      // Clear the message area
      tft.fillRect(50, 200, 220, 30, BLACK);
    }
  }

  // Small delay to prevent rapid toggling
  delay(10);
}

// Function to load settings from EEPROM
void loadSettings() {
  // Load currentPage
  currentPage = EEPROM.read(EEPROM_CURRENT_PAGE);
  if (currentPage < 1 || currentPage > 6) {
    currentPage = 1; // Default to Page 1 if invalid
  }

  // Load currentGear
  currentGear = EEPROM.read(EEPROM_CURRENT_GEAR);
  if (currentGear < 1 || currentGear > 5) {
    currentGear = 1; // Default to Gear 1 if invalid
  }

  // Load selectedCheckbox for each gear (Pages 2-6)
  for (int i = 0; i < 5; i++) {
    selectedCheckbox[i] = EEPROM.read(EEPROM_SELECTED_CHECKBOX_START + i);
    if (selectedCheckbox[i] < 0 || selectedCheckbox[i] > 7) {
      selectedCheckbox[i] = 0; // Default to first checkbox if invalid
    }
  }

  // Load outputs for each gear (Pages 2-6)
  for (int page = 0; page < 5; page++) {
    for (int out = 0; out < 8; out++) {
      outputs[page][out] = EEPROM.read(EEPROM_OUTPUTS_START + (page * 8) + out);
    }
  }

  // Apply outputs for the current gear
  applyGearOutputs();

  Serial.println("Settings Loaded from EEPROM.");
}

// Function to save current settings to EEPROM
void saveSettings() {
  // Save currentPage
  EEPROM.update(EEPROM_CURRENT_PAGE, currentPage);

  // Save currentGear
  EEPROM.update(EEPROM_CURRENT_GEAR, currentGear);

  // Save selectedCheckbox for each gear (Pages 2-6)
  for (int i = 0; i < 5; i++) {
    EEPROM.update(EEPROM_SELECTED_CHECKBOX_START + i, selectedCheckbox[i]);
  }

  // Save outputs for each gear (Pages 2-6)
  for (int page = 0; page < 5; page++) {
    for (int out = 0; out < 8; out++) {
      EEPROM.update(EEPROM_OUTPUTS_START + (page * 8) + out, outputs[page][out]);
    }
  }

  Serial.println("Settings Saved to EEPROM.");
}

// Function to draw the current page
void drawPage(int page) {
  tft.fillScreen(BLACK); // Clear the screen

  if (page == 1) {
    drawGearIndicator();
  } else if (page >= 2 && page <= 6) {
    drawConfigurationPage(page);
  }
}

// Function to draw the Gear Indicator (Page 1)
void drawGearIndicator() {
  // Static Elements
  tft.setTextColor(CYAN);
  tft.setTextSize(3);
  tft.setCursor(340, 10);
  tft.print("Gear");

  // Dynamic Elements
  tft.setTextSize(20);
  tft.setTextColor(GREEN);
  tft.setCursor(300, 80);
  tft.print(currentGear); // Display current gear number
}

// Function to update only the gear number on Page 1
void updateGearNumber() {
  tft.setTextSize(20);
  tft.setTextColor(GREEN);
  tft.fillRect(300, 80, 200, 150, BLACK); // Clear previous gear number
  tft.setCursor(300, 80);
  tft.print(currentGear); // Display updated gear number
}

// Function to draw Solenoid Configuration Pages (Pages 2-6)
void drawConfigurationPage(int page) {
  int gearIndex = page - 2; // Pages 2-6 correspond to Gears 1-5

  // Static Elements
  tft.setTextColor(CYAN);
  tft.setTextSize(3);
  tft.setCursor(50, 20);
  tft.print("Configure Gear ");
  tft.print(gearIndex + 1); // Display gear number

  // Draw 8 checkboxes for the current gear
  for (int i = 0; i < 8; i++) {
    drawCheckbox(page, i);
  }
}

// Function to draw a single checkbox
void drawCheckbox(int page, int checkboxIndex) {
  int gearIndex = page - 2; // Pages 2-6 correspond to Gears 1-5
  int x = 50;
  int y = 60 + checkboxIndex * 30;

  // Clear the entire area where the checkbox (including border) is located
  tft.fillRect(x - 2, y - 2, 24, 24, BLACK);

  // Draw checkbox outline
  tft.drawRect(x, y, 20, 20, WHITE);

  // Fill checkbox if the output is ON
  if (outputs[gearIndex][checkboxIndex]) {
    tft.fillRect(x + 2, y + 2, 16, 16, GREEN);
  } else {
    tft.fillRect(x + 2, y + 2, 16, 16, BLACK); // Clear if OFF
  }

  // Highlight the selected checkbox
  if (checkboxIndex == selectedCheckbox[gearIndex]) {
    tft.drawRect(x - 2, y - 2, 24, 24, RED); // Add a red border
  }

  // Label the checkbox
  tft.setCursor(x + 30, y + 2);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.print("Solenoid ");
  tft.print(checkboxIndex + 1);
}

// Function to update a specific checkbox (used for partial updates)
void updateCheckbox(int page, int checkboxIndex) {
  drawCheckbox(page, checkboxIndex);
}

// Function to toggle the selected checkbox
void toggleSelectedCheckbox() {
  // Ensure we're on a configuration page
  if (currentPage < 2 || currentPage > 6) return;

  int gearIndex = currentPage - 2; // Pages 2-6 correspond to Gears 1-5
  int checkboxIndex = selectedCheckbox[gearIndex];

  // Toggle the output state of the selected checkbox
  outputs[gearIndex][checkboxIndex] = !outputs[gearIndex][checkboxIndex];

  // Update the actual output pin
  digitalWrite(outputPins[checkboxIndex], outputs[gearIndex][checkboxIndex] ? HIGH : LOW);

  // Redraw the affected checkbox
  updateCheckbox(currentPage, checkboxIndex);

  // Debugging information
  Serial.println("Gear " + String(currentGear) + " - Gear " + String(gearIndex + 1) +
                 " - Solenoid " + String(checkboxIndex + 1) + ": " + 
                 (outputs[gearIndex][checkboxIndex] ? "ON" : "OFF"));

  // Save the updated settings to EEPROM
  saveSettings();
}

// Function to switch to the next page
void switchPage() {
  currentPage++;
  if (currentPage > 6) {
    currentPage = 1; // Wrap around to Page 1
  }
  drawPage(currentPage);
  Serial.println("Switched to Page " + String(currentPage));

  // Save the updated page to EEPROM
  saveSettings();
}

// Function to handle rotary encoder button presses (toggle or switch page)
void handleRotaryEncoderButton() {
  // Read the current state of the rotary encoder button
  bool isPressed = (digitalRead(SW) == LOW);
  
  if (isPressed) {
    if (!buttonPressed) {
      // Button has just been pressed
      buttonPressed = true;
      buttonPressTime = millis(); // Record the time when button was pressed
    } else {
      // Button is being held down
      if (!buttonHeld && (millis() - buttonPressTime >= LONG_PRESS_DURATION)) {
        buttonHeld = true; // Mark that a long press has been detected
        switchPage();      // Execute page switch
      }
    }
  } else {
    if (buttonPressed) {
      if (!buttonHeld && (millis() - buttonPressTime < LONG_PRESS_DURATION)) {
        // Button was released before long press duration: short press
        if (currentPage >= 2 && currentPage <= 6) { // Only toggle on configuration pages
          toggleSelectedCheckbox();
        }
      }
      // Reset button state
      buttonPressed = false;
      buttonHeld = false;
    }
  }
}

// Function to handle rotary encoder rotation for navigating pages and selecting checkboxes
void handleRotaryEncoderRotation() {
  long newPosition = myEnc.read() / 4; // Adjust sensitivity if needed
  static long lastPosition = -1;

  if (newPosition != lastPosition) {
    // Determine rotation direction based on change in position
    if (newPosition > lastPosition) {
      // Rotated clockwise
      navigateRotation(true);
    } else {
      // Rotated counter-clockwise
      navigateRotation(false);
    }
    lastPosition = newPosition;
  }
}

// Function to navigate rotation direction with partial updates
void navigateRotation(bool clockwise) {
  if (currentPage == 1) {
    // On Gear Indicator page, no rotation action
    return;
  }

  // On configuration pages (2-6), navigate between checkboxes
  int gearIndex = currentPage - 2; // Pages 2-6 correspond to Gears 1-5
  int oldCheckbox = selectedCheckbox[gearIndex];

  if (clockwise) {
    selectedCheckbox[gearIndex]++;
    if (selectedCheckbox[gearIndex] > 7) selectedCheckbox[gearIndex] = 0;
  } else {
    selectedCheckbox[gearIndex]--;
    if (selectedCheckbox[gearIndex] < 0) selectedCheckbox[gearIndex] = 7;
  }

  int newCheckbox = selectedCheckbox[gearIndex];

  // Update only the affected checkboxes
  updateCheckbox(currentPage, oldCheckbox); // Unhighlight old
  updateCheckbox(currentPage, newCheckbox); // Highlight new

  Serial.println("Gear " + String(currentGear) + " - Page " + String(currentPage) +
                 " - Selected Output: " + String(selectedCheckbox[gearIndex] + 1));
}

// Function to handle gear up/down buttons with non-blocking debounce
void handleGearButtons() {
  unsigned long currentMillis = millis();

  // Handle gear up button (+)
  if (digitalRead(BUTTON_PLUS) == LOW && !plusButtonPressed && (currentMillis - lastPressPlus > debounceDelay)) {
    plusButtonPressed = true;
    lastPressPlus = currentMillis;
    incrementGear();
  }

  if (digitalRead(BUTTON_PLUS) == HIGH) {
    plusButtonPressed = false;
  }

  // Handle gear down button (-)
  if (digitalRead(BUTTON_MINUS) == LOW && !minusButtonPressed && (currentMillis - lastPressMinus > debounceDelay)) {
    minusButtonPressed = true;
    lastPressMinus = currentMillis;
    decrementGear();
  }

  if (digitalRead(BUTTON_MINUS) == HIGH) {
    minusButtonPressed = false;
  }
}

// Function to increment gear with boundary checks and partial LCD update
void incrementGear() {
  if (currentGear < 5) {
    currentGear++;
    Serial.println("Gear Up: Current Gear = " + String(currentGear));
    saveSettings();

    // Apply the output pattern for the new gear
    applyGearOutputs();

    // If currently on Page 1, update only the gear number
    if (currentPage == 1) {
      updateGearNumber();
    }
  } else {
    // Provide feedback that gear cannot be increased further
    Serial.println("Gear Up: Already at maximum gear (5).");
    displayMessage("Max Gear Reached");
  }
}

// Function to decrement gear with boundary checks and partial LCD update
void decrementGear() {
  if (currentGear > 1) {
    currentGear--;
    Serial.println("Gear Down: Current Gear = " + String(currentGear));
    saveSettings();

    // Apply the output pattern for the new gear
    applyGearOutputs();

    // If currently on Page 1, update only the gear number
    if (currentPage == 1) {
      updateGearNumber();
    }
  } else {
    // Provide feedback that gear cannot be decreased further
    Serial.println("Gear Down: Already at minimum gear (1).");
    displayMessage("Min Gear Reached");
  }
}

// Function to display a temporary message on the LCD
void displayMessage(String msg) {
  showMessage = true;
  messageText = msg;
  messageStartTime = millis();

  // Display the message
  tft.setTextColor(RED);
  tft.setTextSize(2);
  tft.setCursor(50, 200);
  tft.print(msg);
}

// *** Newly Added Function ***
// Function to apply outputs based on the current gear
void applyGearOutputs() {
  int gearIndex = currentGear - 1; // gears 1-5 correspond to indices 0-4
  for (int i = 0; i < 8; i++) {
    digitalWrite(outputPins[i], outputs[gearIndex][i] ? HIGH : LOW);
  }

  // Optional: Provide debug information
  Serial.print("Applied outputs for Gear ");
  Serial.println(currentGear);
}

// Good luck with your TCU project and happy coding! Im so ghavii 
