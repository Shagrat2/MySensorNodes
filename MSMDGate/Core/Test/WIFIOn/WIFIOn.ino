#define WIFIPOWER_PIN 4

void setup() { 
  // Setup locally attached sensors

  // Set power on WIFI
  pinMode(WIFIPOWER_PIN, OUTPUT); 
  digitalWrite(WIFIPOWER_PIN, 0);
}

void loop() { 
 
}
 
