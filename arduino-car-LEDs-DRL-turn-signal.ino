#include <Adafruit_NeoPixel.h>

#define BRIGHTNESS 255
#define DRL_PIN 7 // analog
#define LEFT_TURN_SIGNAL_PIN 5 // analog
#define RIGHT_TURN_SIGNAL_PIN 3 // analog
#define STRIP_LEFT_PIN 5
#define STRIP_RIGHT_PIN 6
#define TURN_SWITCH_PIN 8
#define DRL_SWITCH_PIN 9
#define LEDS 72
#define TURN_ON_INTERVAL 4
#define TURN_OFF_INTERVAL 2
#define INITIALIZATION_INTERVAL 8
#define MIDDLE_PHASE_DURATION 150
#define FIX_INTERVAL 1200
#define BOTH_TURN_SIGNALS_TOLERATION 20
#define ANALOG_MIN_VALUE_DRL 200
#define ANALOG_MIN_VALUE_TURN 100
#define GROUP 4 // number of LEDs animating together
#define INITIALIZATION_GROUP 32 // number of LEDs moving during initialization

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

struct WWA {
  int cool;
  int warm;
  int amber;
};

WWA colorOFF = {0, 0, 0};
WWA orangeON = {0, 0, 255};
WWA orange = orangeON;
WWA whiteDay = colorOFF;
WWA whiteNight = {255, 20, 0};
WWA white = whiteDay;

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

    initializeAnimationUp();
    initializeAnimationDown();
    animateStrip(white, true, true, INITIALIZATION_INTERVAL);
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
  if (DRLOn) {
    white = whiteDay;
    orange = colorOFF;
  } else {
    white = whiteNight;
    orange = orangeON;
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
  animateStrip(orange, leftTurnSignal, rightTurnSignal, TURN_ON_INTERVAL);
}

void middlePhase() {
  delay(MIDDLE_PHASE_DURATION);
}

void turnDownPhase() {
  animateStrip(colorOFF, leftTurnSignal, rightTurnSignal, TURN_OFF_INTERVAL);
}

void endPhase() {
  if (leftTurnSignal) lastLeftFixMillis = millis();
  if (rightTurnSignal) lastRightFixMillis = millis();
  leftTurnSignal = false;
  rightTurnSignal = false;
}

void initializeAnimationUp() {
  for (int led = LEDS + 2 * INITIALIZATION_GROUP - 1; led >= -GROUP; led -= GROUP) {
    setPixelGroupColor(led);
  }
}

void initializeAnimationDown() {
  for (int led = 0; led < LEDS + 2 * INITIALIZATION_GROUP + GROUP; led += GROUP) {
    setPixelGroupColor(led);
  }
}

// used for initialization, could have color, delay and left/right strip parameters added for general use
void setPixelGroupColor(int led) {
  setStripColor(colorOFF, true, true, false);
  for (int groupLed = 0; groupLed < INITIALIZATION_GROUP; groupLed++) {
    setStripPixelColor(white, led - groupLed, true, true, false);
  }
  refreshStrip(true, true);
  delay(INITIALIZATION_INTERVAL);
}

void animateStrip(WWA color, bool changeLeft, bool changeRight, int delayTime) {
  for (int led = LEDS - 1; led >= GROUP - 1; led -= GROUP) {
    for (int singleLed = GROUP - 1; singleLed >= 0; singleLed--) {
      setStripPixelColor(color, led - singleLed, changeLeft, changeRight, false);
    }
    refreshStrip(changeLeft, changeRight);
    delay(delayTime);
  }
}

void setStripPixelColor(WWA color, int led, bool changeLeft, bool changeRight) {
  setStripPixelColor(color, led, changeLeft, changeRight, true);
}

void setStripPixelColor(WWA color, int led, bool changeLeft, bool changeRight, bool refresh) {
  if (led < 0 || led >= LEDS) return;
  if (changeLeft) {
    stripLeft.setPixelColor(led, color.amber, color.cool, color.warm);
  }
  if (changeRight) {
    stripRight.setPixelColor(led, color.amber, color.cool, color.warm);
  }
  if (refresh) refreshStrip(changeLeft, changeRight);
}

void setStripColor(WWA color, bool changeLeft, bool changeRight) {
  setStripColor(color, changeLeft, changeRight, true);
}

void setStripColor(WWA color, bool changeLeft, bool changeRight, bool refresh) {
  for (int led = 0; led < LEDS; led++) {
    setStripPixelColor(color, led, changeLeft, changeRight, false);
  }
  if (refresh) refreshStrip(changeLeft, changeRight);
}

void refreshStrip(bool changeLeft, bool changeRight) {
  if (changeLeft) stripLeft.show();
  if (changeRight) stripRight.show();
}
