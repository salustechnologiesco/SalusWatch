// Libs

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>

// =======================
// OLED SETTINGS
// =======================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // init display

// =======================
// MANUAL START TIME
// =======================
//
// Since this version has no WiFi (setup rn) the ESP does not know the real time by itself. Maybe we should look into the NISt Hz

const int START_HOUR = 14;      // 24-hour format but could change later
const int START_MINUTE = 30;
const int START_SECOND = 0;

const int START_MONTH = 5;
const int START_DAY = 23;
const int START_YEAR = 2026;

// Used as ref point for the software clock
unsigned long bootMillis = 0;

// Controls how often the OLED redraws the display 
const unsigned long DISPLAY_UPDATE_MS = 500;
unsigned long lastDisplayUpdate = 0;

// =======================
// CLOCK DATA STRUCT
// =======================
struct ClockTime {
  int hour;
  int minute;
  int second;
  int month;
  int day;
  int year;
};

// =======================
// DATE / TIME HELPERS
// =======================
bool isLeapYear(int year) {   // Dont rlly need but good to have for final prod
  if (year % 400 == 0) return true;
  if (year % 100 == 0) return false;
  return year % 4 == 0;
}

int daysInMonth(int month, int year) {   // Make sure days in month are correct 
  switch (month) {
    case 1: return 31;
    case 2: return isLeapYear(year) ? 29 : 28;
    case 3: return 31;
    case 4: return 30;
    case 5: return 31;
    case 6: return 30;
    case 7: return 31;
    case 8: return 31;
    case 9: return 30;
    case 10: return 31;
    case 11: return 30;
    case 12: return 31;
    default: return 30;
  }
}

String getMonthName(int month) { // Assign num to 3 letter month
  switch (month) {
    case 1: return "Jan";
    case 2: return "Feb";
    case 3: return "Mar";
    case 4: return "Apr";
    case 5: return "May";
    case 6: return "Jun";
    case 7: return "Jul";
    case 8: return "Aug";
    case 9: return "Sep";
    case 10: return "Oct";
    case 11: return "Nov";
    case 12: return "Dec";
    default: return "---";
  }
}

String twoDigit(int value) {
  if (value < 10) {
    return "0" + String(value);
  }

  return String(value);
}

// Updates the manual clock based on how many seconds have passed since the ESP started running
ClockTime getCurrentTime() {
  ClockTime now;

  unsigned long elapsedSeconds = (millis() - bootMillis) / 1000;

  unsigned long startTotalSeconds =
    (START_HOUR * 3600UL) +
    (START_MINUTE * 60UL) +
    START_SECOND;

  unsigned long totalSeconds = startTotalSeconds + elapsedSeconds;

  unsigned long daysPassed = totalSeconds / 86400UL;
  unsigned long secondsToday = totalSeconds % 86400UL;

  now.hour = secondsToday / 3600UL;
  secondsToday %= 3600UL;

  now.minute = secondsToday / 60UL;
  now.second = secondsToday % 60UL;

  now.month = START_MONTH;
  now.day = START_DAY;
  now.year = START_YEAR;

  // Move the date forward if clock passes midnight
  while (daysPassed > 0) {
    now.day++;

    if (now.day > daysInMonth(now.month, now.year)) {
      now.day = 1;
      now.month++;

      if (now.month > 12) {
        now.month = 1;
        now.year++;
      }
    }

    daysPassed--;
  }

  return now;
}

String get12HourTime(int hour, int minute) {
  int displayHour = hour % 12;

  if (displayHour == 0) {
    displayHour = 12;
  }

  return String(displayHour) + ":" + twoDigit(minute);
}

String getAMPM(int hour) {
  return hour >= 12 ? "PM" : "AM";
}

// =======================
// OLED DRAWING HELPERS
// =======================

void printCenter(const String text, int y, int textSize) {   // Prints in middle
  int16_t x1, y1;
  uint16_t w, h;

  display.setTextSize(textSize);
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);

  display.setCursor((SCREEN_WIDTH - w) / 2, y);
  display.print(text);
}

// Draws tide section (missing actual tide implementation tho)
void drawTideSection() {
  display.drawLine(6, 46, 121, 46, SSD1306_WHITE);

  display.setTextSize(1);

  display.setCursor(6, 50);
  display.print("TIDES");

  display.setCursor(48, 50);
  display.print("Next: --:--");

  display.setCursor(6, 58);
  display.print("Height: --.- ft");

  display.setCursor(88, 58);
  display.print("H/L: --");
}

// Draws the full display
void drawWatchFace() {
  ClockTime now = getCurrentTime();

  String dateText =
    getMonthName(now.month) + " " +
    String(now.day) + ", " +
    String(now.year);

  String clockText = get12HourTime(now.hour, now.minute);
  String ampmText = getAMPM(now.hour);
  String secondsText = ":" + twoDigit(now.second);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Outer watch border
  display.drawRoundRect(0, 0, 128, 64, 6, SSD1306_WHITE);

  // Date line
  printCenter(dateText, 5, 1);

  // Large main time
  display.setTextSize(3);

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(clockText, 0, 0, &x1, &y1, &w, &h);

  int clockX = (SCREEN_WIDTH - w) / 2 - 6;

  if (clockX < 3) {
    clockX = 3;
  }

  display.setCursor(clockX, 20);
  display.print(clockText);

  // AM/PM and secs sit to the right of the main time
  display.setTextSize(1);

  display.setCursor(104, 24);
  display.print(ampmText);

  display.setCursor(104, 36);
  display.print(secondsText);

  // Tide info (not implemented yet tho)
  drawTideSection();

  display.display();
}

// =======================
// SETUP / LOOP
// =======================

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  bootMillis = millis();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("OLED started");
  display.println("Offline watch mode");
  display.println("Tide section ready");

  display.display();
  delay(1000);
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_MS) {
    drawWatchFace();
    lastDisplayUpdate = currentMillis;
  }
}
