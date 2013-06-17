#include <Ethernet.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/wdt.h>

#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 9

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int numberOfDevices;
DeviceAddress tempDeviceAddress;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(172,23,23,211);
IPAddress gateway(172,23,23,1); 
IPAddress subnet(255, 255, 254, 0);

EthernetServer server(80);

void setup() {
  Serial.begin(9600);
  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();
  for(int i=0;i<numberOfDevices; i++) {
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i)) {
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
      
      Serial.print("Setting resolution to ");
      Serial.println(TEMPERATURE_PRECISION, DEC);
      sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
      
      Serial.print("Resolution actually set to: ");
      Serial.print(sensors.getResolution(tempDeviceAddress), DEC); 
      Serial.println();
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }

  Ethernet.begin(mac, ip);
  server.begin();
  delay(1000);
}

void loop() { 
  listenForEthernetClients();
}


void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}


void listenForEthernetClients() {
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Got a client");
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/json");
          client.println();
          sensors.requestTemperatures();
          client.print("{\"sensors\": [");
          // Loop through each device, print out temperature data
          for(int i=0;i<numberOfDevices; i++) {
            // Search the wire for address
            if(sensors.getAddress(tempDeviceAddress, i)) {
              client.print("{\"device_id\": \"");
              for (uint8_t i = 0; i < 8; i++) {
                if (tempDeviceAddress[i] < 16) Serial.print("0");
                client.print(tempDeviceAddress[i], HEX);
              }
              client.print("\", \"temperature_c\": ");
              float tempC = sensors.getTempC(tempDeviceAddress);
              if(tempC == 85) {
                client.print("NaN}], \"error\": true, \"restarting\": true}");
                softReset();
                break;
              }
              client.print(tempC);
              client.print("}");
              if(numberOfDevices-i != 1)
                client.print(",");
            } 
          }
          client.println("]}");
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop();
  }
} 

void softReset() {
  wdt_enable(WDTO_15MS);
  for(;;); // LG
}

