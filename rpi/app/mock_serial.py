import json
import time
import random
from pathlib import Path

from app.utils import crc16, build_packet

STATE_FILE = Path("mock_serial_state.json")

class MockDevice:
    def __init__(self, id, address=0x00, valve_state=False, moisture=None):
        self.id = id
        self.address = address
        self.moisture = moisture if moisture is not None else random.randint(300, 700)
        self.valve_state = valve_state
        self.response_delay = random.uniform(0.01, 0.2)

    def is_initialized(self):
        return self.address != 0x00


class MockSerial:
    def __init__(self, *args, **kwargs):
        self.devices = []
        self.load_state()

        self.buffer = b''
        self.last_cmd = None
        self.last_addr = None
        print("[MockSerial] Инициализация заглушки с 10 неинициализированными устройствами")

    def save_state(self):
        print("[MockSerial] Сохраняем состояние:")
        for d in self.devices:
            print(
                f"  Устройство {d.id}: адрес={d.address}, клапан={'открыт' if d.valve_state else 'закрыт'}, влага={d.moisture}")

        data = [
            {
                "id": d.id,
                "address": d.address,
                "valve_state": d.valve_state,
                "moisture": d.moisture
            }
            for d in self.devices
        ]
        with open(STATE_FILE, "w") as f:
            json.dump(data, f, indent=2)


    def load_state(self):
        if not STATE_FILE.exists():
            print("[MockSerial] Файл состояния не найден, создаю заново.")
            self.devices = [MockDevice(id=i + 1) for i in range(10)]
            return
        with open(STATE_FILE, "r") as f:
            raw = json.load(f)
        self.devices = [MockDevice(**d) for d in raw]
        print(f"[MockSerial] Загружено {len(self.devices)} устройств из файла")


    def write(self, data: bytes):
        # Минимальная длина пакета: адрес (1) + команда (1) + CRC (2)
        if len(data) < 5:
            print("[MockSerial] Пакет слишком короткий")
            self.buffer = build_packet(0x00, 0xFF)  # NACK
            return

        addr = data[0]
        cmd = data[1]
        length = data[2]

        expected_length = 3 + length + 2
        if len(data) != expected_length:
            print(f"[MockSerial] ❌ Неверная длина пакета: ожидается {expected_length}, получено {len(data)}")
            self.buffer = build_packet(addr, 0xFF)  # NACK
            return

        payload = data[3:-2]
        crc_received = int.from_bytes(data[-2:], "little")
        crc_calc = crc16(data[:-2])

        print(f"[MockSerial] ← Адрес={addr}, Команда={cmd:02X}, Длина={length}, CRC={hex(crc_received)}")

        if crc_received != crc_calc:
            print("[MockSerial] ❌ CRC не совпадает")
            self.buffer = build_packet(addr, 0xFF)  # ошибка NACK
            return

        # Обработка автоадресации
        if addr == 0x00 and cmd == 0xF0:
            uninitialized = [d for d in self.devices if not d.is_initialized()]
            if not uninitialized:
                self.buffer = build_packet(0x00, 0xFF)  # нет устройств для назначения
                return

            chosen = min(uninitialized, key=lambda d: d.response_delay)
            if not payload:
                print("[MockSerial] ❌ Нет адреса для назначения")
                self.buffer = build_packet(0x00, 0xFF)
                return

            new_addr = payload[0]
            chosen.address = new_addr
            print(f"[MockSerial] Устройство {chosen.id} ← назначен адрес {new_addr}")

            self.save_state()
            self.buffer = build_packet(0x00, 0xF0)
            return

        # Поиск устройства
        device = next((d for d in self.devices if d.address == addr), None)
        if not device:
            print(f"[MockSerial] ❌ Устройство с адресом {addr} не найдено")
            self.buffer = build_packet(addr, 0xFF)
            return

        # Обработка команд устройства
        if cmd == 0x01:  # ping
            self.buffer = build_packet(addr, cmd)  # ACK без payload
        elif cmd == 0x02:  # запрос влажности
            # Случайное значение влажности (например, от 20 до 80%)
            moist = random.randint(300, 700)
            device.moisture = moist
            moist_bytes = moist.to_bytes(2, 'big')
            self.buffer = build_packet(addr, cmd, moist_bytes)  # payload + CRC
        elif cmd == 0x03:  # управление клапаном
            if payload:
                state = payload[0]
                device.valve_state = (state == 1)
                self.buffer = build_packet(addr, cmd)  # ACK
            else:
                print("[MockSerial] ⚠️ Отсутствует payload для управления клапаном")
                self.buffer = build_packet(addr, 0xFF)
        else:
            print(f"[MockSerial] ⚠️ Неизвестная команда 0x{cmd:02X}")
            self.buffer = build_packet(addr, 0xFF)

    def read(self, size=1):
        time.sleep(0.01)  # эмуляция задержки

        if not self.buffer:
            print(f"[MockSerial] → Чтение {size} байт: буфер пуст")
            return b''

        result = self.buffer[:size]
        self.buffer = self.buffer[size:]

        print(f"[MockSerial] → Чтение {len(result)} байт: {result.hex()}")
        return result

    def close(self):
        print("[MockSerial] Порт закрыт.")