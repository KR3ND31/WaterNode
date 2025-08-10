import os
import sys


class Config:
    SQLALCHEMY_DATABASE_URI = "sqlite:///autowater.db"
    SQLALCHEMY_TRACK_MODIFICATIONS = False
    SERIAL_PORT = "/dev/ttyS0"
    BAUDRATE = 9600

    # использовать ли мок (эмуляцию устройств)
    USE_MOCK_SERIAL = os.getenv("MOCK_SERIAL", "true").lower() == "true" or sys.platform == "win32"