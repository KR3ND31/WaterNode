from apscheduler.schedulers.background import BackgroundScheduler
from datetime import datetime
from app.extensions import db
from app.models import Device, MoistureReading

def poll_devices(app):
    with app.app_context():
        devices = Device.query.all()
        for device in devices:
            from app.serial_comm import ping_device, read_moisture

            if ping_device(device.address):
                device.status = "online"
                device.last_seen = datetime.now()
                moisture = read_moisture(device.address)
                if moisture is not None:
                    device.raw_moisture = moisture
                    reading = MoistureReading(device_id=device.id, value=moisture)
                    db.session.add(reading)
            else:
                device.status = "offline"
        db.session.commit()

def start_scheduler(app):
    scheduler = BackgroundScheduler()
    scheduler.add_job(lambda: poll_devices(app), trigger='interval', seconds=1)
    scheduler.start()
