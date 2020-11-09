// ===========================================================
// import library

#include <RTCZero.h> // for sleep
#include "RTClib.h"

RTC_PCF8523 rtc;

#include <SPI.h> // SD

#include <RHRouter.h> // LoRa
#include <RH_RF95.h>            // LoRa

#include <SD.h> // SD and LoRa

#include <Adafruit_GPS.h> // GPS

// SERCOM-expand Serial
#include <Arduino.h>        // required before wiring_private.h
#include "wiring_private.h" // pinPeripheral() function

#if defined(ARDUINO_ARCH_SAMD)
#endif

// ===========================================================
// for sleep - power saving

RTCZero rtcZero;
/* Change these values to set the current initial time */
const byte set_seconds = 0;
const byte set_minutes = 00;
const byte set_hours = 17;

/* Change these values to set the current initial date */
const byte set_day = 17;
const byte set_month = 11;
const byte set_year = 15;

// for confirmation
void ISR()
{
    digitalWrite(LED_BUILTIN, HIGH);
}

// ===========================================================

// ===========================================================
// SD logger
const int SD_CS = 10;
String fileName = "log1.csv";

// log sensed data
void SD_log();
void SD_setup();

// ===========================================================
// LoRa

#define SERVER_ADDRESS 0
#define CLIENT_ADDRESS 1
#define ROUTER_ADDRESS 50


#define RF95_CS 8
#define RF95_INT 3

#define RF95_FREQ 915.0

RH_RF95 driver(RF95_CS, RF95_INT);
RHRouter manager(driver, CLIENT_ADDRESS);

#define LORA_DATA_LEN 100
uint8_t LoRa_data[LORA_DATA_LEN];

uint8_t LoRa_buf[RH_RF95_MAX_MESSAGE_LEN];

unsigned int LoRa_idx = 0;

String LoRa_sendRst;

void LoRa_setup();
void LoRa_sendMsg();

// ===========================================================
// for usning SD & LoRa at the same time
//    adaloger wing and lora feather wing use SPI
//    in SPI  ( CS low = turn on )
//        ( CS high = turn off )
void SPI_toLoRa();
void SPI_toSD();

// ===========================================================
// for GPS
#define GPSSerial Serial1
#define GPSECHO false

Adafruit_GPS GPS(&GPSSerial);

const int GPS_EN = 12; // when pullup GPS_EN pin, GPS sleep

int GPS_charCount = 0;

char GPS_logData[40];

char GPS_logDateData[30];


void GPS_setup();
void GPS_sleep();
void GPS_wakeup();

// ===========================================================
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

Uart Serial2(&sercom5, A5, 6, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM5_Handler()
{
    Serial2.IrqHandler();
}

#define PMS Serial2

void PMS_setup();
void PMS_writeCMD(uint8_t *cmd);
void PMS_bufFlush();
void PMS_readData();
void PMS_print();
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

void setup()
{

    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    pinMode(BAT, INPUT);  // BAT setup for reading battery voltage
    pinMode(LM35, INPUT); // LM35 setup for reading temperature

    rtcZero.begin();

    if (!rtc.begin()){
        Serial.println("Couldn't find rtc");
        while (1);
    }

    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    if (!rtc.initialized()){
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        // rtc.adjust(DateTime(2020, 1, 16, 10, 45, 50));  
    }
    
    DateTime now = rtc.now();
    pre_date = now.day();

    SPI_pinSet();

    SD_setup();
    PMS_setup();
    LoRa_setup();
    GPS_setup();
    GPS_wakeup();
    // GPS_sleep();

    int gps_fix = 0;
    while (gps_fix == 0){

        GPS.read();
        if (GPS.newNMEAreceived()){
            GPS.parse(GPS.lastNMEA());
            if (GPS.fix){
                GPS_charCount = sprintf(GPS_logData, "%.4f,%.4f,%.2f", GPS.latitudeDegrees, GPS.longitudeDegrees, GPS.altitude);
                gps_fix = 1;
                GPS_sleep();
            }
        }
    }
    SPI_toLoRa();

    digitalWrite(LED_BUILTIN, LOW);

    rtcZero.setTime(set_hours, set_minutes, set_seconds);
    rtcZero.setDate(set_day, set_month, set_year);

    rtcZero.setAlarmMinutes((rtcZero.getAlarmMinutes() + 9) % 60);
    rtcZero.enableAlarm(rtcZero.MATCH_MMSS);
    rtcZero.attachInterrupt(ISR);
    delay(100);
    rtcZero.standbyMode();
}

File datafile;


void loop(){
    GPS_sleep();
    PMS_wakeup();
    
    LM35_readTemp();
    BAT_readV();

    unsigned long pre_time = millis();
    unsigned long sensing_timer = millis();


    while (millis() - pre_time < 30000){
        PMS_readData();

        if (millis() - sensing_timer > 5000)
        {
            sensing_timer = millis();
            PMS_writeCMD(CMD_readData);
        }
    }

    PMS_writeCMD(CMD_sleep);
    delay(10);
    PMS_bufFlush();

    //=====================================
    char RTC_logData[] = "YY/MM/DD,hh:mm:ss";
    DateTime now = rtc.now();


    
    cur_date = now.day();
    if(cur_date != pre_date){
        DAY_LOG_IDX = 0;
        pre_date = cur_date;    
    }
    DAY_LOG_IDX++;
    


    int tmp_LoRaCharCount = 0;
    char tmp_LoRaData[100] = {'\0'};
    LoRa_idx++;

    tmp_LoRaCharCount = sprintf(tmp_LoRaData, "%d,%d,%d,%s,%s,%s,%.2f,%.1f", CLIENT_ADDRESS, LoRa_idx, DAY_LOG_IDX, now.toString(RTC_logData), GPS_logData, PMS_logData, LM35_logData, BAT_logData);
    Serial.println(tmp_LoRaData);

    for (int i = 0; i < LORA_DATA_LEN; i++)
    {
        if (i < tmp_LoRaCharCount)
            LoRa_data[i] = (uint8_t)(tmp_LoRaData[i]);
        else
            LoRa_data[i] = 0;
    }
    LoRa_sendmsg();
    delay(10);

    //=====================================
    SPI_toSD();
    datafile = SD.open(fileName, FILE_WRITE);
    if (datafile){
        String log = "idx: " + String(++LOG_IDX);
        datafile.println(log);
        datafile.print(DAY_LOG_IDX);
        datafile.print(',');
        datafile.print(now.toString(RTC_logData));
        datafile.print(',');
        datafile.print(GPS_logData);
        datafile.print(',');
        datafile.print(PMS_logData);
        datafile.print(',');
        datafile.print(LM35_logData, 2);
        datafile.print(',');
        datafile.println(BAT_logData, 1);
        datafile.println(LoRa_sendRst);
        datafile.print("\n\n");
        datafile.close();
    }

    SPI_toLoRa();
    // board sleep
    delay(1000);
    driver.sleep(); // sleep lora module

    digitalWrite(LED_BUILTIN, LOW);

    rtcZero.setAlarmMinutes((rtcZero.getAlarmMinutes() + 9) % 60);
    rtcZero.enableAlarm(rtcZero.MATCH_MMSS);
    rtcZero.attachInterrupt(ISR);
    delay(100);
    rtcZero.standbyMode();
}

// ===========================================================

// ===========================================================

// ===========================================================

void SPI_pinSet()
{
    pinMode(SD_CS, OUTPUT);
    pinMode(RF95_CS, OUTPUT);
}

// ===========================================================
// SD logger function definition
void SD_setup()
{
    SPI_toSD();

    if (!SD.begin(SD_CS))
    {
        Serial.println("Card failed, or not present");
        // don't do anything more:
        //while (1);
    }
    Serial.println("card initialized.");
}

void SD_log()
{

    datafile = SD.open(fileName, FILE_WRITE);
    if (datafile)
    {
        datafile.print(String(++LOG_IDX));
        //datafile.println(log_data);
        datafile.close();
    }
    else
    {
        Serial.println("error opening file");
    }
}

// ===========================================================
// LoRa function definition

void LoRa_setup()
{

    SPI_toLoRa();

    if (!manager.init())
        Serial.println("init failed");
    else
        Serial.println("init success");

    driver.setFrequency(915.0);
    driver.setTxPower(23, false);
    driver.setCADTimeout(10000);
    manager.setTimeout(3000);
    manager.addRouteTo(SERVER_ADDRESS, SERVER_ADDRESS);
    
}

void LoRa_sendmsg(){
    if (manager.sendtoWait(LoRa_data, sizeof(LoRa_data), SERVER_ADDRESS) == RH_ROUTER_ERROR_NONE){
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

// ===========================================================
// function definition for SD logger and LoRa on one board

void SPI_toLoRa()
{
    digitalWrite(SD_CS, HIGH);
    digitalWrite(RF95_CS, LOW);
    delay(1);
}

void SPI_toSD()
{
    digitalWrite(RF95_CS, HIGH);
    digitalWrite(SD_CS, LOW);
    delay(1);
}

// ===========================================================
// GPS function definition

void GPS_setup()
{
    GPS.begin(9600);
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
    GPS.sendCommand(PGCMD_ANTENNA);
    delay(1000);

    GPSSerial.println(PMTK_Q_RELEASE);

    delay(100);
    pinMode(GPS_EN, OUTPUT);
}

void GPS_sleep()
{
    digitalWrite(GPS_EN, HIGH);
}

void GPS_wakeup()
{
    digitalWrite(GPS_EN, LOW);
}

// ===========================================================
// PMS function definition

void PMS_setup()
{
    PMS.begin(9600);
    pinPeripheral(6, PIO_SERCOM);
    pinPeripheral(A5, PIO_SERCOM_ALT);
    PMS_writeCMD(CMD_sleep);
}

void PMS_writeCMD(uint8_t *cmd)
{
    for (int i = 0; i < CMD_len; i++)
        PMS.write(cmd[i]);
}

void PMS_bufFlush()
{
    int tmp = 0;
    char c;
    if (tmp = PMS.available())
    {

        //        Serial.print("PMS available: ");
        //        Serial.println(tmp);
        //        Serial.println("PMS's trash: ");
        while (tmp--)
        {
            c = PMS.read();
            //            Serial.write(c);
        }
        //        Serial.println();
    }
}

void PMS_readData()
{
    unsigned char pms[32];
    if (PMS.available() > 32)
    {
        char c = PMS.read();
        //Serial.write( c = PMS.read());
        if (c == 0x42)
        {
            //Serial.write( c = PMS.read());
            c = PMS.read();
            if (c == 0x4D)
            {
                pms[0] = 0x42;
                pms[1] = 0x4D;
                if (PMS.available() > 30)
                {
                    //Serial.println();
                    //Serial.print("read data:");
                    for (int j = 2; j < 32; j++)
                    {
                        pms[j] = PMS.read();
                    }
                    //for(int j=0; j<32 ; j++){
                    //    // for debug
                    //  Serial.write(pms[j]);
                    //}
                    PM1_0 = (pms[10] << 8) | pms[11];
                    PM2_5 = (pms[12] << 8) | pms[13];
                    PM10 = (pms[14] << 8) | pms[15];

                    sprintf(PMS_logData, "%d,%d,%d", PM1_0, PM2_5, PM10);

                    // PMS_print(); for debug
                }
            }
        }
    }
}

void PMS_print()
{
    Serial.println();
    Serial.print("PM 1.0 (ug/m3): ");
    Serial.print(PM1_0);
    Serial.print("\t");
    Serial.print("PM 2.5 (ug/m3): ");
    Serial.print(PM2_5);
    Serial.print("\t");
    Serial.print("PM 10.0 (ug/m3): ");
    Serial.println(PM10);
    Serial.println();
}

void PMS_wakeup()
{
    PMS_writeCMD(CMD_wakeup);

    delay(100);
    PMS_bufFlush();

    PMS_writeCMD(CMD_passiveMode);

    PMS_bufFlush();
    delay(50);
}

void PMS_sleep()
{
    PMS_writeCMD(CMD_sleep);
    delay(10);
    PMS_bufFlush();
}

// ===========================================================

void LM35_readTemp()
{
    LM35_logData = analogRead(LM35);
    LM35_logData = (LM35_logData * 500) / 1023; // Degree Celsius

}

void BAT_readV()
{
    BAT_logData = analogRead(BAT);
    BAT_logData *= 2;
    BAT_logData *= 3.3;
    BAT_logData /= 1024;

}
