from flask import Flask, request, render_template, jsonify
from flask_socketio import SocketIO, emit
import sqlite3
import time

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="eventlet")

DATABASE = "alerts.db"

# Initialize the database
def init_db():
    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS alerts (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        date TEXT,
        time TEXT,
        sensor_type TEXT,
        level TEXT
    )
    """)
    conn.commit()
    conn.close()

# Ensure database is initialized when app starts
init_db()

# Database connection helper
def get_db_connection():
    conn = sqlite3.connect(DATABASE, check_same_thread=False)
    return conn, conn.cursor()

# Function to log alerts
def log_alert(sensor_type, level):
    current_time = time.localtime()
    date = time.strftime("%Y-%m-%d", current_time)
    time_of_day = time.strftime("%H:%M:%S", current_time)
    conn, cursor = get_db_connection()
    cursor.execute("INSERT INTO alerts (date, time, sensor_type, level) VALUES (?, ?, ?, ?)",
                   (date, time_of_day, sensor_type, level))
    conn.commit()
    conn.close()

    # Emit real-time update to the dashboard
    socketio.emit("new_alert", {"date": date, "time": time_of_day, "sensor": sensor_type, "level": level})

# API Endpoint to receive alerts
@app.route("/api/alerts", methods=["POST"])
def receive_alert():
    data = request.get_json()
    sensor_type = data.get("sensor")
    level = data.get("level")

    if sensor_type and level:
        log_alert(sensor_type, level)
        return jsonify({"status": "success", "message": "Alert logged"}), 200
    return jsonify({"status": "error", "message": "Invalid data"}), 400

# Route to render the dashboard
@app.route("/")
def dashboard():
    conn, cursor = get_db_connection()
    cursor.execute("SELECT * FROM alerts ORDER BY id DESC")
    alerts = cursor.fetchall()
    conn.close()
    return render_template("index.html", alerts=alerts)

if __name__ == "__main__":
    print("Starting Flask server on http://localhost:1337")
    socketio.run(app, host="0.0.0.0", port=1337, debug=True)