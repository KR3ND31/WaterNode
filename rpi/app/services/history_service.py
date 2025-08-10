from datetime import datetime, timedelta
from app.models import MoistureReading
from app.utils import normalize_adc

def get_cutoff(period: str) -> datetime:
    return {
        "1h": datetime.now() - timedelta(hours=1),
        "24h": datetime.now() - timedelta(days=1),
        "7d": datetime.now() - timedelta(days=7),
        "30d": datetime.now() - timedelta(days=30),
    }.get(period, datetime.now() - timedelta(days=1))

def format_readings(device, readings):
    return {
        "address": device.address,
        "name": device.name,
        "readings": [{
            "t": r.timestamp.isoformat(),
            "v": normalize_adc(r.value, device.calib_min or 0, device.calib_max or 1023),
            "raw": r.value
        } for r in readings]
    }

def get_device_with_readings(device, cutoff):
    readings = (
        MoistureReading.query
        .filter_by(device_id=device.id)
        .filter(MoistureReading.timestamp >= cutoff)
        .order_by(MoistureReading.timestamp)
        .all()
    )
    return format_readings(device, readings)
