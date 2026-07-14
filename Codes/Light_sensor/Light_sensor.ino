unsigned long lastBright = 0;

void setupLightSensor() {
  lightOk = lightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  lastBright = millis();
}

void setupServo() {
  lineServo.attach(SERVO_PIN);
  lineServo.write(SERVO_CLOSED);
}

void readLightSensor() {
  if (lightOk) {
    lightLux = lightSensor.readLightLevel();

    if (lightLux >= 0) {
      if (filteredLight < 0) {
        filteredLight = lightLux;
      } else {
        filteredLight = 0.4 * filteredLight + 0.6 * lightLux;
      }
    }
  }
  updateBrightTime();
}

void updateBrightTime() {
  unsigned long now = millis();
  if (now - lastBright >= 1000) {
    lastBright = now;
    if (lightOk && filteredLight >= OPEN_LIGHT) {
      brightSeconds++;
    }
  }
}

void controlServo() {
  if (!lightOk || filteredLight < 0) return;

  if (!lineOpen && filteredLight >= OPEN_LIGHT) {
    lineOpen = true;
    lineServo.write(SERVO_OPEN);
  } else if (lineOpen && filteredLight <= CLOSE_LIGHT) {
    lineOpen = false;
    lineServo.write(SERVO_CLOSED);
  }
}