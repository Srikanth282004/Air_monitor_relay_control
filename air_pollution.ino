#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT11.h>
#include <MQ2.h>

#define DHT_PIN 2
#define MQ2_PIN A0
#define RELAY_PIN 16  // Output control pin

DHT11 dht11(DHT_PIN);
MQ2 mq2(MQ2_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* ssid = "air";
const char* password = "12345678";

ESP8266WebServer server(80);
float lpg, co, smoke;
int temperature = 0, humidity = 0;

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(0, INPUT_PULLUP);  // Ensure GPIO 0 works properly as an input

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP address: " + WiFi.localIP().toString());

  lcd.clear();
  lcd.print("IP: ");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  mq2.begin();  // Initialize MQ2 Sensor

  server.on("/", HTTP_GET, []() {
    String page = "<html><head><title>IOT Based Air Pollution Monitoring System</title>";
    page += "<style>";
    page += "body {background-image: url('https://wallpaperaccess.com/full/396082.jpg'); ";
    page += "background-size: cover; background-position: center; background-attachment: fixed; ";
    page += "text-align: center; font-family: Arial, sans-serif; color: white;}";
    page += "h1 {background: rgba(0, 0, 0, 0.7); display: inline-block; padding: 10px; border-radius: 10px;}";
    page += "table {margin: auto; border-collapse: collapse; width: 60%; background: rgba(255, 255, 255, 0.8); color: black; border-radius: 10px; overflow: hidden;}";
    page += "th, td {padding: 15px; text-align: center; border: 1px solid black;}";
    page += "th {background-color: #4CAF50; color: white;}";
    page += "td {font-size: 18px; font-weight: bold;}";
    page += "</style>";
    page += "<script>";
    page += "function updateStatus() {";
    page += "var xhttp = new XMLHttpRequest();";
    page += "xhttp.onreadystatechange = function() {";
    page += "if (this.readyState == 4 && this.status == 200) {";
    page += "document.getElementById('statusTable').innerHTML = this.responseText;}};";
    page += "xhttp.open('GET', '/status', true); xhttp.send();}";
    page += "setInterval(updateStatus, 2000);";  // Update every 2 seconds
    page += "</script></head><body>";
    page += "<h1>IOT Based Air Pollution Monitoring System</h1>";
    page += "<table id='statusTable'><tr><th>Parameter</th><th>Status</th></tr></table></body></html>";
    server.send(200, "text/html", page);
  });

  server.on("/status", HTTP_GET, []() {
    String response = "<tr><th>Condition</th><th>Status</th></tr>";
    response += "<tr><td>🌡️ TEMPERATURE</td><td>" + String(temperature) + "°C</td></tr>";
    response += "<tr><td>💧 HUMIDITY</td><td>" + String(humidity) + "%</td></tr>";
    response += "<tr><td>🔥 LPG</td><td>" + String(lpg) + "</td></tr>";
    response += "<tr><td>🚗 CO</td><td>" + String(co) + "</td></tr>";
    response += "<tr><td>💨 Smoke</td><td>" + String(smoke) + "</td></tr>";
    server.send(200, "text/html", response);
  });

  server.begin();
}

void loop() {
  server.handleClient();

  // Read DHT11 Sensor Data
  int result = dht11.readTemperatureHumidity(temperature, humidity);
  if (result != 0) {
    Serial.println("DHT11 Read Failed!");
  }

  // Read MQ2 Sensor Data
  float* values = mq2.read(true);
  lpg = values[0];
  co = values[1];
  smoke = values[2];

  // Display on LCD (prevent flickering by updating only if values change)
  static int lastTemp = -1, lastHum = -1;
  static float lastLpg = -1, lastCo = -1, lastSmoke = -1;

  if (temperature != lastTemp || humidity != lastHum) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: " + String(temperature) + "C");
    lcd.setCursor(0, 1);
    lcd.print("Hum: " + String(humidity) + "%");
    lastTemp = temperature;
    lastHum = humidity;
    delay(2000);
  }

  if (lpg != lastLpg || co != lastCo) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LPG: " + String(lpg));
    lcd.setCursor(0, 1);
    lcd.print("CO: " + String(co));
    lastLpg = lpg;
    lastCo = co;
    delay(2000);
  }

  if (smoke != lastSmoke) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Smoke: " + String(smoke));
    lastSmoke = smoke;
    delay(2000);
  }

  // Control Relay Based on Button Press (GPIO 0)
  if (digitalRead(0) == LOW) {  // Button pressed (Pulled LOW)
    digitalWrite(RELAY_PIN, HIGH);  // Turn on relay
    Serial.println("Relay ON");
  } else {
    digitalWrite(RELAY_PIN, LOW);  // Turn off relay
    Serial.println("Relay OFF");
  }
}
