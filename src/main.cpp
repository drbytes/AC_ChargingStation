#include <Arduino.h>
#include "PWM.h"

const int CP_OUT = 10;
const int CP_IN = A0;
const int RELAY_PIN = 16;
const int LED_TOP = 6;
const int LED_BOTTOM = 2;
const int LED_MIDDLE_D = 3;
const int LED_MIDDLE_U = 4;

const int FREQUENCY = 1000;
const int PEAK_VOLTAGE_THRESHOLD_A = 970;
const int PEAK_VOLTAGE_THRESHOLD_B = 870;
const int PEAK_VOLTAGE_THRESHOLD_C = 780;
const int DUTY_CYCLE_10A = 42;
const int DUTY_CYCLE_MAX = 255;

const int COUNTER_THRESHOLD = 10;
const int PEAK_VOLTAGE_SAMPLES = 1000;

enum State {
  STATE_A,
  STATE_B,
  STATE_C,
  STATE_F
};

int findPeakVoltage();
void updateLEDs(State state, bool charging);
void updateRelay(State state);
void updatePWM(State state);

int peakVoltage = 0;
State currentState = STATE_F;
bool charging = false;

void setup() {
  Serial.begin(9600);

  InitTimersSafe();
  bool success = SetPinFrequencySafe(CP_OUT, FREQUENCY);
  if (success) {
    pinMode(CP_OUT, OUTPUT);
  }

  pinMode(LED_TOP, OUTPUT);
  pinMode(LED_BOTTOM, OUTPUT);
  pinMode(LED_MIDDLE_D, OUTPUT);
  pinMode(LED_MIDDLE_U, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_TOP, HIGH);
  digitalWrite(LED_BOTTOM, LOW);
}

void loop() {
  static int counter = 0;
  static int counter2 = 0;

  if (++counter > COUNTER_THRESHOLD) {
    counter = 0;
    Serial.println("Counter is at 10");
    Serial.print("Peak Voltage: ");
    Serial.println(peakVoltage);
    Serial.println(currentState);
  }

  if (charging) {
    if (++counter2 > COUNTER_THRESHOLD) {
      counter2 = 0;
      digitalWrite(LED_MIDDLE_D, !digitalRead(LED_MIDDLE_D));
      digitalWrite(LED_MIDDLE_U, !digitalRead(LED_MIDDLE_U));
    }
  }

  peakVoltage = findPeakVoltage();

  if (peakVoltage > PEAK_VOLTAGE_THRESHOLD_A) {
    currentState = STATE_A;
    charging = false;
  } else if (peakVoltage > PEAK_VOLTAGE_THRESHOLD_B) {
    currentState = STATE_B;
    charging = false;
  } else if (peakVoltage > PEAK_VOLTAGE_THRESHOLD_C) {
    currentState = STATE_C;
    charging = true;
  } else {
    currentState = STATE_F;
    charging = false;
  }

  updateLEDs(currentState, charging);
  updateRelay(currentState);
  updatePWM(currentState);
}

int findPeakVoltage() {
  int peakVoltage = 0;
  for (int i = 0; i < PEAK_VOLTAGE_SAMPLES; i++) {
    int currentVoltage = analogRead(CP_IN);
    peakVoltage = max(peakVoltage, currentVoltage);
  }
  return peakVoltage;
}

void updateLEDs(State state, bool charging) {
  switch (state) {
    case STATE_A:
    case STATE_F:
      digitalWrite(LED_BOTTOM, LOW);
      digitalWrite(LED_MIDDLE_D, LOW);
      digitalWrite(LED_MIDDLE_U, LOW);
      break;
    case STATE_B:
    case STATE_C:
      digitalWrite(LED_BOTTOM, HIGH);
      break;
  }
}

void updateRelay(State state) {
  switch (state) {
    case STATE_C:
      digitalWrite(RELAY_PIN, HIGH);
      break;
    default:
      digitalWrite(RELAY_PIN, LOW);
      break;
  }
}

void updatePWM(State state) {
  switch (state) {
    case STATE_B:
    case STATE_C:
      pwmWrite(CP_OUT, DUTY_CYCLE_10A);
      break;
    default:
      pwmWrite(CP_OUT, DUTY_CYCLE_MAX);
      break;
  }
}