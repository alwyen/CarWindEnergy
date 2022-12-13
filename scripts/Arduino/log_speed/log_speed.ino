#include "LoRaWan_APP.h"
#include "LoRa_APP.h"
#include "Arduino.h"
#include <EEPROM.h>
//#include "LoRaWan_102.h"
//#include "GPS_Air530.h"
#include "GPS_Air530Z.h"

// Watchdog library definitions
#ifdef __asr650x__
#include "innerWdt.h"
#endif

#define RF_FREQUENCY 903900000
#define EEPROM_ADDRESS 1
#define NUM_BYTES_PAYLOAD 8
#define TIME_INTERVAL 2 //this is in seconds; might actually be double

Air530ZClass GPS;

/* 
 *  Transmit power change
 *  PROBLEM: it uses "Radio" instead of "LoRaWAN" so I'm not sure if it's the correct way
 *  https://github.com/HelTecAutomation/CubeCell-Arduino/blob/master/libraries/LoRa/examples/LoRaBasic/LoRaSender/LoRaSender.ino
 *  Okay, so I think you can hard code in the transmit power: ~/libraries/LoraWan102/src/loramac/region/RegionUS915.c (line 605)
 */

/*
 * set LoraWan_RGB to Active,the RGB active in loraWan
 * RGB red means sending;
 * RGB purple means joined done;
 * RGB blue means RxWindow1;
 * RGB yellow means RxWindow2;
 * RGB green means received done;
 */

 /*
  * Uplink Data Rate Chart
  * 
  * Data Rate | Configuration (SF + BW)
  * ----------|------------------------
  * 0         | LoRa: SF10 / 125 kHz
  * 1         | LoRa: SF9 / 125 kHz
  * 2         | LoRa: SF8 / 125 kHz
  * 3         | LoRa: SF7 / 125 kHz
  * 4         | LoRa: SF8 / 500 kHz
  */

 /* 
  * PLEASE EDIT THE 4 PARAMS TO YOUR LIKING
  * Set time between sending packets
  *   Set customTime to "true" if you want to set your own time
  *   Edit interval to set the time between packets sent; value in [ms]
  *   Edit hardResetTime to set the time that passes before there is a hard reset; value in [s]
  *   Edit dataRate to whatever data rate you want the device to use 
  *       (refer to Uplink Data Rate Chart for data rate)
  */

 bool customTime = true;

 //not sure why the actual value is doubled; change 1000 to 500
 uint32_t interval = TIME_INTERVAL * 500;
 uint32_t hardResetTime = 60;

/* OTAA para*/

/* Using Yalex_Board devices's params */
uint8_t devEui[] = {0x60, 0x81, 0xF9, 0xBF, 0x65, 0xE8, 0x4B, 0x2C};
uint8_t appEui[] = {0x60, 0x81, 0xF9, 0xCB, 0xF1, 0x19, 0xC1, 0x79};
uint8_t appKey[] = {0x74, 0xAD, 0x04, 0x3B, 0x0A, 0x39, 0xAC, 0x7B, 0xCD, 0x9A, 0x4F, 0x08, 0x38, 0xDA, 0xE3, 0xCF};
uint8_t dataRate = 0;


/* ABP para*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t devAddr =  ( uint32_t )0x007e6ae1;

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0xFF00,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = LORAWAN_CLASS;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000;

/*OTAA or ABP*/
bool overTheAirActivation = LORAWAN_NETMODE;

/*ADR enable*/
bool loraWanAdr = LORAWAN_ADR;

/* set LORAWAN_Net_Reserve ON, the node could save the network info to flash, when node reset not need to join again */
bool keepNet = LORAWAN_NET_RESERVE;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = LORAWAN_UPLINKMODE;

/* Application port */
uint8_t appPort = 2;
/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;

/* Some variables I created */
char DEVNAME[] = "heltec_1";
char SEPARATOR[] = ",";
unsigned long count;
//uint8_t count;
TimerSysTime_t sysTimeCurrentHardReset;
bool disabledWdt = false;

bool gps_fix = true;

/*
 * For asr650x, the max feed time is 2.8 seconds.
 * For asr6601, the max feed time is 24 seconds.
 */
#ifdef __asr650x__
#define MAX_FEEDTIME 2800// ms; this is the max feed time for the AB02A board
#else
#define MAX_FEEDTIME 24000// ms
#endif

bool autoFeed = false;

/*
 * UTC Unix Time Calculation (From January 1st, 1970)
 * [0]: seconds
 * [1]: minutes
 * [2]: hours
 * [3]: days
 * [4]: months
 * [5]: years
 * 
 * According to website: https://www.unixtimestamp.com/
 * 1 hour:                3600 seconds
 * 1 day:                 86400 seconds
 * 1 week:                604800 seconds
 * 1 month (30.44 days):  2629743 seconds
 * 1 year (365.24 days):  31556926 seconds
 * 
 */
uint32_t convert_to_unix_time(uint32_t time_array[6]){
//Debugging
  Serial.println("Years: " + String(time_array[5]));
  Serial.println("Months: " + String(time_array[4]));
  Serial.println("Days " + String(time_array[3]));
  Serial.println("Hours: " + String(time_array[2]));
  Serial.println("Minutes: " + String(time_array[1]));
  Serial.println("Seconds: " + String(time_array[0]));
  
  uint32_t years_to_seconds = (time_array[5]-1970) * 31556926;
  uint32_t months_to_seconds = (time_array[4]-1) * 2629743;
  uint32_t days_to_seconds = (time_array[3]-1) * 86400;

//  account for time zone difference?
  time_array[2] -= 10;
  uint32_t hours_to_seconds = (time_array[2]) * 60 * 60;

//  time difference still
  time_array[1] += 25;
  uint32_t minutes_to_seconds = time_array[1] * 60;

//  small time difference in seconds (approx 25 seconds)
  time_array[0] -= 26;
  uint32_t time_seconds = time_array[0] + minutes_to_seconds + hours_to_seconds + days_to_seconds + months_to_seconds + years_to_seconds;
  
  return time_seconds;
}

/* Prepares the payload of the frame */
static void prepareTxFrame( uint8_t port )
{
  /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
  *appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
  *if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
  *if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
  *for example, if use REGION_CN470, 
  *the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
  */
    uint8_t payload[NUM_BYTES_PAYLOAD];
    
    uint32_t time_array[6];
    time_array[0] = (uint32_t)GPS.time.second();
    time_array[1] = (uint32_t)GPS.time.minute();
    time_array[2] = (uint32_t)GPS.time.hour();
    time_array[3] = (uint32_t)GPS.date.day();
    time_array[4] = (uint32_t)GPS.date.month();
    time_array[5] = (uint32_t)GPS.date.year();

  
    float speedkmph = (float)(GPS.speed.kmph()); //converted to kmph
    uint32_t unix_time = convert_to_unix_time(time_array);

    Serial.println("Speed: " + String(speedkmph));
    Serial.println("Time: " + String(unix_time));

    memcpy(payload, &speedkmph, sizeof(speedkmph));
    memcpy(payload + sizeof(speedkmph), &unix_time, sizeof(unix_time));

    uint8_t payloadIdx = 0;
    uint8_t idx = 0;

//    if(speedkmph != 0 && GPS.location.isValid()){
      while (payloadIdx < NUM_BYTES_PAYLOAD) {
        appData[idx] = payload[payloadIdx];
        payloadIdx++;
        idx++;
      }

    

    /* Set the payload size */
//    appDataSize = idx;
      appDataSize = NUM_BYTES_PAYLOAD;
}

void setup() {
  boardInitMcu();
  Serial.begin(115200);
  GPS.begin();
//  EEPROM.begin(512);
#if(AT_SUPPORT)
  enableAt();
#endif
//  Radio.SetChannel(RF_FREQUENCY);
  deviceState = DEVICE_STATE_INIT;
  LoRaWAN.setDataRateForNoADR(dataRate);
  LoRaWAN.ifskipjoin();
  sysTimeCurrentHardReset = TimerGetSysTime();
}

void loop()
{
  /* 
   * Hard resets after a certain amount of time.
   * This is because the Heltec device stop successfully sending packages after a while.
   */
//  TimerSysTime_t timeNow = TimerGetSysTime();
//  uint32_t resetTimeDiff = (timeNow.Seconds * 1000 + timeNow.SubSeconds) - (sysTimeCurrentHardReset.Seconds * 1000 + sysTimeCurrentHardReset.SubSeconds);
//  if (resetTimeDiff > hardResetTime * 1000)
//  {
//    innerWdtEnable(autoFeed);
//  }

//  GPS stuff
  uint32_t starttime = millis();
  while( (millis()-starttime) < 1000 )
  {
    while (GPS.available() > 0)
    {
      GPS.encode(GPS.read());
    }
  }
  
  if (millis() > 5000 && GPS.charsProcessed() < 10)
  {
    Serial.println("No GPS detected: check wiring.");
    while(true);
  }
//  Heltec LoRa device states
  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
#if(AT_SUPPORT)
      getDevParam();
#endif
//      Radio.SetChannel(RF_FREQUENCY);
      printDevParam();
      LoRaWAN.init(loraWanClass,loraWanRegion);
      deviceState = DEVICE_STATE_JOIN;
      Serial.println("INIT");
      break;
    }
    case DEVICE_STATE_JOIN:
    {
//      Radio.SetChannel(RF_FREQUENCY);
      LoRaWAN.join();
      Serial.println("JOIN");
      break;
    }
    case DEVICE_STATE_SEND:
    {
      prepareTxFrame( appPort );
//      Radio.SetChannel(RF_FREQUENCY);
      Serial.println("Count: " + String(count));
      LoRaWAN.send();
      count += 1;
//      EEPROM.write(EEPROM_ADDRESS, count);
//      if (EEPROM.commit()){
//        Serial.println("Current Send Value: " + String(count - 1)); 
//        Serial.println("Next Send Value: " + String(count)); 
//      }
      deviceState = DEVICE_STATE_CYCLE;
//      Serial.println("SEND");
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
//      Radio.SetChannel(RF_FREQUENCY);
      // Schedule next packet transmission
      if (customTime)
      {
        txDutyCycleTime = interval;
      }
      else {
        // APP_TX_DUTYCYCLE_RND = 1000 (i.e., 1 sec)
        txDutyCycleTime = appTxDutyCycle + randr( 0, APP_TX_DUTYCYCLE_RND );
      }
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
//      Serial.println("CYCLE");
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.sleep();
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      Serial.print("DEFAULT");
      break;
    }
  }
}
