#include "LoRa_endNode.h"

LoRaEndNode node(0, 50, 1);

void setup(){
    node.setup();
}

void loop(){
    node.loop();
}