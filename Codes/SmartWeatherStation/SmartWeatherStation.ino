#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <BH1750.h>
#include <Servo.h>
#include <Adafruit_BMP280.h>
#include <HX711.h>
#include "config.h"

#define DHT_PIN 2
#define DHT_TYPE DHT22
#define BUTTON_PIN 7
#define SERVO_PIN 8
#define HX711_SCK 10
#define HX711_DT 11
#define BUZZER_PIN 12
#define FLOW_PIN 18
#define MQ135_PIN A1

const float TEMP_LIMIT = 30.0;
const float OPEN_LIGHT = 105.0;
const float CLOSE_LIGHT = 95.0;

const int SERVO_OPEN = 90;
const int SERVO_CLOSED = 0;


const unsigned long LCD_INTERVAL = 1000;
const unsigned long CSV_INTERVAL = 60000;
const unsigned long TREND_INTERVAL = 30000;
const unsigned long BUTTON_DELAY = 250;

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHT_TYPE);
BH1750 lightSensor;
Adafruit_BMP280 bmp;
Servo lineServo;
HX711 scale;

bool lightOk = false;
bool bmpOk = false;
bool scaleOk = false;
bool lineOpen = false;
bool tempHistoryReady = false;
bool pressureReady = false;
bool buzzerOn = false;

float temp = NAN;
float hum = NAN;
float lightLux = -1;
float pressure = 0;
float altitude = 0;
float weightKg = 0;
float flowRate = 0;
float totalLiters = 0;

float filteredTemp = NAN;
float filteredHum = NAN;
float filteredLight = -1;
float filteredFlow = 0;

float minTemp = 0;
float maxTemp = 0;
float oldPressure = 0;

int airQuality = 0;
int screen = 0;

const int LAST_SCREEN = 10;

String weather = "Stable";
String pressureTrend = "Stable";

unsigned long lastLcd = 0;
unsigned long lastCsv = 0;
unsigned long lastTrend = 0;
unsigned long lastButton = 0;
unsigned long lastFlow = 0;
unsigned long lastBright = 0;
unsigned long lastBuzzer = 0;

unsigned long brightSeconds = 0;

bool previousButton = HIGH;

volatile unsigned long flowPulses = 0;

void countFlowPulse()
{
  flowPulses++;
}

void setup()
{
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FLOW_PIN, INPUT_PULLUP);

  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin();

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Smart Weather");
  lcd.setCursor(0, 1);
  lcd.print("Station");

  delay(2000);

  dht.begin();

  lightOk = lightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  if (bmp.begin(0x76) || bmp.begin(0x77))
    bmpOk = true;

  lineServo.attach(SERVO_PIN);
  lineServo.write(SERVO_CLOSED);

  scale.begin(HX711_DT, HX711_SCK);
  delay(500);

  if (scale.is_ready())
  {
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

  attachInterrupt(
    digitalPinToInterrupt(FLOW_PIN),
    countFlowPulse,
    FALLING
  );

  lastFlow = millis();
  lastCsv = millis();
  lastTrend = millis();
  lastBright = millis();

  Serial.println(
    "Time_min,Temp_C,Hum_percent,Light_lux,"
    "AirQuality,Pressure_hPa,Altitude_m,"
    "Weight_kg,Flow_L_min,Total_L,"
    "Weather,PressureTrend,BrightTime_min,"
    "Clothesline"
  );

  lcd.clear();
}

void loop()
{
  readSensors();
  updateFlow();
  filterValues();
  updateTemperatureHistory();
  updatePressureTrend();
  updateWeather();
  updateBrightTime();
  controlServo();
  controlBuzzer();
  readButton();

  if (millis() - lastLcd >= LCD_INTERVAL)
  {
    lastLcd = millis();
    showScreen();
  }

  if (millis() - lastCsv >= CSV_INTERVAL)
  {
    lastCsv = millis();
    printCsv();
  }
}

void readSensors()
{
  temp = dht.readTemperature();
  hum = dht.readHumidity();

  airQuality = analogRead(MQ135_PIN);

  if (lightOk)
    lightLux = lightSensor.readLightLevel();

  if (bmpOk)
  {
    pressure = bmp.readPressure() / 100.0;
    altitude = bmp.readAltitude(1013.25);
  }

  if (scale.is_ready())
  {
    scaleOk = true;
    weightKg = scale.get_units(5);

    if (weightKg > -0.005 && weightKg < 0.005)
      weightKg = 0;
  }
}

void filterValues()
{
  if (!isnan(temp) && temp > 5 && temp < 60)
  {
    if (isnan(filteredTemp))
      filteredTemp = temp;
    else
      filteredTemp = 0.8 * filteredTemp + 0.2 * temp;
  }

  if (!isnan(hum) && hum >= 0 && hum <= 100)
  {
    if (isnan(filteredHum))
      filteredHum = hum;
    else
      filteredHum = 0.8 * filteredHum + 0.2 * hum;
  }

  if (lightLux >= 0)
  {
    if (filteredLight < 0)
      filteredLight = lightLux;
    else
      filteredLight = 0.4 * filteredLight + 0.6 * lightLux;
  }

  filteredFlow = 0.8 * filteredFlow + 0.2 * flowRate;
}

void updateTemperatureHistory()
{
  if (isnan(filteredTemp))
    return;

  if (!tempHistoryReady)
  {
    minTemp = filteredTemp;
    maxTemp = filteredTemp;
    tempHistoryReady = true;
  }
  else
  {
    if (filteredTemp < minTemp)
      minTemp = filteredTemp;

    if (filteredTemp > maxTemp)
      maxTemp = filteredTemp;
  }
}

void updateFlow()
{
  unsigned long now = millis();

  if (now - lastFlow >= 1000)
  {
    unsigned long elapsed = now - lastFlow;

    noInterrupts();

    unsigned long pulses = flowPulses;
    flowPulses = 0;

    interrupts();

    float frequency = pulses * 1000.0 / elapsed;

    flowRate = frequency / FLOW_FACTOR;
    totalLiters += flowRate * elapsed / 60000.0;

    lastFlow = now;
  }
}

void controlServo()
{
  if (!lightOk || filteredLight < 0)
    return;

  if (!lineOpen && filteredLight >= OPEN_LIGHT)
  {
    lineOpen = true;
    lineServo.write(SERVO_OPEN);
  }
  else if (lineOpen && filteredLight <= CLOSE_LIGHT)
  {
    lineOpen = false;
    lineServo.write(SERVO_CLOSED);
  }
}

void controlBuzzer()
{
  if (isnan(filteredTemp) || filteredTemp <= TEMP_LIMIT)
  {
    buzzerOn = false;
    digitalWrite(BUZZER_PIN, LOW);
    return;
  }

  unsigned long now = millis();

  if (buzzerOn && now - lastBuzzer >= 300)
  {
    buzzerOn = false;
    digitalWrite(BUZZER_PIN, LOW);
    lastBuzzer = now;
  }
  else if (!buzzerOn && now - lastBuzzer >= 700)
  {
    buzzerOn = true;
    digitalWrite(BUZZER_PIN, HIGH);
    lastBuzzer = now;
  }
}

void updatePressureTrend()
{
  if (!bmpOk)
  {
    pressureTrend = "No BMP";
    return;
  }

  if (!pressureReady)
  {
    oldPressure = pressure;
    pressureReady = true;
    return;
  }

  unsigned long now = millis();

  if (now - lastTrend >= TREND_INTERVAL)
  {
    float difference = pressure - oldPressure;

    if (difference > 0.2)
      pressureTrend = "Rising";
    else if (difference < -0.2)
      pressureTrend = "Falling";
    else
      pressureTrend = "Stable";

    oldPressure = pressure;
    lastTrend = now;
  }
}

void updateWeather()
{
  if (isnan(filteredTemp) || isnan(filteredHum) || !bmpOk)
  {
    weather = "Sensor Error";
    return;
  }

  if (filteredHum >= 75 && pressureTrend == "Falling")
    weather = "Rain Possible";
  else if (filteredHum >= 80 && pressure < 1008)
    weather = "Rain Possible";
  else if (pressureTrend == "Rising" &&
           filteredLight >= OPEN_LIGHT)
    weather = "Clear";
  else if (filteredTemp >= 30 && filteredHum < 50)
    weather = "Hot and Dry";
  else if (filteredHum >= 70)
    weather = "Humid";
  else
    weather = "Stable";
}

void updateBrightTime()
{
  unsigned long now = millis();

  if (now - lastBright >= 1000)
  {
    lastBright = now;

    if (lightOk && filteredLight >= OPEN_LIGHT)
      brightSeconds++;
  }
}

void readButton()
{
  bool buttonState = digitalRead(BUTTON_PIN);

  if (
    previousButton == HIGH &&
    buttonState == LOW &&
    millis() - lastButton >= BUTTON_DELAY
  )
  {
    screen++;

    if (screen > LAST_SCREEN)
      screen = 0;

    lastButton = millis();
    lcd.clear();
  }

  previousButton = buttonState;
}

void showScreen()
{
  lcd.clear();

  switch (screen)
  {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("Temp:");

      if (isnan(filteredTemp))
        lcd.print(" Error");
      else
      {
        lcd.print(filteredTemp, 1);
        lcd.print((char)223);
        lcd.print("C");
      }

      lcd.setCursor(0, 1);

      if (!isnan(filteredTemp) && filteredTemp > TEMP_LIMIT)
      {
        lcd.print("HIGH TEMP!");
      }
      else
      {
        lcd.print("Hum:");

        if (isnan(filteredHum))
          lcd.print(" Error");
        else
        {
          lcd.print(filteredHum, 1);
          lcd.print("%");
        }
      }
      break;

    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Light:");

      if (!lightOk || filteredLight < 0)
        lcd.print(" Error");
      else
      {
        lcd.print(filteredLight, 0);
        lcd.print(" lux");
      }

      lcd.setCursor(0, 1);

      if (!lightOk)
        lcd.print("Sensor Error");
      else if (lineOpen)
        lcd.print("Line: OPEN");
      else
        lcd.print("Line: CLOSED");

      break;

    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Air Quality:");

      lcd.setCursor(0, 1);
      lcd.print(airQuality);

      if (airQuality < 400)
        lcd.print(" Good");
      else if (airQuality < 600)
        lcd.print(" Medium");
      else
        lcd.print(" Poor");

      break;

    case 3:
      if (bmpOk)
      {
        lcd.setCursor(0, 0);
        lcd.print("P:");
        lcd.print(pressure, 1);
        lcd.print("hPa");

        lcd.setCursor(0, 1);
        lcd.print("Alt:");
        lcd.print(altitude, 1);
        lcd.print("m");
      }
      else
      {
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

      if (!scaleOk)
        lcd.print("HX711 Error");
      else
      {
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
      if (!tempHistoryReady)
      {
        lcd.setCursor(0, 0);
        lcd.print("Temp History");

        lcd.setCursor(0, 1);
        lcd.print("Waiting...");
      }
      else
      {
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

      if (
        lightOk &&
        bmpOk &&
        scaleOk &&
        !isnan(filteredTemp) &&
        !isnan(filteredHum)
      )
        lcd.print("All Sensors OK");
      else
        lcd.print("Check Sensors");

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

void printCsv()
{
  Serial.print(millis() / 60000.0, 2);
  Serial.print(",");

  if (isnan(filteredTemp))
    Serial.print("ERROR");
  else
    Serial.print(filteredTemp, 1);

  Serial.print(",");

  if (isnan(filteredHum))
    Serial.print("ERROR");
  else
    Serial.print(filteredHum, 1);

  Serial.print(",");

  if (filteredLight < 0)
    Serial.print("ERROR");
  else
    Serial.print(filteredLight, 1);

  Serial.print(",");
  Serial.print(airQuality);
  Serial.print(",");

  if (bmpOk)
    Serial.print(pressure, 1);
  else
    Serial.print("ERROR");

  Serial.print(",");

  if (bmpOk)
    Serial.print(altitude, 1);
  else
    Serial.print("ERROR");

  Serial.print(",");

  if (scaleOk)
    Serial.print(weightKg, 3);
  else
    Serial.print("ERROR");

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

  if (lineOpen)
    Serial.println("OPEN");
  else
    Serial.println("CLOSED");
}