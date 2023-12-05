void update_started() {
  #if SERIAL_DEBUG == 1
  Serial.println(F("CALLBACK:  HTTP update process started"));
  #endif
}

void update_finished() {
  #if SERIAL_DEBUG == 1
  Serial.println(F("CALLBACK:  HTTP update process finished"));
  #endif
}

void update_progress(int cur, int total) {
  #if SERIAL_DEBUG == 1
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
  #endif
}

void update_error(int err) {
  #if SERIAL_DEBUG == 1
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
  #endif
}

void handle_update_file(const char *file_address){
  WiFiClient client;
  // Add optional callback notifiers
    ESPhttpUpdate.onStart(update_started);
    ESPhttpUpdate.onEnd(update_finished);
    ESPhttpUpdate.onProgress(update_progress);
    ESPhttpUpdate.onError(update_error);

    t_httpUpdate_return ret = ESPhttpUpdate.update(client, file_address);
    // Or:
    //t_httpUpdate_return ret = ESPhttpUpdate.update(client, "server", 80, "file.bin");

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
}