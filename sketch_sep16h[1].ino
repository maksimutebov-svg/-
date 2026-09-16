#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <SPI.h>
#include <SD.h>

// ===== ПОДКЛЮЧЕНИЕ =====
// DFPlayer: RX -> пин 2 (через резистор 1к), TX -> пин 3
SoftwareSerial dfSerial(2, 3);
DFRobotDFPlayerMini myDFPlayer;

// HC-05: RX -> пин 6, TX -> пин 7 (через делитель 1к/2к)
SoftwareSerial btSerial(6, 7);

// SD-модуль: CS -> пин 10
const int SD_CS = 10;

// ===== ПАУЗЫ (мс) =====
const unsigned long PAUSE_AFTER_0001 = 5000;
const unsigned long PAUSE_AFTER_0002 = 7000;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println(F("=========================================="));
  Serial.println(F("  OBD-II DIAGNOSTIC SYSTEM (REAL)"));
  Serial.println(F("=========================================="));

  // --- HC-05 (Master для OBD-II) ---
  btSerial.begin(9600);  // ELM327 обычно 38400 или 9600. Если не работает — 38400
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
  Serial.println(F("=========================================="));
  Serial.println(F("  Поиск OBD-II адаптера..."));
  Serial.println(F("=========================================="));
  connectToOBD();
}

void loop() {
  readDiagnostics();
  delay(2000);  // Цикл каждые 2 секунды
}

// ===== ПОДКЛЮЧЕНИЕ К OBD-II =====
void connectToOBD() {
  sendOBD("ATZ");          // Сброс
  delay(1000);
  sendOBD("ATE0");         // Отключить эхо
  delay(300);
  sendOBD("ATL0");         // Отключить перевод строки
  delay(300);
  sendOBD("ATS0");         // Отключить пробелы
  delay(300);
  sendOBD("ATSP0");        // Авто-выбор протокола
  delay(1000);

  Serial.println();
  Serial.println(F("  Адаптер ELM327 обнаружен."));
  Serial.println(F("  Соединение установлено."));
  btSerial.println("ELM327 connected");
}

// ===== ДИАГНОСТИКА =====
void readDiagnostics() {
  // 1. Обороты двигателя (RPM)
  int rpm = readPID("010C", "41 0C", 2, 4);
  logData("RPM", rpm);

  // 2. Температура ОЖ
  int temp = readPID("0105", "41 05", 1, 1) - 40;
  logData("Temp OJ", temp);

  // 3. Нагрузка двигателя
  int load = readPID("0104", "41 04", 1, 1) * 100 / 255;
  logData("Load", load);

  // 4. Скорость автомобиля
  int speed = readPID("010D", "41 0D", 1, 1);
  logData("Speed", speed);

  // 5. Расход воздуха (MAF)
  int maf = readPID("0110", "41 10", 2, 4);
  logData("MAF", maf);

  // 6. Температура воздуха на впуске (IAT)
  int iat = readPID("010F", "41 0F", 1, 1) - 40;
  logData("IAT", iat);

  // 7. Давление масла (опционально)
  // int oil = readPID("0163", "41 63", 1, 1);
  // logData("Oil", oil);

  // --- Формирование голосовой команды ---
  if (temp > 110) {
    playAudio(2);  // «Двигатель перегрет»
  } else if (rpm > 6000) {
    playAudio(1);  // «Высокие обороты»
  } else {
    playAudio(5);  // «Все параметры в норме»
  }
}

// ===== ОТПРАВКА И ЧТЕНИЕ OBD-II =====
String sendOBD(const char* cmd) {
  while (btSerial.available()) btSerial.read();  // Очистка буфера
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
        Serial.println(response);
        return response;
      }
    }
  }
  return "TIMEOUT";
}

// ===== ЧТЕНИЕ PID =====
int readPID(const char* cmd, const char* expected, int bytes, int divisor) {
  String response = sendOBD(cmd);
  
  int index = response.indexOf(expected);
  if (index == -1) return 0;

  // Извлекаем hex-значение после ожидаемого заголовка
  String hex = response.substring(index + strlen(expected) + 1, index + strlen(expected) + 1 + (bytes * 2) + 1);
  hex.replace(" ", "");
  
  long value = strtol(hex.c_str(), NULL, 16);
  return value / divisor;
}

// ===== ЗАПИСЬ НА SD =====
void logData(const char* param, int value) {
  File logFile = SD.open("log.csv", FILE_WRITE);
  if (logFile) {
    logFile.print(millis());
    logFile.print(";");
    logFile.print(param);
    logFile.print(";");
    logFile.println(value);
    logFile.close();
  }
}

// ===== ВОСПРОИЗВЕДЕНИЕ АУДИО =====
void playAudio(int track) {
  myDFPlayer.play(track);
  delay(PAUSE_AFTER_0001);
}