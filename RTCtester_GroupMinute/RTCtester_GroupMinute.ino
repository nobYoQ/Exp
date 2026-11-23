// 改自实验准备目录中的 RTCtester 例程。适用 Arduino UNO。
#include <Wire.h>
#include <RTClib.h>
#include <TM1637.h>

const unsigned long groupNumber = 51;
const uint8_t displayGroup = groupNumber % 100;  // 四位数码管左侧只容纳两位组号。
const uint8_t clockPin = 10;
const uint8_t dataPin = 9;

RTC_DS1307 rtc;
TM1637 display(clockPin, dataPin);

void driveLow(uint8_t pin) {
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
  delayMicroseconds(10);
}

void releaseLine(uint8_t pin) {
  pinMode(pin, INPUT_PULLUP);
  delayMicroseconds(10);
}

void recoverClockBus() {
  releaseLine(SDA);
  releaseLine(SCL);
  if (digitalRead(SDA) == HIGH || digitalRead(SCL) == LOW) {
    return;
  }

  // SDA 被拉低时，最多发送九个 SCL 脉冲让从设备完成剩余位。
  for (uint8_t pulse = 0; pulse < 9 && digitalRead(SDA) == LOW; ++pulse) {
    driveLow(SCL);
    releaseLine(SCL);
  }
  if (digitalRead(SDA) == HIGH) {
    driveLow(SCL);
    driveLow(SDA);
    releaseLine(SCL);
    releaseLine(SDA);
  }
}

bool displayResponds() {
  // 先检测应答，避免旧版 TM1637 库在数码管断线时等待应答。
  releaseLine(clockPin);
  releaseLine(dataPin);
  if (digitalRead(clockPin) == LOW || digitalRead(dataPin) == LOW) {
    return false;
  }

  driveLow(dataPin);
  driveLow(clockPin);
  uint8_t command = ADDR_AUTO;
  for (uint8_t bit = 0; bit < 8; ++bit) {
    if (command & 1) {
      releaseLine(dataPin);
    } else {
      driveLow(dataPin);
    }
    releaseLine(clockPin);
    driveLow(clockPin);
    command >>= 1;
  }

  releaseLine(dataPin);
  releaseLine(clockPin);
  const bool acknowledged = digitalRead(dataPin) == LOW;
  driveLow(clockPin);
  driveLow(dataPin);
  releaseLine(clockPin);
  releaseLine(dataPin);
  return acknowledged;
}

bool readClock(DateTime &now) {
  Wire.clearWireTimeoutFlag();
  if (!rtc.begin()) {
    return false;
  }
  const bool running = rtc.isrunning();
  if (Wire.getWireTimeoutFlag()) {
    return false;
  }
  if (!running) {
    // 仅在时钟停走时，以编译时间启动 DS1307。
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  now = rtc.now();
  return !Wire.getWireTimeoutFlag() &&
         now.month() >= 1 && now.month() <= 12 && now.isValid();
}

void showMinute(uint8_t minute) {
  // 探测应答后，两条信号线已释放为高电平。恢复库需要的输出模式。
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  int8_t digits[] = {
    static_cast<int8_t>(displayGroup / 10),
    static_cast<int8_t>(displayGroup % 10),
    static_cast<int8_t>(minute / 10),
    static_cast<int8_t>(minute % 10)
  };
  display.point(POINT_ON);
  display.display(digits);
}

void printTwoDigits(uint8_t value) {
  if (value < 10) {
    Serial.print('0');
  }
  Serial.print(value);
}

void setup() {
  Serial.begin(9600);
  recoverClockBus();
  Wire.setWireTimeout(25000, true);
  display.set(BRIGHT_TYPICAL);
}

void loop() {
  DateTime now;
  const bool clockReady = readClock(now);
  const bool displayReady = displayResponds();

  Serial.print(F("Group "));
  Serial.print(groupNumber);
  Serial.print(F(" | "));
  if (clockReady) {
    char timestamp[] = "YYYY-MM-DD hh:mm:ss";
    Serial.print(F("RTC "));
    Serial.print(now.toString(timestamp));
    if (displayReady) {
      showMinute(now.minute());
      Serial.print(F(" | TM1637 "));
      printTwoDigits(displayGroup);
      Serial.print(':');
      printTwoDigits(now.minute());
    }
  } else if (Wire.getWireTimeoutFlag()) {
    Serial.print(F("DS1307 I2C timeout (SDA A4, SCL A5)"));
  } else {
    Serial.print(F("DS1307 missing or invalid time (SDA A4, SCL A5)"));
  }
  if (!displayReady) {
    Serial.print(F(" | TM1637 no ACK (CLK D10, DIO D9)"));
  }
  Serial.println();
  delay(1000);
}
