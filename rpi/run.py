from app import create_app
from app.extensions import db
from app.scheduler import start_scheduler

app = create_app()

with app.app_context():
    db.create_all()
    start_scheduler(app)

if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0", use_reloader=False)
