#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

// assign a MAC address for the ethernet controller.
byte mac[] = {0x3A, 0x59, 0xE1, 0x0F, 0xAB, 0x1B};

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 31, 100);
IPAddress myDns(192, 168, 31, 1);
IPAddress server(192, 168, 31, 140);


// initialize the library instance:
EthernetClient client;

unsigned long lastConnectionTime = 0;            // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 10 * 1000; // delay between updates, in milliseconds

//Declare global variables
boolean isActive ;
double currentTemp;
double triggerTemp ;

void setup()
{
  // start serial port:
  Serial.begin(9600);

  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

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

  JsonObject& root = jsonBuffer.parse(client);

  if (!root.success())
  {    
    return;
  }

  boolean newStatus = root["isActive"].as<bool>();

  if (newStatus != isActive) {

    isActive = newStatus;

    if (isActive) {
      Serial.println("Thermostat set to ON");
      digitalWrite(7, HIGH);
      delay(1000);

    } else {
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

void loop()
{
  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  if (client.available())
  {
    readJson();
  }

  // if ten seconds have passed since your last connection,
  // then connect again and send data:
  if (millis() - lastConnectionTime > postingInterval)
  {
    httpRequest();
  }
}

// this method makes a HTTP connection to the server:
void httpRequest()
{
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();
  delay(1000);

  // if there's a successful connection:
  if (client.connect(server, 3000))
  {
    Serial.println("connecting...");
    // send the HTTP GET request:
    client.println("GET /v1/thermostat/ HTTP/1.1");
    client.println("Host: http://192.168.31.140");
    client.println("User-Agent: arduino-ethernet");
    client.println("Connection: close");
    client.println("");

    // note the time that the connection was made:
    lastConnectionTime = millis();
  }
  else
  {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    delay(5000);
  }
}
