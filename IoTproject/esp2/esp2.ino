#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>

// Wi-Fi Configuration
const char* ssid = "Connect4G-WiFi-DA3A";
const char* password = "23785782";
const char* flaskServerUrl = "http://192.168.8.104:1337/api/alerts";

// Pushover Configuration
const char* pushoverAPI = "https://api.pushover.net/1/messages.json";
const char* apiToken = "ah4zyu42noekpvzpcp4pk1xo26wd4a";
const char* userKey = "udjzgeredeexwes3hq46tkacjrn16n";

WiFiUDP udp;
WiFiClientSecure client;
WiFiClient httpClient;

const int localUdpPort = 4210;  // UDP Port for receiving data
char incomingPacket[255];
String lastGPSLocation = "Unknown Location";  // Store the last GPS location received

// Notification Rate Limiting
unsigned long lastNotificationTime = 0;  // Timestamp of the last notification
const unsigned long notificationCooldown = 25000;  // 20 seconds in milliseconds

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("ESP2 Connected to Wi-Fi!");

  // Display ESP2 IP Address
  Serial.print("ESP2 IP Address: ");
  Serial.println(WiFi.localIP());

  // Start UDP Listener
  udp.begin(localUdpPort);
  Serial.println("Listening for UDP packets...");
  
  // Allow insecure HTTPS for Pushover
  client.setInsecure();
}

void loop() {
  int packetSize = udp.parsePacket();

  if (packetSize) {
    // Read UDP Packet
    int len = udp.read(incomingPacket, sizeof(incomingPacket) - 1);
    incomingPacket[len] = '\0';
    String alertMessage = String(incomingPacket);

    // Handle Alerts
    if (alertMessage.startsWith("MOTION")) {
      handleMotionAlert();
    } else if (alertMessage.startsWith("GPS:")) {
      lastGPSLocation = alertMessage.substring(5);  // Extract GPS location data
      Serial.println("Updated GPS Location: " + lastGPSLocation);
    }
  }
}

void handleMotionAlert() {
  unsigned long currentTime = millis();

  // Check if cooldown period has passed
  if (currentTime - lastNotificationTime >= notificationCooldown) {
    lastNotificationTime = currentTime;  // Update the timestamp
    String message = "Motion detected! Location: " + lastGPSLocation;

    // Send Pushover Notification
    sendPushoverNotification(message);

    // Forward Alert to Flask Server
    sendToFlaskServer("motion", message);

    Serial.println("Motion alert sent with location: " + lastGPSLocation);
  } else {
    Serial.println("Motion alert ignored (cooldown active).");
  }
}

void sendPushoverNotification(String message) {
  HTTPClient http;

  String postData = "token=" + String(apiToken) + "&user=" + String(userKey) +
                    "&message=" + message;

  http.begin(client, pushoverAPI);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http.POST(postData);
  if (httpResponseCode > 0) {
    Serial.println("Pushover Notification Sent Successfully!");
  } else {
    Serial.println("Failed to send Pushover Notification!");
  }
  http.end();
}

void sendToFlaskServer(String sensorType, String level) {
  HTTPClient http;

  http.begin(httpClient, flaskServerUrl);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"sensor\": \"" + sensorType + "\", \"level\": \"" + level + "\"}";
  Serial.println("Sending to Flask: " + payload);

  int httpResponseCode = http.POST(payload);
  if (httpResponseCode > 0) {
    Serial.println("Flask Server Received Alert Successfully!");
  } else {
    Serial.println("Failed to send alert to Flask Server!");
  }
  http.end();
}