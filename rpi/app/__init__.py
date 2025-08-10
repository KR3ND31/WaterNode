from flask import Flask
from flask_migrate import Migrate
from app.extensions import db

from .routes.routes import bp as main_bp
from .routes.history import bp as history_bp
from .routes.devices import bp as devices_bp


def create_app():
    app = Flask(__name__)
    app.config.from_object("app.config.Config")

    migrate = Migrate(app, db)

    db.init_app(app)

    app.register_blueprint(main_bp)
    app.register_blueprint(history_bp)
    app.register_blueprint(devices_bp)

    return app