#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <SPI.h>
#include <SD.h>

// ===== ПОДКЛЮЧЕНИЕ =====
SoftwareSerial dfSerial(2, 3);   // DFPlayer: RX=2, TX=3
DFRobotDFPlayerMini myDFPlayer;

SoftwareSerial btSerial(6, 7);   // HC-05: RX=6, TX=7

const int SD_CS = 10;            // SD-модуль CS

// ===== ПАРАМЕТРЫ ТРЕНДА =====
const int WINDOW_SIZE = 5;        // Скользящее окно из 5 измерений
const unsigned long INTERVAL = 2000; // 2 секунды между измерениями

// Массивы для хранения последних 5 измерений
float tempHistory[WINDOW_SIZE] = {0};
float rpmHistory[WINDOW_SIZE] = {0};
float voltHistory[WINDOW_SIZE] = {0};
int historyIndex = 0;
unsigned long lastMeasureTime = 0;

// ===== НАСТРОЙКА =====
void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println(F("=========================================="));
  Serial.println(F("  OBD-II DIAGNOSTIC SYSTEM (REAL + TREND)"));
  Serial.println(F("=========================================="));

  // --- HC-05 ---
  btSerial.begin(9600);  // ELM327 обычно 38400 или 9600
  delay(500);
  Serial.println(F("[1/3] Bluetooth HC-05: OK"));

  // --- SD-модуль ---
  if (!SD.begin(SD_CS)) {
    Serial.println(F("Ошибка: SD-модуль не найден!"));
    while (true);
  }
  Serial.println(F("[2/3] SD-модуль: OK"));

  // --- DFPlayer ---
  dfSerial.begin(9600);
  delay(2000);
  if (!myDFPlayer.begin(dfSerial)) {
    Serial.println(F("Ошибка: DFPlayer не найден!"));
    while (true);
  }
  Serial.println(F("[3/3] DFPlayer: OK"));
  myDFPlayer.volume(30);
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  delay(1000);

  // --- Подключение к OBD-II ---
  Serial.println();
  Serial.println(F("  Поиск OBD-II адаптера..."));
  connectToOBD();

  // --- Первое измерение ---
  lastMeasureTime = millis();
}

// ===== ОСНОВНОЙ ЦИКЛ =====
void loop() {
  if (millis() - lastMeasureTime >= INTERVAL) {
    lastMeasureTime = millis();
    readDiagnostics();
  }
}

// ===== ПОДКЛЮЧЕНИЕ К OBD-II =====
void connectToOBD() {
  sendOBD("ATZ");   delay(1000);
  sendOBD("ATE0");  delay(300);
  sendOBD("ATL0");  delay(300);
  sendOBD("ATS0");  delay(300);
  sendOBD("ATSP0"); delay(1000);

  Serial.println(F("  Адаптер ELM327 обнаружен."));
  Serial.println(F("  Соединение установлено."));
}

// ===== ДИАГНОСТИКА С ТРЕНДАМИ =====
void readDiagnostics() {
  // --- Считывание параметров ---
  float rpm  = readPID("010C", "41 0C", 2, 4);
  float temp = readPID("0105", "41 05", 1, 1) - 40.0;
  float volt = readPID("0142", "41 42", 2, 1000.0);

  Serial.print(F("RPM: "));   Serial.println(rpm);
  Serial.print(F("Temp: "));  Serial.println(temp);
  Serial.print(F("Volt: "));  Serial.println(volt);

  // --- Сохранение в историю ---
  addMeasurement(temp, rpm, volt);

  // --- Вычисление трендов ---
  float tempTrend = calculateTrend(tempHistory);
  float rpmTrend  = calculateTrend(rpmHistory);
  float voltTrend = calculateTrend(voltHistory);

  Serial.print(F("Trend Temp: ")); Serial.println(tempTrend);
  Serial.print(F("Trend RPM: "));  Serial.println(rpmTrend);
  Serial.print(F("Trend Volt: ")); Serial.println(voltTrend);

  // --- Запись на SD ---
  logData(rpm, temp, volt, tempTrend);

  // --- Проверка трендов (ИИ) ---
  if (tempTrend > 2.0) {
    // Температура растёт быстрее 2°C/сек
    Serial.println(F(">> Внимание! Опасная динамика температуры!"));
    playAudio(6);  // «Внимание! Обнаружена опасная динамика»
    delay(5000);
  }
  else if (rpmTrend > 500.0) {
    // Обороты растут быстрее 500 об/мин/сек
    Serial.println(F(">> Внимание! Резкий рост оборотов!"));
    playAudio(1);  // «Высокие обороты двигателя»
    delay(5000);
  }
  else if (voltTrend < -0.3) {
    // Напряжение падает быстрее 0.3 В/сек
    Serial.println(F(">> Внимание! Низкий заряд батареи!"));
    playAudio(4);  // «Низкий уровень заряда батареи»
    delay(5000);
  }
  else if (temp > 110) {
    Serial.println(F(">> Двигатель перегрет!"));
    playAudio(2);  // «Двигатель перегрет»
    delay(5000);
  }
  else {
    Serial.println(F(">> Все параметры в норме"));
    playAudio(5);  // «Все параметры в норме»
    delay(3000);
  }
}

// ===== ОТПРАВКА И ЧТЕНИЕ OBD-II =====
String sendOBD(const char* cmd) {
  while (btSerial.available()) btSerial.read();
  btSerial.print(cmd);
  btSerial.print("\r");
  delay(500);

  String response = "";
  unsigned long timeout = millis();
  while (millis() - timeout < 2000) {
    while (btSerial.available()) {
      char c = btSerial.read();
      response += c;
      if (c == '>') {
        response.trim();
        return response;
      }
    }
  }
  return "TIMEOUT";
}

// ===== ЧТЕНИЕ PID =====
float readPID(const char* cmd, const char* expected, int bytes, float divisor) {
  String response = sendOBD(cmd);

  int index = response.indexOf(expected);
  if (index == -1) return 0;

  String hex = response.substring(index + strlen(expected) + 1,
                                   index + strlen(expected) + 1 + (bytes * 2) + 1);
  hex.replace(" ", "");

  long value = strtol(hex.c_str(), NULL, 16);
  return value / divisor;
}

// ===== ДОБАВЛЕНИЕ ИЗМЕРЕНИЯ В ИСТОРИЮ =====
void addMeasurement(float temp, float rpm, float volt) {
  tempHistory[historyIndex] = temp;
  rpmHistory[historyIndex]  = rpm;
  voltHistory[historyIndex] = volt;
  historyIndex = (historyIndex + 1) % WINDOW_SIZE;
}

// ===== ВЫЧИСЛЕНИЕ ТРЕНДА (СКОЛЬЗЯЩЕЕ ОКНО) =====
float calculateTrend(float* history) {
  // Разница между последним и первым измерением в окне
  int lastIndex = (historyIndex + WINDOW_SIZE - 1) % WINDOW_SIZE;
  int firstIndex = historyIndex;

  float delta = history[lastIndex] - history[firstIndex];
  float timeSec = (WINDOW_SIZE - 1) * (INTERVAL / 1000.0);

  return delta / timeSec;
}

// ===== ЗАПИСЬ НА SD =====
void logData(float rpm, float temp, float volt, float tempTrend) {
  File logFile = SD.open("log.csv", FILE_WRITE);
  if (logFile) {
    logFile.print(millis());
    logFile.print(";");
    logFile.print(rpm);
    logFile.print(";");
    logFile.print(temp);
    logFile.print(";");
    logFile.print(volt);
    logFile.print(";");
    logFile.println(tempTrend);
    logFile.close();
  }
}

// ===== ВОСПРОИЗВЕДЕНИЕ АУДИО =====
void playAudio(int track) {
  myDFPlayer.play(track);
}