import time

from app.config import Config
from app.models import Device
from app.extensions import db

if Config.USE_MOCK_SERIAL:
    from app.mock_serial import MockSerial as Serial
else:
    from serial import Serial

from app.utils import log_packet, build_packet, parse_packet

ser = Serial(Config.SERIAL_PORT, Config.BAUDRATE, timeout=0.2)

ACK = b'\xAA'
NACK = b'\xFF'

def send_packet(addr: int, cmd: int, data: bytes = b'', retries: int = 3,
                delay: float = 0.05, timeout: float = 1.0) -> bytes | None:
    """
    Отправка команды устройству и получение ответа переменной длины с таймаутом.

    :param addr: адрес устройства
    :param cmd: код команды
    :param data: дополнительные данные
    :param retries: число попыток
    :param delay: задержка после отправки перед чтением
    :param timeout: максимальное время ожидания ответа (в секундах)
    :return: полный байтовый ответ, либо None
    """
    packet = build_packet(addr, cmd, data)

    for attempt in range(1, retries + 1):
        log_packet("TX", packet)
        ser.write(packet)
        time.sleep(delay)

        start_time = time.time()
        header = b''

        # Чтение заголовка (3 байта: addr, cmd, len)
        while len(header) < 3 and time.time() - start_time < timeout:
            chunk = ser.read(3 - len(header))
            if chunk:
                header += chunk
            else:
                time.sleep(0.005)

        if len(header) < 3:
            print(f"[WARN] Заголовок не получен, попытка {attempt}")
            continue

        # Извлекаем длину payload
        _, _, length = header
        total_remaining = length + 2  # payload + CRC
        body = b''
        start_time = time.time()

        while len(body) < total_remaining and time.time() - start_time < timeout:
            chunk = ser.read(total_remaining - len(body))
            if chunk:
                body += chunk
            else:
                time.sleep(0.005)

        if len(body) < total_remaining:
            print(f"[WARN] Таймаут при чтении тела ответа, попытка {attempt}")
            continue

        resp = header + body
        log_packet("RX", resp)

        parsed = parse_packet(resp)
        if parsed:
            addr_recv, cmd_recv, _, _ = parsed
            if addr_recv == addr and cmd_recv == cmd:
                print(f"[INFO] Устройство {addr} ответило командой 0x{cmd:02X}")
                return resp  # Возвращаем весь пакет — пользователь сам решит, нужен ли payload
            else:
                print(f"[WARN] Ответ cmd/addr не совпадает: addr={addr_recv}, cmd={cmd_recv}")
        else:
            print(f"[WARN] Невалидный пакет от {addr}, попытка {attempt}")

        time.sleep(0.1)

    return None

def read_moisture(addr: int) -> int | None:
    resp = send_packet(addr, 0x02)  # addr(1) + cmd(1) + cmd(len) + moist(2) + crc(2)
    if resp:
        parsed = parse_packet(resp)
        if parsed:
            addr_recv, cmd_recv, payload, _ = parsed
            if addr_recv == addr and cmd_recv == 0x02 and len(payload) == 2:
                return int.from_bytes(payload, "big")
    return None

def toggle_valve(addr: int, on: bool) -> bool:
    """
    Отправляет команду управления клапаном (открыть или закрыть).
    Ожидает подтверждение от устройства.

    :param addr: адрес устройства
    :param on: True — открыть клапан, False — закрыть
    :return: True, если устройство подтвердило выполнение команды
    """
    data = b'\x01' if on else b'\x00'
    resp = send_packet(addr, 0x03, data)
    if resp:
        parsed = parse_packet(resp)
        if parsed:
            addr_recv, cmd_recv, _, _ = parsed
            return addr_recv == addr and cmd_recv == 0x03
    return False

def ping_device(addr: int) -> bool:
    resp = send_packet(addr, 0x01)
    if resp:
        parsed = parse_packet(resp)
        if parsed:
            addr_recv, cmd_recv, payload, crc = parsed
            if addr_recv == addr and cmd_recv == 0x01:
                return True

def autodiscover_bulk(max_addr=20):
    """
    Автоматически обнаруживает неинициализированные устройства и присваивает им адреса.

    :param max_addr: максимальное количество адресов для назначения
    :return: список назначенных адресов
    """
    discovered = []

    for new_addr in range(1, max_addr + 1):
        resp = send_packet(0x00, 0xF0, bytes([new_addr]))
        if resp:
            parsed = parse_packet(resp)
            if parsed:
                addr_recv, cmd_recv, _, _ = parsed
                if cmd_recv == 0xF0:
                    print(f"[AutoDiscover] Устройство успешно инициализировано с адресом {new_addr}")
                    device = Device(address=new_addr, name=f"Устр {new_addr}", status="online")
                    db.session.add(device)
                    db.session.commit()
                    discovered.append(new_addr)
                    continue
            print(f"[AutoDiscover] ⚠️ Ответ не соответствует ожиданиям: {resp.hex()}")
        else:
            print(f"[AutoDiscover] ❌ Нет ответа на адрес {new_addr}, завершаем поиск.")
            time.sleep(0.1)
            break

    return discovered