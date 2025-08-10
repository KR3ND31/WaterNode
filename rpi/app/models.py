from app.extensions import db
from datetime import datetime

class Device(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    address = db.Column(db.Integer, unique=True, nullable=False)
    name = db.Column(db.String(50))
    status = db.Column(db.String(10), default="offline")  # online/offline
    last_seen = db.Column(db.DateTime)
    raw_moisture = db.Column(db.Float)
    control_mode = db.Column(db.String(20), default="manual")  # timer/humidity/manual
    threshold = db.Column(db.Float, default=30.0)
    calib_min = db.Column(db.Integer, default=0)
    calib_max = db.Column(db.Integer, default=1023)
    last_watered = db.Column(db.DateTime, nullable=True)

class MoistureReading(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    device_id = db.Column(db.Integer, db.ForeignKey("device.id"))
    timestamp = db.Column(db.DateTime, default=datetime.now)
    value = db.Column(db.Float)