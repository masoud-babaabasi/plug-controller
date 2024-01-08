/*
  ESP8266 MCU: Plug Control By Masoud Babaabasi
  date created : 2023-06-25
  Last date modifed: 2023-08-10
*/
#include <DS3231.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h> 
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <string.h>
#include <stdio.h>
#include <AT24Cxx.h>
#include "ccronexpr.h"
#include "data_time_class.h"
/* defines ***************************************************************************************************************************/
#define I2C_SCL     13
#define I2C_SDA     12
#define DS3231_INT  16
#define LED_W       2
#define LED_G       LED_W
#define LED_R       4
#define RELAY       14
#define KEY_IN      5

#define i2c_address 0x50

#define RED_INTENSITY     10
#define SERIAL_DEBUG      1

#define SERIAL_BAUD   115200

#define SSID_LENGHT           20
#define SERVER_BASE_LENGTH    255
#define device_ID_LENGTH      32
#define ssid_AP               F("Rosha_Controller")
#define password_AP           F("123456789")
#define VERSION               1
#define MAX_SCHEDULE          10
#define MAX_SCHEDULE_JSON_LEN 511

#define RTC_CHECK_INTERVAL    5

#define wifi_connect_time_out   5000
/* Global Variables *******************************************************************************************************************/
char ssid[SSID_LENGHT] ;
char password[SSID_LENGHT];
//char device_type[32];
char device_ID[device_ID_LENGTH];
int device_ID_address = SSID_LENGHT * 2  + SERVER_BASE_LENGTH;

char HTTP_SERVER_ADDRESS[512];

char HTTP_SERVER_BASE[SERVER_BASE_LENGTH + 1] = { 0 };
int HTTP_SERVER_BASE_address = SSID_LENGHT * 2;

uint64_t device_UID;
char device_UID_str[24];

char schedule_json[MAX_SCHEDULE_JSON_LEN + 1] = { 0 };
int schedule_json_address = SSID_LENGHT * 2  + SERVER_BASE_LENGTH + device_ID_LENGTH + 1;

uint8_t relay_output = 0;

uint8_t input_key_enable = 0;
int input_key_enable_address = SSID_LENGHT * 2  + SERVER_BASE_LENGTH + device_ID_LENGTH;

uint8_t key_val = 0 , pre_key_val = 0;

DS3231        myRTC;
RTClib        rtc_lib;
DateTime      rtc_data;
uint8_t       secs = 0;
bool          century = false;
bool          h12Flag;
bool          pmFlag;
//uint8_t       alarmDay, alarmHour, alarmMinute, alarmSecond, alarmBits;
//bool          alarmDy, alarmH12Flag, alarmPmFlag;
uint32_t      start_time_led_red,start_time;
uint8_t       led_state = 0;
StaticJsonDocument<1024> doc;
volatile uint8_t wifi_connected = 0;

AT24Cxx eep(i2c_address, 4);

ESP8266WebServer server(80);
WiFiClient My_client;
HTTPClient My_http;

schedule alarm_crons[MAX_SCHEDULE] = {0};
int cron_nums = 0;

uint8_t pre_minute = -1 , now_minute = 0;

/* Fuction prototyps *************************************************************************************************************/
uint8_t WIFI_connect(uint32_t timeout_con);
void handle_WIFI_satatus();
String get_stat_string();
void handle_http_address();
void handle_update_file(const char *file_address);
void handel_cron_expresions_function(const char *input_expresions , int nums);
uint8_t check_alarm_condition(uint8_t num);
/*Set Up function******************************************************************************************************************/
void setup() {
  pinMode(LED_W, OUTPUT);     
  pinMode(LED_R, OUTPUT); 
  pinMode(RELAY, OUTPUT);
  pinMode(KEY_IN, INPUT);
  
  digitalWrite(LED_W, LOW);
  digitalWrite(LED_R, LOW);
  digitalWrite(RELAY, LOW);

  Serial.begin(SERIAL_BAUD);
  Wire.begin(I2C_SDA , I2C_SCL); 
  String temp;
  device_UID = (uint64_t)(ESP.getChipId()) | ((uint64_t)(ESP.getFlashChipId()) << 32 );
  sprintf(device_UID_str , "%08x%08x", (uint32_t)(device_UID >> 32) ,  (uint32_t)(device_UID & 0xffffffff));
  /*EEPROM**************************************************/
  EEPROM.begin(SSID_LENGHT * 2 + SERVER_BASE_LENGTH + device_ID_LENGTH + 1 + MAX_SCHEDULE_JSON_LEN);  
  for(int i=0 ; i < SSID_LENGHT ; i++)
    ssid[i] = EEPROM.read(i);
  for(int i=0 ; i < SSID_LENGHT ; i++)
    password[i] = EEPROM.read(SSID_LENGHT + i);
  for(int i=0 ; i < SERVER_BASE_LENGTH ; i++)
    HTTP_SERVER_BASE[i] = EEPROM.read(HTTP_SERVER_BASE_address + i);
  for(int i=0 ; i < device_ID_LENGTH ; i++)
    device_ID[i] = EEPROM.read(device_ID_address + i);
  input_key_enable =  EEPROM.read(input_key_enable_address);
  for(int i=0 ; i < MAX_SCHEDULE_JSON_LEN ; i++)
    schedule_json[i] = EEPROM.read(schedule_json_address + i);

  /***********************************************************/
  strncpy(HTTP_SERVER_ADDRESS , HTTP_SERVER_BASE , SERVER_BASE_LENGTH + 1);
  strcat(HTTP_SERVER_ADDRESS , "/api/v1/devices/");
  strcat(HTTP_SERVER_ADDRESS , device_ID);
  strcat(HTTP_SERVER_ADDRESS , "/update");
   //strcpy(ssid,"RS");
  //strcpy(password,"00000");
  if( strstr(schedule_json ,"crons") ){
    deserializeJson(doc, schedule_json);
    const char *cron = doc[F("crons")];
    cron_nums = doc[F("num")];
    #if SERIAL_DEBUG >= 1
        Serial.printf("received crons nums = %d\n" , cron_nums);
        Serial.printf("crons = %s\n" , cron);
    #endif
    if( cron_nums > MAX_SCHEDULE){
      cron_nums = MAX_SCHEDULE;
      #if SERIAL_DEBUG >= 1
        Serial.printf("received crons is bigger than max schedule(max = %d)",MAX_SCHEDULE);
      #endif
    }
    /*
    cron fields : 
      1-minute(0-59) 
      2-hour(0-23) 
      3-date(1-31) 
      4-month(1-12) 
      5-dayOfWeek(0-6) 
      6-year 
      7-action(0:off 1:on 2:toggle) 
      8-activation(0:disable 1:enable)] 
    seconds is always considerd zero
    */
    if( cron != NULL ){
      handel_cron_expresions_function(cron , cron_nums);
    }
  }


  if( WIFI_connect(wifi_connect_time_out) ){
    wifi_connected = 1;
    for(int i=0 ; i < SSID_LENGHT ; i++)
       EEPROM.write(i , ssid[i]);
   for(int i=0 ; i < SSID_LENGHT ; i++)
     EEPROM.write(SSID_LENGHT + i , password[i]);
   EEPROM.commit();
    #if SERIAL_DEBUG >= 1
    Serial.println(F("WiFi connected."));
    Serial.println(ssid);
    Serial.println(password);
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
    #endif
    
    if (My_http.begin(My_client, HTTP_SERVER_ADDRESS )) { 
      #if SERIAL_DEBUG >= 1
      Serial.print(F("[HTTP] POST...\n"));
      #endif
      My_http.addHeader(F("Content-Type"), F("application/json"));
      String post_body = F("{\"field\": \"device_ip\",\"ip\":\"");
      post_body += (WiFi.localIP()).toString();
      post_body += F("\",\"ssid\":\"");
      post_body += ssid;
      post_body += F("\",\"device_type\":\"");
      post_body += "plug";
      post_body += F("\",\"unique_id\":\"");
      post_body += device_UID_str;
      post_body += F("\"}");
      #if SERIAL_DEBUG >= 1
      Serial.print("http server addres: ");Serial.println(HTTP_SERVER_ADDRESS);
      Serial.print("Post body:");Serial.println(post_body);
      #endif
      int httpCode = My_http.POST(post_body);
      if (httpCode > 0) {
        #if SERIAL_DEBUG >= 1
        Serial.printf_P(PSTR("[HTTP] POST... code: %d\n"), httpCode);
        #endif
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = My_http.getString();
          #if SERIAL_DEBUG >= 1
          Serial.println(payload);
          #endif
        }
      } else {
        #if SERIAL_DEBUG >= 1
        Serial.printf_P(PSTR("[HTTP] GET... failed, error: %s\n"), My_http.errorToString(httpCode).c_str());
        #endif
      }
      My_http.end();
    }
  }
  if( wifi_connected != 1){
    WiFi.mode(WIFI_STA);
    WiFi.softAP(ssid_AP, password_AP);
    #if SERIAL_DEBUG >= 1
    Serial.println(F("WiFi  not connected."));
    Serial.println(F("Hot spot created:"));
    Serial.print("ssid_AP : ");
    Serial.println(ssid_AP);
    Serial.print("password_AP : ");
    Serial.println(password_AP);
    Serial.print("local IP : ");
    Serial.println(WiFi.softAPIP());
    #endif
  }
  server.on(F("/api/v1/wifi/config")  ,HTTP_POST ,  handle_WIFICONF);
  server.on(F("/api/v1/wifi/status")  ,HTTP_GET ,  handle_WIFI_satatus);
  server.on(F("/api/v1/wifi/address")  ,HTTP_POST ,  handle_http_address);
  server.on(F("/api/v1/wifi/address")  ,HTTP_GET ,  handle_http_address_GET);
  server.on(F("/api/v1/change/status")  ,HTTP_POST ,  handle_change_status_POST);
  server.on(F("/api/v1/change/key")  ,HTTP_POST ,  handle_change_key_POST);
  server.on(F("/api/v1/status")  ,HTTP_GET ,  handle_status_GET);
  server.on(F("/api/v1/key")  ,HTTP_GET ,  handle_key_GET);
  server.on(F("/api/v1/time/get")  ,HTTP_GET ,  handle_get_time_GET);
  server.on(F("/api/v1/time/set")  ,HTTP_POST ,  handle_set_time_POST);
  server.on(F("/api/v1/firmware/update") , HTTP_POST ,  handle_WIFIUPDATE);
  server.on(F("/api/v1/schedule/status") , HTTP_POST ,  handle_scheduel_set);
  server.on(F("/api/v1/schedule/status") , HTTP_GET ,  handle_scheduel_get);

  myRTC.setClockMode(false);  // set to 24h
  server.begin(); 
  start_time_led_red = millis(); 
  start_time = millis(); 

  // alarm_times[0].hour = 10;
}

void loop() {
  server.handleClient();
  if( millis() - start_time_led_red >= 1000){
    start_time_led_red = millis();
    if( wifi_connected ){
      led_state ^= 0x01; // blink every 1s when connected to a wifi succesfully
    }else{
      led_state = 0x01; // always on when hotspot is ON 
    }
    //digitalWrite(LED_R , led_state); 
    analogWrite(LED_R , led_state * RED_INTENSITY);

    secs++; // every RTC_CHECK_INTERVAL seconds check the data of ds3231 module
    if( secs == RTC_CHECK_INTERVAL){
      secs = 0;
      now_minute = myRTC.getMinute();
      if( pre_minute != now_minute){ // check to see if minute has changed
        pre_minute = now_minute;
        rtc_data = rtc_lib.now(); // if minute changed check the cron alarms
        for(uint8_t i = 0 ; i < cron_nums ; i++){
          if( alarm_crons[i].active && check_alarm_condition(i) ){
            switch(alarm_crons[i].action){
              case 0 :
                digitalWrite(RELAY , LOW);
                relay_output = 0;
                break;
              case 1 :
                digitalWrite(RELAY , HIGH);
                relay_output = 1;
                break;
              case 2 :
                relay_output ^= 1;
                digitalWrite(RELAY , relay_output); 
                break;
              default:
              digitalWrite(RELAY , LOW);
                relay_output = 0;
            }
          }
        }
      }
    }
  }
  if( millis() - start_time >= 50){
    start_time = millis(); 
    if( relay_output ){
      digitalWrite(LED_G , HIGH); 
    }else{
      digitalWrite(LED_G , LOW); 
    }
    if( input_key_enable ) { // if hardware input button is enabled 
    pre_key_val = key_val;
    key_val = digitalRead(KEY_IN);
    if( pre_key_val == 0 && key_val == 1){
      relay_output ^= 1;
      digitalWrite(RELAY , relay_output); 
    }
  }  
  }          
}
/******************************************************************/
uint8_t check_alarm_condition(uint8_t num){
  if( cron_get_bit(alarm_crons[num].date_time.minutes , rtc_data.minute()) == 0) return 0;
  if( cron_get_bit(alarm_crons[num].date_time.hours , rtc_data.hour()) == 0)return 0;
  if( cron_get_bit(alarm_crons[num].date_time.days_of_month , rtc_data.day()) == 0) return 0;
  if( cron_get_bit(alarm_crons[num].date_time.days_of_week , myRTC.getDoW()) == 0) return 0;
  if( cron_get_bit(alarm_crons[num].date_time.months , rtc_data.month() - 1) == 0) return 0;
  if( alarm_crons[num].year != -1 && alarm_crons[num].year != rtc_data.year() ) return 0;
  return 1;
}