#!/bin/bash
# ============================================================================
# ESP32 Washka - Скрипт автоматической прошивки (Linux/Mac)
# ============================================================================

set -e

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo ""
echo "========================================"
echo "ESP32 Washka - Прошивка"
echo "========================================"
echo ""

# Проверка наличия esptool
if ! command -v esptool.py &> /dev/null; then
    echo -e "${RED}[ОШИБКА]${NC} esptool.py не найден!"
    echo ""
    echo "Установите esptool:"
    echo "  pip install esptool"
    echo "  или"
    echo "  pip3 install esptool"
    echo ""
    exit 1
fi

# Проверка наличия файлов
if [ ! -f ".pio/build/esp32dev/firmware.bin" ]; then
    echo -e "${RED}[ОШИБКА]${NC} Файлы прошивки не найдены!"
    echo ""
    echo "Сначала скомпилируйте проект:"
    echo "  pio run -e esp32dev"
    echo "  pio run -e esp32dev -t buildfs"
    echo ""
    exit 1
fi

# Определение COM-порта
echo "Поиск ESP32..."
echo ""

# Попытка автоматического определения порта
if command -v pio &> /dev/null; then
    PORT=$(pio device list | grep -o '/dev/tty[^ ]*' | head -n 1)
fi

# Если не найден, попробовать стандартные порты
if [ -z "$PORT" ]; then
    if [ -e "/dev/ttyUSB0" ]; then
        PORT="/dev/ttyUSB0"
    elif [ -e "/dev/ttyACM0" ]; then
        PORT="/dev/ttyACM0"
    elif [ -e "/dev/cu.usbserial-0001" ]; then
        PORT="/dev/cu.usbserial-0001"
    fi
fi

# Если все еще не найден, запросить у пользователя
if [ -z "$PORT" ]; then
    echo "Не удалось автоматически определить порт."
    echo ""
    echo "Доступные порты:"
    ls /dev/tty* 2>/dev/null | grep -E "(USB|ACM|usbserial)" || echo "  Нет доступных портов"
    echo ""
    read -p "Введите порт (например, /dev/ttyUSB0): " PORT
fi

echo -e "${GREEN}Используется порт:${NC} $PORT"
echo ""

# Проверка прав доступа (только для Linux)
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if [ ! -w "$PORT" ]; then
        echo -e "${YELLOW}[ВНИМАНИЕ]${NC} Нет прав доступа к порту!"
        echo ""
        echo "Выполните одну из команд:"
        echo "  sudo chmod 666 $PORT"
        echo "  или"
        echo "  sudo usermod -a -G dialout \$USER"
        echo "  (после второй команды нужно перезайти в систему)"
        echo ""
        read -p "Продолжить с sudo? (y/n): " USE_SUDO
        if [ "$USE_SUDO" = "y" ]; then
            SUDO="sudo"
        else
            exit 1
        fi
    fi
fi

# Выбор типа прошивки
echo "Выберите тип прошивки:"
echo ""
echo "1. Полная прошивка (первая установка)"
echo "2. Только прошивка (обновление)"
echo "3. Только файловая система (веб-интерфейс)"
echo "4. Стереть Flash полностью"
echo ""
read -p "Ваш выбор (1-4): " CHOICE

case $CHOICE in
    1)
        echo ""
        echo "========================================"
        echo "Полная прошивка"
        echo "========================================"
        echo ""
        echo "Прошиваются:"
        echo "  - Bootloader (0x1000)"
        echo "  - Partitions (0xe000)"
        echo "  - Firmware (0x10000)"
        echo "  - Filesystem (0x310000)"
        echo ""
        echo "Это займет около 30-60 секунд..."
        echo ""

        $SUDO esptool.py --chip esp32 --port $PORT --baud 921600 \
          --before default_reset --after hard_reset write_flash -z \
          --flash_mode dio --flash_freq 40m --flash_size 4MB \
          0x1000 .pio/build/esp32dev/bootloader.bin \
          0xe000 .pio/build/esp32dev/partitions.bin \
          0x10000 .pio/build/esp32dev/firmware.bin \
          0x310000 .pio/build/esp32dev/spiffs.bin

        if [ $? -eq 0 ]; then
            echo ""
            echo "========================================"
            echo -e "${GREEN}Прошивка завершена успешно!${NC}"
            echo "========================================"
            echo ""
            echo "Следующие шаги:"
            echo "1. Откройте Serial Monitor: pio device monitor"
            echo "2. Подключитесь к WiFi: Washka-Setup"
            echo "3. Откройте браузер: http://192.168.4.1"
            echo ""
        else
            echo ""
            echo -e "${RED}[ОШИБКА] Прошивка не удалась!${NC}"
            echo ""
            echo "Попробуйте:"
            echo "1. Проверить порт"
            echo "2. Войти в режим прошивки вручную"
            echo "3. Использовать меньшую скорость: --baud 115200"
            echo ""
        fi
        ;;

    2)
        echo ""
        echo "========================================"
        echo "Обновление прошивки"
        echo "========================================"
        echo ""
        echo "Прошивается только firmware.bin..."
        echo ""

        $SUDO esptool.py --chip esp32 --port $PORT --baud 921600 \
          --before default_reset --after hard_reset write_flash -z \
          --flash_mode dio --flash_freq 40m --flash_size 4MB \
          0x10000 .pio/build/esp32dev/firmware.bin

        if [ $? -eq 0 ]; then
            echo ""
            echo "========================================"
            echo -e "${GREEN}Прошивка обновлена успешно!${NC}"
            echo "========================================"
            echo ""
        else
            echo ""
            echo -e "${RED}[ОШИБКА] Обновление не удалось!${NC}"
            echo ""
        fi
        ;;

    3)
        echo ""
        echo "========================================"
        echo "Обновление файловой системы"
        echo "========================================"
        echo ""
        echo "Прошивается только spiffs.bin (веб-интерфейс)..."
        echo ""

        $SUDO esptool.py --chip esp32 --port $PORT --baud 921600 \
          --before default_reset --after hard_reset write_flash -z \
          --flash_mode dio --flash_freq 40m --flash_size 4MB \
          0x310000 .pio/build/esp32dev/spiffs.bin

        if [ $? -eq 0 ]; then
            echo ""
            echo "========================================"
            echo -e "${GREEN}Файловая система обновлена успешно!${NC}"
            echo "========================================"
            echo ""
        else
            echo ""
            echo -e "${RED}[ОШИБКА] Обновление не удалось!${NC}"
            echo ""
        fi
        ;;

    4)
        echo ""
        echo "========================================"
        echo -e "${RED}ВНИМАНИЕ: Полное стирание Flash!${NC}"
        echo "========================================"
        echo ""
        echo "Это удалит ВСЕ данные с ESP32, включая:"
        echo "- Прошивку"
        echo "- Настройки WiFi"
        echo "- Конфигурацию"
        echo "- Файловую систему"
        echo ""
        read -p "Вы уверены? (yes/no): " CONFIRM

        if [ "$CONFIRM" != "yes" ]; then
            echo "Отменено."
            exit 0
        fi

        echo ""
        echo "Стирание Flash..."
        echo ""

        $SUDO esptool.py --chip esp32 --port $PORT erase_flash

        if [ $? -eq 0 ]; then
            echo ""
            echo "========================================"
            echo -e "${GREEN}Flash успешно стерт!${NC}"
            echo "========================================"
            echo ""
            echo "Теперь прошейте ESP32 заново (выбор 1)."
            echo ""
        else
            echo ""
            echo -e "${RED}[ОШИБКА] Стирание не удалось!${NC}"
            echo ""
        fi
        ;;

    *)
        echo -e "${RED}[ОШИБКА]${NC} Неверный выбор!"
        exit 1
        ;;
esac

echo ""
