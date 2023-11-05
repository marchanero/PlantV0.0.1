#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Pin definitions
const int AirValue = 1024;   // you need to replace this value with Value_1
const int WaterValue = 426; // you need to replace this value with Value_2
const int SensorPin = A0;
int soilMoistureValue = 0;
int soilmoisturepercent = 0;
// LED Pin
const int ledPin = 2;

int measure_time = 5000; // 5 seconds

// Wifi connection variables
#define wifi_ssid "DIGIFIBRA-cF5T"
#define wifi_password "P92sKt3FGfsy"

// MQTT connection variables
#define mqtt_server "192.168.1.43"
#define mqtt_user "root"
#define mqtt_password "orangepi.hass"

#define mqtt_topic "bonsai/soil_moisture"

// functions
void setup_wifi();
void reconnect();
bool checkBound(float newValue, float prevValue, float maxDiff);
void callback(char *topic, byte *payload, unsigned int length);
void ledLoopBlink(int repetitions);

WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
    // Inicializa el LED como salida
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200); // open serial port, set the baud rate to 9600 bps
  Serial.print("*********************************************");
  Serial.println("soil Moisture Sensor Test!"); // prints title with ending line break
  delay(1000);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  Serial.println("Mqtt connected to: ");
  Serial.println(mqtt_server);
  Serial.println("Setup complete.");
  Serial.print("*********************************************");
}

// Variables for loop
long lastMsg = 0;
float prevMoisture = 0;

// threshold for sending a message
float maxDiff = 1.0;

void loop()
{
  digitalWrite(ledPin, HIGH);

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > measure_time)
  {
    soilMoistureValue = analogRead(SensorPin); // put Sensor insert into soil
    Serial.println(soilMoistureValue);
    soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
    if (soilmoisturepercent > 100)
    {
      Serial.println("100 %");
    }
    else if (soilmoisturepercent < 0)
    {
      Serial.println("0 %");
    }
    else if (soilmoisturepercent >= 0 && soilmoisturepercent <= 100)
    {
      Serial.print("Humedad: ");
      Serial.print(soilmoisturepercent);
      Serial.println("%");
    }
    if (checkBound(soilmoisturepercent, prevMoisture, maxDiff))
    {
      prevMoisture = soilmoisturepercent;
      Serial.print("Publish message: ");
      Serial.println(soilmoisturepercent);
      String message = String(soilmoisturepercent);
      client.publish(mqtt_topic, (const uint8_t*)message.c_str(), message.length());
      callback(mqtt_topic,(uint8_t*)message.c_str(), message.length());
      ledLoopBlink(2);
    }
    lastMsg = now;


  }
  delay(5000);
  Serial.println("-------------------------------------------------");
}

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
    digitalWrite(ledPin, LOW);
    delay(250);
    digitalWrite(ledPin, HIGH);

  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  ledLoopBlink(3);
}
void ledLoopBlink(int repetitions){
  for (int i = 0; i < repetitions; i++)
  {
    digitalWrite(ledPin, HIGH);
    delay(250);
    digitalWrite(ledPin, LOW);
    delay(250);
    digitalWrite(ledPin, HIGH);
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool checkBound(float newValue, float prevValue, float maxDiff)
{
  return !isnan(newValue) &&
         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
