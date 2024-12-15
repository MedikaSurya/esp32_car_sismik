// ESP32 Client Program
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Replace with your access point credentials
const char* ssid = "ESP32_AP";
const char* password = "12345678";

// Server IP and port (Fixed IP for ESP32 AP)
const char* serverIP = "192.168.4.1"; // Fixed IP address of the server
const uint16_t serverPort = 80;

// Set a fixed IP address for the client
IPAddress client_IP(192, 168, 4, 3); // Fixed IP for this client
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// Maximum power level (19.5 dBm)
wifi_power_t maxPower = WIFI_POWER_19_5dBm;

AsyncWebSocket ws("/ws");
AsyncWebServer server(serverPort);

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.println("WebSocket Connected");
      break;
    case WS_EVT_DISCONNECT:
      Serial.println("WebSocket Disconnected");
      break;
    case WS_EVT_DATA:
      Serial.printf("Received: %s\n", data);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void setup() {
  Serial.begin(115200);

  // Configure the client with a fixed IP
  WiFi.config(client_IP, gateway, subnet);
  WiFi.begin(ssid, password);

  // Set the transmit power to maximum
  WiFi.setTxPower(maxPower);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize WebSocket
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    String rssi = String(WiFi.RSSI());
    String anchorID = "2";
    String message = rssi + "$" + anchorID;
    
    ws.textAll(message);
    Serial.print("Sent: ");
    Serial.println(message);
  }

  delay(2000); // Wait for 2 seconds before sending the next message
}