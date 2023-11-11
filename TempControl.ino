// LCD
#include <LiquidCrystal.h>
const int rs = 19, en = 23, d4 = 18, d5 = 17, d6 = 16, d7 = 15; // LCD Pins
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// WiFi
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>
// WiFi Details
const char* ssid = "zahra";
const char* password =  "9211559938";

AsyncWebServer server(80);
ESPDash dashboard(&server); 

// Dashboard Cards
Card TemperatureCard(&dashboard, TEMPERATURE_CARD, "Temperature", "°C");
Card HumidityCard(&dashboard, HUMIDITY_CARD, "Humidity", "%");

Card CoolerStatusCard(&dashboard, STATUS_CARD, "Cooler Status", "danger");
Card HeaterStatusCard(&dashboard, STATUS_CARD, "Heater Status", "danger");

Card ManualModeCard(&dashboard, BUTTON_CARD, "Enable/Disable Manual Mode");
Card ManualCoolerCard(&dashboard, BUTTON_CARD, "Turn On/Off Cooler (Manual Mode Only)");
Card ManualHeaterCard(&dashboard, BUTTON_CARD, "Turn On/Off Heater (Manual Mode Only)");

bool ManualModeState = false;
bool ManualCoolerState = false;
bool ManualHeaterState = false;

// DHT22
#include <DHT.h>
#define DHTPIN 14
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Relay Pins
#define CoolerPin 12
#define HeaterPin 13

// Min & Max Temp Values
#define MinTemp 22
#define MaxTemp 27

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
  dht.begin();
  pinMode(CoolerPin, OUTPUT);
  pinMode(HeaterPin, OUTPUT);

  // Server and Dashboard Setup
  WiFi.begin(ssid, password);
   
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..\n");
  }
   
  Serial.println(WiFi.localIP());
  Serial.print("\n");
   
  server.begin();

  CoolerStatusCard.update("Off", "danger");
  HeaterStatusCard.update("Off", "danger");

  // Manual Mode Button Callback
  ManualModeCard.attachCallback([&](int value){
    Serial.println("Manual Mode Button: "+String((value == 1)?"Enabled":"Disabled"));
    Serial.print("\n");
    if (value == 1) {
      ManualModeState = true;
    } else {
      ManualModeState = false;
    }
    ManualModeCard.update(value);
    dashboard.sendUpdates();
  });

  // Manual Cooler Button Callback
  ManualCoolerCard.attachCallback([&](int value){
    if (ManualModeState) {
      Serial.println("Manual Cooler Button: "+String((value == 1)?"Enabled":"Disabled"));
      Serial.print("\n");
      if (value == 1) {
        digitalWrite(CoolerPin, LOW);
        ManualCoolerState = true;
        CoolerStatusCard.update("On", "success");
      } else {
        digitalWrite(CoolerPin, HIGH);
        ManualHeaterState = false;
        CoolerStatusCard.update("Off", "danger");
      }
      ManualCoolerCard.update(value);
      dashboard.sendUpdates();
    }
  });

  // Manual Heater Button Callback
  ManualHeaterCard.attachCallback([&](int value){
    if (ManualModeState) {
      Serial.println("Manual Heater Button: "+String((value == 1)?"Enabled":"Disabled"));
      Serial.print("\n");
      if (value == 1) {
        digitalWrite(HeaterPin, LOW);
        ManualHeaterState = true;
        HeaterStatusCard.update("On", "success");
      } else {
        digitalWrite(HeaterPin, HIGH);
        ManualHeaterState = false;
        HeaterStatusCard.update("Off", "danger");
      }
      ManualHeaterCard.update(value);
      dashboard.sendUpdates();
    }
  });
}

void loop() {
  // Get Temperature and Humidity Values from DHT22
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Update Temperature and Humidity in Server
  TemperatureCard.update(temperature);
  HumidityCard.update(humidity);
  dashboard.sendUpdates();

  // Check if any errors occurred while reading the sensor
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read data from DHT22 sensor.\n");
    return;
  }

  // Print Temperature and Humidity to Serial Monitor
  Serial.print("Hum: ");
  Serial.print(humidity);
  Serial.print("%\t");
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.println("°C");

  // Handle Relays (Cooler and Heater)
  if(!ManualModeState) {
    if(temperature < MinTemp) {
      digitalWrite(HeaterPin, LOW);
      digitalWrite(CoolerPin, HIGH);
      ManualHeaterCard.update(true);
      ManualCoolerCard.update(false);
      ManualHeaterState = true;
      ManualCoolerState = false;
      HeaterStatusCard.update("On", "success");
      CoolerStatusCard.update("Off", "danger");
      Serial.println("Heater: On");
      Serial.println("Cooler: Off");
      Serial.print("\n");
    } else if (temperature > MaxTemp) {
      digitalWrite(HeaterPin, HIGH);
      digitalWrite(CoolerPin, LOW);
      ManualHeaterState = false;
      ManualCoolerState = true;
      ManualCoolerCard.update(true);
      ManualHeaterCard.update(false);
      CoolerStatusCard.update("On", "success");
      HeaterStatusCard.update("Off", "danger");
      Serial.println("Heater: Off");
      Serial.println("Cooler: On");
      Serial.print("\n");
    } else {
      digitalWrite(HeaterPin, HIGH);
      digitalWrite(CoolerPin, HIGH);
      ManualHeaterState = false;
      ManualCoolerState = false;
      ManualHeaterCard.update(false);
      ManualCoolerCard.update(false);
      CoolerStatusCard.update("Off", "danger");
      HeaterStatusCard.update("Off", "danger");
      Serial.println("Heater: Off");
      Serial.println("Cooler: Off");
      Serial.print("\n");
    }
    dashboard.sendUpdates();
  } else {
    Serial.println("Heater: " + String(ManualHeaterState ? "On" : "Off"));
    Serial.println("Cooler: " + String(ManualCoolerState ? "On" : "Off"));
    Serial.print("\n");
  }

  // Print Temperature and Humidity to LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hum: ");
  lcd.print(humidity);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("C");

  delay(1000);
}