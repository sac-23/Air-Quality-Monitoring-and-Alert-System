#include <DHTesp.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <HTTPClient.h>
#include <base64.h>

#define DHT_PIN 18
#define MQ2_PIN 34

const char *ssid = "Your SSID";
const char *password = "Your Password";

long myChannelNumber = 123456;  // Replace with your ThingSpeak Channel Number
const char* myWriteAPIKey = "Your_Write_API_Key";

const String account_sid = "Your Account Sid";
const String auth_token = "Your authToken";
const String twilio_phone_number = "Your_Twilio_Phone_Number";
const String to_phone_number = "Recipient_Phone_Number";

WiFiClient client;
DHTesp DHT;

void setup() {
  Serial.begin(115200);
  DHT.setup(DHT_PIN, DHTesp::DHT11);
  WiFiSetup();
  ThingSpeak.begin(client); 
}

void loop() {
  static unsigned long lastUpdate = 0;
  unsigned long currentMillis = millis();

  if (currentMillis - lastUpdate >= 10000) { 
    lastUpdate = currentMillis; 

    int airQualityLevel = AirQuality();
    TempHumidity data = DHT_Data();
    
    sendDataToThingSpeak(airQualityLevel, data.humidity, data.temperature);

    if (airQualityLevel == 2) { 
       String message = "The air quality is poor. Please take necessary precautions.";
       sendSMS(message);
    }
  }
}


void WiFiSetup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi...");
    unsigned long startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
        
        if (millis() - startTime > 20000) {  // Timeout after 20 seconds
            Serial.println("\nWiFi connection failed! Restarting...");
            ESP.restart();
        }
    }
}

struct TempHumidity {
  float temperature;
  float humidity;
};

TempHumidity DHT_Data() {
  TempAndHumidity data = DHT.getTempAndHumidity();
  TempHumidity result;
  result.temperature = data.temperature;
  result.humidity = data.humidity;
  
  Serial.println("------");
  Serial.print("Humidity: ");
  Serial.println(result.humidity);
  Serial.print("Temperature: ");
  Serial.println(result.temperature);

  return result;
}

int AirQuality() {
  int SoftThreshold = 1000;
  int HardThreshold = 3500;
  int MQ2Value = analogRead(MQ2_PIN);
  int AirQualityLevel;
  
  Serial.print("MQ-2 Sensor Reading:");
  Serial.println(MQ2Value);

  if (MQ2Value < SoftThreshold) {
    Serial.println("------");
    Serial.println("Air Quality is GOOD");
    Serial.println("------");
    AirQualityLevel = 0;
  } else if (MQ2Value < HardThreshold) {
    Serial.println("------");
    Serial.println("Air Quality is MODERATE");
    Serial.println("------");
    AirQualityLevel = 1;
  } else {
    Serial.println("------");
    Serial.println("Air Quality is BAD");
    Serial.println("------");
    AirQualityLevel = 2;
  }
  return AirQualityLevel;
}

void sendSMS(String message) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        String url = "https://api.twilio.com/2010-04-01/Accounts/" + account_sid + "/Messages.json";
        String auth = account_sid + ":" + auth_token;
        String encoded_auth = base64::encode(auth);

        String postData = "To=" + to_phone_number + "&From=" + twilio_phone_number + "&Body=" + message;

        http.begin(url);
        http.addHeader("Authorization", "Basic " + encoded_auth);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        int httpResponseCode = http.POST(postData);

        if (httpResponseCode > 0) {
            Serial.print("SMS Sent! Response Code: ");
            Serial.println(httpResponseCode);
        } else {
            Serial.print("Error sending SMS: ");
            Serial.println(http.errorToString(httpResponseCode));
        }

        http.end();
    } else {
        Serial.println("WiFi not connected!");
    }
}

void sendDataToThingSpeak(float AirQuality, float Humidity, float Temperature) {
    ThingSpeak.setField(1, AirQuality);
    ThingSpeak.setField(2, Humidity);
    ThingSpeak.setField(3, Temperature);

    int responseCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (responseCode == 200) {
        Serial.println(" Data sent to ThingSpeak successfully.");
    } else {
        Serial.print("Error sending data to ThingSpeak. Response code: ");
        Serial.println(responseCode);
    }
}