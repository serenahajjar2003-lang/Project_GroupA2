void setupDHT() {
  dht.begin();
}

void readDHT() {
  temp = dht.readTemperature();
  hum = dht.readHumidity();

  // Filter Temperature
  if (!isnan(temp) && temp > 5 && temp < 60) {
    if (isnan(filteredTemp)) {
      filteredTemp = temp;
    } else {
      filteredTemp = 0.8 * filteredTemp + 0.2 * temp;
    }
  }

  // Filter Humidity
  if (!isnan(hum) && hum >= 0 && hum <= 100) {
    if (isnan(filteredHum)) {
      filteredHum = hum;
    } else {
      filteredHum = 0.8 * filteredHum + 0.2 * hum;
    }
  }

  updateTemperatureHistory();
}

void updateTemperatureHistory() {
  if (isnan(filteredTemp)) return;

  if (!tempHistoryReady) {
    minTemp = filteredTemp;
    maxTemp = filteredTemp;
    tempHistoryReady = true;
  } else {
    if (filteredTemp < minTemp) minTemp = filteredTemp;
    if (filteredTemp > maxTemp) maxTemp = filteredTemp;
  }
}