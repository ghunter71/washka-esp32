# ✅ Прошивка ESP32 Washka готова!

## 📦 Скомпилированные файлы

Все файлы находятся в папке `.pio/build/esp32dev/`:

```
✓ bootloader.bin  - 28 KB   (адрес 0x1000)
✓ partitions.bin  - 8 KB    (адрес 0xe000)
✓ firmware.bin    - 1.1 MB  (адрес 0x10000)
✓ spiffs.bin      - 960 KB  (адрес 0x310000)
```

## 📊 Статистика сборки

- **RAM использовано**: 14.6% (47,860 байт из 327,680 байт)
- **Flash использовано**: 74.0% (1,163,397 байт из 1,572,864 байт)
- **Версия прошивки**: 1.0.0
- **Платформа**: ESP32 WROOM-32
- **Framework**: Arduino ESP32

## 🚀 Быстрый старт

### Вариант 1: Автоматический скрипт (Рекомендуется)

#### Windows
```cmd
flash_esp32.bat
```

#### Linux/Mac
```bash
chmod +x flash_esp32.sh
./flash_esp32.sh
```

### Вариант 2: PlatformIO

```bash
# Прошить основную прошивку
pio run -e esp32dev -t upload

# Прошить веб-интерфейс
pio run -e esp32dev -t uploadfs
```

### Вариант 3: Вручную через esptool

#### Windows
```cmd
esptool.py --chip esp32 --port COM3 --baud 921600 ^
  --before default_reset --after hard_reset write_flash -z ^
  --flash_mode dio --flash_freq 40m --flash_size 4MB ^
  0x1000 .pio\build\esp32dev\bootloader.bin ^
  0xe000 .pio\build\esp32dev\partitions.bin ^
  0x10000 .pio\build\esp32dev\firmware.bin ^
  0x310000 .pio\build\esp32dev\spiffs.bin
```

#### Linux/Mac
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  --before default_reset --after hard_reset write_flash -z \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 .pio/build/esp32dev/bootloader.bin \
  0xe000 .pio/build/esp32dev/partitions.bin \
  0x10000 .pio/build/esp32dev/firmware.bin \
  0x310000 .pio/build/esp32dev/spiffs.bin
```

## 📝 Адреса прошивки

| Компонент | Адрес | Размер | Описание |
|-----------|-------|--------|----------|
| Bootloader | 0x1000 | 28 KB | Загрузчик ESP32 |
| Partitions | 0xe000 | 8 KB | Таблица разделов |
| Firmware | 0x10000 | 1.5 MB | Основная прошивка (OTA slot 0) |
| SPIFFS | 0x310000 | 960 KB | Файловая система (веб-интерфейс) |

## 🔧 Инструменты для прошивки

### 1. PlatformIO (Рекомендуется)
- Автоматическая прошивка
- Встроенный Serial Monitor
- Поддержка OTA

### 2. esptool.py
```bash
pip install esptool
```

### 3. ESP Flash Download Tool (Windows GUI)
- Скачать: https://www.espressif.com/en/support/download/other-tools
- Графический интерфейс
- Массовая прошивка

## 🌐 Первое подключение

После прошивки:

1. **ESP32 создаст WiFi сеть:**
   - SSID: `Washka-Setup`
   - Пароль: нет

2. **Подключитесь к этой сети**

3. **Откройте браузер:**
   - http://192.168.4.1

4. **Настройте WiFi:**
   - Введите SSID и пароль вашей сети
   - Сохраните настройки

5. **После перезагрузки:**
   - ESP32 подключится к вашей сети
   - Найдите IP адрес в Serial Monitor
   - Откройте http://[IP_ADDRESS]

## 📱 Веб-интерфейс

После подключения доступны страницы:

- `/` - Главная страница управления
- `/config-gpio.html` - Настройка GPIO пинов
- `/config-timing.html` - Настройка таймингов
- `/config-system.html` - Системные настройки
- `/update` - OTA обновление
- `/webserial` - Веб-консоль отладки

## 🔄 OTA обновление

После первой прошивки можно обновлять по воздуху:

1. Откройте: http://[IP_ADDRESS]/update
2. Выберите файл: `.pio/build/esp32dev/firmware.bin`
3. Нажмите "Update"
4. Дождитесь перезагрузки (~30 секунд)

## 📡 REST API

Доступен REST API для управления:

- `GET /api/status` - Статус системы
- `GET /api/config` - Конфигурация
- `POST /api/config` - Обновить конфигурацию
- `POST /api/control/start` - Запустить цикл
- `POST /api/control/stop` - Остановить цикл
- `GET /api/pins` - Информация о пинах WROOM-32
- `GET /api/docs` - OpenAPI документация

## 🤖 Telegram Bot

Настройте Telegram бот:

1. Создайте бота через @BotFather
2. Получите токен
3. Введите токен в веб-интерфейсе
4. Отправьте `/start` боту
5. Управляйте системой через Telegram

Команды:
- `/start` - Список команд
- `/status` - Текущий статус
- `/startwash` - Запустить цикл
- `/stop` - Остановить цикл

## 🔍 Отладка

### Serial Monitor

```bash
# PlatformIO
pio device monitor -e esp32dev

# Или напрямую
python -m serial.tools.miniterm COM3 115200
```

### WebSerial

Откройте в браузере: http://[IP_ADDRESS]/webserial

## 📚 Документация

Полная документация в файлах:

- `FLASHING_INSTRUCTIONS.md` - Подробная инструкция по прошивке
- `QUICK_FLASH_GUIDE_RU.md` - Краткое руководство на русском
- `docs/PIN_LEGEND_DOCUMENTATION.md` - Документация по пинам
- `docs/WROOM32_PIN_REFERENCE.md` - Справочник по пинам ESP32
- `docs/REST_API_DOCUMENTATION.md` - Документация API

## ⚠️ Важные замечания

### GPIO пины

- **Безопасные пины**: 4, 5, 13, 14, 16-19, 21-23, 25-27, 32-33
- **Избегайте**: 0, 2, 12, 15 (strapping pins)
- **Никогда не используйте**: 6-11 (flash pins)
- **Только вход**: 34-39 (input only)

### Питание

- Напряжение: 3.3V (НЕ 5V!)
- Ток на пин: максимум 12mA (рекомендуется)
- Используйте транзисторы для больших нагрузок

### Безопасность

- Измените пароль WiFi после первой настройки
- Настройте API токен для REST API
- Ограничьте доступ к Telegram боту

## 🛠️ Устранение проблем

### ESP32 не прошивается

1. Проверьте COM-порт
2. Проверьте USB-кабель (должен поддерживать данные)
3. Войдите в режим прошивки вручную:
   - Зажмите BOOT
   - Нажмите RESET
   - Отпустите RESET
   - Отпустите BOOT

### Веб-интерфейс не открывается

1. Проверьте, что файловая система прошита
2. Проверьте IP адрес в Serial Monitor
3. Убедитесь, что вы в той же WiFi сети

### ESP32 перезагружается

1. Проверьте питание (нужен стабильный источник 5V/1A+)
2. Проверьте Serial Monitor на наличие ошибок
3. Проверьте подключение периферии

## 📞 Поддержка

Если возникли проблемы:

1. Проверьте Serial Monitor
2. Проверьте WebSerial
3. Проверьте документацию
4. Проверьте настройки GPIO пинов

## ✨ Возможности системы

- ✅ Автоматическое управление циклами мойки
- ✅ Веб-интерфейс с темной темой Bootstrap
- ✅ REST API для интеграции
- ✅ Telegram бот для удаленного управления
- ✅ OTA обновления по воздуху
- ✅ Настройка GPIO пинов через веб
- ✅ Контроль уровня воды по геркону
- ✅ Безопасное управление актуаторами
- ✅ Сохранение настроек в NVS
- ✅ WebSerial для отладки
- ✅ Восстановление после сбоев

## 🎯 Следующие шаги

1. ✅ Прошить ESP32
2. ⬜ Настроить WiFi
3. ⬜ Настроить GPIO пины
4. ⬜ Настроить таймеры
5. ⬜ Подключить актуаторы
6. ⬜ Подключить геркон
7. ⬜ Протестировать систему
8. ⬜ Настроить Telegram бот (опционально)

---

**Готово к прошивке! Удачи! 🚀**
