from colorama import Fore, Style, init

init(autoreset=True)


def build_packet(addr: int, cmd: int, data: bytes = b'') -> bytes:
    """
    Собирает пакет с адресом, командой, данными и CRC16.

    :param addr: адрес устройства
    :param cmd: код команды
    :param data: дополнительные байты данных
    :return: готовый байтовый пакет для отправки
    """
    length = len(data)
    base = bytes([addr, cmd, length]) + data
    crc = crc16(base)
    return base + crc.to_bytes(2, 'little')


def parse_packet(packet: bytes):
    """
    Разбирает входящий пакет, проверяет CRC и возвращает его составляющие.

    :param packet: входной байтовый пакет (адрес + команда + данные + CRC)
    :return: кортеж (addr, cmd, payload, crc) при успешной проверке,
             иначе None, если пакет слишком короткий или CRC некорректен
    """
    if len(packet) < 5:
        return None  # слишком короткий пакет

    addr = packet[0]
    cmd = packet[1]
    length = packet[2]

    expected_len = 3 + length + 2  # addr + cmd + len + data + crc
    if len(packet) != expected_len:
        return None  # длина не совпадает

    payload = packet[3:-2]
    crc_received = int.from_bytes(packet[-2:], 'little')
    crc_calc = crc16(packet[:-2])

    if crc_received != crc_calc:
        return None  # ошибка CRC

    return addr, cmd, payload, crc_received


def log_packet(prefix: str, packet: bytes, decode: bool = True):
    hex_str = ' '.join(f'{b:02X}' for b in packet)

    # Подсветка CRC (последние 2 байта)
    if len(packet) >= 3:
        body = ' '.join(f'{b:02X}' for b in packet[:-2])
        crc = ' '.join(f'{Fore.RED}{b:02X}{Style.RESET_ALL}' for b in packet[-2:])
        print(f"[{prefix}] {body} {crc}")
    else:
        print(f"[{prefix}] {hex_str}")

    if decode and len(packet) >= 3:
        addr = packet[0]
        cmd = packet[1]
        payload = packet[2:-2]
        print(
            f"{Style.DIM} └─ Адрес: {addr}, Команда: 0x{cmd:02X}, Payload: {payload.hex()}, CRC: {packet[-2:].hex()}\n")


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc


def normalize_adc(adc_val, min_val, max_val):
    if max_val == min_val:
        return 0
    percent = (adc_val - min_val) / (max_val - min_val)
    return round(100 * max(0, min(percent, 1)), 1)
