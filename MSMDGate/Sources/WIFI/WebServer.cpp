#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266SSDP.h>
#include "config.h"

const char* WebLogin = WEB_LOGIN;
const char* WebPassword = WEB_PASS;
const char* update_path = WEB_UPDATEPATH;

ESP8266WebServer WebSrv(80);
ESP8266HTTPUpdateServer httpUpdater;

void WebServerHandle(){
  WebSrv.handleClient();  
}

//Check if header is present and correct
bool is_authentified(){
  Serial.println("Enter is_authentified");
  
  if (WebSrv.hasHeader("Cookie")){   
    Serial.print("Found cookie: ");
    String cookie = WebSrv.header("Cookie");
    Serial.println(cookie);
    if (cookie.indexOf("ESPSESSIONID=1") != -1) {
      Serial.println("Authentification Successful");
      return true;
    }
  }
  Serial.println("Authentification Failed");
  return false;  
}

void handleRoot() {
  if (!is_authentified()){
    String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    WebSrv.sendContent(header);
    return;
  }

  String content = "<h1>MySensors MajorDomo Gateway</h1>";
  content += "<a href=\"/options\">Options</a><br/>";
  content += "<a href=\"/firmware\">Update firmware</a>";

  WebSrv.send(200, "text/html", content);
}

//login page, also called for disconnect
void handleLogin(){
  String msg;
  if (WebSrv.hasHeader("Cookie")){   
    Serial.print("Found cookie: ");
    String cookie = WebSrv.header("Cookie");
    Serial.println(cookie);
  }
  if (WebSrv.hasArg("DISCONNECT")){
    Serial.println("Disconnection");
    String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=0\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    WebSrv.sendContent(header);
    return;
  }
  if (WebSrv.hasArg("USERNAME") && WebSrv.hasArg("PASSWORD")){
    if (WebSrv.arg("USERNAME") == WebLogin &&  WebSrv.arg("PASSWORD") == WebPassword ){
      String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=1\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
      WebSrv.sendContent(header);
      Serial.println("Log in Successful");
      return;
    }
  msg = "Wrong username/password! try again.";
  Serial.println("Log in Failed");
  }
  String content = "<html><body><form action='/login' method='POST'>Default login: admin/admin<br>";
  content += "<table>";
  content += "<tr><td>User:</td><td><input type='text' name='USERNAME' placeholder='user name'></td></tr>";
  content += "<tr><td>Password:</td><td><input type='password' name='PASSWORD' placeholder='password'></td></tr>";
  content += "<tr><td></td><td><input type='submit' name='SUBMIT' value='Login'></td></tr>";
  content += "</table></form>" + msg + "<br>";
  WebSrv.send(200, "text/html", content);
}

void handleOptions(){
  if (!is_authentified()){
    String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    WebSrv.sendContent(header);
    return;
  }
  
  String content = "<html><body>";
  
  content += "<h1>AP config</h1><br>";
  content += "<form action='/options' method='POST'>";
  content += "<table>";  
  content += "<tr><td>AP name:</td><td><input type='text' name='AP' placeholder='AP name'></td></tr>";
  content += "<tr><td>Password:</td><td><input type='password' name='PASSWORD' placeholder='password'></td></tr>";
  content += "<tr><td></td><td><input type='submit' name='SUBMIT' value='Save'></td></tr>";
  content += "</table><input type='hidden' name='form' value='ap'/></form>";

  content += "<h1>Connect to AP</h1><br>";
  content += "<form action='/options' method='POST'>";
  content += "<table>";  
  content += "<tr><td>AP name:</td><td><input type='text' name='AP' placeholder='AP name'></td></tr>";
  content += "<tr><td>Password:</td><td><input type='password' name='PASSWORD' placeholder='password'></td></tr>";
  content += "<tr><td></td><td><input type='submit' name='SUBMIT' value='Save'></td></tr>";
  content += "</table><input type='hidden' name='form' value='connect'/></form>";

  content += "<h1>Admin panel</h1><br>";
  content += "<form action='/options' method='POST'>";
  content += "<table>";  
  content += "<tr><td>Password:</td><td><input type='password' name='PASSWORD' placeholder='password'></td></tr>";
  content += "<tr><td></td><td><input type='submit' name='SUBMIT' value='Save'></td></tr>";
  content += "</table><input type='hidden' name='form' value='login'/></form>";

  content += "<br><a href='/'>Back</a>";
    
  WebSrv.send(200, "text/html", content);
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
  //== Update
  httpUpdater.setup(&WebSrv, update_path);  

  //== Web server
  WebSrv.on("/", handleRoot);
  WebSrv.on("/login", handleLogin);
  WebSrv.on("/options", handleOptions);
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
