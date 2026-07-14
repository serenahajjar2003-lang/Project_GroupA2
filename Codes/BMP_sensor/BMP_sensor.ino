unsigned long lastTrend = 0;
const unsigned long TREND_INTERVAL = 30000;

void setupBMP() {
  if (bmp.begin(0x76) || bmp.begin(0x77)) {
    bmpOk = true;
  }
  lastTrend = millis();
}

void readBMP() {
  if (bmpOk) {
    pressure = bmp.readPressure() / 100.0;
    altitude = bmp.readAltitude(1013.25);
    updatePressureTrend();
    updateWeather();
  }
}

void updatePressureTrend() {
  if (!pressureReady) {
    oldPressure = pressure;
    pressureReady = true;
    return;
  }

  unsigned long now = millis();
  if (now - lastTrend >= TREND_INTERVAL) {
    float difference = pressure - oldPressure;

    if (difference > 0.2) {
      pressureTrend = "Rising";
    } else if (difference < -0.2) {
      pressureTrend = "Falling";
    } else {
      pressureTrend = "Stable";
    }

    oldPressure = pressure;
    lastTrend = now;
  }
}

void updateWeather() {
  if (isnan(filteredTemp) || isnan(filteredHum) || !bmpOk) {
    weather = "Sensor Error";
    return;
  }

  if (filteredHum >= 75 && pressureTrend == "Falling") {
    weather = "Rain Possible";
  } else if (filteredHum >= 80 && pressure < 1008) {
    weather = "Rain Possible";
  } else if (pressureTrend == "Rising" && filteredLight >= OPEN_LIGHT) {
    weather = "Clear";
  } else if (filteredTemp >= 30 && filteredHum < 50) {
    weather = "Hot and Dry";
  } else if (filteredHum >= 70) {
    weather = "Humid";
  } else {
    weather = "Stable";
  }
}