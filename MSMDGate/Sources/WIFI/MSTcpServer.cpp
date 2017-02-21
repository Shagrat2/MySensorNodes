#include <ESP8266WebServer.h>
#include <WiFiClient.h> 
#include "config.h"

WiFiServer MYSSrv(5003);
WiFiClient MYSSrvClients[MAX_SRV_CLIENTS];

void MSTcpServerInit(){
  //== MySensor server
  MYSSrv.begin();
  MYSSrv.setNoDelay(true);
}

void MSTcpServerHandle(){
  uint8_t i;
  
  // MYS server check connection new clients
  if (MYSSrv.hasClient()){
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      //find free/disconnected spot
      if (!MYSSrvClients[i] || !MYSSrvClients[i].connected()){
        if(MYSSrvClients[i]) MYSSrvClients[i].stop();
        MYSSrvClients[i] = MYSSrv.available();
        Serial1.print("New client: "); Serial1.print(i);
        continue;
      }
    }
    
    //no free/disconnected spot so reject
    WiFiClient serverClient = MYSSrv.available();
    serverClient.stop();
  }

  // MYS check clients for data
  for(i = 0; i < MAX_SRV_CLIENTS; i++){
    if (MYSSrvClients[i] && MYSSrvClients[i].connected()){
      if(MYSSrvClients[i].available()){
        //get data from the telnet client and push it to the UART
        while(MYSSrvClients[i].available()) {
          Serial.write(MYSSrvClients[i].read());
        }
      }
    }
  }

  //check UART for data
  if(Serial.available()){
    size_t len = Serial.available();
    uint8_t sbuf[len];
    Serial.readBytes(sbuf, len);
    //push UART data to all connected telnet clients
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      if (MYSSrvClients[i] && MYSSrvClients[i].connected()){
        MYSSrvClients[i].write(sbuf, len);
        delay(1);
      }
    }
  }
  
}

