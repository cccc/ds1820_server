#include <Ethernet.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include <avr/wdt.h>

#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 9
#define CLIENTID "kuehlschrank"
#define SEC_DELAY_BETWEEN_PUBLISHES 5
#define NUMSENSORS 5

typedef struct {
  DeviceAddress address;
  char topic[64];
} Sensor;

Sensor sensortopics[NUMSENSORS] = {
  {
    {0x28, 0xFF, 0xA1, 0xCD, 0x01, 0x00, 0x00, 0xB9},
    "temp/kuehlschrank/mitte"
  },
  {
    {0x28, 0x71, 0xC6, 0x61, 0x01, 0x00, 0x00, 0xBB},
    "temp/kuehlschrank/oben"
  },
  {
    {0x22, 0xC9, 0xF3, 0x1F, 0x00, 0x00, 0x00, 0xD6},
    "temp/kuehlschrank/unten"
  },
  {
    {0x28, 0x43, 0x89, 0x77, 0x03, 0x00, 0x00, 0xC0},
    "temp/kuehlschrank/gefrier"
  },
  {
    {0x28, 0x08, 0x88, 0x77, 0x03, 0x00, 0x00, 0x80},
    "temp/kuehlschrank/aussen"
  },
};

void callback(char* topic, byte* payload, unsigned int length);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int numberOfDevices;
DeviceAddress tempDeviceAddress;

byte mac[] = {0xAC, 0xDE, 0x48, 0x23, 0x23, 0x05};
byte broker[] = {172, 23, 23, 110};

EthernetClient ethClient;
PubSubClient client(broker, 1883, callback, ethClient);

void setup() {
  Serial.begin(9600);
  
  Serial.println("Configuring ethernet");
  if(Ethernet.begin(mac) == 0) {
    Serial.println("DHCP config failed, resetting board");
    softReset();
  } else {
    Serial.println("DHCP config successful");
    Serial.print("IP: ");
    Serial.println(Ethernet.localIP());
  }
  
  Serial.println("Creating pubsub channel");
  if(!client.connect(CLIENTID)) {
    Serial.println("Broker connection failed, killing myself and rebooting :(");
    softReset();
  }
  
  Serial.println("Enumerating sensors...");
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
      
      Serial.print("Current temperature is ");
      Serial.print(sensors.getTempC(tempDeviceAddress));
      Serial.println(" deg celsius");
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }

    Serial.println("Enumeration done...");
  }

  delay(1000);
}

void loop() { 
  client.loop();
  
  sensors.requestTemperatures();
  
  for(int i = 0; i < numberOfDevices; i++) {
    if(sensors.getAddress(tempDeviceAddress, i)) {
      float temp = sensors.getTempC(tempDeviceAddress);
      uint8_t floatToSend[sizeof(float)];
      memcpy(floatToSend, &temp, sizeof(float));
      
      for(int j = 0; j < NUMSENSORS; j++) {
        if(memcmp(sensortopics[j].address, tempDeviceAddress, sizeof(DeviceAddress)) == 0) {
          client.publish(sensortopics[j].topic, (uint8_t*)floatToSend, sizeof(float));
        }
      }
    }
  }
  
  delay(SEC_DELAY_BETWEEN_PUBLISHES*1000);
}


void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void softReset() {
  wdt_enable(WDTO_15MS);
  for(;;); // LG
}

void callback(char* topic, byte* payload, unsigned int length) {
}

