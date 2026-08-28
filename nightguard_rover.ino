/*
  ============================================================
                       NIGHTGUARD
            Urban Night Safety Inspection Rover
  ============================================================

  BOARD:
    AI-Thinker ESP32-CAM

  HARDWARE:
    - AI-Thinker ESP32-CAM
    - NEO-6M GPS
    - IR illumination
    - FlySky FS-i6X (manual rover control)

  FEATURES:
    - High-resolution live camera stream
    - Maximum practical OV2640 resolution: UXGA 1600x1200
    - NEO-6M GPS location
    - Satellite count
    - IR light ON/OFF from webpage
    - Google Maps location button
    - ESP32 creates its own Wi-Fi hotspot
    - Clean webpage without decorative images

  IMPORTANT:
    Standard AI-Thinker ESP32-CAM / OV2640 cannot produce
    true 4K video. UXGA 1600x1200 is used here.

  WIFI:
    SSID:     YOUR_WIFI_NAME
    PASSWORD: YOUR_WIFI_PASSWORD
*/

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <TinyGPSPlus.h>

// ============================================================
// WIFI ACCESS POINT
// ============================================================

const char* AP_SSID = "YOUR_WIFI_NAME";
const char* AP_PASSWORD = "YOUR_WIFI_PASSWORD";
// ============================================================
// WEB SERVER
// ============================================================

WebServer server(80);

// ============================================================
// GPS
// ============================================================

TinyGPSPlus gps;

#define GPS_RX_PIN 13
#define GPS_TX_PIN 14

HardwareSerial GPS_Serial(2);

// ============================================================
// IR LIGHT
// ============================================================

#define IR_LED_PIN 4

bool irState = false;

// ============================================================
// AI-THINKER ESP32-CAM CAMERA PINS
// ============================================================

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ============================================================
// GPS VARIABLES
// ============================================================

double latitude = 0.0;
double longitude = 0.0;

int satellites = 0;

bool gpsValid = false;

String gpsStatus = "WAITING FOR GPS";

// ============================================================
// UPDATE GPS
// ============================================================

void updateGPS()
{
  while (GPS_Serial.available())
  {
    gps.encode(GPS_Serial.read());
  }

  if (gps.location.isValid())
  {
    latitude = gps.location.lat();
    longitude = gps.location.lng();

    if (gps.satellites.isValid())
    {
      satellites = gps.satellites.value();
    }
    else
    {
      satellites = 0;
    }

    gpsValid = true;
    gpsStatus = "GPS FIXED";
  }
  else
  {
    gpsValid = false;
    gpsStatus = "SEARCHING FOR GPS";
  }
}

// ============================================================
// CAMERA INITIALIZATION
// ============================================================

bool startCamera()
{
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;

  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;

  config.pixel_format = PIXFORMAT_JPEG;

  /*
     Maximum practical resolution of OV2640:
     UXGA = 1600 x 1200

     We use PSRAM because high-resolution streaming
     requires the additional frame buffer memory.
  */

  if (psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA;

    /*
       Lower number = higher JPEG quality.
       8 gives a relatively high-quality image.
    */
    config.jpeg_quality = 8;

    /*
       Two frame buffers improve streaming performance.
    */
    config.fb_count = 2;
  }
  else
  {
    /*
       UXGA with no PSRAM can cause memory problems.
       Fall back to SVGA.
    */

    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK)
  {
    Serial.printf(
      "Camera initialization failed: 0x%x\n",
      err
    );

    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();

  if (sensor != NULL)
  {
    /*
       Slight brightness increase for night inspection.
    */
    sensor->set_brightness(sensor, 1);

    sensor->set_contrast(sensor, 0);

    sensor->set_saturation(sensor, 0);
  }

  return true;
}

// ============================================================
// WEBPAGE
// ============================================================

void handleRoot()
{
  String html = R"rawliteral(
<!DOCTYPE html>

<html>

<head>

<meta name="viewport"
      content="width=device-width, initial-scale=1">

<title>NIGHTGUARD</title>

<style>

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  font-family: Arial, sans-serif;
  background: #0b0b0b;
  color: white;
}

.header {
  text-align: center;
  padding: 20px;
  background: #151515;
}

.header h1 {
  margin: 0;
  font-size: 30px;
}

.header p {
  margin: 8px 0 0 0;
  color: #aaaaaa;
}

.container {
  max-width: 1100px;
  margin: auto;
  padding: 15px;
}

.card {
  background: #171717;
  border-radius: 12px;
  padding: 18px;
  margin-bottom: 15px;
}

.camera {
  width: 100%;
  display: block;
  border-radius: 8px;
}

.title {
  font-size: 20px;
  margin-bottom: 12px;
}

.data {
  font-size: 18px;
  margin: 8px 0;
}

.status {
  font-size: 20px;
  font-weight: bold;
}

button {
  border: none;
  border-radius: 8px;
  padding: 13px 20px;
  margin: 5px;
  font-size: 16px;
  cursor: pointer;
}

.irOn {
  background: #16803c;
  color: white;
}

.irOff {
  background: #b91c1c;
  color: white;
}

.mapButton {
  background: #2563eb;
  color: white;
}

.footer {
  text-align: center;
  padding: 20px;
  color: #777777;
}

</style>

</head>

<body>

<div class="header">

<h1>NIGHTGUARD</h1>

<p>Urban Night Safety Inspection Rover</p>

</div>

<div class="container">

<div class="card">

<div class="title">
LIVE CAMERA
</div>

<img class="camera" src="/stream">

</div>


<div class="card">

<div class="title">
GPS STATUS
</div>

<div id="gpsStatus" class="status">
WAITING FOR GPS
</div>

</div>


<div class="card">

<div class="title">
GPS LOCATION
</div>

<div id="location" class="data">
Waiting for GPS fix...
</div>

<div class="data">
Satellites:
<span id="satellites">0</span>
</div>

<button class="mapButton"
        onclick="openMap()">

OPEN LOCATION ON MAP

</button>

</div>


<div class="card">

<div class="title">
IR ILLUMINATION
</div>

<div id="irStatus" class="status">
OFF
</div>

<br>

<button class="irOn"
        onclick="controlIR('on')">

TURN IR ON

</button>

<button class="irOff"
        onclick="controlIR('off')">

TURN IR OFF

</button>

</div>


<div class="card">

<div class="title">
ROVER CONTROL
</div>

<p>
Rover movement is manually controlled using
the FlySky FS-i6X transmitter.
</p>

</div>

</div>


<div class="footer">

NIGHTGUARD

</div>


<script>

let currentLat = 0;
let currentLng = 0;


// ============================================================
// UPDATE GPS
// ============================================================

function updateGPS()
{
  fetch('/gps')

  .then(response => response.json())

  .then(data =>
  {
    currentLat = data.latitude;
    currentLng = data.longitude;

    document.getElementById("gpsStatus").innerHTML =
      data.status;

    document.getElementById("satellites").innerHTML =
      data.satellites;

    if (data.valid)
    {
      document.getElementById("location").innerHTML =
        data.latitude.toFixed(6) +
        " , " +
        data.longitude.toFixed(6);
    }
    else
    {
      document.getElementById("location").innerHTML =
        "Waiting for GPS fix...";
    }
  })

  .catch(error =>
  {
    console.log(error);
  });
}


// ============================================================
// IR CONTROL
// ============================================================

function controlIR(state)
{
  fetch('/ir?state=' + state)

  .then(response => response.text())

  .then(data =>
  {
    document.getElementById("irStatus").innerHTML =
      data;
  });
}


// ============================================================
// GOOGLE MAPS
// ============================================================

function openMap()
{
  if (currentLat == 0 && currentLng == 0)
  {
    alert("GPS location is not available yet.");
    return;
  }

  let url =
    "https://www.google.com/maps?q=" +
    currentLat +
    "," +
    currentLng;

  window.open(url, "_blank");
}


// ============================================================
// GPS UPDATE EVERY 2 SECONDS
// ============================================================

setInterval(updateGPS, 2000);

updateGPS();

</script>

</body>

</html>

)rawliteral";

  server.send(
    200,
    "text/html",
    html
  );
}

// ============================================================
// GPS API
// ============================================================

void handleGPS()
{
  String json = "{";

  json += "\"valid\":";
  json += gpsValid ? "true" : "false";

  json += ",";

  json += "\"latitude\":";
  json += String(latitude, 6);

  json += ",";

  json += "\"longitude\":";
  json += String(longitude, 6);

  json += ",";

  json += "\"satellites\":";
  json += String(satellites);

  json += ",";

  json += "\"status\":\"";
  json += gpsStatus;
  json += "\"";

  json += "}";

  server.send(
    200,
    "application/json",
    json
  );
}

// ============================================================
// IR CONTROL API
// ============================================================

void handleIR()
{
  if (!server.hasArg("state"))
  {
    server.send(
      400,
      "text/plain",
      "Missing state"
    );

    return;
  }

  String state =
    server.arg("state");

  if (state == "on")
  {
    irState = true;

    digitalWrite(
      IR_LED_PIN,
      HIGH
    );

    server.send(
      200,
      "text/plain",
      "ON"
    );
  }
  else
  {
    irState = false;

    digitalWrite(
      IR_LED_PIN,
      LOW
    );

    server.send(
      200,
      "text/plain",
      "OFF"
    );
  }
}

// ============================================================
// CAMERA STREAM
// ============================================================

void handleStream()
{
  WiFiClient client =
    server.client();

  String response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
    "Cache-Control: no-cache\r\n"
    "Pragma: no-cache\r\n"
    "Connection: close\r\n\r\n";

  client.print(response);

  while (client.connected())
  {
    camera_fb_t *fb =
      esp_camera_fb_get();

    if (!fb)
    {
      Serial.println(
        "Camera capture failed"
      );

      break;
    }

    client.printf(
      "--frame\r\n"
      "Content-Type: image/jpeg\r\n"
      "Content-Length: %u\r\n\r\n",
      fb->len
    );

    client.write(
      fb->buf,
      fb->len
    );

    client.print("\r\n");

    esp_camera_fb_return(fb);

    /*
       Small delay prevents the ESP32 from being
       overloaded while streaming.
    */

    delay(30);
  }
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println(
    "=========================================="
  );

  Serial.println(
    "          NIGHTGUARD STARTING"
  );

  Serial.println(
    "=========================================="
  );


  // ==========================================================
  // IR
  // ==========================================================

  pinMode(
    IR_LED_PIN,
    OUTPUT
  );

  digitalWrite(
    IR_LED_PIN,
    LOW
  );


  // ==========================================================
  // CAMERA
  // ==========================================================

  Serial.println(
    "Starting camera..."
  );

  if (!startCamera())
  {
    Serial.println(
      "CAMERA INITIALIZATION FAILED"
    );

    while (true)
    {
      delay(1000);
    }
  }

  Serial.println(
    "Camera initialized successfully."
  );


  // ==========================================================
  // GPS
  // ==========================================================

  Serial.println(
    "Starting GPS..."
  );

  GPS_Serial.begin(
    9600,
    SERIAL_8N1,
    GPS_RX_PIN,
    GPS_TX_PIN
  );

  Serial.println(
    "GPS initialized."
  );


  // ==========================================================
  // WIFI
  // ==========================================================

  Serial.println(
    "Starting NIGHTGUARD Wi-Fi..."
  );

  WiFi.mode(WIFI_AP);

  WiFi.softAP(
    AP_SSID,
    AP_PASSWORD
  );

  IPAddress IP =
    WiFi.softAPIP();


  Serial.println();

  Serial.println(
    "=========================================="
  );

  Serial.println(
    "          NIGHTGUARD READY"
  );

  Serial.println(
    "=========================================="
  );

  Serial.print(
    "Wi-Fi Name: "
  );

  Serial.println(
    AP_SSID
  );

  Serial.print(
    "Wi-Fi Password: "
  );

  Serial.println(
    AP_PASSWORD
  );

  Serial.print(
    "Webpage: http://"
  );

  Serial.println(
    IP
  );

  Serial.println(
    "=========================================="
  );


  // ==========================================================
  // WEB SERVER ROUTES
  // ==========================================================

  server.on(
    "/",
    HTTP_GET,
    handleRoot
  );

  server.on(
    "/gps",
    HTTP_GET,
    handleGPS
  );

  server.on(
    "/ir",
    HTTP_GET,
    handleIR
  );

  server.on(
    "/stream",
    HTTP_GET,
    handleStream
  );


  server.begin();

  Serial.println(
    "Web server started."
  );
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
  updateGPS();

  server.handleClient();

  delay(2);
}