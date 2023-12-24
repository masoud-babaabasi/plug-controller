/************************************************
 *
 ************************************************/
uint8_t WIFI_connect(uint32_t timeout_con){
  uint32_t pre_time = millis();
  uint8_t itteration = 0;
  #if SERIAL_DEBUG >= 1
    Serial.print("\nConnecting to ");
    Serial.println(ssid);
    Serial.println(password);
  #endif

    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED) {
      led_state ^= 0x01;
      //digitalWrite(LED_R , led_state);  
      analogWrite(LED_R , led_state * RED_INTENSITY * 2); 
      pre_time = millis();
      delay(250);
      #if SERIAL_DEBUG >= 1
      Serial.print(".");
      #endif
      #if SERIAL_BT == 1
      SerialBT.print(".");
      #endif
      itteration++; 
      if( itteration >= 30 ||  millis() - pre_time >= timeout_con){
        #if SERIAL_DEBUG >= 1
        Serial.print("\ncould not connect to ");
        Serial.println(ssid);
        Serial.println(password);
        #endif

        return 0;
      }
    }
    return 1; //succesful connection
 }