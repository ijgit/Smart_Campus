// import library
#include <SPI.h>      // LoRa
#include <RHRouter.h> // LoRa
#include <RH_RF95.h>  // LoRa


#define SERVER_ADDRESS 0
#define CLIENT_ADDRESS 13
#define ROUTER_ADDRESS 50
char GPS_info[] = "36.7707,126.9294,37";
// ===========================================================
// for sleep - power saving

/* Change these values to set the current initial time */
const byte set_seconds = 0;
const byte set_minutes = 00;
const byte set_hours = 17;

/* Change these values to set the current initial date */
const byte set_day = 17;
const byte set_month = 11;
const byte set_year = 15;

// for confirmation
void ISR(){
    digitalWrite(LED_BUILTIN, HIGH);
}

// ===========================================================
// LoRa

 
#define RF95_CS 8
#define RF95_INT 3


RH_RF95 driver(RF95_CS, RF95_INT);
RHRouter manager(driver, CLIENT_ADDRESS);

#define LORA_DATA_LEN 100
uint8_t LoRa_data[LORA_DATA_LEN];

uint8_t LoRa_buf[RH_RF95_MAX_MESSAGE_LEN];

unsigned int LoRa_idx = 0;

String LoRa_sendRst;

void LoRa_setup();
void LoRa_sendMsg();


// PMS7003

int PM1_0 = 0, PM2_5 = 0, PM10 = 0;
uint8_t CMD_readData[] = {0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71};
uint8_t CMD_sleep[] = {0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73};
uint8_t CMD_wakeup[] = {0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74};
uint8_t CMD_passiveMode[] = {0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70};
uint8_t CMD_activeMode[] = {0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71};
uint8_t CMD_len = 7;

char PMS_logData[40];

int PMS_charCount = 0;

#define PMS Serial1

void PMS_setup();
void PMS_writeCMD(uint8_t *cmd);
void PMS_bufFlush();
void PMS_readData();
void PMS_wakeup();
void PMS_sleep();


// ===========================================================
// LM35 - temperature

const int LM35 = A4; // lm35 - temperature sensor
float LM35_logData = 0;
void LM35_readTemp();

// ===========================================================
// Battery
const int BAT = A7; // battery input pin
float BAT_logData = 0;
void BAT_readV();
// ===========================================================

int LOG_IDX = 0;
uint8_t DAY_LOG_IDX = 0;
uint8_t pre_date = 0;
uint8_t cur_date = 0;


void setup(){
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    pinMode(BAT, INPUT);  // BAT setup for reading battery voltage
    pinMode(LM35, INPUT); // LM35 setup for reading temperature
    PMS_setup();
    LoRa_setup();
}

unsigned long timer = 0;

void loop(){
    PMS_wakeup();

    if(millis() - timer> 2*60*1000){
      unsigned long timer = millis();
    
        PMS_writeCMD(CMD_sleep);
        delay(10);
        PMS_bufFlush();
    
        LM35_readTemp();
        BAT_readV();    
        //=====================================
    
        int tmp_LoRaCharCount = 0;
        char tmp_LoRaData[100] = {'\0'};
        LoRa_idx++;
    
        tmp_LoRaCharCount = sprintf(tmp_LoRaData, "%d,%d,%s,%s,%.2f,%.1f", CLIENT_ADDRESS, LoRa_idx, GPS_info, PMS_logData, LM35_logData, BAT_logData);
        //Serial.println(tmp_LoRaData);
    
        for (int i = 0; i < LORA_DATA_LEN; i++)
        {
            if (i < tmp_LoRaCharCount)
                LoRa_data[i] = (uint8_t)(tmp_LoRaData[i]);
            else
                LoRa_data[i] = 0;
        }
        LoRa_sendmsg();
        delay(10);
    
        // board sleep
        delay(1000);
        driver.sleep(); // sleep lora module
    }
}

// ===========================================================
// LoRa function definition

void LoRa_setup()
{

    pinMode(RF95_CS, OUTPUT);

    if (!manager.init()){
        while(1);
    }
    else    ;

    driver.setFrequency(923.0);
    driver.setTxPower(23, false);
    //driver.setCADTimeout(10000);
    manager.setTimeout(3000);
    manager.addRouteTo(SERVER_ADDRESS, SERVER_ADDRESS);
    
}

void LoRa_sendmsg(){
    if (manager.sendtoWait(LoRa_data, sizeof(LoRa_data), SERVER_ADDRESS) == RH_ROUTER_ERROR_NONE){
        // Now wait for a reply from the server
        uint8_t len = sizeof(LoRa_buf);
        uint8_t from;
        Serial.println("success");
        LoRa_sendRst = "success";
    }
    else{
        Serial.println("sendtoWait fail");
        LoRa_sendRst = "sendtoWait failed";
    }
}


// ===========================================================
// PMS function definition

void PMS_setup(){
    PMS.begin(9600);
    delay(1000);
    PMS_writeCMD(CMD_sleep);
}

void PMS_writeCMD(uint8_t *cmd){
    for (int i = 0; i < CMD_len; i++)
        PMS.write(cmd[i]);
}

void PMS_bufFlush(){
    int tmp = 0;
    char c;
    if (tmp = PMS.available()){
        while (tmp--){
            c = PMS.read();
        }
    }
}

void PMS_readData(){
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


void PMS_wakeup(){
    PMS_writeCMD(CMD_wakeup);
    delay(100);
    PMS_bufFlush();
    
    delay(10000);
    PMS_bufFlush();
    PMS_writeCMD(CMD_passiveMode);

    delay(50);
}

void PMS_sleep(){
    PMS_writeCMD(CMD_sleep);
    delay(10);
    PMS_bufFlush();
}

// ===========================================================

void LM35_readTemp(){
    LM35_logData = analogRead(LM35);
    LM35_logData = (LM35_logData * 330) / 1023; // Degree Celsius
}

void BAT_readV(){
    BAT_logData = analogRead(BAT);
    BAT_logData *= 2;
    BAT_logData *= 3.3;
    BAT_logData /= 1024;
}
