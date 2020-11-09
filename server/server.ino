//rf95_router_server

#include <RHRouter.h>
#include <RH_RF95.h>
#include <SPI.h>

#define SERVER_ADDRESS 0
#define CLIENT1_ADDRESS 1
#define CLIENT2_ADDRESS 2
#define CLIENT3_ADDRESS 3
#define CLIENT4_ADDRESS 4
#define CLIENT5_ADDRESS 5
#define CLIENT6_ADDRESS 6
#define CLIENT7_ADDRESS 7
#define CLIENT8_ADDRESS 8
#define CLIENT9_ADDRESS 9
#define CLIENT10_ADDRESS 10
#define CLIENT11_ADDRESS 11
#define CLIENT12_ADDRESS 12
#define CLIENT13_ADDRESS 13
#define CLIENT14_ADDRESS 14
#define CLIENT15_ADDRESS 15
#define CLIENT16_ADDRESS 16
#define CLIENT17_ADDRESS 17

#define ROUTER_ADDRESS 50
#define CLIENT51_ADDRESS 51
#define CLIENT52_ADDRESS 52
#define CLIENT53_ADDRESS 53
#define CLIENT54_ADDRESS 54
#define CLIENT55_ADDRESS 55


// Singleton instance of the radio
RH_RF95 driver(8,3);

// Class to manage message delivery and receipt, using the driver declared above
RHRouter manager(driver, SERVER_ADDRESS);

void setup() 
{
  Serial.begin(9600);
  //while (!Serial);
  Serial1.begin(9600);  // uart - raspberry pi (nCube Thyme)
  //while (!Serial1);
  
  if (!manager.init())
    Serial.println("init failed");
  else
    Serial.println("init success");

  driver.setTxPower(23, false);
  driver.setFrequency(923.0);
  driver.setCADTimeout(1000);  // default: 0
  manager.setTimeout(5000); // default: 200ms
  
  // Manually define the routes for this network
  manager.addRouteTo(ROUTER_ADDRESS, ROUTER_ADDRESS);
  
  manager.addRouteTo(CLIENT1_ADDRESS, CLIENT1_ADDRESS);
  manager.addRouteTo(CLIENT2_ADDRESS, CLIENT2_ADDRESS);
  manager.addRouteTo(CLIENT3_ADDRESS, CLIENT3_ADDRESS);
  manager.addRouteTo(CLIENT4_ADDRESS, CLIENT4_ADDRESS);
  manager.addRouteTo(CLIENT5_ADDRESS, CLIENT5_ADDRESS);
  manager.addRouteTo(CLIENT6_ADDRESS, CLIENT6_ADDRESS);
  manager.addRouteTo(CLIENT7_ADDRESS, CLIENT7_ADDRESS);
  manager.addRouteTo(CLIENT8_ADDRESS, CLIENT8_ADDRESS);
  manager.addRouteTo(CLIENT9_ADDRESS, CLIENT9_ADDRESS);
  manager.addRouteTo(CLIENT10_ADDRESS, CLIENT10_ADDRESS);
  manager.addRouteTo(CLIENT11_ADDRESS, CLIENT11_ADDRESS);
  manager.addRouteTo(CLIENT12_ADDRESS, CLIENT12_ADDRESS);
  manager.addRouteTo(CLIENT13_ADDRESS, CLIENT13_ADDRESS);
  manager.addRouteTo(CLIENT14_ADDRESS, CLIENT14_ADDRESS);
  manager.addRouteTo(CLIENT15_ADDRESS, CLIENT15_ADDRESS);
  manager.addRouteTo(CLIENT16_ADDRESS, CLIENT16_ADDRESS);
  manager.addRouteTo(CLIENT17_ADDRESS, CLIENT17_ADDRESS);
  
  manager.addRouteTo(CLIENT51_ADDRESS, ROUTER_ADDRESS);
  manager.addRouteTo(CLIENT52_ADDRESS, ROUTER_ADDRESS);
  manager.addRouteTo(CLIENT53_ADDRESS, ROUTER_ADDRESS);
  manager.addRouteTo(CLIENT54_ADDRESS, ROUTER_ADDRESS);
  manager.addRouteTo(CLIENT55_ADDRESS, ROUTER_ADDRESS);
}

// Dont put this on the stack:
uint8_t buf[RH_ROUTER_MAX_MESSAGE_LEN];

void loop()
{
  uint8_t len = sizeof(buf);
  uint8_t from;
  if (manager.recvfromAck(buf, &len, &from)){
    Serial.print("got request from : 0x");
    Serial.print(from, HEX);
    Serial.print(": ");
    Serial.println((char*)buf);
    Serial1.println((char*)buf);      // talk to Raspberry pi (nCube Thyme)
  }
}
