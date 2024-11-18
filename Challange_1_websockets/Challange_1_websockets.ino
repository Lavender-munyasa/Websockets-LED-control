#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Replace with your network credentials
const char* ssid = "enter wifi name";
const char* password = "enter wifi password";

// GPIO pin definitions
const int redPin = 16;  // Pin for Red LED
const int greenPin = 19; // Pin for Green LED

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// HTML page served to the client
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; text-align: center; }
    h1 { font-size: 2rem; }
    .button { font-size: 1.5rem; padding: 10px 20px; cursor: pointer; }
    .state { font-size: 1.5rem; color: #333; }
  </style>
</head>
<body>
  <h1>ESP32 LED Control</h1>
  <button id="redButton" class="button">Toggle Red (LED 1)</button><br><br>
  <button id="greenButton" class="button">Toggle Green (LED 2)</button><br><br>
  <p class="state">Red LED state: <span id="redState">OFF</span></p>
  <p class="state">Green LED state: <span id="greenState">OFF</span></p>

  <script>
    var websocket = new WebSocket('ws://' + window.location.hostname + '/ws');
    websocket.onopen = function() {
      console.log("WebSocket connection established");
    };
    
    websocket.onmessage = function(event) {
      var data = event.data.split(',');
      document.getElementById('redState').innerText = (data[0] === "1") ? "ON" : "OFF";
      document.getElementById('greenState').innerText = (data[1] === "1") ? "ON" : "OFF";
    };

    document.getElementById('redButton').addEventListener('click', function() {
      websocket.send('toggleRed');
    });

    document.getElementById('greenButton').addEventListener('click', function() {
      websocket.send('toggleGreen');
    });
  </script>
</body>
</html>
)rawliteral";

// Updated handleWebSocketMessage function
void handleWebSocketMessage(AsyncWebSocket *server, 
                            AsyncWebSocketClient *client, 
                            AwsEventType type, 
                            void *arg, 
                            uint8_t *data, 
                            size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      data[len] = 0;  // Null-terminate the data
      String message = String((char*)data);

      Serial.println("Received WebSocket message: " + message);  // Debugging

      if (message == "toggleRed") {
        digitalWrite(redPin, !digitalRead(redPin));  // Toggle Red LED
        notifyClients();
      } else if (message == "toggleGreen") {
        digitalWrite(greenPin, !digitalRead(greenPin));  // Toggle Green LED
        notifyClients();
      }
    }
  }
}

// Notify all connected clients with the updated LED states
void notifyClients() {
  String state = String(digitalRead(redPin)) + "," + String(digitalRead(greenPin));
  ws.textAll(state);
}

void setup() {
  // Start Serial Monitor for debugging
  Serial.begin(115200);

  // Set LED pins to output mode
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  // Print the ESP32 IP address after it connects to Wi-Fi
  Serial.println("Connected to WiFi");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // Serve the WebSocket
  ws.onEvent(handleWebSocketMessage);
  server.addHandler(&ws);

  // Serve the main HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Start server
  server.begin();
}

void loop() {
  ws.cleanupClients();
}

