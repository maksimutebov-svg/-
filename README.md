# # АвтоГолос — интеллектуальная голосовая система диагностики автомобиля
## Назначение
Система считывает телеметрию с OBD-II, анализирует данные и выдаёт голосовые команды водителю.

## Состав решения
- Arduino UNO R3
- Bluetooth-модуль HC-05
- MP3-плеер DFPlayer Mini
- SD-модуль
- OBD-II адаптер ELM327

## Требования к окружению
- Arduino IDE v2.x
- Библиотеки: SoftwareSerial, SPI, SD, DFRobotDFPlayerMini

## Порядок сборки и запуска
1. Подключить компоненты по схеме. HC 05 в режим Мастер
2. Загрузить скетч в Arduino.
3. Подать питание.
4. Система автоматически подключится к OBD-II и начнёт диагностику.

## Авторы
Команда «ТехноПервые» Утебова Дарья, Палагота Андрей, Кирилин Максим. г. Астрахань
Проект «АвтоГолос» — интеллектуальная голосовая система диагностики авто. Подключается к OBD-II, считывает данные с датчиков, анализирует тренды и предупреждает о поломках голосом. Работает без экрана, безопасно для водителя. Есть мобильное приложение и запись логов на SD-карту. 

Итоговая таблица всех соединений
№	Откуда	Куда	Провод
1	OBD-II	ELM327	Разъём 16-pin
2	ELM327	HC-05	Bluetooth
3	HC-05 TX	Arduino D10	UART
4	HC-05 RX	Arduino D11	UART
5	HC-05 VCC	Arduino 5V	Питание
6	HC-05 GND	Arduino GND	Земля
7	DFPlayer RX	Arduino D3	UART
8	DFPlayer TX	Arduino D2	UART
9	DFPlayer VCC	Arduino 5V	Питание
10	DFPlayer GND	Arduino GND	Земля
11	SD CS	Arduino D4	SPI
12	SD MOSI	Arduino D11	SPI
13	SD MISO	Arduino D12	SPI
14	SD SCK	Arduino D13	SPI
15	SD VCC	Arduino 5V	Питание
16	SD GND	Arduino GND	Земля
17	Динамик +	DFPlayer SPK_1	Аудио
18	Динамик −	DFPlayer SPK_2	Аудио
