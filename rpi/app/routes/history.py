from flask import request, jsonify, Blueprint

from app.models import Device
from app.services.history_service import get_device_with_readings, get_cutoff

bp = Blueprint("history", __name__)

@bp.route("/history")
def get_history_all():
    period = request.args.get("period", "24h")
    cutoff = get_cutoff(period)

    devices = Device.query.all()
    result = [get_device_with_readings(d, cutoff) for d in devices]

    return jsonify(result)

@bp.route("/history/<device_address>")
def get_history_single(device_address):
    period = request.args.get("period", "24h")
    cutoff = get_cutoff(period)

    device = Device.query.filter_by(address=device_address).first()
    if not device:
        return jsonify({"error": "Device not found"}), 404

    result = get_device_with_readings(device, cutoff)
    return jsonify(result)