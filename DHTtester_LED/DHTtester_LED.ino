// 改自 Adafruit 的 DHTtester 例程（ladyada，公有领域）。
#include <DHT.h>

const uint8_t sensorPin = 2;
const uint8_t ledPin = LED_BUILTIN;
const char groupNumber[] = "51";  // 更换组号时修改此处。

const unsigned long sampleIntervalMs = 2000;
const float slowTemperatureC = 15.0;
const float fastTemperatureC = 35.0;
const unsigned long slowHalfPeriodMs = 1000;
const unsigned long fastHalfPeriodMs = 150;

DHT dht(sensorPin, DHT11);

unsigned long lastSampleMs;
unsigned long lastLedToggleMs;
unsigned long ledHalfPeriodMs = 0;
bool ledOn = false;

void printGroupNumber() {
  Serial.print(F("Group "));
  Serial.print(groupNumber);
  Serial.print(F(" | "));
}

unsigned long halfPeriodForTemperature(float temperatureC) {
  // 15–35 °C 内线性调速，超出范围时使用对应端点的速度。
  const float limitedTemperature = constrain(temperatureC, slowTemperatureC, fastTemperatureC);
  const float fraction = (limitedTemperature - slowTemperatureC) /
                         (fastTemperatureC - slowTemperatureC);
  return static_cast<unsigned long>(
      slowHalfPeriodMs - fraction * (slowHalfPeriodMs - fastHalfPeriodMs) + 0.5f);
}

void updateLed(unsigned long now) {
  // 第一次成功读取前保持熄灭。一次读失败时维持上一次的闪烁速度。
  if (ledHalfPeriodMs == 0 || now - lastLedToggleMs < ledHalfPeriodMs) {
    return;
  }
  lastLedToggleMs = now;
  ledOn = !ledOn;
  digitalWrite(ledPin, ledOn ? HIGH : LOW);
}

void readAndPrintSensor() {
  const float humidity = dht.readHumidity();
  const float temperatureC = dht.readTemperature();

  printGroupNumber();
  if (isnan(humidity) || isnan(temperatureC)) {
    Serial.println(F("DHT11 read failed. Check 5V, GND and DATA on D2."));
    return;
  }

  ledHalfPeriodMs = halfPeriodForTemperature(temperatureC);
  const float temperatureF = dht.convertCtoF(temperatureC);
  const float heatIndexC = dht.computeHeatIndex(temperatureC, humidity, false);
  const float heatIndexF = dht.convertCtoF(heatIndexC);

  Serial.print(F("Humidity: "));
  Serial.print(humidity, 1);
  Serial.print(F("%  Temperature: "));
  Serial.print(temperatureC, 1);
  Serial.print(F(" C / "));
  Serial.print(temperatureF, 1);
  Serial.print(F(" F  Heat index: "));
  Serial.print(heatIndexC, 1);
  Serial.print(F(" C / "));
  Serial.print(heatIndexF, 1);
  Serial.print(F(" F  LED half-period: "));
  Serial.print(ledHalfPeriodMs);
  Serial.println(F(" ms"));
}

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  Serial.begin(9600);
  dht.begin();
  lastSampleMs = millis();

  printGroupNumber();
  Serial.println(F("DHT11 + LED ready (DATA D2, 9600 baud)"));
}

void loop() {
  const unsigned long now = millis();
  updateLed(now);
  if (now - lastSampleMs >= sampleIntervalMs) {
    readAndPrintSensor();
    lastSampleMs = millis();
  }
}
