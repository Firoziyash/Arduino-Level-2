/*
 * Pulse Monitoring System with ESP8266 & HW-827 Sensor
 * Developed by: Yash Kumar C (firoziyash.in.net)
 * 
 * Features:
 * - Real-time BPM monitoring with animated visualization
 * - Web-based interface accessible via ESP8266 AP mode
 * - Live pulse waveform graphing
 * - No internet required - standalone operation
 * 
 * Hardware Required:
 * - ESP8266 NodeMCU
 * - HW-827 Pulse Sensor
 * 
 * Part of elecBazaar.shop project collection
 * 
 * Last Updated: 12/04/2025
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// AP Mode Configuration
const char* ap_ssid = "PulseMonitor";
const char* ap_password = "12345678";  // 8-63 characters

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Pulse sensor variables
int pulseValue = 0;
int BPM = 0;
bool pulseDetected = false;
unsigned long lastBeatTime = 0;
unsigned long lastSerialPrintTime = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  
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
  
  // Print to Serial Monitor every 200ms
  if (millis() - lastSerialPrintTime > 200) {
    Serial.print("Pulse Value: ");
    Serial.print(pulseValue);
    Serial.print(" | BPM: ");
    Serial.println(BPM);
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
      
      // Send data to all connected WebSocket clients
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
  <title>Pulse Monitor AP</title>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 20px; background: #f5f5f5; }
    .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
    #pulse { 
      width: 200px; 
      height: 200px; 
      margin: 20px auto; 
      border-radius: 50%; 
      background: #ffcccc; 
      display: flex; 
      align-items: center; 
      justify-content: center; 
      transition: all 0.3s; 
      box-shadow: 0 0 20px rgba(255,0,0,0.3);
    }
    #bpm { font-size: 3em; font-weight: bold; color: #ff0000; }
    #graph-container { width: 100%; margin: 20px auto; }
    #graph { width: 100%; height: 200px; border: 1px solid #ccc; background: #f9f9f9; }
    #pulseValue { font-weight: bold; color: #d00; }
    .info { margin: 10px 0; font-size: 1.2em; }
    .ap-info { margin-top: 20px; padding: 10px; background: #eef; border-radius: 5px; }
    .status { font-weight: bold; }
    .connected { color: green; }
    .disconnected { color: red; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Pulse Monitoring System</h1>
    <div class="ap-info">
      <div>Network: <strong>%AP_SSID%</strong></div>
      <div>IP: <strong>%AP_IP%</strong></div>
    </div>
    
    <div id='pulse'><div id='bpm'>--</div></div>
    <div class='info'>Current Pulse Value: <span id='pulseValue'>0</span></div>
    <div class='info'>Status: <span id='status' class='status'>Waiting for data...</span></div>
    
    <div id='graph-container'>
      <canvas id='graph'></canvas>
    </div>
  </div>
  
  <script>
    // DOM elements
    const bpmElement = document.getElementById('bpm');
    const pulseElement = document.getElementById('pulse');
    const pulseValueElement = document.getElementById('pulseValue');
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
    let lastBeatTime = 0;
    
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
      const scale = 1 + 0.1 * Math.sin(now / 300); // Gentle pulse animation
      pulseElement.style.transform = `scale(${scale})`;
      requestAnimationFrame(animatePulse);
    }
    animatePulse();
    
    // Handle WebSocket messages
    ws.onmessage = function(event) {
      try {
        const data = JSON.parse(event.data);

        // Firoziyash
        if (data.type === 'pulse') {
          bpmElement.textContent = data.bpm;
          pulseValueElement.textContent = data.value;
          
          // Bigger pulse animation when beat detected
          pulseElement.style.transform = 'scale(1.3)';
          pulseElement.style.backgroundColor = '#ff9999';
          setTimeout(() => {
            pulseElement.style.transform = 'scale(1)';
            pulseElement.style.backgroundColor = '#ffcccc';
          }, 200);
          
        } else if (data.type === 'graph') {
          // Add new point to graph
          points.push(data.value);
          if (points.length > maxPoints) points.shift();
          
          // Redraw graph
          ctx.clearRect(0, 0, canvas.width, canvas.height);
          drawGrid();
          ctx.beginPath();
          
          const step = canvas.width / maxPoints;
          const scale = canvas.height / 1024; // Assuming 10-bit ADC (0-1023)
          
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
        // Send initial data to the new client
        sendPulseData();
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] Received text: %s\n", num, payload);
      break;
  }
}