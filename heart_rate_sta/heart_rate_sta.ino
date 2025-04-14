#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

const char* ssid = "JioFiber-5AaaE";
const char* password = "@jayanisha12345@";

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

int pulseValue = 0;
int BPM = 0;
bool pulseDetected = false;
unsigned long lastBeatTime = 0;
unsigned long lastSerialPrintTime = 0;

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Handle HTTP requests
  server.on("/", handleRoot);
  server.begin();
  
  Serial.println("HTTP server started");
  Serial.println("WebSocket server started on port 81");
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
  
  // Simple pulse detection algorithm
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
      DynamicJsonDocument doc(1024);
      doc["type"] = "pulse";
      doc["value"] = pulseValue;
      doc["bpm"] = BPM;
      String jsonStr;
      serializeJson(doc, jsonStr);
      webSocket.broadcastTXT(jsonStr);
    }
  } else {
    pulseDetected = false;
    
    // Send continuous data for graph
    if (millis() % 50 == 0) { // Send every 50ms
      DynamicJsonDocument doc(1024);
      doc["type"] = "graph";
      doc["value"] = pulseValue;
      String jsonStr;
      serializeJson(doc, jsonStr);
      webSocket.broadcastTXT(jsonStr);
    }
  }
  
  delay(10);
}

void handleRoot() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Pulse Monitor</title>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 20px; }
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
    #graph-container { width: 100%; max-width: 800px; margin: 20px auto; }
    #graph { width: 100%; height: 200px; border: 1px solid #ccc; background: #f9f9f9; }
    #pulseValue { font-weight: bold; color: #d00; }
    .info { margin: 10px 0; font-size: 1.2em; }
    .ip-address { margin-top: 20px; font-size: 0.9em; color: #666; }
  </style>
</head>
<body>
  <h1>Real-time Pulse Monitoring</h1>
  <div id='pulse'><div id='bpm'>--</div></div>
  <div class='info'>Current Pulse Value: <span id='pulseValue'>0</span></div>
  <div class='info'>Status: <span id='status'>Waiting for data...</span></div>
  
  <div id='graph-container'>
    <canvas id='graph'></canvas>
  </div>
  
  <div class='ip-address' id='ipAddress'></div>
  
  <script>
    // DOM elements
    const bpmElement = document.getElementById('bpm');
    const pulseElement = document.getElementById('pulse');
    const pulseValueElement = document.getElementById('pulseValue');
    const statusElement = document.getElementById('status');
    const canvas = document.getElementById('graph');
    const ctx = canvas.getContext('2d');
    const ipElement = document.getElementById('ipAddress');
    
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
    ipElement.textContent = 'Connected to: ' + location.hostname;
    
    // Graph variables
    const maxPoints = 200;
    let points = [];
    let lastBeatTime = 0;
    
    // WebSocket connection
    const ws = new WebSocket('ws://' + location.hostname + ':81/');
    
    ws.onopen = function() {
      statusElement.textContent = 'Connected to device';
      statusElement.style.color = '#0a0';
    };
    
    ws.onclose = function() {
      statusElement.textContent = 'Disconnected - trying to reconnect...';
      statusElement.style.color = '#a00';
      setTimeout(() => location.reload(), 2000);
    };
    
    ws.onerror = function(error) {
      statusElement.textContent = 'Connection error: ' + error.message;
      statusElement.style.color = '#a00';
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
        DynamicJsonDocument doc(1024);
        doc["type"] = "init";
        doc["bpm"] = BPM;
        doc["value"] = pulseValue;
        String jsonStr;
        serializeJson(doc, jsonStr);
        webSocket.sendTXT(num, jsonStr);
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] Received text: %s\n", num, payload);
      break;
  }
}