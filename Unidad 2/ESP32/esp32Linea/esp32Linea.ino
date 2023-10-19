#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>


// Define los pines para el sensor DHT11
#define DHTPIN 18
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

const char *ssid = "W_Aula_WB11";
const char *password = "itcolima6";

String serverName = "http://4a30-187-190-35-202.ngrok-free.app";

unsigned long lastTime = 0;
unsigned long timerDelay = 3000;

// Define los pines para los botones
const int buttonPinAsc = 2; // Cambia al número de pin que estás utilizando
const int buttonPinDesc = 4; // Cambia al número de pin que estás utilizando

const int ledPin = 5;

bool ascRequested = false; // Bandera para controlar la solicitud "asc"
bool descRequested = false; // Bandera para controlar la solicitud "desc"

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  // Configura los pines de los botones como entradas
  pinMode(buttonPinAsc, INPUT_PULLUP);
  pinMode(buttonPinDesc, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop() {
  // Verifica si el botón "asc" se ha presionado
  if (digitalRead(buttonPinAsc) == HIGH && !ascRequested) {
    ascRequested = true;
  }

  // Verifica si el botón "desc" se ha presionado
  if (digitalRead(buttonPinDesc) == HIGH && !descRequested) {
    descRequested = true;
  }

  HTTPClient http;

  String serverPathLed = serverName + "/led";

  http.begin(serverPathLed.c_str());

  // Send HTTP GET request
  int httpResponseCodeGET = http.GET();
  
  if (httpResponseCodeGET>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCodeGET);
    String payload = http.getString();
    Serial.println(payload);

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    int led = doc["status"];
    Serial.println(led);

    if (led == 0) {
      digitalWrite(ledPin, LOW);
    }
    if (led == 1) {
      digitalWrite(ledPin, HIGH);
    }
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCodeGET);
  }

  http.end();

  if (ascRequested || descRequested) {
    if ((millis() - lastTime) > timerDelay) {
      if (WiFi.status() == WL_CONNECTED) {
        String serverPath = serverName + "/led";

        http.begin(serverPath.c_str());

        DynamicJsonDocument jsonDoc(64);

        if (ascRequested) {
          jsonDoc["action"] = "asc";
          ascRequested = false; // Restablece la bandera después de la solicitud
        } else if (descRequested) {
          jsonDoc["action"] = "desc";
          descRequested = false; // Restablece la bandera después de la solicitud
        }
        jsonDoc["quantity"] = 1; // Cambia la cantidad según tu necesidad

        String jsonString;
        serializeJson(jsonDoc, jsonString);

        http.addHeader("Content-Type", "application/json");

        int httpResponseCode = http.POST(jsonString);

        if (httpResponseCode > 0) {
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
          String payload = http.getString();
          Serial.println(payload);
        } else {
          Serial.print("Error code: ");
          Serial.println(httpResponseCode);
        }

        http.end();
      } else {
        Serial.println("WiFi Disconnected");
      }
      lastTime = millis();
    }
  }
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(h) && !isnan(t)) {
    // Crear un objeto JSON para enviar los datos de temperatura y humedad
    DynamicJsonDocument dataJson(256);
    dataJson["temperature"] = t;
    dataJson["humidity"] = h;

    // Convertir el objeto JSON en una cadena
    String jsonString;
    serializeJson(dataJson, jsonString);

    // Enviar los datos de temperatura al servidor
    String serverPathTemp = serverName + "/temperature";
    http.begin(serverPathTemp.c_str());
    http.addHeader("Content-Type", "application/json");
    int httpResponseCodeTemp = http.POST(jsonString);

    if (httpResponseCodeTemp > 0) {
      Serial.print("HTTP Response code for temperature: ");
      Serial.println(httpResponseCodeTemp);
    } else {
      Serial.print("Error code for temperature: ");
      Serial.println(httpResponseCodeTemp);
    }

    http.end();

    // Enviar los datos de humedad al servidor
    String serverPathHumidity = serverName + "/humidity";
    http.begin(serverPathHumidity.c_str());
    http.addHeader("Content-Type", "application/json");
    int httpResponseCodeHumidity = http.POST(jsonString);

    if (httpResponseCodeHumidity > 0) {
      Serial.print("HTTP Response code for humidity: ");
      Serial.println(httpResponseCodeHumidity);
    } else {
      Serial.print("Error code for humidity: ");
      Serial.println(httpResponseCodeHumidity);
    }

    http.end();
  } else {
    Serial.println(F("Failed to read from DHT sensor!"));
  }
}
