from flask import jsonify, Blueprint, request

from app.extensions import db
from app.models import Device
from app.utils import normalize_adc

bp = Blueprint("devices", __name__)

@bp.route("/devices", methods=["GET"])
def get_all_devices():
    devices = Device.query.all()
    return jsonify([{
        "id": d.id,
        "address": d.address,
        "name": d.name,
        "status": d.status,
        "moisture": normalize_adc(d.raw_moisture, d.calib_min or 0, d.calib_max or 1023) if d.raw_moisture is not None else None,
        "raw_moisture": d.raw_moisture,
        "calib_min": d.calib_min,
        "calib_max": d.calib_max,
    } for d in devices])

@bp.route("/device/<address>", methods=["GET"])
def get_single_device(address):
    device = Device.query.filter_by(address=address).first()
    if not device:
        return jsonify({"error": "Device not found"}), 404

    return jsonify({
        "id": device.id,
        "address": device.address,
        "name": device.name,
        "status": device.status,
        "moisture": normalize_adc(device.raw_moisture, device.calib_min or 0, device.calib_max or 1023)
                    if device.raw_moisture is not None else None,
        "raw_moisture": device.raw_moisture,
        "calib_min": device.calib_min,
        "calib_max": device.calib_max,
        "last_watered": device.last_watered.isoformat() if device.last_watered else None
    })


@bp.route("/device/<string:address>/calibrate", methods=["POST"])
def calibrate_device(address):
    data = request.json or {}
    device = Device.query.filter_by(address=address).first()

    if not device:
        return jsonify({"error": "device not found"}), 404

    changed = False
    if "calib_min" in data:
        device.calib_min = data["calib_min"]
        changed = True
    if "calib_max" in data:
        device.calib_max = data["calib_max"]
        changed = True

    if changed:
        db.session.commit()
        return jsonify({
            "status": "calibration updated",
            "calib_min": device.calib_min,
            "calib_max": device.calib_max
        })

    return jsonify({"error": "no calib_min or calib_max provided"}), 400

@bp.route("/device/<string:address>/rename", methods=["POST"])
def rename_device(address):
    data = request.get_json()
    name = data.get("name", "").strip()

    if not name:
        return jsonify({"error": "Empty name"}), 400

    device = Device.query.filter_by(address=address).first()
    if not device:
        return jsonify({"error": "Device not found"}), 404

    device.name = name
    db.session.commit()
    return jsonify({"success": True})