#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266SSDP.h>
#include <EEPROM.h>
#include "config.h"
#include "WebServerHtml.h"
#include "eepromutils.h"

ESP8266WebServer WebSrv(80);
ESP8266HTTPUpdateServer httpUpdater;

char WebLogin[cSizeOfSaveVsals];
char WebPass[cSizeOfSaveVsals];

void WebServerHandle(){
  WebSrv.handleClient();  
}

void handleRestart(){
  if(!WebSrv.authenticate(WebLogin, WebPass)) 
    return WebSrv.requestAuthentication();
    
  delay(5000);
  ESP.restart();  
}

void handleRoot() {
  if(!WebSrv.authenticate(WebLogin, WebPass)) 
    return WebSrv.requestAuthentication();

  WebSrv.send(200, "text/html", MainHtml);
}

void handleOptionsSave(){
  if(!WebSrv.authenticate(WebLogin, WebPass)) 
    return WebSrv.requestAuthentication();

  String fType = WebSrv.arg("form");
  if (fType == "ap") {
    String fAP = WebSrv.arg("AP");
    String fPass = WebSrv.arg("PASSWORD");

    if ((fAP == "") || (fPass == "")) {
      WebSrv.send(500, "text/html", cInvalidParamsHtml);
      return;
    }

    // Save data
    EEPROM.begin(512);
    EEPROMWriteStr(cAPName, fAP.c_str(), cSizeOfSaveVsals);
    EEPROMWriteStr(cAPPass, fPass.c_str(), cSizeOfSaveVsals);
    EEPROM.commit();
    EEPROM.end();
     
    WebSrv.send(200, "text/html", cSaveOkHtml);
    return;
  } else if (fType == "connect") {
    String fAP = WebSrv.arg("AP");
    String fPass = WebSrv.arg("PASSWORD");

    if ((fAP == "") || (fPass == "")) {
      WebSrv.send(500, "text/html", cInvalidParamsHtml);
      return;
    }

    // Save data
    EEPROM.begin(512);
    EEPROMWriteStr(cConnectAPName, fAP.c_str(), cSizeOfSaveVsals);
    EEPROMWriteStr(cConnectAPPass, fPass.c_str(), cSizeOfSaveVsals);
    EEPROM.commit();
    EEPROM.end();
    
    WebSrv.send(200, "text/html", cSaveOkHtml);
    return;
  } else if (fType == "login") {
    String fUser = WebSrv.arg("USER");
    String fPass = WebSrv.arg("PASSWORD");  

    if ((fUser == "") || (fPass == "")) {
      WebSrv.send(500, "text/html", cInvalidParamsHtml);
      return;
    }

    // Save data
    EEPROM.begin(512);
    EEPROMWriteStr(cAddrWebLogin, fUser.c_str(), cSizeOfSaveVsals);
    EEPROMWriteStr(cAddrWebPass,  fPass.c_str(), cSizeOfSaveVsals);
    EEPROM.commit();
    EEPROM.end();
    
    WebSrv.send(200, "text/html", cSaveOkHtml);
    return;
  } else {
    WebSrv.send(500, "text/html", "Error params");
    return;
  }  
}

void handleOptions(){
  if(!WebSrv.authenticate(WebLogin, WebPass)) 
    return WebSrv.requestAuthentication();

  String Page = cOptionsHtml;
  Page.replace("%WEBLOGIN%", WebLogin);
  WebSrv.send(200, "text/html", Page);
}

void StartSSDP(){
  SSDP.setSchemaURL("description.xml");
  SSDP.setHTTPPort(80);
  SSDP.setName("MSMD Gate");
  SSDP.setSerialNumber("1");
  SSDP.setURL("index.html");
  SSDP.setModelName("MySensors Majordomo Gate");
  SSDP.setModelNumber("929000226503");
  SSDP.setModelURL("http://http://majordomo.smartliving.ru");
  SSDP.setManufacturer("MSMD");
  SSDP.setManufacturerURL("http://http://majordomo.smartliving.ru");
  SSDP.begin();
}

void WebServerInit(){    
  // Load EEPROM
  EEPROM.begin(512);  
  EEPROMReadStr(cAddrWebLogin, WebLogin, sizeof(WebLogin));
  EEPROMReadStr(cAddrWebPass,  WebPass, sizeof(WebPass));  
  EEPROM.end();

  if (WebLogin[0] == 0x00) {
    strcpy(WebLogin, WEB_LOGIN);
  }
  if (WebPass[0] == 0x00){
    strcpy(WebPass, WEB_PASS);
  }
   
  Serial.printf("Web login: %s\n", WebLogin);
  Serial.printf("Web password: %s\n", WebPass);
  
  //== Update
  httpUpdater.setup(&WebSrv, WEB_UPDATEPATH);  

  //== Web server
  WebSrv.on("/", handleRoot);
  WebSrv.on("/restart", handleRestart);
  WebSrv.on("/options", HTTP_GET, handleOptions);
  WebSrv.on("/options", HTTP_POST, handleOptionsSave);
  WebSrv.on("/description.xml", HTTP_GET, [](){ 
    SSDP.schema(WebSrv.client());
  });

  //here the list of headers to be recorded
  const char * headerkeys[] = {"User-Agent","Cookie"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  //ask server to track these headers
  WebSrv.collectHeaders(headerkeys, headerkeyssize );
  
  WebSrv.begin();
  Serial.println("HTTP server started"); 

  //== SSDP
  StartSSDP();
}
