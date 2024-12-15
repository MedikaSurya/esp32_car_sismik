#include <Arduino.h>
#include <WiFi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const char* ssid = "ESP32_AP";
const char* password = "12345678";

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(80);

// Buffer to store messages from anchors
String anchorMessages[3] = {"", "", ""}; // Assume 3 anchors
bool anchorReceived[3] = {false, false, false};

void getrssi(void* parameter) 
{
  while (true) 
  {
    WiFiClient client = server.available();

    if (client) 
    {
      Serial.println("New Client Connected");
      while (client.connected()) 
      {
        if (client.available()) 
        {
          String message = client.readStringUntil('\n');
          message.trim(); // Remove trailing newline
          Serial.print("Received: ");
          Serial.println(message);

          // Parse the message: RSSI$AnchorID
          int delimiterIndex = message.indexOf('$');
          if (delimiterIndex != -1) 
          {
            String rssi = message.substring(0, delimiterIndex);
            String anchorID = message.substring(delimiterIndex + 1);

            Serial.print("RSSI: ");
            Serial.println(rssi);
            Serial.print("Anchor ID: ");
            Serial.println(anchorID);

            // Map Anchor ID to buffer index
            int anchorIndex = -1;
            if (anchorID == "1") anchorIndex = 0;
            else if (anchorID == "2") anchorIndex = 1;
            else if (anchorID == "3") anchorIndex = 2;

            if (anchorIndex != -1) 
            {
              anchorMessages[anchorIndex] = message;
              anchorReceived[anchorIndex] = true;
            }
          }

          else 
          {
            Serial.println("Invalid message format");
          }

          client.flush();
          break;
        }
      }
      client.stop();
      Serial.println("Client Disconnected");
    }

    // Check if all anchors have sent their messages
    bool allReceived = true;
    for (int i = 0; i < 3; i++) 
    {
      if (!anchorReceived[i]) 
      {
        allReceived = false;
        break;
      }
    }

    if (allReceived) 
    {
      Serial.println("All anchors have sent their messages:");
      for (int i = 0; i < 3; i++) 
      {
        Serial.print("Anchor ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(anchorMessages[i]);
        // Reset for the next cycle
        anchorMessages[i] = "";
        anchorReceived[i] = false;
      }
      Serial.println("-----");
    }

    vTaskDelay(10 / portTICK_PERIOD_MS); // Small delay to avoid hogging the CPU
  }
}

void setup() 
{
  Serial.begin(115200);

  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);

  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.begin();

  // Create the getrssi task on core 0
  xTaskCreatePinnedToCore(
    getrssi,       // Function to implement the task
    "GetRSSI",    // Name of the task
    4096,          // Stack size in words
    NULL,          // Task input parameter
    1,             // Priority of the task
    NULL,          // Task handle
    0              // Core where the task should run
  );
}

void loop() 
{
  // Empty loop since the task is running on core 0
}