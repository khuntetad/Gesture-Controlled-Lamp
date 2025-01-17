#include <esp32cam.h>
#include <WiFi.h>
#include <WebServer.h>

// feed in the wifi creds: use hotspot + max compatibility settings
const char* WIFI_SSID = "iPhone (379)";
const char* WIFI_PASS = "KineticGuest";

// use port 80
WebServer server(80);

void handleRoot() {
  server.send(200, "text/plain", "ESP32-CAM Live Stream Server is running.");
}

// create a handler for initiating the mjpg stream
void handleJpgStream() {
  // use the client to connect to the server
  WiFiClient client = server.client();
  if (!client.connected()) {
    Serial.println("Client disconnected before stream started.");
    return;
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println();

  // Stream JPEG frames continuously
  while (client.connected()) {
    // capture the frames fromt esp32
    auto frame = esp32cam::Camera.capture();
    if (!frame) {
      Serial.println("Failed to capture frame!");
      break;
    }

    client.print("--frame\r\n");
    client.print("Content-Type: image/jpeg\r\n");
    client.print("Content-Length: ");
    client.print(frame->size());
    client.print("\r\n\r\n");
    client.write(frame->data(), frame->size());
    client.print("\r\n");

    frame.reset();  // Correct way to release the frame

    delay(30);  // Delay to control the frame rate
  }

  Serial.println("Stream ended.");
}

// Setup function
void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting ESP32-CAM...");

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize the camera
  esp32cam::Config cfg;
  cfg.setPins(esp32cam::pins::AiThinker);       // AiThinker board pin configuration
  cfg.setResolution(esp32cam::Resolution::find(640, 480)); // QVGA resolution
  cfg.setJpeg(8);                               // Lower JPEG quality for performance

  if (!esp32cam::Camera.begin(cfg)) {
    Serial.println("Camera initialization failed!");
    while (true) delay(100);  // Halt if camera fails
  }
  Serial.println("Camera initialized successfully!");

  // setup server endpoints and start the server
  server.on("/", handleRoot);          
  server.on("/stream", handleJpgStream); 
  server.begin();

  Serial.println("Server started.");
  Serial.print("Access the live stream at: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/stream");
}

// Main loop
void loop() {
  server.handleClient();  
}
