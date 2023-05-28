#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <U8g2lib.h>

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

WebServer server(80);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 22, /* data=*/ 21);

const int canvasWidth = 128;
const int canvasHeight = 64;

bool canvas[canvasWidth][canvasHeight];

void handleRoot() {
  String html = "<html><body><h1>ESP32 Canvas</h1>";
  html += "<p>Click and drag on the canvas to draw.</p>";
  html += "<button onclick='clearCanvas()'>Clear</button>";
  html += "<canvas id='canvas' width='128' height='64' style='border:1px solid black'></canvas>";
  html += "<script>";
  html += "var canvas = document.getElementById('canvas');";
  html += "var ctx = canvas.getContext('2d');";
  html += "var isDrawing = false;";
  html += "canvas.addEventListener('mousedown', startDrawing);";
  html += "canvas.addEventListener('mousemove', draw);";
  html += "canvas.addEventListener('mouseup', stopDrawing);";
  html += "function startDrawing(e) { isDrawing = true; draw(e); }";
  html += "function stopDrawing() { isDrawing = false; }";
  html += "function draw(e) {";
  html += "  if (!isDrawing) return;";
  html += "  var rect = canvas.getBoundingClientRect();";
  html += "  var x = Math.floor((e.clientX - rect.left) / 2) - 1;";  // Adjust X offset
  html += "  var y = Math.floor((e.clientY - rect.top) / 2) - 1;";  // Adjust Y offset
  html += "  ctx.fillStyle = '#000000';";
  html += "  ctx.fillRect(x, y, 2, 2);";
  html += "  ctx.stroke();";
  html += "  var data = { x: x, y: y };";
  html += "  fetch('/draw', { method: 'POST', body: JSON.stringify(data) });";
  html += "}";
  html += "function clearCanvas() {";
  html += "  ctx.clearRect(0, 0, canvas.width, canvas.height);";
  html += "  fetch('/clear', { method: 'POST' });";
  html += "}";
  html += "</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleDraw() {
  if (server.method() == HTTP_POST) {
    if (server.hasArg("plain")) {
      String json = server.arg("plain");
      if (json.length() > 0) {
        StaticJsonDocument<64> doc;
        DeserializationError error = deserializeJson(doc, json);
        if (!error) {
          int x = doc["x"];
          int y = doc["y"];
          if (x >= 0 && x < canvasWidth && y >= 0 && y < canvasHeight) {
            canvas[x][y] = true;
            display.drawPixel(x, y);
            display.sendBuffer();
          }
        }
      }
    }
  }
  server.send(200, "text/plain", "");
}

void handleClear() {
  clearCanvas();
  server.send(200, "text/plain", "");
}

void clearCanvas() {
  for (int x = 0; x < canvasWidth; ++x) {
    for (int y = 0; y < canvasHeight; ++y) {
      canvas[x][y] = false;
    }
  }
  display.clearBuffer();
  display.sendBuffer();
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  display.begin();
  display.clearBuffer();
  display.sendBuffer();

  server.on("/", handleRoot);
  server.on("/draw", handleDraw);
  server.on("/clear", handleClear);
  server.begin();

  Serial.println("HTTP server started");

  clearCanvas();
}

void loop() {
  server.handleClient();
}
