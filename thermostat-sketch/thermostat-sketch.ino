#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

// Temperature sensor libraries
#include <OneWire.h>
#include <DallasTemperature.h>

#define GET_TEMP 0
#define SEND_TEMP 1
#define GET_STATUS 2
#define SET_RELAY_STATE 3
int state = 0;

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// assign a MAC address for the ethernet controller.
byte mac[] = {0x3A, 0x59, 0xE1, 0x0F, 0xAB, 0x1B};

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 31, 100);
IPAddress myDns(192, 168, 31, 1);

// Set api server path
char serverName[] = "smart-home-be.herokuapp.com";

// initialize the library instance:
EthernetClient client;

//Declare global variables
boolean isActive;
double currentTemp;
double triggerTemp;

void setup()
{
  // start serial port:
  Serial.begin(9600);

  // Wait for serial to connect
  delay(5000);

  // Start up the library
  sensors.begin();

  // Sets the pin as output
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);

  // Start the Ethernet connection:
  Serial.println("Initializing Ethernet...");
  if (Ethernet.begin(mac) == 0)
  {
    delay(2000);
    Ethernet.begin(mac, ip, myDns);
    Serial.print("My IP address: ");
    Serial.println(Ethernet.localIP());
    delay(2000);  }
}

void readJson()
{
  // Allocate memory for Json buffer
  const size_t bufferSize = JSON_OBJECT_SIZE(5) + 60;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject &root = jsonBuffer.parse(client);

  if (!root.success())
  {
    return;
  }

  isActive = root["isActive"].as<bool>();
  currentTemp = root["currentTemp"];
  triggerTemp = root["triggerTemp"];
}

double requestTemperature()
{
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();
  double temperature = sensors.getTempCByIndex(0);
  Serial.print("Temperature is: ");
  Serial.print(temperature);
  Serial.println("");
  return temperature;
}

void setRelayState()
{
  if (isActive)
  {
    Serial.println("Thermostat set to ON");
    digitalWrite(7, HIGH);
    delay(1000);
  }
  else
  {
    Serial.println("Thermostat set to OFF");
    digitalWrite(7, LOW);
    delay(1000);
  }
}

void getThermostatInfo()
{
  if (client.connect(serverName, 80))
  {
    Serial.println("Getting thermostat info...");

    client.println("GET / HTTP/1.1");
    client.println("Host: smart-home-be.herokuapp.com");
    client.println();

    while (client.connected() && !client.available())
      delay(1);

    while (client.connected() || client.available())
      readJson();

    Serial.println("closing GET connection");
    client.stop();
  }
}

void putCurrentTemp(double tmp)
{
  if (client.connect(serverName, 80))
  {
    Serial.println("Sending new temperature: " + String(tmp, 2));

    client.println("PUT /current/" + String(tmp, 2) + " HTTP/1.1");
    client.println("Host: smart-home-be.herokuapp.com");
    client.println("Content-Type: multipart/form-data");
    client.println("Content-Length: 25\r\n");
    client.println("Accept: */*");
    client.println("");

    while (client.connected() && !client.available())
      delay(1);

    while (client.connected() || client.available())
      String line = client.readStringUntil('\r');

    Serial.println();
    Serial.println("closing PUT connection");
    client.stop();
  }
}

void loop()
{
  // Wait five seconds before next iteration
  delay(5000);

  if (state == GET_TEMP)
  {
    currentTemp = requestTemperature();
    state = SEND_TEMP;
  }
  else if (state == SEND_TEMP)
  {
    putCurrentTemp(currentTemp);
    state = GET_STATUS;
  }
  else if (state == GET_STATUS)
  {
    getThermostatInfo();
    state = SET_RELAY_STATE;
  }
  else if (state == SET_RELAY_STATE)
  {
    setRelayState();
    state = GET_TEMP;
  }
}
