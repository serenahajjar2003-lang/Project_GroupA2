void showScreen() {
  lcd.clear();

  switch (screen) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("Temp:");
      if (isnan(filteredTemp)) {
        lcd.print(" Error");
      } else {
        lcd.print(filteredTemp, 1);
        lcd.print((char)223);
        lcd.print("C");
      }

      lcd.setCursor(0, 1);
      if (!isnan(filteredTemp) && filteredTemp > TEMP_LIMIT) {
        lcd.print("HIGH TEMP!");
      } else {
        lcd.print("Hum:");
        if (isnan(filteredHum)) {
          lcd.print(" Error");
        } else {
          lcd.print(filteredHum, 1);
          lcd.print("%");
        }
      }
      break;

    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Light:");
      if (!lightOk || filteredLight < 0) {
        lcd.print(" Error");
      } else {
        lcd.print(filteredLight, 0);
        lcd.print(" lux");
      }

      lcd.setCursor(0, 1);
      if (!lightOk) {
        lcd.print("Sensor Error");
      } else if (lineOpen) {
        lcd.print("Line: OPEN");
      } else {
        lcd.print("Line: CLOSED");
      }
      break;

    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Air Quality:");
      lcd.setCursor(0, 1);
      lcd.print(airQuality);
      if (airQuality < 400) lcd.print(" Good");
      else if (airQuality < 600) lcd.print(" Medium");
      else lcd.print(" Poor");
      break;

    case 3:
      if (bmpOk) {
        lcd.setCursor(0, 0);
        lcd.print("P:");
        lcd.print(pressure, 1);
        lcd.print("hPa");

        lcd.setCursor(0, 1);
        lcd.print("Alt:");
        lcd.print(altitude, 1);
        lcd.print("m");
      } else {
        lcd.setCursor(0, 0);
        lcd.print("BMP280 Error");
        lcd.setCursor(0, 1);
        lcd.print("Check wiring");
      }
      break;

    case 4:
      lcd.setCursor(0, 0);
      lcd.print("Weight:");
      lcd.setCursor(0, 1);
      if (!scaleOk) {
        lcd.print("HX711 Error");
      } else {
        lcd.print(weightKg, 3);
        lcd.print(" kg");
      }
      break;

    case 5:
      lcd.setCursor(0, 0);
      lcd.print("Flow:");
      lcd.print(filteredFlow, 2);
      lcd.print("L/m");

      lcd.setCursor(0, 1);
      lcd.print("Total:");
      lcd.print(totalLiters, 2);
      lcd.print("L");
      break;

    case 6:
      if (!tempHistoryReady) {
        lcd.setCursor(0, 0);
        lcd.print("Temp History");
        lcd.setCursor(0, 1);
        lcd.print("Waiting...");
      } else {
        lcd.setCursor(0, 0);
        lcd.print("Min:");
        lcd.print(minTemp, 1);
        lcd.print("C");

        lcd.setCursor(0, 1);
        lcd.print("Max:");
        lcd.print(maxTemp, 1);
        lcd.print("C");
      }
      break;

    case 7:
      lcd.setCursor(0, 0);
      lcd.print("System Status");
      lcd.setCursor(0, 1);
      if (lightOk && bmpOk && scaleOk && !isnan(filteredTemp) && !isnan(filteredHum)) {
        lcd.print("All Sensors OK");
      } else {
        lcd.print("Check Sensors");
      }
      break;

    case 8:
      lcd.setCursor(0, 0);
      lcd.print("Forecast:");
      lcd.setCursor(0, 1);
      lcd.print(weather);
      break;

    case 9:
      lcd.setCursor(0, 0);
      lcd.print("Pressure Trend");
      lcd.setCursor(0, 1);
      lcd.print(pressureTrend);
      break;

    case 10:
      lcd.setCursor(0, 0);
      lcd.print("Bright Time:");
      lcd.setCursor(0, 1);
      lcd.print(brightSeconds / 60.0, 1);
      lcd.print(" min");
      break;
  }
}

void printCsv() {
  Serial.print(millis() / 60000.0, 2);
  Serial.print(",");

  if (isnan(filteredTemp)) Serial.print("ERROR");
  else Serial.print(filteredTemp, 1);
  Serial.print(",");

  if (isnan(filteredHum)) Serial.print("ERROR");
  else Serial.print(filteredHum, 1);
  Serial.print(",");

  if (filteredLight < 0) Serial.print("ERROR");
  else Serial.print(filteredLight, 1);
  Serial.print(",");

  Serial.print(airQuality);
  Serial.print(",");

  if (bmpOk) Serial.print(pressure, 1);
  else Serial.print("ERROR");
  Serial.print(",");

  if (bmpOk) Serial.print(altitude, 1);
  else Serial.print("ERROR");
  Serial.print(",");

  if (scaleOk) Serial.print(weightKg, 3);
  else Serial.print("ERROR");
  Serial.print(",");

  Serial.print(filteredFlow, 2);
  Serial.print(",");
  Serial.print(totalLiters, 2);
  Serial.print(",");
  Serial.print(weather);
  Serial.print(",");
  Serial.print(pressureTrend);
  Serial.print(",");
  Serial.print(brightSeconds / 60.0, 1);
  Serial.print(",");

  if (lineOpen) Serial.println("OPEN");
  else Serial.println("CLOSED");
}