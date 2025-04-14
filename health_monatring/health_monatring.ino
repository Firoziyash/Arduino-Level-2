/*
 * Health Monitoring System with ESP8266
 * Developed by: Yash Kumar C (firoziyash.in.net)
 * 
 * Features:
 * - Real-time BPM monitoring with animated visualization
 * - Temperature and pressure monitoring (BMP280)
 * - Web-based interface accessible via ESP8266 AP mode
 * - Live pulse waveform graphing
 * - No internet required - standalone operation
 * 
 * Hardware Required:
 * - ESP8266 NodeMCU
 * - HW-827 Pulse Sensor
 * - BMP280 Temperature/Pressure Sensor
 * 
 * Part of elecBazaar.shop project collection
 * 
 * Last Updated: 14/04/2025
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

// AP Mode Configuration
const char* ap_ssid = "HealthMonitor";
const char* ap_password = "12345678";  // 8-63 characters

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
Adafruit_BMP280 bmp; // I2C Interface

// Sensor variables
int pulseValue = 0;
int BPM = 0;
float temperature = 0;
float pressure = 0;
bool pulseDetected = false;
unsigned long lastBeatTime = 0;
unsigned long lastSerialPrintTime = 0;
unsigned long lastEnvReadTime = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  
  // Initialize BMP280
  if (!bmp.begin(0x76)) { // 0x76 or 0x77 depending on your module
    Serial.println("Could not find BMP280 sensor!");
    while (1);
  }
  
  // Set up Access Point
  Serial.println();
  Serial.print("Setting up AP Mode...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  
  Serial.println("AP Mode Activated");
  Serial.print("AP SSID: ");
  Serial.println(ap_ssid);
  Serial.print("AP Password: ");
  Serial.println(ap_password);
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Start WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started on port 81");

  // Handle HTTP requests
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started on port 80");
}

void loop() {
  webSocket.loop();
  server.handleClient();
  
  // Read pulse sensor value
  pulseValue = analogRead(A0);
  
  // Read environmental data every 2 seconds
  if (millis() - lastEnvReadTime > 2000) {
    temperature = bmp.readTemperature();
    pressure = bmp.readPressure() / 100.0F; // Convert to hPa
    lastEnvReadTime = millis();
    sendEnvData();
  }
  
  // Print to Serial Monitor every 200ms
  if (millis() - lastSerialPrintTime > 200) {
    Serial.print("Pulse: ");
    Serial.print(pulseValue);
    Serial.print(" | BPM: ");
    Serial.print(BPM);
    Serial.print(" | Temp: ");
    Serial.print(temperature);
    Serial.print("°C | Pressure: ");
    Serial.print(pressure);
    Serial.println("hPa");
    lastSerialPrintTime = millis();
  }
  
  // Pulse detection algorithm
  if (pulseValue > 600) { // Adjust threshold based on your sensor
    if (!pulseDetected) {
      pulseDetected = true;
      if (lastBeatTime > 0) { // Only calculate BPM if we have a previous beat
        BPM = 60000 / (millis() - lastBeatTime);
        Serial.print("Beat detected! BPM: ");
        Serial.println(BPM);
      }
      lastBeatTime = millis();
      sendPulseData();
    }
  } else {
    pulseDetected = false;
    
    // Send continuous data for graph
    if (millis() % 50 == 0) { // Send every 50ms
      sendGraphData();
    }
  }
  
  delay(10);
}

void sendPulseData() {
  DynamicJsonDocument doc(1024);
  doc["type"] = "pulse";
  doc["value"] = pulseValue;
  doc["bpm"] = BPM;
  String jsonStr;
  serializeJson(doc, jsonStr);
  webSocket.broadcastTXT(jsonStr);
}

void sendEnvData() {
  DynamicJsonDocument doc(1024);
  doc["type"] = "environment";
  doc["temp"] = temperature;
  doc["pressure"] = pressure;
  String jsonStr;
  serializeJson(doc, jsonStr);
  webSocket.broadcastTXT(jsonStr);
}

void sendGraphData() {
  DynamicJsonDocument doc(1024);
  doc["type"] = "graph";
  doc["value"] = pulseValue;
  String jsonStr;
  serializeJson(doc, jsonStr);
  webSocket.broadcastTXT(jsonStr);
}

void handleRoot() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Health Monitor</title>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <style>
    body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f5f5f5; }
    .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
    h1 { text-align: center; color: #333; }
    .sensor-container { display: flex; flex-wrap: wrap; gap: 20px; margin-bottom: 20px; }
    .sensor-card { flex: 1; min-width: 200px; background: #f9f9f9; padding: 15px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
    .sensor-title { font-weight: bold; margin-bottom: 10px; color: #444; }
    .sensor-value { font-size: 1.5em; }
    #bpm-value { color: #e74c3c; }
    #temp-value { color: #3498db; }
    #pressure-value { color: #2ecc71; }
    #pulse { 
      width: 200px; height: 200px; margin: 20px auto; 
      border-radius: 50%; background: #ffcccc; 
      display: flex; align-items: center; justify-content: center; 
      transition: all 0.3s; box-shadow: 0 0 20px rgba(255,0,0,0.3);
    }
    #graph-container { width: 100%; margin-top: 20px; }
    #graph { width: 100%; height: 200px; border: 1px solid #ddd; background: #f9f9f9; }
    .ap-info { margin-top: 20px; padding: 10px; background: #eef; border-radius: 5px; text-align: center; }
    .status { font-weight: bold; margin-top: 10px; }
    .connected { color: #27ae60; }
    .disconnected { color: #e74c3c; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Health Monitoring System</h1>
    <div class="ap-info">
      <div>Network: <strong>%AP_SSID%</strong></div>
      <div>IP: <strong>%AP_IP%</strong></div>
    </div>
    
    <div class="sensor-container">
      <div class="sensor-card">
        <div class="sensor-title">Heart Rate</div>
        <div id="pulse"><div id="bpm-value">--</div></div>
        <div>Pulse Value: <span id="pulse-value">0</span></div>
      </div>
      
      <div class="sensor-card">
        <div class="sensor-title">Temperature</div>
        <div id="temp-value" class="sensor-value">-- °C</div>
      </div>
      
      <div class="sensor-card">
        <div class="sensor-title">Pressure</div>
        <div id="pressure-value" class="sensor-value">-- hPa</div>
      </div>
    </div>
    
    <div id="graph-container">
      <canvas id="graph"></canvas>
    </div>
    
    <div class="status" id="status">Connecting...</div>
  </div>
  
  <script>
    // DOM elements
    const bpmElement = document.getElementById('bpm-value');
    const pulseElement = document.getElementById('pulse');
    const pulseValueElement = document.getElementById('pulse-value');
    const tempElement = document.getElementById('temp-value');
    const pressureElement = document.getElementById('pressure-value');
    const statusElement = document.getElementById('status');
    const canvas = document.getElementById('graph');
    const ctx = canvas.getContext('2d');
    
    // Set canvas size
    function resizeCanvas() {
      canvas.width = canvas.offsetWidth;
      canvas.height = canvas.offsetHeight;
      drawGrid();
    }
    
    // Draw grid background
    function drawGrid() {
      ctx.strokeStyle = '#eee';
      ctx.lineWidth = 1;
      
      // Horizontal lines
      for (let y = 0; y < canvas.height; y += canvas.height / 5) {
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(canvas.width, y);
        ctx.stroke();
      }
      
      // Vertical lines
      for (let x = 0; x < canvas.width; x += canvas.width / 10) {
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, canvas.height);
        ctx.stroke();
      }
    }
    
    // Initial setup
    resizeCanvas();
    window.addEventListener('resize', resizeCanvas);
    
    // Graph variables
    const maxPoints = 200;
    let points = [];
    
    // WebSocket connection
    const ws = new WebSocket('ws://' + location.hostname + ':81/');
    
    ws.onopen = function() {
      statusElement.textContent = 'Connected to device';
      statusElement.className = 'status connected';
    };
    
    ws.onclose = function() {
      statusElement.textContent = 'Disconnected - trying to reconnect...';
      statusElement.className = 'status disconnected';
      setTimeout(() => location.reload(), 2000);
    };
    
    ws.onerror = function(error) {
      statusElement.textContent = 'Connection error';
      statusElement.className = 'status disconnected';
    };
    
    // Animation function
    function animatePulse() {
      const now = Date.now();
      const scale = 1 + 0.1 * Math.sin(now / 300);
      pulseElement.style.transform = `scale(${scale})`;
      requestAnimationFrame(animatePulse);
    }
    animatePulse();
    
    // Handle WebSocket messages
    ws.onmessage = function(event) {
      try {
        const data = JSON.parse(event.data);
        
        if (data.type === 'pulse') {
          bpmElement.textContent = data.bpm;
          pulseValueElement.textContent = data.value;
          
          // Pulse animation
          pulseElement.style.transform = 'scale(1.3)';
          pulseElement.style.backgroundColor = '#ff9999';
          setTimeout(() => {
            pulseElement.style.transform = 'scale(1)';
            pulseElement.style.backgroundColor = '#ffcccc';
          }, 200);
          
        } else if (data.type === 'environment') {
          tempElement.textContent = data.temp.toFixed(1) + " °C";
          pressureElement.textContent = data.pressure.toFixed(1) + " hPa";
          
        } else if (data.type === 'graph') {
          // Add new point to graph
          points.push(data.value);
          if (points.length > maxPoints) points.shift();
          
          // Redraw graph
          ctx.clearRect(0, 0, canvas.width, canvas.height);
          drawGrid();
          ctx.beginPath();
          
          const step = canvas.width / maxPoints;
          const scale = canvas.height / 1024;
          
          points.forEach((point, i) => {
            const x = i * step;
            const y = canvas.height - (point * scale);
            if (i === 0) ctx.moveTo(x, y);
            else ctx.lineTo(x, y);
          });
          
          ctx.strokeStyle = '#ff0000';
          ctx.lineWidth = 2;
          ctx.stroke();
        }
      } catch (e) {
        console.error('Error parsing message:', e);
      }
    };
  </script>
</body>
</html>
)=====";

  // Replace placeholders with actual AP info
  html.replace("%AP_SSID%", ap_ssid);
  html.replace("%AP_IP%", WiFi.softAPIP().toString());
  
  server.send(200, "text/html", html);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Client disconnected\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connection from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
        // Send initial data to new client
        sendPulseData();
        sendEnvData();
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] Received text: %s\n", num, payload);
      break;
  }
}
