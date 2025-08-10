from flask import Blueprint, jsonify, request, render_template

from app.extensions import db
from app.models import Device
from app.serial_comm import autodiscover_bulk

bp = Blueprint("main", __name__)

@bp.route("/")
def index():
    return render_template("index.html")

@bp.route("/valve/<int:addr>", methods=["POST"])
def control_valve(addr):
    action = request.json.get("action")  # "on" / "off"
    from app.serial_comm import toggle_valve
    success = toggle_valve(addr, action == "on")
    return jsonify({"status": "ok" if success else "fail"})

@bp.route("/autodiscover_bulk", methods=["POST"])
def autodiscover_route():
    new_devices = autodiscover_bulk()
    return jsonify({"status": "ok", "new_devices": new_devices})