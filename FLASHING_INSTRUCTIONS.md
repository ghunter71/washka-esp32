# Инструкция по прошивке ESP32 Washka System

## Скомпилированные файлы

После успешной компиляции созданы следующие файлы:

```
.pio/build/esp32dev/
├── bootloader.bin      - Загрузчик ESP32
├── partitions.bin      - Таблица разделов
├── firmware.bin        - Основная прошивка
└── spiffs.bin          - Файловая система (веб-интерфейс)
```

## Размеры файлов

- **RAM**: 14.6% (47,860 байт из 327,680 байт)
- **Flash**: 74.0% (1,163,397 байт из 1,572,864 байт)

## Адреса прошивки

Согласно таблице разделов `partitions.csv`:

| Файл | Адрес | Размер | Описание |
|------|-------|--------|----------|
| bootloader.bin | 0x1000 | ~24KB | Загрузчик ESP32 |
| partitions.bin | 0xe000 | 8KB | Таблица разделов |
| firmware.bin | 0x10000 | 1.5MB | Основная прошивка (OTA slot 0) |
| spiffs.bin | 0x310000 | 960KB | Файловая система LittleFS |

## Методы прошивки

### Метод 1: Через PlatformIO (Рекомендуется)

#### Полная прошивка (первая установка)

```bash
# Прошить всё сразу (bootloader + partitions + firmware + filesystem)
pio run -e esp32dev -t upload

# Прошить файловую систему
pio run -e esp32dev -t uploadfs
```

#### Только прошивка (обновление)

```bash
# Только основная прошивка (без файловой системы)
pio run -e esp32dev -t upload
```

#### Только файловая система

```bash
# Только веб-интерфейс
pio run -e esp32dev -t uploadfs
```

### Метод 2: Через esptool.py (Ручная прошивка)

#### Установка esptool.py

```bash
# Через pip
pip install esptool

# Или через pip3
pip3 install esptool
```

#### Полная прошивка (первая установка)

```bash
esptool.py --chip esp32 --port COM3 --baud 921600 \
  --before default_reset --after hard_reset write_flash -z \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 .pio/build/esp32dev/bootloader.bin \
  0xe000 .pio/build/esp32dev/partitions.bin \
  0x10000 .pio/build/esp32dev/firmware.bin \
  0x310000 .pio/build/esp32dev/spiffs.bin
```

**Замените COM3 на ваш порт:**
- Windows: COM3, COM4, COM5, и т.д.
- Linux: /dev/ttyUSB0, /dev/ttyUSB1, и т.д.
- macOS: /dev/cu.usbserial-*

#### Только прошивка (обновление)

```bash
esptool.py --chip esp32 --port COM3 --baud 921600 \
  --before default_reset --after hard_reset write_flash -z \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x10000 .pio/build/esp32dev/firmware.bin
```

#### Только файловая система

```bash
esptool.py --chip esp32 --port COM3 --baud 921600 \
  --before default_reset --after hard_reset write_flash -z \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x310000 .pio/build/esp32dev/spiffs.bin
```

### Метод 3: Через ESP Flash Download Tool (Windows GUI)

1. **Скачайте ESP Flash Download Tool:**
   - https://www.espressif.com/en/support/download/other-tools

2. **Запустите программу:**
   - Выберите "ESP32 DownloadTool"

3. **Настройте параметры:**
   
   | Файл | Адрес | Отметить |
   |------|-------|----------|
   | bootloader.bin | 0x1000 | ✓ |
   | partitions.bin | 0xe000 | ✓ |
   | firmware.bin | 0x10000 | ✓ |
   | spiffs.bin | 0x310000 | ✓ |

4. **Настройки прошивки:**
   - SPI SPEED: 40MHz
   - SPI MODE: DIO
   - FLASH SIZE: 32Mbit (4MB)
   - COM PORT: Выберите ваш порт
   - BAUD: 921600

5. **Нажмите START**

### Метод 4: Через Arduino IDE (Не рекомендуется)

Если вы хотите использовать Arduino IDE, вам нужно будет:
1. Скопировать все исходные файлы
2. Установить все библиотеки вручную
3. Настроить параметры компиляции

**Рекомендуется использовать PlatformIO вместо Arduino IDE.**

## Определение COM-порта

### Windows

```cmd
# Через Device Manager
devmgmt.msc

# Или через PowerShell
Get-WmiObject Win32_SerialPort | Select-Object Name,DeviceID

# Или через PlatformIO
pio device list
```

### Linux

```bash
# Список портов
ls /dev/ttyUSB*
ls /dev/ttyACM*

# Или через PlatformIO
pio device list

# Права доступа (если нужно)
sudo usermod -a -G dialout $USER
# Перезайдите в систему после этой команды
```

### macOS

```bash
# Список портов
ls /dev/cu.*

# Или через PlatformIO
pio device list
```

## Подготовка к прошивке

### 1. Подключение ESP32

1. Подключите ESP32 к компьютеру через USB
2. Убедитесь, что драйверы установлены:
   - CP2102: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
   - CH340: http://www.wch.cn/downloads/CH341SER_ZIP.html

### 2. Режим прошивки (Boot Mode)

ESP32 должен войти в режим прошивки автоматически. Если не получается:

1. Зажмите кнопку **BOOT** (или **IO0**)
2. Нажмите кнопку **RESET** (или **EN**)
3. Отпустите кнопку **RESET**
4. Отпустите кнопку **BOOT**
5. Начните прошивку

## Проверка прошивки

### 1. Через Serial Monitor

```bash
# PlatformIO
pio device monitor -e esp32dev

# Или esptool
python -m serial.tools.miniterm COM3 115200
```

### 2. Ожидаемый вывод

```
Washka ESP32 v1.0.0
Build: [DATE] [TIME]
====================

✓ ConfigManager initialized
✓ StateManager initialized (IDLE)
✓ ActuatorManager initialized
✓ WaterControl initialized
✓ WiFiManager starting...

[WiFi] No saved credentials found
[WiFi] Starting Access Point mode
[WiFi] AP SSID: Washka-Setup
[WiFi] AP IP: 192.168.4.1

✓ WebInterface initialized
✓ RestAPI initialized
✓ DebugLogger initialized
✓ OTAUpdater initialized

System ready!
Free heap: 180000 bytes
```

### 3. Подключение к веб-интерфейсу

1. **Первый запуск (AP режим):**
   - Подключитесь к WiFi сети: **Washka-Setup**
   - Откройте браузер: http://192.168.4.1
   - Настройте WiFi через веб-интерфейс

2. **После настройки WiFi:**
   - ESP32 подключится к вашей сети
   - Найдите IP адрес в Serial Monitor
   - Откройте браузер: http://[IP_ADDRESS]

## Устранение проблем

### Ошибка: "Failed to connect"

1. Проверьте COM-порт
2. Проверьте драйверы USB-UART
3. Попробуйте другой USB-кабель
4. Попробуйте другой USB-порт
5. Уменьшите скорость: `--baud 115200`

### Ошибка: "Timed out waiting for packet header"

1. Войдите в режим прошивки вручную (см. выше)
2. Проверьте подключение USB
3. Попробуйте другую скорость передачи

### Ошибка: "A fatal error occurred: MD5 of file does not match"

1. Перекомпилируйте прошивку: `pio run -e esp32dev -t clean && pio run -e esp32dev`
2. Попробуйте прошить снова

### ESP32 не загружается после прошивки

1. Проверьте Serial Monitor на наличие ошибок
2. Убедитесь, что все файлы прошиты по правильным адресам
3. Попробуйте стереть flash: `esptool.py --chip esp32 --port COM3 erase_flash`
4. Прошейте заново

### Веб-интерфейс не открывается

1. Проверьте, что файловая система прошита: `pio run -e esp32dev -t uploadfs`
2. Проверьте IP адрес в Serial Monitor
3. Убедитесь, что вы в той же сети WiFi

## OTA обновление (Over-The-Air)

После первой прошивки можно обновлять через WiFi:

### Через веб-интерфейс

1. Откройте http://[IP_ADDRESS]/update
2. Выберите файл `firmware.bin`
3. Нажмите "Update"
4. Дождитесь перезагрузки

### Через PlatformIO

```bash
# Настройте upload_port в platformio.ini
upload_port = 192.168.1.100

# Загрузите прошивку
pio run -e esp32dev -t upload --upload-port 192.168.1.100
```

### Через esptool (HTTP)

```bash
curl -F "file=@.pio/build/esp32dev/firmware.bin" http://[IP_ADDRESS]/update
```

## Резервное копирование

### Сохранить текущую прошивку

```bash
esptool.py --chip esp32 --port COM3 --baud 921600 \
  read_flash 0x0 0x400000 backup.bin
```

### Восстановить из резервной копии

```bash
esptool.py --chip esp32 --port COM3 --baud 921600 \
  --before default_reset --after hard_reset write_flash -z \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x0 backup.bin
```

## Полное стирание Flash

Если нужно полностью очистить ESP32:

```bash
esptool.py --chip esp32 --port COM3 erase_flash
```

После этого нужно прошить заново все файлы.

## Дополнительные команды

### Информация о чипе

```bash
esptool.py --chip esp32 --port COM3 chip_id
esptool.py --chip esp32 --port COM3 flash_id
```

### Чтение MAC адреса

```bash
esptool.py --chip esp32 --port COM3 read_mac
```

### Проверка подключения

```bash
esptool.py --chip esp32 --port COM3 chip_id
```

## Структура Flash памяти

```
0x000000 - 0x000FFF : Зарезервировано
0x001000 - 0x007FFF : Bootloader (28KB)
0x008000 - 0x008FFF : Зарезервировано
0x009000 - 0x00DFFF : NVS (20KB)
0x00E000 - 0x00FFFF : OTA Data (8KB)
0x010000 - 0x18FFFF : App0 (OTA Slot 0) - 1.5MB
0x190000 - 0x30FFFF : App1 (OTA Slot 1) - 1.5MB
0x310000 - 0x3FFFFF : SPIFFS/LittleFS (960KB)
```

## Рекомендации

1. **Первая прошивка**: Используйте полную прошивку через USB
2. **Обновления**: Используйте OTA через веб-интерфейс
3. **Разработка**: Используйте PlatformIO для быстрой итерации
4. **Производство**: Используйте ESP Flash Download Tool для массовой прошивки

## Контрольные суммы

После прошивки можно проверить контрольные суммы:

```bash
# MD5 прошивки
md5sum .pio/build/esp32dev/firmware.bin

# MD5 файловой системы
md5sum .pio/build/esp32dev/spiffs.bin
```

## Поддержка

Если возникли проблемы:
1. Проверьте Serial Monitor на наличие ошибок
2. Убедитесь, что используете правильный COM-порт
3. Проверьте версию esptool: `esptool.py version`
4. Попробуйте уменьшить скорость передачи до 115200

## Версия прошивки

- **Версия**: 1.0.0
- **Дата сборки**: [Автоматически из __DATE__]
- **Время сборки**: [Автоматически из __TIME__]
- **Платформа**: ESP32 (WROOM-32)
- **Framework**: Arduino ESP32

---

**Успешной прошивки!** 🚀
