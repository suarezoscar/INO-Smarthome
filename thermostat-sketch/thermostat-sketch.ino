#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

// Temperature sensor libraries
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// assign a MAC address for the ethernet controller.
byte mac[] = {0x3A, 0x59, 0xE1, 0x0F, 0xAB, 0x1B};

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 31, 100);
IPAddress myDns(192, 168, 31, 1);
// IPAddress server(192, 168, 31, 140);
char serverName[] = "smart-home-be.herokuapp.com";

// initialize the library instance:
EthernetClient client;

const unsigned long getInterval = 10 * 1000; // delay between updates, in milliseconds
const unsigned long putInterval = 10 * 1000; // delay between updates, in milliseconds

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

  Serial.println("Dallas Temperature IC Control Library");
  // Start up the library
  sensors.begin();

  // sets the pin as output
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);

  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      while (true)
      {
        delay(1); // do nothing, no point running without Ethernet hardware
      }
    }
    if (Ethernet.linkStatus() == LinkOFF)
    {
      Serial.println("Ethernet cable is not connected.");
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
    Serial.print("My IP address: ");
    Serial.println(Ethernet.localIP());
  }
  else
  {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);
}

void readJson()
{
  // Allocate memory for Json buffer
  StaticJsonBuffer<512> jsonBuffer;
  JsonObject &root = jsonBuffer.parse(client);

  if (!root.success())
  {
    return;
  }

  boolean newStatus = root["isActive"].as<bool>();

  if (newStatus != isActive)
  {
    isActive = newStatus;

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

  currentTemp = root["currentTemp"];
  triggerTemp = root["triggerTemp"];

  Serial.println(isActive);
  Serial.println(currentTemp);
  Serial.println(triggerTemp);
}

double requestTemperature()
{
  Serial.print(" Requesting temperatures...");
  sensors.requestTemperatures();
  double temperature = sensors.getTempCByIndex(0);
  Serial.print("Temperature is: ");
  Serial.print(temperature);
  Serial.println("");
  return temperature;
}

void loop()
{
  httpRequest();
}

void getThermostatInfo()
{
  if (client.connect(serverName, 80))
  {
    Serial.println("GET REQUEST connecting...");

    client.println("GET / HTTP/1.1");
    client.println("Host: smart-home-be.herokuapp.com");
    // client.println("User-Agent: arduino-ethernet");
    // client.println("Accept: */*");
    // client.println("Content-Type: multipart/form-data");
    // client.println("Content-Length: 25");
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
    Serial.println("PUT REQUEST current temp " + String(tmp, 2));

    client.println("PUT /current/" + String(tmp, 2) + " HTTP/1.1");
    client.println("Host: smart-home-be.herokuapp.com");
    client.println("Content-Type: multipart/form-data");
    client.println("Content-Length: 25\r\n");
    client.println("Accept: */*");
    client.println("");

    // Read all the lines of the reply from server and print them to Serial
    while (client.connected() && !client.available())
      delay(1);

    while (client.connected() || client.available())
    {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }

    Serial.println();
    Serial.println("closing PUT connection");
    client.stop();
  }
}

//CALL ALL HTTP REQUESTS METHODS
void httpRequest()
{
  delay(500); //wait half second before next request
  getThermostatInfo();
  delay(500); //wait half second before next request
  putCurrentTemp(requestTemperature());
}
