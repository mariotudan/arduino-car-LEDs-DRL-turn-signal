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
#define TAIL 16
#define TAIL_REVERSE 15
#define TURN_INTERVAL 5
#define INITIALIZATION_INTERVAL 15
#define MIDDLE_PHASE_DURATION 44 // times TURN_INTERVAL
#define FIX_INTERVAL 500

#define BLINKING_OFF_TIME 600
#define BLINKING_EMP_FIX_TIME 100 // EMP or voltage spike fix
#define BOTH_TURN_SIGNALS_TOLERATION 400
//#define BLINKING_OFF_TIME 1
//#define BLINKING_EMP_FIX_TIME 1 // EMP or voltage spike fix
//#define BOTH_TURN_SIGNALS_TOLERATION 0

#define TURN_SIGNAL_DELAY 0
#define ANALOG_MIN_VALUE_DRL 200
#define ANALOG_MIN_VALUE_TURN 200

Adafruit_NeoPixel stripLeft = Adafruit_NeoPixel(LEDS, STRIP_LEFT_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripRight = Adafruit_NeoPixel(LEDS, STRIP_RIGHT_PIN, NEO_GRB + NEO_KHZ800);

bool initialized = false;
bool DRLOn = true;
bool leftTurnSignal = false;
bool rightTurnSignal = false;

bool left = false;
bool right = false;
bool lastLeft = false;
bool lastRight = false;

unsigned long lastMillis = 0;
unsigned long lastFixMillis = 0;
bool tryFix = false;

unsigned long leftTurnOffMillis = 0;
unsigned long rightTurnOffMillis = 0;
bool leftTurnOffStarted = false;
bool rightTurnOffStarted = false;

unsigned long leftTurnOnMillis = 0;
unsigned long rightTurnOnMillis = 0;
bool leftTurnOnStarted = false;
bool rightTurnOnStarted = false;

int turnUp = 0 - TAIL;
int turnDown = LEDS + TAIL;

bool leftReset = false;
bool rightReset = false;
int resetLED = LEDS - 1;
unsigned long lastResetMillis = 0;

bool DRLSwitchOn = true;
bool turnSwitchOn = true;
bool turnEnabled = true;

struct RGB {
  int r;
  int g;
  int b;
};
RGB orange = {255, 79, 0};
RGB whiteDay = {255, 200, 255};
RGB whiteNight = {255, 160, 90};
RGB white = whiteDay;
RGB loopWhite = white;

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
  initialize();
  checkDRL();
  readTurnSignalInputs();

  unsigned long currentMillis = millis();

  if (leftReset || rightReset) {
    int resetInterval = TURN_INTERVAL;
    if (initialized == false) {
      resetInterval = INITIALIZATION_INTERVAL;
    }
    if (DRLSwitchOn == false || turnEnabled == false) {
      resetInterval = 1;
    }
    if (currentMillis - lastResetMillis >= resetInterval) {
      lastResetMillis = currentMillis;
      //Serial.write("RESETING\n");
      resetPhase();
    }
  } else  if (/*TEMP false &*/ turnEnabled && (leftTurnSignal || rightTurnSignal || turnUp + TAIL > 0 || turnDown - TAIL < LEDS)) {
    if (currentMillis - lastMillis >= TURN_INTERVAL) {
      lastMillis = currentMillis;
      // on phase start
      if (isIdleStateActive()) {
        //Serial.write("TURN PHASE START: ");
        //Serial.print(millis());
        //Serial.write(" ms\n");
        left = leftTurnSignal;
        right = rightTurnSignal;
        loopWhite = white;

        if (lastLeft == false && lastRight == false) {
          //turnUp = 0 - TAIL;
        }
      }
      if (turnUp < LEDS && turnDown - TAIL == LEDS) {
        //Serial.write("TURN UP PHASE\n");
        turnUpPhase();
      }
      if (turnUp == LEDS) {
        //Serial.write("MIDDLE PHASE\n");
        turnUp = 0 - TAIL;
        turnDown = 0 - MIDDLE_PHASE_DURATION;
        left = leftTurnSignal;
        right = rightTurnSignal;
      }
      if (turnDown - TAIL < LEDS) {
        //Serial.write("TURN DOWN PHASE\n");
        turnDownPhase();
      }
      // reset to idle state after turn phase if needed
      if (isIdleStateActive()) {
        //Serial.write("TURN PHASE END:   ");
        //Serial.print(millis());
        //Serial.write(" ms\n");
        readTurnSignalInputs(false);
        turnEnabled = turnSwitchOn;
        leftReset = left && !leftTurnSignal || !turnEnabled;
        rightReset = right && !rightTurnSignal || !turnEnabled;
        lastLeft = left;
        lastRight = right;
        left = leftTurnSignal;
        right = rightTurnSignal;
      }
      tryFix = true;
      lastFixMillis = millis();
    }
  }
  checkForFix();
}

bool isIdleStateActive() {
  return turnUp + TAIL == 0 && turnDown - TAIL == LEDS;
}

void initialize() {
  if (initialized == false) {
    leftReset = true;
    rightReset = true;
  }
}

void readSwitchesState() {
  bool lastTurnSwitchOn = turnSwitchOn;
  bool lastDRLSwitchOn = DRLSwitchOn;
  // TODO revert to non invert
  turnSwitchOn = !digitalRead(TURN_SWITCH_PIN);
  turnEnabled = !isIdleStateActive() || turnSwitchOn;
  // TODO revert to non invert
  DRLSwitchOn = !digitalRead(DRL_SWITCH_PIN);
  if (lastTurnSwitchOn != turnEnabled) {
    tryFix = true;
  }
  if (lastDRLSwitchOn != DRLSwitchOn) {
    if (DRLSwitchOn) {
      DRLOn = !DRLOn;
      checkDRL();
      DRLOn = !DRLOn;
      checkDRL();
    } else {
      white = {0, 0, 0};
      tryFix = true;
    }
  }
}

void checkDRL() {
  bool lastDRLOn = DRLOn;
  DRLOn = analogRead(DRL_PIN) > ANALOG_MIN_VALUE_DRL;
  if (lastDRLOn != DRLOn && DRLSwitchOn) {
    //Serial.write("DRL changed\n");
    if (DRLOn) {
      white = whiteDay;
    } else {
      white = whiteNight;
    }
    tryFix = true;
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

  unsigned long currentMillis = millis();

  if (!leftTurnSignalInput && leftTurnSignal) {
    if (!leftTurnOffStarted) {
      leftTurnOffStarted = true;
      leftTurnOffMillis = millis();
    } else if (currentMillis - leftTurnOffMillis >= BLINKING_OFF_TIME) {
      leftTurnOffStarted = false;
      /*if (leftTurnSignal == true) {
        String text = "";
        text += millis();
        text += ";\n";
        Serial.print(text);
      }*/
      leftTurnSignal = false;
    }
  } else if (leftTurnSignalInput && !leftTurnSignal) {
    if (!leftTurnOnStarted) {
      leftTurnOnStarted = true;
      leftTurnOnMillis = millis();
    }
    else if (currentMillis - leftTurnOnMillis >= BLINKING_EMP_FIX_TIME) {
      leftTurnOnStarted = false;
      /*if (leftTurnSignal == false) {
        String text = "";
        text += millis();
        text += ";";
        Serial.print(text);
      }*/
      leftTurnSignal = true;
    }
  } else {
    leftTurnOffStarted = false;
    leftTurnOnStarted = false;
    //leftTurnSignal = leftTurnSignalInput;
  }

  if (!rightTurnSignalInput && rightTurnSignal) {
    if (!rightTurnOffStarted) {
      rightTurnOffStarted = true;
      rightTurnOffMillis = millis();
    } else if (currentMillis - rightTurnOffMillis >= BLINKING_OFF_TIME) {
      rightTurnOffStarted = false;
      rightTurnSignal = false;
    }
  } else if (rightTurnSignalInput && !rightTurnSignal) {
    if (!rightTurnOnStarted) {
      rightTurnOnStarted = true;
      rightTurnOnMillis = millis();
    }
    else if (currentMillis - rightTurnOnMillis >= BLINKING_EMP_FIX_TIME) {
      rightTurnOnStarted = false;
      rightTurnSignal = true;
    }
  } else {
    rightTurnOffStarted = false;
    rightTurnOnStarted = false;
    //rightTurnSignal = rightTurnSignalInput;
  }

  if (!doubleCheck && (!lastLeftTurnSignal && leftTurnSignal || !lastRightTurnSignal && rightTurnSignal)) {
    delay(TURN_SIGNAL_DELAY);
  }

  if (doubleCheck && (leftTurnSignal || rightTurnSignal) && !lastLeftTurnSignal && !lastRightTurnSignal && isIdleStateActive()) {
    delay(BOTH_TURN_SIGNALS_TOLERATION);
    readTurnSignalInputs(false);
  }
}

void resetPhase() {
  if (resetLED == LEDS - 1) {
    loopWhite = white;
  }
  if (resetLED >= 0) {
    if (leftReset) {
      stripLeft.setPixelColor(resetLED, loopWhite.r, loopWhite.g, loopWhite.b);
      stripLeft.show();
    }
    if (rightReset) {
      stripRight.setPixelColor(resetLED, loopWhite.r, loopWhite.g, loopWhite.b);
      stripRight.show();
    }
    resetLED--;
  } else {
    leftReset = false;
    rightReset = false;
    initialized = true;
    resetLED = LEDS - 1;
  }
}

void turnUpPhase() {
  for (int i = turnUp; i < turnUp + TAIL; i++) {
    int led = LEDS - i - 1;
    if (i < LEDS) {
      int colorDivider = i - turnUp + 1;
      if (left) {
        stripLeft.setPixelColor(led, orange.r / colorDivider, orange.g / colorDivider, orange.b);
      }
      if (right) {
        stripRight.setPixelColor(led, orange.r / colorDivider, orange.g / colorDivider, orange.b);
      }
    }
  }
  stripLeft.show();
  stripRight.show();
  turnUp++;
}

void turnDownPhase() {
  for (int i = turnDown - TAIL_REVERSE; turnDown - i >= 0; i++) {
    if (i >= 0) {
      int led = LEDS - i - 1;
      if (turnDown - i == TAIL_REVERSE) {
        if (left) {
          stripLeft.setPixelColor(led, 0, 0, 0);
        } else {
          stripLeft.setPixelColor(led, loopWhite.r, loopWhite.g, loopWhite.b);
        }
        if (right) {
          stripRight.setPixelColor(led, 0, 0, 0);
        } else {
          stripRight.setPixelColor(led, loopWhite.r, loopWhite.g, loopWhite.b);
        }
      } else {
        int colorDivider = turnDown - i + 1;
        if (left) {
          stripLeft.setPixelColor(led, orange.r / colorDivider, orange.g / colorDivider, orange.b);
        }
        if (right) {
          stripRight.setPixelColor(led, orange.r / colorDivider, orange.g / colorDivider, orange.b);
        }
      }
    }
  }
  stripLeft.show();
  stripRight.show();
  turnDown++;
}

void checkForFix() {
  unsigned long currentFixMillis = millis();
  if (tryFix && currentFixMillis - lastFixMillis >= FIX_INTERVAL) {
    //Serial.write("FIX STARTED\n");
    lastFixMillis = currentFixMillis;
    leftReset = (!leftTurnSignal || !turnEnabled) && isIdleStateActive();
    rightReset = (!rightTurnSignal || !turnEnabled) && isIdleStateActive();
    tryFix = !leftReset && !rightReset;
    //Serial.write("FIX STARTED, tryFix: ");
    //Serial.print(tryFix);
    //Serial.write("\n");
  }
}
