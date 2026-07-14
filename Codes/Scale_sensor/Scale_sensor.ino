void setupScale() {
  scale.begin(HX711_DT, HX711_SCK);
  delay(500);

  if (scale.is_ready()) {
    scaleOk = true;
    scale.set_scale(SCALE_FACTOR);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Remove weight");
    lcd.setCursor(0, 1);
    lcd.print("Taring...");

    delay(2000);
    scale.tare(15);
  }
}

void readScale() {
  if (scale.is_ready()) {
    scaleOk = true;
    weightKg = scale.get_units(5);

    if (weightKg > -0.005 && weightKg < 0.005) {
      weightKg = 0;
    }
  }
}