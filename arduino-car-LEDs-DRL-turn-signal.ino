#include <Adafruit_NeoPixel.h>

#define BRIGHTNESS 255
#define DRL_PIN 7 // analog
#define LEFT_TURN_SIGNAL_PIN 5 // analog
#define RIGHT_TURN_SIGNAL_PIN 3 // analog
#define STRIP_LEFT_PIN 5
#define STRIP_RIGHT_PIN 6
#define TURN_SWITCH_PIN 8
#define DRL_SWITCH_PIN 9
#define LEDS 30
#define TURN_ON_INTERVAL 4
#define TURN_OFF_INTERVAL 2
#define INITIALIZATION_INTERVAL 15
#define MIDDLE_PHASE_DURATION 150
#define FIX_INTERVAL 1200
#define BOTH_TURN_SIGNALS_TOLERATION 20
#define ANALOG_MIN_VALUE_DRL 200
#define ANALOG_MIN_VALUE_TURN 100

Adafruit_NeoPixel stripLeft = Adafruit_NeoPixel(LEDS, STRIP_LEFT_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripRight = Adafruit_NeoPixel(LEDS, STRIP_RIGHT_PIN, NEO_GRB + NEO_KHZ800);

bool initialized = false;
bool DRLOn = true;
bool leftTurnSignal = false;
bool rightTurnSignal = false;

bool DRLSwitchOn = true;
bool turnSwitchOn = true;
bool turnEnabled = true;

unsigned long lastLeftFixMillis = 0;
unsigned long lastRightFixMillis = 0;

unsigned long lastTurnSignalOnMillis = 0;

struct RGB {
  int r;
  int g;
  int b;
};

RGB orange = {255, 79, 0};
RGB whiteDay = {255, 200, 255};
RGB whiteNight = {255, 160, 90};
RGB colorOFF = {0, 0, 0};
RGB white = whiteDay;

void setup() {
  stripLeft.setBrightness(BRIGHTNESS);
  stripLeft.begin();
  stripLeft.show();

  stripRight.setBrightness(BRIGHTNESS);
  stripRight.begin();
  stripRight.show();
  //Serial.begin(9600);
}

void loop() {
  readSwitchesState();
  checkDRL();
  initialize();
  fixDRL();
  readTurnSignalInputs();

  if (turnEnabled && (leftTurnSignal || rightTurnSignal)) {
    turnUpPhase();
    middlePhase();
    turnDownPhase();
    endPhase();
  }
}

void initialize() {
  if (initialized == false) {
    initialized = true;
    for (int led = LEDS - 1; led >= 0; led--) {
      setStripPixelColor(white, led, true, true);
      delay(INITIALIZATION_INTERVAL);
    }
  }
}

void readSwitchesState() {
  bool lastTurnSwitchOn = turnSwitchOn;
  bool lastDRLSwitchOn = DRLSwitchOn;
  // TODO revert to non invert
  turnSwitchOn = !digitalRead(TURN_SWITCH_PIN);
  turnEnabled = turnSwitchOn;
  // TODO revert to non invert
  DRLSwitchOn = !digitalRead(DRL_SWITCH_PIN);

  if (lastTurnSwitchOn != turnEnabled) {
    // empty
  }

  if (lastDRLSwitchOn != DRLSwitchOn) {
    if (DRLSwitchOn) {
      // empty
    } else {
      white = colorOFF;
    }
  }
}

void checkDRL() {
  bool lastDRLOn = DRLOn;
  DRLOn = analogRead(DRL_PIN) > ANALOG_MIN_VALUE_DRL;
  if (lastDRLOn != DRLOn && DRLSwitchOn) {
    //Serial.write("DRL changed\n");
  }
  if (DRLOn) {
    white = whiteDay;
  } else {
    white = whiteNight;
  }
}

void fixDRL() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastLeftFixMillis > FIX_INTERVAL) {
    setStripColor(white, !leftTurnSignal, false);
    lastLeftFixMillis = currentMillis;
  }
  if (currentMillis - lastRightFixMillis > FIX_INTERVAL) {
    setStripColor(white, false, !rightTurnSignal);
    lastRightFixMillis = currentMillis;
  }
}

void readTurnSignalInputs() {
  readTurnSignalInputs(true);
}

void readTurnSignalInputs(bool doubleCheck) {
  bool lastLeftTurnSignal = leftTurnSignal;
  bool lastRightTurnSignal = rightTurnSignal;

  bool leftTurnSignalInput = analogRead(LEFT_TURN_SIGNAL_PIN) > ANALOG_MIN_VALUE_TURN;
  bool rightTurnSignalInput = analogRead(RIGHT_TURN_SIGNAL_PIN) > ANALOG_MIN_VALUE_TURN;

  leftTurnSignal = leftTurnSignalInput;
  rightTurnSignal = rightTurnSignalInput;

  if (doubleCheck) {
    delay(BOTH_TURN_SIGNALS_TOLERATION);
    readTurnSignalInputs(false);
  }
}

void turnUpPhase() {
  for (int led = LEDS - 1; led >= 0; led--) {
    setStripPixelColor(orange, led, leftTurnSignal, rightTurnSignal);
    delay(TURN_ON_INTERVAL);
  }
}

void middlePhase() {
  delay(MIDDLE_PHASE_DURATION);
}

void turnDownPhase() {
  for (int led = LEDS - 1; led >= 0; led--) {
    setStripPixelColor(colorOFF, led, leftTurnSignal, rightTurnSignal);
    delay(TURN_OFF_INTERVAL);
  }
}

void endPhase() {
  if (leftTurnSignal) lastLeftFixMillis = millis();
  if (rightTurnSignal) lastRightFixMillis = millis();
  leftTurnSignal = false;
  rightTurnSignal = false;
}

void setStripPixelColor(RGB color, int led, bool changeLeft, bool changeRight) {
  setStripPixelColor(color, led, changeLeft, changeRight, true);
}

void setStripPixelColor(RGB color, int led, bool changeLeft, bool changeRight, bool refresh) {
  if (changeLeft) {
    stripLeft.setPixelColor(led, color.r, color.g, color.b);
  }
  if (changeRight) {
    stripRight.setPixelColor(led, color.r, color.g, color.b);
  }
  if (refresh) refreshStrip(changeLeft, changeRight);
}

void setStripColor(RGB color, bool changeLeft, bool changeRight) {
  for (int led = 0; led < LEDS; led++) {
    setStripPixelColor(color, led, changeLeft, changeRight, false);
  }
  refreshStrip(changeLeft, changeRight);
}

void refreshStrip(bool changeLeft, bool changeRight) {
  if (changeLeft) stripLeft.show();
  if (changeRight) stripRight.show();
}
