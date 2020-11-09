

#include <RTCZero.h>
#include "RTClib.h"

RTC_PCF8523 rtc;

#include <SPI.h> 
#include <RHRouter.h>
#include <RH_RF95.h>


#if defined(ARDUINO_ARCH_SAMD)
#endif


// #define SERVER_ADDRESS 0
// #define CLIENT_ADDRESS 1
// #define ROUTER_ADDRESS 50

#define RF95_CS 8
#define RF95_INT 3
#define RF95_FREQ 915.0

#define LM35 A4 // lm35 - temperature sensor
#define BAT A7

#define PMS Serial1

class LoRaEndNode{
    public:
        LoRaEndNode();
        RH_RF95 driver(RF95_CS, RF95_INT);
        RHRouter manager(driver, clientAddr);

        uint8_t LoRa_buf[RH_RF95_MAX_MESSAGE_LEN];
        unsigned int LoRa_idx = 0;

        void Setup();
        void loop();

    private:
        int serverAddr;
        int clientAddr;
        int routerAddr;


        RTCZero rtcZero;
        /* Change these values to set the current initial time */
        const byte set_seconds = 0;
        const byte set_minutes = 00;
        const byte set_hours = 17;
        /* Change these values to set the current initial date */
        const byte set_day = 17;
        const byte set_month = 11;
        const byte set_year = 15;


        void ISR();

        void LoRa_setup();
        void LoRa_sendMsg();

        
        // PMS
        int PM1_0 = 0, PM2_5 = 0, PM10 = 0;
        uint8_t CMD_readData[] = {0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71};
        uint8_t CMD_sleep[] = {0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73};
        uint8_t CMD_wakeup[] = {0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74};
        uint8_t CMD_passiveMode[] = {0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70};
        uint8_t CMD_activeMode[] = {0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71};
        uint8_t CMD_len = 7;

        char PMS_logData[40];
        int PMS_charCount = 0;
        void PMS_setup();
        void PMS_writeCMD(uint8_t *cmd);
        void PMS_bufFlush();
        void PMS_readData();
        void PMS_print();
        void PMS_wakeup();
        void PMS_sleep();


        // LM35 - temperature
        float LM35_logData = 0;
        void LM35_readTemp();

        // Battery
        float BAT_logData = 0;
        void BAT_readV();

        int LOG_IDX = 0;
        uint8_t DAY_LOG_IDX = 0;
        uint8_t pre_date = 0;
        uint8_t cur_date = 0;
};

void LoRaEndNode::setup(){
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    pinMode(BAT, INPUT);  // BAT setup for reading battery voltage
    pinMode(LM35, INPUT); // LM35 setup for reading temperature

    rtcZero.begin();
    SPI_pinSet();
    PMS_setup();
    LoRa_setup();

    SPI_toLoRa();
    digitalWrite(LED_BUILTIN, LOW);
    
    rtcZero.setTime(set_hours, set_minutes, set_seconds);
    rtcZero.setDate(set_day, set_month, set_year);

    rtcZero.setAlarmMinutes((rtcZero.getAlarmMinutes() + 2) % 60);
    rtcZero.enableAlarm(rtcZero.MATCH_MMSS);
    rtcZero.attachInterrupt(ISR);
    delay(100);
    rtcZero.standbyMode();
}

void LoRaEndNode::loop(){
    PMS_wakeup();
    LM35_readTemp();
    BAT_readV();

    unsigned long pre_time = millis();
    unsigned long sensing_timer = millis();

    while (millis() - pre_time < 30000){
        PMS_readData();

        if (millis() - sensing_timer > 3000){
            sensing_timer = millis();
            PMS_writeCMD(CMD_readData);
        }
    }
    PMS_writeCMD(CMD_sleep);
    delay(10);
    PMS_bufFlush();

    int tmp_LoRaCharCount = 0;
    char tmp_LoRaData[100] = {'\0'};
    LoRa_idx++;

    tmp_LoRaCharCount = sprintf(tmp_LoRaData, "%d,%d,%s,%s,%.2f,%.2f", clientAddr, LoRa_idx, GPS_logData, PMS_logData, LM35_logData, BAT_logData);
    Serial.println(tmp_LoRaData);

    for (int i = 0; i < LORA_DATA_LEN; i++){
        if (i < tmp_LoRaCharCount)
            LoRa_buf[i] = (uint8_t)(tmp_LoRaData[i]);
        else
            LoRa_buf[i] = 0;
    }
    LoRa_sendmsg();
    delay(10);
    delay(1000);
    driver.sleep(); // sleep lora module

    digitalWrite(LED_BUILTIN, LOW);
    rtcZero.setAlarmMinutes((rtcZero.getAlarmMinutes() + 9) % 60);
    rtcZero.enableAlarm(rtcZero.MATCH_MMSS);
    rtcZero.attachInterrupt(ISR);
    delay(100);
    rtcZero.standbyMode();
}

LoRaEndNode::LoRaEndNode(int serverAddr = 0, int routerAddr=50, int clientAddr=1){
    this.clientAddr = clientAddr;
    this.routerAddr = routerAddr;
    this.serverAddr = serverAddr;
}

void LoRaEndNode::ISR(){
    digitalWrite(LED_BUILTIN, HIGH);
}

void LoRaEndNode::LoRa_setup(){
    if (!manager.init())
        Serial.println("init failed");
    else
        Serial.println("init success");

    driver.setFrequency(915.0);
    driver.setTxPower(23, false);
    driver.setCADTimeout(3000);
    manager.setTimeout(3000);
    manager.addRouteTo(serverAddr, serverAddr);
}

void LoRaEndNode::LoRa_sendMsg(){
    if (manager.sendtoWait(LoRa_buf, sizeof(LoRa_buf), serverAddr) == RH_ROUTER_ERROR_NONE){
        // Now wait for a reply from the server
        uint8_t len = sizeof(LoRa_buf);
        uint8_t from;
        LoRa_sendRst = "success";
    }
    else{
        LoRa_sendRst = "sendtoWait failed";
        Serial.println("sendtoWait failed");
    }
}

void LoRaEndNode::PMS_setup(){
    PMS.begin(9600);
    PMS_writeCMD(CMD_sleep);
}

void LoRaEndNode::PMS_writeCMD(){
    for (int i = 0; i < CMD_len; i++)
        PMS.write(cmd[i]);
}
void LoRaEndNode::PMS_bufFlush(){
    int tmp = 0;
    char c;
    if (tmp = PMS.available()){
        while (tmp--){
            c = PMS.read();
        }
    }
}
void LoRaEndNode::PMS_readData(){
unsigned char pms[32];
    if (PMS.available() > 32){
        char c = PMS.read();
        if (c == 0x42){
            c = PMS.read();
            if (c == 0x4D){
                pms[0] = 0x42;
                pms[1] = 0x4D;
                if (PMS.available() > 30){
                    for (int j = 2; j < 32; j++){
                        pms[j] = PMS.read();
                    }
                    PM1_0 = (pms[10] << 8) | pms[11];
                    PM2_5 = (pms[12] << 8) | pms[13];
                    PM10 = (pms[14] << 8) | pms[15];

                    sprintf(PMS_logData, "%d,%d,%d", PM1_0, PM2_5, PM10);
                }
            }
        }
    }
}
void LoRaEndNode::PMS_wakeup(){
    PMS_writeCMD(CMD_wakeup);

    delay(100);
    PMS_bufFlush();

    PMS_writeCMD(CMD_passiveMode);

    PMS_bufFlush();
    delay(50);
}
void LoRaEndNode::PMS_sleep(){
    PMS_writeCMD(CMD_sleep);
    delay(10);
    PMS_bufFlush();    
}

void LoRaEndNode::LM35_readTemp(){
    LM35_logData = analogRead(LM35);
    LM35_logData = (LM35_logData * 500) / 1023; // Degree Celsius

}

void LoRaEndNode::BAT_readV(){
    BAT_logData = analogRead(BAT);
    BAT_logData *= 2;
    BAT_logData *= 3.3;
    BAT_logData /= 1024;

}
