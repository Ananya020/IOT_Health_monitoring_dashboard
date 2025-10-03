#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>

// Insert your Wi-Fi credentials
#define WIFI_SSID "Your_WiFi_Name"
#define WIFI_PASSWORD "Your_WiFi_Password"

// Insert Firebase project API Key & Database URL
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "https://your-project-id.firebaseio.com/" 

// Firebase setup
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Hardware pins
#define RELAY_PIN  26   // Change to your relay pin
#define HEART_SENSOR A0 // AD8232 OUT pin to ESP32 ADC

// Thresholds
float TEMP_LOW = 36.5;
float TEMP_HIGH = 37.5;

// Variables
float temperature = 0.0;
int heartRateRaw = 0;
bool heatingOn = false;

// ==================== MAX30205 I2C ====================
#define MAX30205_ADDRESS 0x48

float readTemperature() {
  Wire.beginTransmission(MAX30205_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission(false);
  Wire.requestFrom(MAX30205_ADDRESS, 2);

  uint16_t raw = (Wire.read() << 8) | Wire.read();
  return raw * 0.00390625; // Convert to Celsius
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" Connected!");

  // Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth, &fbdo);
  Firebase.reconnectWiFi(true);
}

// ==================== LOOP ====================
void loop() {
  // Read sensors
  temperature = readTemperature();
  heartRateRaw = analogRead(HEART_SENSOR);

  Serial.printf("Temp: %.2f °C | HR Raw: %d\n", temperature, heartRateRaw);

  // Heating control
  if (temperature < TEMP_LOW && !heatingOn) {
    digitalWrite(RELAY_PIN, HIGH);
    heatingOn = true;
    Firebase.RTDB.setString(&fbdo, "/alerts", "Temperature dropped below 36.5°C, Heating Pad ON");
  } 
  else if (temperature >= TEMP_HIGH && heatingOn) {
    digitalWrite(RELAY_PIN, LOW);
    heatingOn = false;
    Firebase.RTDB.setString(&fbdo, "/alerts", "Temperature reached 37.5°C, Heating Pad OFF");
  }

  // Send sensor data
  Firebase.RTDB.setFloat(&fbdo, "/temperature", temperature);
  Firebase.RTDB.setInt(&fbdo, "/heartRateRaw", heartRateRaw);
  Firebase.RTDB.setBool(&fbdo, "/heatingStatus", heatingOn);

  delay(2000); // Every 2 seconds
}
