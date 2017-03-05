
const char MainHtml[] PROGMEM  = {
  "<html><body>"
  "<h1>MySensors MajorDomo Gateway</h1>"
  "<a href=\"/options\">Options</a><br/>"
  "<a href=\"/firmware\">Update firmware</a><br>"
  "<a href=\"/restart\" onclick = \"if (! confirm('Restart?')) { return false; }\">Restart</a>"
};

const char cSaveOkHtml[] PROGMEM  = {
  "<html><body>"
  "<h1>Save ok</h1>"
  "<p>For apply parameters, plaese reboot device</p>"  
  "<a href=\"/restart\">Reboot</a><br>"
  "<a href=\"/\">back to main</a><br>"
};

const char cOptionsHtml[] PROGMEM  = {
  // Show form
  "<html><body>"
  
  "<h1>AP config</h1><br>"
  "<form action='/options' method='POST' enctype='multipart/form-data'>"
  "<table>"
  "<tr><td>AP name:</td><td><input type='text' name='AP' placeholder='AP name' pattern=\".{2,32}\"></td><td></td></tr>"
  "<tr><td>Password:</td><td><input type='password' name='PASSWORD' placeholder='password' pattern=\".{8,32}\"></td><td>Minimum length 8</td></tr>"
  "<tr><td></td><td><input type='submit' name='SUBMIT' value='Save'></td><td></td></tr>"
  "</table><input type='hidden' name='form' value='ap'/></form>"

  "<h1>Connect to AP</h1><br>"
  "<form action='/options' method='POST' enctype='multipart/form-data'>"
  "<table>"
  "<tr><td>AP name:</td><td><input type='text' name='AP' placeholder='AP name' pattern=\".{2,32}\"></td><td></td></tr>"
  "<tr><td>Password:</td><td><input type='password' name='PASSWORD' placeholder='password' pattern=\".{8,32}\"></td><td>Minimum length 8</td></tr>"
  "<tr><td></td><td><input type='submit' name='SUBMIT' value='Save'></td><td></td></tr>"
  "</table><input type='hidden' name='form' value='connect'/></form>"

  "<h1>Admin panel</h1><br>"
  "<form action='/options' method='POST' enctype='multipart/form-data'>"
  "<table>"
  "<tr><td>User:</td><td><input type='text' name='USER' value='%WEBLOGIN%' pattern=\".{2,32}\"></td></tr>"
  "<tr><td>Password:</td><td><input type='password' name='PASSWORD' placeholder='password' pattern=\".{8,32}\"></td></tr>"
  "<tr><td></td><td><input type='submit' name='SUBMIT' value='Save'></td></tr>"
  "</table><input type='hidden' name='form' value='login'/></form>"
  "<br><a href='/'>Back</a>"
};

const char cInvalidParamsHtml[] PROGMEM  = {
  "<html><body>"
  "<h1>Invalid params</h1>"
  "<a href=\"/\">back to main</a><br>"
};

