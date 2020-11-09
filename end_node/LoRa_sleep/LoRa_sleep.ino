// import library
#include <RTCZero.h>  // for sleep
#include <SPI.h>      // LoRa
#include <RHRouter.h> // LoRa
#include <RH_RF95.h>  // LoRa

#define SERVER_ADDRESS 0
#define CLIENT_ADDRESS 8
#define ROUTER_ADDRESS 50

RTCZero rtcZero;
#define SLEEP_TIME 1
/* Change these values to set the current initial time */
const byte set_seconds = 0;
const byte set_minutes = 00;
const byte set_hours = 17;

/* Change these values to set the current initial date */
const byte set_day = 15;
const byte set_month = 07;
const byte set_year = 20;

// for confirmation
void ISR(){
    digitalWrite(LED_BUILTIN, HIGH);
}


#define RF95_CS 8
#define RF95_INT 3

RH_RF95 driver(RF95_CS, RF95_INT);
RHRouter manager(driver, CLIENT_ADDRESS);


void setup(){
    rtcZero.begin();
    if (!manager.init()){
        while(1);
    }
    else    ;
    driver.setFrequency(923.0);
    driver.setTxPower(23, false);
    manager.setTimeout(3000);
    manager.addRouteTo(SERVER_ADDRESS, SERVER_ADDRESS);
    delay(1000);


    digitalWrite(LED_BUILTIN, LOW);
    rtcZero.setTime(set_hours, set_minutes, set_seconds);
    rtcZero.setDate(set_day, set_month, set_year);


    rtcZero.setAlarmMinutes((rtcZero.getAlarmMinutes() + SLEEP_TIME) % 60);
    rtcZero.enableAlarm(rtcZero.MATCH_MMSS);
    rtcZero.attachInterrupt(ISR);
    delay(100);
    rtcZero.standbyMode();
}

uint8_t data[100] = {'\0',};
uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
unsigned int idx = 0;

void loop(){
    idx++;
    int tmp_LoRaCharCount = 0;
    char tmp_LoRaData[100] = {'\0',};
    tmp_LoRaCharCount = sprintf(tmp_LoRaData, "%d,%d", CLIENT_ADDRESS, idx);
    for (int i = 0; i < 100; i++){
        if (i < tmp_LoRaCharCount)
            data[i] = (uint8_t)(tmp_LoRaData[i]);
        else
            data[i] = 0;
    }

    if (manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS) == RH_ROUTER_ERROR_NONE){
        ;
    }
    else{
      ;
    }
    delay(1000);
    driver.sleep(); // sleep lora module

    //=====================================

    digitalWrite(LED_BUILTIN, LOW);
    rtcZero.setAlarmMinutes((rtcZero.getAlarmMinutes() + SLEEP_TIME) % 60);
    rtcZero.enableAlarm(rtcZero.MATCH_MMSS);
    rtcZero.attachInterrupt(ISR);
    delay(100);
    rtcZero.standbyMode();
}
