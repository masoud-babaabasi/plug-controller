void handle_http_address(){
  String arg_post = server.arg(F("plain"));
  arg_post.toLowerCase();
  if( arg_post.indexOf(F("address")) ){
    server.send(200, F("application/json"), F("{\"success\":true,\"msg\":\"Http server address received.\"}"));
    deserializeJson(doc, arg_post);
    const char *add_str = doc[F("address")];
    //memcpy( HTTP_SERVER_ADDRESS , add_str , 255);
    memset(HTTP_SERVER_BASE , 0 , SERVER_BASE_LENGTH);
    strcpy(HTTP_SERVER_BASE , add_str);
    for(int i=0 ; i < SERVER_BASE_LENGTH ; i++)
      EEPROM.write(HTTP_SERVER_BASE_address + i , HTTP_SERVER_BASE[i]);
    EEPROM.commit();
  }
}
/*******************************************************************************************************************************/
void handle_http_address_GET(){
  String output = "{\"http address for request\":\"";
  output+= HTTP_SERVER_BASE;
  output += "\"}";
   server.send(200, F("application/json"),output);
}
/*******************************************************************************************************************************/
/*
 * @briefe : recieves ssid and password of the wifi to connect to over Hotspot
 */
void handle_WIFICONF(){
  String arg_post = server.arg(F("plain"));
  //String ssid_string,pass_string;
  //uint8_t qutation_idx[8];
  if( arg_post.indexOf(F("ssid")) &&  arg_post.indexOf(F("pass")) ){
    //server.send(200, F("application/json"), F("{\"success\":true,\"msg\":\"SSID received\"}"));
    deserializeJson(doc, arg_post);
    const char *s = doc[F("ssid")];
    const char *p = doc[F("pass")];
    const char *id= doc[F("id")];
    if( s != NULL ) strcpy( ssid , s );
    if( p != NULL ) strcpy( password , p );
    if( id != NULL ) strcpy(device_ID , id);
    if( WIFI_connect(wifi_connect_time_out) ){
      wifi_connected = 1;
      String string_request_response = F("{\"success\":true,\"msg\":\"WIFI connected.\" , \"data\":\"");
      string_request_response += (WiFi.localIP()).toString() + F("\"}");
      for(int i =0; i < 1 ; i++) {
        server.send(200, F("application/json"), string_request_response);
        delay(50);
      }
      //WiFi.softAPdisconnect(true);
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

      #if SERIAL_BT == 1
      SerialBT.println(F("WiFi connected."));
      SerialBT.println(ssid);
      SerialBT.println(password);
      SerialBT.print(F("IP address: "));
      SerialBT.println(WiFi.localIP());
      #endif
       #if SERIAL_DEBUG >= 1
      Serial.print(F("Device id"));
      Serial.println(device_ID);
      Serial.println(HTTP_SERVER_ADDRESS);
      #endif
      memset(HTTP_SERVER_ADDRESS , 0 , 512);      
      // for(int i=0 ; i < 255 ; i++)
      //   HTTP_SERVER_ADDRESS[i] = EEPROM.read(SSID_LENGHT * 2 + i);
      strncpy(HTTP_SERVER_ADDRESS , HTTP_SERVER_BASE , SERVER_BASE_LENGTH + 1);
      strcat(HTTP_SERVER_ADDRESS , "/api/v1/devices/");
      strcat(HTTP_SERVER_ADDRESS , device_ID);
      strcat(HTTP_SERVER_ADDRESS , "/update");
      for(int i=0 ; i < device_ID_LENGTH ; i++){
         EEPROM.write(device_ID_address + i , device_ID[i]);
      }
      EEPROM.commit();
      if (My_http.begin(My_client, HTTP_SERVER_ADDRESS )) { 
       #if SERIAL_DEBUG >= 1
      Serial.print(F("[HTTP] POST...\n"));
      #endif
      #if SERIAL_BT == 1
      SerialBT.print(F("[HTTP] POST...\n"));
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
      Serial.print("Post body:");Serial.println(post_body);
      #endif
      int httpCode = My_http.POST(post_body);
      if (httpCode > 0) {
         #if SERIAL_DEBUG >= 1
        Serial.printf_P(PSTR("[HTTP] POST... code: %d\n"), httpCode);
        #endif
        #if SERIAL_BT == 1
        SerialBT.printf_P(PSTR("[HTTP] POST... code: %d\n"), httpCode);
        #endif
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = My_http.getString();
           #if SERIAL_DEBUG >= 1
          Serial.println(payload);
          #endif
          #if SERIAL_BT == 1
          SerialBT.println(payload);
          #endif
        }
      } else {
         #if SERIAL_DEBUG >= 1
        Serial.printf_P(PSTR("[HTTP] GET... failed, error: %s\n"), My_http.errorToString(httpCode).c_str());
        #endif
        #if SERIAL_BT == 1
        SerialBT.printf_P(PSTR("[HTTP] GET... failed, error: %s\n"), My_http.errorToString(httpCode).c_str());
        #endif
      }
      My_http.end();
      WiFi.softAPdisconnect(true);
    }
    }else{
      server.send(200, F("application/json"), F("{\"success\":false,\"msg\":\"Fail to connect to WIFI please try again.\"}"));
    }
  }
}
/*******************************************************************************************************************************/
void handle_WIFI_satatus(){
  String output = get_stat_string();
  server.send(200, "application/json", output);
}
/*******************************************************************************************************************************/
void handle_change_status_POST(void){
  String arg_post = server.arg(F("plain"));
  if( arg_post.indexOf(F("status")) ){
    deserializeJson(doc, arg_post);
    const char *s = doc[F("status")];
    if( s != NULL ){
      String stat = s;
      stat.toLowerCase();
      if( stat == "off") {
        digitalWrite(RELAY , LOW);
        relay_output = 0;
        server.send(200, F("application/json"), F("{\"success\":true,\"status\":\"off\",\"msg\":\"Relay is OFF\"}"));
      }
      else if(stat == "on") {
        digitalWrite(RELAY , HIGH); 
        relay_output = 1;
        server.send(200, F("application/json"), F("{\"success\":true,\"status\":\"on\",\"msg\":\"Relay is ON\"}"));
      }        
      
    }
  }
}
/*******************************************************************************************************************************/
void handle_get_time_GET(void){
  String output = "{\"date\":\"";
  output += (String)(myRTC.getYear() + 2000);
  output += "/" + (String)myRTC.getMonth(century);
  output += "/" + (String)myRTC.getDate() + "\",";
  output += "\"time\":\"";
  output +=  (String)myRTC.getHour(h12Flag, pmFlag);
  output += ":" + (String)myRTC.getMinute();
  output += ":" + (String)myRTC.getSecond() + "\"}";    
  server.send(200, "application/json", output);  
}
/*******************************************************************************************************************************/
void handle_set_time_POST(void){
  String arg_post = server.arg(F("plain"));
  if( arg_post.indexOf(F("date")) && arg_post.indexOf(F("time")) ){
    deserializeJson(doc, arg_post);
    const char *d = doc[F("date")];
    const char *t = doc[F("time")];
    if( d != NULL ){  
      int year, month, date;
      if( sscanf(d ,"%d/%d/%d" , &year , & month , &date) == 3 ){
        year -= 2000;
        myRTC.setYear(year);
        myRTC.setMonth(month);
        myRTC.setDate(date);
      }
    }
    if( t != NULL){
      int hour , min , sec;
      if( sscanf(t , "%d:%d:%d" , &hour , &min ,&sec) == 3){
        myRTC.setHour(hour);
        myRTC.setMinute(min);
        myRTC.setSecond(sec);
      }
    }
    server.send(200, F("application/json"), F("{\"success\":true,\"msg\":\"Date received.\"}"));
  }
}
/*******************************************************************************************************************************/
String get_stat_string(){
  String output= "{\"SSID\":\"";
  output += ssid;
  output += "\",\"password\":\"";
  output += password;
  output += "\",\"hotspot\":\"";
  if( wifi_connected ) output += "Connected to wifi";
  else output += "Hot spot ON";
  output += "\",\"IP\":\"";
  if( wifi_connected ) output += (WiFi.localIP()).toString();
  else output += (WiFi.softAPIP()).toString();
  output += "\"}";
  return output;
}
/*******************************************************************************************************************************/
void handle_WIFIUPDATE(void){
    memset(HTTP_SERVER_ADDRESS , 0 , 512);      
    strncpy(HTTP_SERVER_ADDRESS , HTTP_SERVER_BASE , SERVER_BASE_LENGTH + 1);
    strcat(HTTP_SERVER_ADDRESS , "/api/v1/files/update/");

    if (My_http.begin(My_client, HTTP_SERVER_ADDRESS )) { 
       #if SERIAL_DEBUG >= 1
      Serial.print(F("[HTTP] POST...\n"));
      #endif
      My_http.addHeader(F("Content-Type"), F("application/json"));
      String post_body = F("{\"type\": \"plug\" , \"version\":");
      post_body += (String)VERSION + "}";
       #if SERIAL_DEBUG >= 1
      Serial.print("Post body:");Serial.println(post_body);
      #endif
      int httpCode = My_http.POST(post_body);
      if (httpCode > 0) {
         #if SERIAL_DEBUG >= 1
        Serial.printf_P(PSTR("[HTTP] POST... code: %d\n"), httpCode);
        #endif
        if (httpCode == HTTP_CODE_OK) {
          server.send(200, F("application/json"), F("{\"success\":true,\"msg\":\"Downloading update.\"}"));
          String arg_post = My_http.getString();
           #if SERIAL_DEBUG >= 1
          Serial.println(arg_post);
          #endif
            if( arg_post.indexOf(F("data")) ){
              String data = arg_post.substring(arg_post.indexOf(F("data")) + 6 , arg_post.length() - 1);
               #if SERIAL_DEBUG >= 1
                Serial.println(data);
              #endif
              deserializeJson(doc, data);
              const char *f = doc[F("file")];
              if( f != NULL ){ 
                // deserializeJson(doc, d);
                // const char *address = doc[F("file")];
                 #if SERIAL_DEBUG >= 1
                Serial.println(f);
                #endif
                handle_update_file(f);
              }
            }

        }
      } else {
         #if SERIAL_DEBUG >= 1
        Serial.printf_P(PSTR("[HTTP] GET... failed, error: %s\n"), My_http.errorToString(httpCode).c_str());
        #endif
        String response = F("{\"success\":false,\"msg\":\"server error: ");
        response += My_http.errorToString(httpCode);
        response += "\"}";
        server.send(200, F("application/json"), response);
      }
      My_http.end();
    }else{
      server.send(200, F("application/json"), F("{\"success\":false,\"msg\":\"Could not connect to server.\"}"));
    }
}
/*******************************************************************************************************************************/
void handle_status_GET(void){
  if( relay_output ){
    server.send(200, F("application/json"), F("{\"status\":\"on\"}"));
  }else{
    server.send(200, F("application/json"), F("{\"status\":\"off\"}"));
  }
}
/*******************************************************************************************************************************/
void handle_change_key_POST(void){
  String arg_post = server.arg(F("plain"));
  if( arg_post.indexOf(F("status")) ){
    deserializeJson(doc, arg_post);
    const char *s = doc[F("status")];
    if( s != NULL ){
      String stat = s;
      stat.toLowerCase();
      if( stat == "disable") {
        input_key_enable = 0;
        server.send(200, F("application/json"), F("{\"success\":true,\"status\":\"disable\",\"msg\":\"Input touch key is disabled.\"}"));
      }
      else if(stat == "enable") {
        input_key_enable = 1;
        server.send(200, F("application/json"), F("{\"success\":true,\"status\":\"enable\",\"msg\":\"Input touch key is enabled.\"}"));
      }
      EEPROM.write(input_key_enable_address , input_key_enable);
      EEPROM.commit();        
    }else{
      server.send(200, F("application/json"), F("{\"success\":false,\"status\":\"\",\"msg\":\"wrong input.\"}"));
    }
  }else{
    server.send(200, F("application/json"), F("{\"success\":false,\"status\":\"\",\"msg\":\"wrong input.\"}"));
  }
}
/*******************************************************************************************************************************/
void handle_key_GET(void){
  if( input_key_enable ){
    server.send(200, F("application/json"), F("{\"status\":\"enable\"}"));
  }else{
    server.send(200, F("application/json"), F("{\"status\":\"disable\"}"));
  }
}
/*******************************************************************************************************************************/
void handel_cron_expresions_function(const char *input_expresions , int nums){
      char str_crons[MAX_SCHEDULE * 32];
      strcpy(str_crons , input_expresions);
      char *str_ptr , *str_ptr2 ,*str_ptr3 , *temp;
      const char *parse_error;
      char cron_expresion[32] ={0};
      str_ptr = str_crons;
      for(uint8_t i=0 ; i < nums ; i++){
        temp = strstr(str_ptr , "~");
        str_ptr2 = str_ptr;
        if( temp != NULL){
          *temp = 0;
          str_ptr = temp + 1;
        }
        str_ptr3 = str_ptr2;
        for(uint8_t j=0 ; j < 7;j++){
          str_ptr3 = strstr(str_ptr3 , " ") + 1;
          if( j == 4) {
            *(str_ptr3 - 1) = 0;
            sprintf(cron_expresion, "00 %s",str_ptr2);
            cron_parse_expr(cron_expresion , &(alarm_crons[i].date_time) , &parse_error);
            #if SERIAL_DEBUG >= 2
            Serial.printf("cron expresion %d = %s \n " , i , cron_expresion);
            #endif
            str_ptr2 = str_ptr3;
          }
          if( j == 5){
            *(str_ptr3 - 1) = 0;
            if( *(str_ptr2) == '*') alarm_crons[i].year = -1;
            else alarm_crons[i].year = atoi(str_ptr2);
            str_ptr2 = str_ptr3;
          }
          if( j == 6){
            *(str_ptr3 - 1) = 0;
            alarm_crons[i].action = atoi(str_ptr2);
            alarm_crons[i].active = atoi(str_ptr3);
          }
          
        }

        #if SERIAL_DEBUG >= 2
        Serial.printf("schedule number %d : \n" ,i);
        Serial.printf("cron seconds ");
        for(uint8_t k = 0 ; k < 8 ; k++)
          Serial.printf("%02x" , alarm_crons[i].date_time.seconds[k]);

        Serial.printf("\n cron minutes ");
        for(uint8_t k = 0 ; k < 8 ; k++)
          Serial.printf("%02x" , alarm_crons[i].date_time.minutes[k]);

        Serial.printf("\n cron hours ");
        for(uint8_t k = 0 ; k < 3 ; k++)
          Serial.printf("%02x" , alarm_crons[i].date_time.hours[k]);

        Serial.printf("\n cron DOW ");
        for(uint8_t k = 0 ; k < 1 ; k++)
          Serial.printf("%02x" , alarm_crons[i].date_time.days_of_week[k]);

        Serial.printf("\n cron date ");
        for(uint8_t k = 0 ; k < 4 ; k++)
          Serial.printf("%02x" , alarm_crons[i].date_time.days_of_month[k]);

        Serial.printf("\n cron months ");
        for(uint8_t k = 0 ; k < 2 ; k++)
          Serial.printf("%02x" , alarm_crons[i].date_time.months[k]);
        Serial.printf("\n cron year %d \n" ,alarm_crons[i].year );
        Serial.printf("cron action %d \n" ,alarm_crons[i].action );
        Serial.printf("cron active %d \n" ,alarm_crons[i].active );
        #endif
      }
}
void handle_scheduel_set(void){
  String arg_post = server.arg(F("plain"));
  if( arg_post.indexOf(F("crons")) ){
    strncpy(schedule_json , arg_post.c_str() , MAX_SCHEDULE_JSON_LEN);
    for(int i=0 ; i < MAX_SCHEDULE_JSON_LEN ; i++)
          EEPROM.write(schedule_json_address + i , schedule_json[i] );
    EEPROM.commit();
    server.send(200, F("application/json"), F("{\"success\":true,\"msg\":\"Schedule received.\"}"));
    deserializeJson(doc, arg_post);
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
  server.send(200, F("application/json"), F("{\"success\":fauls,\"msg\":\"wrong argument.\"}"));
}
/*******************************************************************************************************************************/
void handle_scheduel_get(void){
  server.send(200, F("application/json"), schedule_json );
}