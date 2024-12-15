#include <Arduino.h>
#include <WiFi.h>
#include <math.h>
#include <ESPAsyncWebServer.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//Parameter Server
const char* ssid = "ESP32_AP";
const char* password = "12345678";

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // Endpoint WebSocket

//Buffer Pesan RSSI dari Anchor
String anchorMessages[3] = {"", "", ""};
bool anchorReceived[3] = {false, false, false};


//Parameter Komunikasi 2 arah Websocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // Endpoint WebSocket

// HTML template yang akan ditampilkan
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Template Tampilan Lokalisasi</title>
    <style>
        body { background-color: rgb(61, 61, 61); }
        .grid-container { position: relative; width: 500px; height: 500px; background-color: #f0f0f0; margin-left: auto; margin-right: auto; }
        .grid-line { position: absolute; background-color: #ccc; }
        .vertical { width: 1px; height: 100%; }
        .horizontal { width: 100%; height: 1px; }
        .label { position: absolute; font-size: 15px; color: black; }
        .point { position: absolute; width: 10px; height: 10px; background-color: red; border-radius: 50%; }
    </style>
</head>
<body>
    <div class="grid-container" id="grid-container">
        <!-- Vertical lines -->
        <div class="grid-line vertical" style="left: 0%;"></div>
        <div class="grid-line vertical" style="left: 12.5%;"></div>
        <div class="grid-line vertical" style="left: 25%;"></div>
        <div class="grid-line vertical" style="left: 37.5%;"></div>
        <div class="grid-line vertical" style="left: 50%;"></div>
        <div class="grid-line vertical" style="left: 62.5%;"></div>
        <div class="grid-line vertical" style="left: 75%;"></div>
        <div class="grid-line vertical" style="left: 87.5%;"></div>
        <div class="grid-line vertical" style="left: 100%;"></div>
        <!-- Horizontal lines -->
        <div class="grid-line horizontal" style="top: 0%;"></div>
        <div class="grid-line horizontal" style="top: 12.5%;"></div>
        <div class="grid-line horizontal" style="top: 25%;"></div>
        <div class="grid-line horizontal" style="top: 37.5%;"></div>
        <div class="grid-line horizontal" style="top: 50%;"></div>
        <div class="grid-line horizontal" style="top: 62.5%;"></div>
        <div class="grid-line horizontal" style="top: 75%;"></div>
        <div class="grid-line horizontal" style="top: 87.5%;"></div>
        <div class="grid-line horizontal" style="top: 100%;"></div>
    </div>
    <script>
        const ws = new WebSocket(`ws://${window.location.host}/ws`);

        ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            const container = document.getElementById('grid-container');
            container.innerHTML = ''; // Clear previous points

            data.points.forEach(point => {
                const div = document.createElement('div');
                div.className = 'point';
                div.style.backgroundColor = point.color;
                const containerWidth = container.offsetWidth;
                const containerHeight = container.offsetHeight;
                const pixelX = ((point.x + 0.5) / 4) * containerWidth;
                const pixelY = containerHeight - ((point.y + 0.5) / 4) * containerHeight;
                div.style.left = `${pixelX - div.offsetWidth / 2}px`;
                div.style.top = `${pixelY - div.offsetHeight / 2}px`;
                container.appendChild(div);
            });
        };
    </script>
</body>
</html>
)rawliteral";


//posisi Anchor
float xAnchor1 = 0;
float yAnchor1 = 0;

float xAnchor2 = 0;
float yAnchor2 = 2.5;

float xAnchor3 = 2.5;
float yAnchor3 = 0;

//Konstanta konversi RSSI
float Att = 23.039;
float Cons = 67.917;

//RSSI Terukur
float rssi1 = 0;
float rssi2 = 0;
float rssi3 = 0;

//Jarak Terukur
float d1 = 0;
float d2 = 0;
float d3 = 0;

//Seed posisi
float seedx1 = 0;
float seedy1 = 0;

float seedx2 = 0;
float seedy2 = 0;

float seedx3 = 0;
float seedy3 = 0;

//Estimasi posisi
float estX = 0;
float estY = 0;

//prosedur untuk mendapatkan RSSI dari Anchor
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->opcode == WS_TEXT) {
      data[len] = 0; // Null-terminate string
      String message = (char *)data;
      message.trim(); // Trim any trailing newline

      Serial.print("Received: ");
      Serial.println(message);

      // Parse the message: RSSI$AnchorID
      int delimiterIndex = message.indexOf('$');
      if (delimiterIndex != -1) {
        String rssi = message.substring(0, delimiterIndex);
        String anchorID = message.substring(delimiterIndex + 1);

        int anchorIndex = anchorID.toInt() - 1;
        if (anchorIndex >= 0 && anchorIndex < 3) {
          anchorMessages[anchorIndex] = rssi;
          anchorReceived[anchorIndex] = true;
        }
      }
    }
  }
}

void getrssi(void* parameter) 
{
  while (true) 
  {
    // Mengecek apakah semua anchor telah mengirimkan data
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
      Serial.println("All anchors have sent their messages");
      rssi1 = anchorMessages[0].toFloat();
      rssi2 = anchorMessages[1].toFloat();
      rssi3 = anchorMessages[2].toFloat();

      // Reset buffer
      for (int i = 0; i < 3; i++)
      {
        anchorReceived[i] = false;
      }
    }

    vTaskDelay(10 / portTICK_PERIOD_MS); 
  }
}

void seed_gen(float x1, float y1, float x2, float y2, float x3, float y3, float d1, float d2, float d3)
{
    //persamaan 1
    float A = 2 * (x2 - x1);
    float B = 2 * (y2 - y1);
    float C = pow(d1, 2) - pow(d2, 2) - pow(x1, 2) + pow(x2, 2) - pow(y1, 2) + pow(y2, 2);

    //persamaan 2
    float D = 2 * (x3 - x2);
    float E = 2 * (y3 - y2);
    float F = pow(d2, 2) - pow(d3, 2) - pow(x2, 2) + pow(x3, 2) - pow(y2, 2) + pow(y3, 2);

    //persamaan 3
    float G = 2 * (x3 - x1);
    float H = 2 * (y3 - y1);
    float I = pow(d1, 2) - pow(d3, 2) - pow(x1, 2) + pow(x3, 2) - pow(y1, 2) + pow(y3, 2);

    //digunakan metode cramer untuk mencari seed
    //seed 1
    float det1 = A * E - B * D;
    float detX1 = C * E - B * F;
    float detY1 = A * F - C * D;
    float seedx1 = detX1 / det1;
    float seedy1 = detY1 / det1;

    //seed 2
    float det2 = A * H - B * G;
    float detX2 = C * H - B * I;
    float detY2 = A * I - C * G;
    float seedx2 = detX2 / det2;
    float seedy2 = detY2 / det2;

    //seed 3
    float det3 = D * H - E * G;
    float detX3 = F * H - E * I;
    float detY3 = D * I - F * G;
    float seedx3 = detX3 / det3;
    float seedy3 = detY3 / det3;
}

float total_error(float px, float py, float x1, float y1, float x2, float y2, float x3, float y3, float d1, float d2, float d3)
{
    float e1 = pow(d1 - sqrt(pow((px - x1), 2) + pow((py - y1), 2)), 2);
    float e2 = pow(d2 - sqrt(pow((px - x2), 2) + pow((py - y2), 2)), 2);
    float e3 = pow(d3 - sqrt(pow((px - x3), 2) + pow((py - y3), 2)), 2);
    return (e1 + e2 + e3) / 3;
}

// Gradient Descent untuk minimisasi error
void grad_descent(float& px, float& py, float learning_rate, int max_iter, float tolerance, float x1, float y1, float x2, float y2, float x3, float y3, float d1, float d2, float d3)
{
    for (int i = 0; i < max_iter; ++i)
    {
        float grad_x =
            2 * ((sqrt(pow((px - x1), 2) + pow((py - y1), 2)) - d1) * (px - x1) / max(1e-6, sqrt(pow((px - x1), 2) + pow((py - y1), 2))) +
                 (sqrt(pow((px - x2), 2) + pow((py - y2), 2)) - d2) * (px - x2) / max(1e-6, sqrt(pow((px - x2), 2) + pow((py - y2), 2))) +
                 (sqrt(pow((px - x3), 2) + pow((py - y3), 2)) - d3) * (px - x3) / max(1e-6, sqrt(pow((px - x3), 2) + pow((py - y3), 2))));

        float grad_y =
            2 * ((sqrt(pow((px - x1), 2) + pow((py - y1), 2)) - d1) * (py - y1) / max(1e-6, sqrt(pow((px - x1), 2) + pow((py - y1), 2))) +
                 (sqrt(pow((px - x2), 2) + pow((py - y2), 2)) - d2) * (py - y2) / max(1e-6, sqrt(pow((px - x2), 2) + pow((py - y2), 2))) +
                 (sqrt(pow((px - x3), 2) + pow((py - y3), 2)) - d3) * (py - y3) / max(1e-6, sqrt(pow((px - x3), 2) + pow((py - y3), 2))));

        float new_px = px - learning_rate * grad_x;
        float new_py = py - learning_rate * grad_y;

        if (fabs(new_px - px) < tolerance && fabs(new_py - py) < tolerance)
            break;

        px = new_px;
        py = new_py;
    }
}

void mse(float x1, float y1, float x2, float y2, float x3, float y3, float d1, float d2, float d3)
{
    // Pilih seed terbaik berdasarkan error
    float best_error = 1e6; // Nilai awal besar
    float best_x = 0, best_y = 0;

    float seeds_x[3] = {seedx1, seedx2, seedx3};
    float seeds_y[3] = {seedy1, seedy2, seedy3};

    for (int i = 0; i < 3; i++)
    {
        float current_x = seeds_x[i];
        float current_y = seeds_y[i];

        grad_descent(current_x, current_y, 0.01, 100, 1e-6, x1, y1, x2, y2, x3, y3, d1, d2, d3);
        float error = total_error(current_x, current_y, x1, y1, x2, y2, x3, y3, d1, d2, d3);

        if (error < best_error)
        {
            best_error = error;
            best_x = current_x;
            best_y = current_y;
        }
    }

    estX = best_x;
    estY = best_y;
}

void localization(void* parameter)
{
    while (true)
    {
        if (anchorReceived[0] && anchorReceived[1] && anchorReceived[2])
        {
            // Konversi RSSI ke jarak
            d1 = pow(10, (Cons - rssi1) / (10 * Att));
            d2 = pow(10, (Cons - rssi2) / (10 * Att));
            d3 = pow(10, (Cons - rssi3) / (10 * Att));

            // Seed generation
            seed_gen(xAnchor1, yAnchor1, xAnchor2, yAnchor2, xAnchor3, yAnchor3, d1, d2, d3);

            // Gradient Descent
            mse(xAnchor1, yAnchor1, xAnchor2, yAnchor2, xAnchor3, yAnchor3, d1, d2, d3);

            Serial.print("Estimated X: ");
            Serial.println(estX);
            Serial.print("Estimated Y: ");
            Serial.println(estY);

            // Reset buffer
            for (int i = 0; i < 3; i++)
            {
                anchorReceived[i] = false;
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void notifyClients() 
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), R"json({"points":[{"x":%.2f,"y":%.2f,"color":"red"}]})json", estX, estY);
    ws.textAll(buffer);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) 
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
    {
        data[len] = 0;
        String message = (char *)data;
        // Handle incoming WebSocket messages if necessary
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) 
{
    switch (type) 
    {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void show_html(void* parameter) 
{
    // Konfigurasi WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
        Serial.print(".");
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    Serial.println("\nWiFi connected");
    Serial.println(WiFi.localIP());

    // Menambahkan route untuk menampilkan halaman HTML
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
        request->send(200, "text/html", htmlPage);
    });

    // Start server
    server.begin();
}

void CarControl(void *parameter) 
{
    while (true) 
    {
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}


void CamControl(void* parameter)
{
    while (true)
    {

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void setup() 
{
  Serial.begin(115200);

  // Configure the server with a fixed IP
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
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

  // Serve the HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(200, "text/html", htmlPage);
  });

  // Create tasks
  xTaskCreatePinnedToCore(
    getrssi,          // Function to implement the task
    "GetRSSI",        // Name of the task
    4096,             // Stack size in words
    NULL,             // Task input parameter
    1,                // Priority of the task
    NULL,             // Task handle
    0                 // Core where the task should run
  );

  xTaskCreatePinnedToCore(
    localization,     // Function to implement the task
    "Localization",   // Name of the task
    4096,             // Stack size in words
    NULL,             // Task input parameter
    2,                // Priority of the task (higher priority)
    NULL,             // Task handle
    1                 // Core where the task should run
  );

  xTaskCreatePinnedToCore(
    show_html,        // Function to implement the task
    "ShowHTML",       // Name of the task
    4096,             // Stack size in words
    NULL,             // Task input parameter
    1,                // Priority of the task
    NULL,             // Task handle
    1                 // Core where the task should run
  );
}

void loop() 
{
  // Empty loop as tasks are running independently
}