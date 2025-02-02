#include <Wire.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// MPU-6050 I2C Address
const int MPU = 0x68;

// GPS Setup
TinyGPSPlus gps;
SoftwareSerial gpsSerial(13, 12);  // GPS RX = GPIO13 (D7), GPS TX = GPIO12 (D6)

// Wi-Fi Configuration
const char* ssid = "Connect4G-WiFi-DA3A";
const char* password = "23785782";
const char* esp2_ip = "192.168.8.103";  // ESP2 IP Address
const int udpPort = 4210;               // UDP Port for ESP2

WiFiUDP udp;

// Define Pin Numbers Explicitly
const int MPU_SDA = 4;  // GPIO4 corresponds to D2
const int MPU_SCL = 5;  // GPIO5 corresponds to D1

// Motion Threshold
const int MOTION_THRESHOLD = 6000;  // Adjust this threshold based on sensitivity requirements

// Variables for MPU-6050 Data
int16_t accelX, accelY, accelZ;

void setup() {
  Serial.begin(115200);

  // MPU-6050 Initialization
  Wire.begin(MPU_SDA, MPU_SCL);  // SDA = GPIO4, SCL = GPIO5
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);  // Power management register
  Wire.write(0);     // Wake up MPU-6050
  Wire.endTransmission();

  // GPS Initialization
  gpsSerial.begin(9600);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("ESP1 Connected to WiFi!");
}

void loop() {
  // Read MPU-6050 Data
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);  // Starting register for accelerometer data
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);
  accelX = Wire.read() << 8 | Wire.read();
  accelY = Wire.read() << 8 | Wire.read();
  accelZ = Wire.read() << 8 | Wire.read();

  // Print Accelerometer Readings
  Serial.print("AccelX: ");
  Serial.print(accelX);
  Serial.print(" | AccelY: ");
  Serial.print(accelY);
  Serial.print(" | AccelZ: ");
  Serial.println(accelZ);

  // Detect Motion
  bool motionDetected = abs(accelX) > MOTION_THRESHOLD || abs(accelY) > MOTION_THRESHOLD;

  // Read GPS Data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
  bool gpsDataAvailable = gps.location.isValid();

  if (gpsDataAvailable) {
    // Print GPS Location
    Serial.print("GPS Location: ");
    Serial.print("Latitude: ");
    Serial.print(gps.location.lat(), 6);
    Serial.print(" | Longitude: ");
    Serial.println(gps.location.lng(), 6);
  } else {
    Serial.println("GPS Location: Not available");
  }

  // Send Alerts to ESP2
  if (motionDetected) {
    Serial.println("Strong Motion Detected! Sending Alert...");
    udp.beginPacket(esp2_ip, udpPort);
    udp.print("MOTION DETECTED");
    udp.endPacket();
    delay(1000);
  }

  if (gpsDataAvailable) {
    String gpsData = "GPS: " + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
    udp.beginPacket(esp2_ip, udpPort);
    udp.print(gpsData);
    udp.endPacket();
    delay(1000);
  }

  delay(500);  // Adjust loop delay for responsiveness
}