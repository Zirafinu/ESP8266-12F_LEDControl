
// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <EEPROM.h>

#include <array>

// this should be part of the compiler
#define static_assert(x) namespace{extern char error[(x)?0:-1];}

struct EEPROM_DATA {
  uint32_t crc{0};
  uint32_t mode{0};
  std::array<char, 0x40> configPassword;
  std::array<char, 0x40> ssid;
  std::array<char, 0x40> password;
  std::array<char, 0x40> hostname;
};

class EEPROM_DATA_Manager {
  private:
    EEPROM_DATA data;
    static const std::array<uint32_t, 2> modes;

    template <typename ArrayType>
    static void AssignString(ArrayType& data, const String& text) {
      if (text.length() >= data.size())
        return;

      int i = 0;
      for (; i < text.length(); ++i) {
        data[i] = text.charAt(i);
      }
      for (; i < data.size(); ++i) {
        data[i] = '\0';
      }
    }

    static inline uint32_t AddByteToCRC(uint32_t crc, char character) {
      // the algorithm/implementation from
      //  'Hacker's Delight 2nd Edition by Henry S Warren, Jr.'
      //  is used to compute the crc 32.
      crc = crc ^ character;
      for (int j = 7; j >= 0; --j) {
        uint32_t mask = -(crc & 1);
        crc = (crc >> 1) ^ (0xEDB88320UL & mask);
      }
      return crc;
    }

    template <typename ArrayType>
    static uint32_t AddToCRC(uint32_t crc, const ArrayType& data) {
      for (const char character : data) {
        crc = AddByteToCRC(crc, character);
      }
      return crc;
    }

    uint32_t computeCRC() const {
      uint32_t crc = 0xFFffFFff;
      crc = AddToCRC(crc, data.configPassword);
      crc = AddToCRC(crc, data.ssid);
      crc = AddToCRC(crc, data.password);
      crc = AddToCRC(crc, data.hostname);
      crc = AddByteToCRC(crc, data.mode);
      return crc ^ 0xFFffFFff;
    }

    void resetDataToDefault() {
      data.crc = 0;
      data.mode = modes[MODE_BASE_STATION];
      for (auto & item : data.configPassword) item = '\0';
      for (auto & item : data.ssid) item = '\0';
      for (auto & item : data.password) item = '\0';
      for (auto & item : data.hostname) item = '\0';
      data.configPassword = std::array<char, 0x40> {'!', 'n', 'i', 't', 'P', 'w', 'd', '1', '2', '3', 0};
      data.ssid = std::array<char, 0x40> {'E', 's', 'p', '1', '2', '_', 'L', 'E', 'D', 0};
      data.password = std::array<char, 0x40> {'1', '2', '3', '4', '5', '6', '7', '8', 0};
    }
  public:
    enum EMODES {
      MODE_BASE_STATION = 0,
      MODE_NODE = MODE_BASE_STATION + 1,

      MODE_COUNT = modes.size()
    };
    EEPROM_DATA_Manager() {}

    void initialize() {
      EEPROM.begin(4096);
      EEPROM.get(0, data);
      uint32_t actualCRC = computeCRC();
      if (actualCRC != data.crc) {
        // data is invalid clear all
        resetDataToDefault();
      }
    }

    void resetToBaseMode() {
      resetDataToDefault();
      EEPROM.put(0, data);
      EEPROM.commit();
    }

    bool isValid() const {
      return data.crc == computeCRC();
    }

    EMODES getMode() const {
      for (int mode = 0; mode < MODE_COUNT; ++mode) {
        if (data.mode == modes[mode]) {
          return EMODES(mode);
        }
      }
      return MODE_BASE_STATION;
    }

    const String getHostname() const {
      return String(data.hostname.data());
    }

    const String getSSID() const {
      return String(data.ssid.data());
    }

    const String getPassword() const {
      return String(data.password.data());
    }

    bool setNewConfig(const String & oldPassword,
                      const String & newPassword,
                      const String & ssid,
                      const String & password,
                      const String & hostname,
                      EMODES mode) {
      Serial.println("Assigning");
      Serial.println(oldPassword);
      Serial.println(newPassword);
      Serial.println(ssid);
      Serial.println(password);
      Serial.println(hostname);
      Serial.println(mode);
      // check the string length
      if (newPassword.length() >= data.configPassword.size()
          || ssid.length() >= data.ssid.size() || ssid.length() < 1
          || password.length() >= data.password.size() || password.length() < 8
          || hostname.length() >= data.hostname.size()
          || mode >= MODE_COUNT) {
            Serial.println("Faild validation.");
        return false;
      }

      for (size_t i = 0; i < data.configPassword.size(); ++i) {
        if (oldPassword[i] != data.configPassword[i])
          return false;
      }

      AssignString(data.configPassword, newPassword);
      AssignString(data.ssid, ssid);
      AssignString(data.password, password);
      AssignString(data.hostname, hostname);
      data.mode = modes[mode];
      data.crc = computeCRC();

      Serial.println("Assigned.");
      EEPROM.put(0, data);
      return EEPROM.commit();
    }
};
decltype(EEPROM_DATA_Manager::modes) EEPROM_DATA_Manager::modes{0xAA55CC33, 0xE1D2C3B4};
static_assert(sizeof(EEPROM_DATA) < 4096);

EEPROM_DATA_Manager configuration;

// Set web server port number to 80
WiFiServer server(80);

// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 20000;

// Red, green, and blue pins for PWM control
// Red is on GPIO 13
// Green is on GPIO 12
// Blue is on GPIO 14
const int pinRGB[3] = {13, 12, 14};
// Setting PWM bit resolution
const int resolution = 256;

// timeout after which the leds should be turned off
int remainingTime = -1;
// color to set once the remaining time has run to <0
int targetRGB [3] = {0, 0, 0};

void setup() {
  Serial.begin(115200);

  // configure LED PWM resolution/range and set pins to LOW
  analogWriteRange(resolution);
  analogWrite(pinRGB[0], 0);
  analogWrite(pinRGB[1], 0);
  analogWrite(pinRGB[2], 0);

  configuration.initialize();

  wifi_station_set_hostname(configuration.getHostname().c_str());
  switch (configuration.getMode()) {
    default:
    case EEPROM_DATA_Manager::EMODES::MODE_BASE_STATION:
      {
        Serial.print("Setting soft-AP ... ");
        Serial.println(WiFi.softAP(configuration.getSSID(), configuration.getPassword()) ? "Ready" : "Failed!");
        Serial.print("Soft-AP IP address = ");
        Serial.println(WiFi.softAPIP());
      } break;
    case EEPROM_DATA_Manager::EMODES::MODE_NODE:
      {
        // Connect to Wi-Fi network with SSID and password
        Serial.print("Connecting to ");
        Serial.println(configuration.getSSID());
        wifi_station_set_auto_connect(true);
        WiFi.begin(configuration.getSSID(), configuration.getPassword());
        while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
        }
        // Print local IP address and start web server
        Serial.println("");
        Serial.println("WiFi connected.");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
      } break;
  }

  server.begin();
}

void updateLedValues() {
  static int currentRGB [3] = {0, 0, 0};
  static int lastUpdate = millis();

  int now = millis();
  if ((now - lastUpdate) > 200) {
    int diff_t = now - lastUpdate;
    lastUpdate = now;

    if (remainingTime > 0) {
      remainingTime -= diff_t;
      if (remainingTime > 0) return;
    }

    for (int i = 0 ; i < 3 ; ++i) {
      int dRGB = targetRGB[i] - currentRGB[i];
      const int diff = 3;
      if (dRGB >= diff || dRGB <= -diff)
        dRGB /= diff;
      else if (dRGB >= 1)
        dRGB = 1;
      else if (dRGB <= -1)
        dRGB = -1;

      if (dRGB != 0) {
        currentRGB[i] += dRGB;
        analogWrite(pinRGB[i], currentRGB[i]);
      }
    }
  }
}

const String websitePartA = "<!DOCTYPE html>\n"
                            "<html>\n"
                            "<head>\n"
                            "<meta charset='UTF-8'/>\n"
                            "<meta http-equiv='content-type' content='text/html'/>\n"
                            "<meta name='viewport' content='width=device-width, initial-scale=1.0'/>\n"
                            "<style>\n"
                            "body { background-color: #222;color: #CCC;font-family: 'Kristen ITC', Comic Sans, sans-serif }\n"
                            "button { border-radius: 12px;background-color: #cf3;border: none;color: black;padding: 10px;text-align: center;text-decoration: none;display: inline-block;font-size: 12px;margin: 4px 2px;cursor: pointer; }\n"
                            "button.predefined_color_button { border-radius: 50%; height: 40px; width:40px; margin: 5px; }\n"
                            "range { background-color: #00000000; }\n"
                            "</style>\n"
                            "<title>Nestbeleuchtung</title>\n"
                            "<script type='text/javascript'>\n"
                            "var red = 0;var green = 0;var blue = 0;\n"
                            "function UpdateColorDisplay(r, g, b, overrideColor) {\n"
                            "  var hexString = ((r << 16) + (g << 8) + b).toString(16);\n"
                            "  var extendedHexString = '000000' + hexString.toUpperCase();\n"
                            "  var colorCode = '#' + (extendedHexString).slice(-6);\n"
                            "  var div = document.getElementById('divpreview');\n"
                            "  if(overrideColor === undefined) { div.style.backgroundColor = colorCode; }\n"
                            "  else { div.style.backgroundColor = overrideColor; }\n"
                            "  div.innerHTML = colorCode;\n"
                            "  if((r + g + b) < 128) { div.style.color = '#FFC'; }\n"
                            "  else { div.style.color = '#000'; }\n"
                            "  document.getElementById('Slider_Rot').value = r;\n"
                            "  document.getElementById('Slider_Gras').value = g;\n"
                            "  document.getElementById('Slider_Blau').value = b;\n"
                            "}\n"
                            "function ReadColor() {\n"
                            "  var xmlHttp = new XMLHttpRequest();\n"
                            "  xmlHttp.onreadystatechange = function() {\n"
                            "    if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {\n"
                            "        var entries = xmlHttp.responseText.split('\\n');\n"
                            "        red = Number(entries[0]);\n"
                            "        green = Number(entries[1]);\n"
                            "        blue = Number(entries[2]);\n"
                            "        UpdateColorDisplay(red, green, blue);\n"
                            "    }\n"
                            "  }\n"
                            "  xmlHttp.open('GET', 'http://";
const String websitePartB = "/Data.plain', true);\n"
                            "  xmlHttp.send(null);\n"
                            "}\n"
                            "function clickColor(r, g, b, time, overrideColor) {\n"
                            "  red = r;\n"
                            "  green = g;\n"
                            "  blue = b;\n"
                            "  var request = new XMLHttpRequest();\n"
                            "  request.open('POST', 'http://";
const String websitePartC = "', true);\n"
                            "  request.send('r' + red + 'g' + green + 'b' + blue + 't' + time);\n"
                            "  UpdateColorDisplay(red, green, blue, overrideColor);\n"
                            "}\n"
                            "ReadColor();\n"
                            "</script>\n"
                            "</head>\n"
                            "<body>\n"
                            "<div>\n"
                            "<h1><u>Bett Beleuchtung</u></h1>\n"
                            "<p><u><font color='#f00'>F</font><font color='#f60'>a</font><font color='#fd0'>r</font><font color='#bf0'>b</font><font color='#4f0'>e</font><font color='#0f2'>i</font><font color='#0f9'>n</font><font color='#0ff'>s</font><font color='#09f'>t</font><font color='#02f'>e</font><font color='#40f'>l</font><font color='#b0f'>l</font><font color='#f0d'>u</font><font color='#f06'>n</font><font color='#f00'>g</font></u>:</p>\n"
                            "<div>\n"
                            "<button class='predefined_color_button' style='background-color: #FFFFEF' onclick='clickColor(0xFF, 0xff, 0x98, 0, getComputedStyle(this)[\"background-color\"])'/>\n"
                            "<button class='predefined_color_button' style='background-color: #D88700' onclick='clickColor(0xA2, 0x2A, 0x00, 0, getComputedStyle(this)[\"background-color\"])'/>\n"
                            "<button class='predefined_color_button' style='background-color: #FF6800' onclick='clickColor(0xFF, 0x28, 0x00, 0, getComputedStyle(this)[\"background-color\"])'/>\n"
                            "</div>\n"
                            "<div>\n"
                            "<button class='predefined_color_button' style='background-color: #F6FD00' onclick='clickColor(0xA4, 0x51, 0x00, 0, getComputedStyle(this)[\"background-color\"])'/>\n"
                            "<button class='predefined_color_button' style='background-color: #600000' onclick='clickColor(0x09, 0x00, 0x00, 0, getComputedStyle(this)[\"background-color\"])'/>\n"
                            "<button class='predefined_color_button' style='background-color: #A1008A' onclick='clickColor(0xA1, 0x00, 0x8A, 0, getComputedStyle(this)[\"background-color\"])'/>\n"
                            "</div>\n"
                            "<div>\n"
                            "<button class='predefined_color_button' style='background-color: #009900' onclick='clickColor(0x00, 0x6c, 0x00, 0, getComputedStyle(this)[\"background-color\"])'/>\n"
                            "<button class='predefined_color_button' style='background-color: #00B39C' onclick='clickColor(0x00, 0x6C, 0x36, 0, getComputedStyle(this)[\"background-color\"])'/>\n"
                            "<button class='predefined_color_button' style='background-color: #A1004C' onclick='clickColor(0xA1, 0x00, 0x1F, 0, getComputedStyle(this)[\"background-color\"])'/>\n"
                            "</div>\n"
                            "<div style='width:236px;'>\n"
                            "<div id='divpreview' style='background-color: rgb(0,0,0); text-align: center;color: #000;font-family: mono;'>&nbsp;</div>\n"
                            "</div>\n"
                            "<div style='margin:10px'>\n"
                            "<u>Timer</u>:<br/>\n"
                            "<button onclick='clickColor(0,0,0,0)'>sofort</button>\n"
                            "<button onclick='clickColor(0,0,0,15*60*1000)'>in 15 min.</button>\n"
                            "<button onclick='clickColor(0,0,0,30*60*1000)'>in 30 min.</button>\n"
                            "</div>\n"
                            "<div style='margin:10px'>\n"
                            "<u>Regler</u>:<br/>\n"
                            "<table>\n"
                            "<tr><td>Rot:</td><td><input type='range' min='0' max='255' value='0' class='slider' id='Slider_Rot'></td></tr>\n"
                            "<tr><td>Gr&uuml;n:</td><td><input type='range' min='0' max='255' value='0' class='slider' id='Slider_Gras'> </td></tr>\n"
                            "<tr><td>Blau:</td><td><input type='range' min='0' max='255' value='0' class='slider' id='Slider_Blau'> </td></tr>\n"
                            "</table>\n"
                            "</div>\n"
                            "<script type='text/javascript'>\n"
                            "var Rot = document.getElementById('Slider_Rot');\n"
                            "Rot.oninput = function() { clickColor (parseInt(this.value), green, blue, 0); }\n"
                            "var Gras = document.getElementById('Slider_Gras');\n"
                            "Gras.oninput = function() { clickColor (red, parseInt(this.value), blue, 0); }\n"
                            "var Blau = document.getElementById('Slider_Blau');\n"
                            "Blau.oninput = function() { clickColor (red, green, parseInt(this.value), 0); }\n"
                            "</script>\n"
                            "</div>\n"
                            "</body>\n"
                            "</html>\n";

const String configsitePartA = "<!DOCTYPE html>\n"
                               "<html>\n"
                               "<head>\n"
                               "<meta charset='UTF-8'/>\n"
                               "<meta http-equiv='content-type' content='text/html'/>\n"
                               "<meta name='viewport' content='width=device-width, initial-scale=1.0'/>\n"
                               "<style>\n"
                               "body { background-color: #222;color: #CCC;font-family: 'Kristen ITC', Comic Sans, sans-serif; }\n"
                               "input, select { background-color: #111;color: #CCC;font-family: 'Kristen ITC', Comic Sans, sans-serif; margin: 4px 2px; border: none; min-width: 250px;}\n"
                               "button { border-radius: 12px;background-color: #cf3;border: none;color: black;padding: 10px;text-align: center;text-decoration: none;display: inline-block;font-size: 12px;margin: 4px 2px;cursor: pointer; }\n"
                               "</style>\n"
                               "<title>Nestbeleuchtung</title>\n"
                               "<script type='text/javascript'>\n"
                               "function SendData() {\n"
                               "  var postData = '';\n"
                               "  var auth_key = document.getElementById('auth_key').value;\n"
                               "  var new_auth_key = document.getElementById('new_auth_key').value;\n"
                               "  postData += ';auth_key:' + auth_key;\n"
                               "  if(new_auth_key == '') {\n"
                               "    postData += ';new_auth_key:' + auth_key;\n"
                               "  } else {\n"
                               "    postData += ';new_auth_key:' + new_auth_key;\n"
                               "  }\n"
                               "  postData += ';mode:' + document.getElementById('mode').value;\n"
                               "  postData += ';wlan_ssid:' + document.getElementById('wlan_ssid').value;\n"
                               "  postData += ';wlan_pwd:' + document.getElementById('wlan_password').value;\n"
                               "  postData += ';hostname:' + document.getElementById('hostname').value;\n"
                               "  var xmlHttp = new XMLHttpRequest();\n"
                               "  xmlHttp.onreadystatechange = function() {\n"
                               "    if (xmlHttp.readyState == 4) {\n"
                               "      if(xmlHttp.status == 200) {\n"
                               "        document.getElementById('result_div').innerHTML = 'Values Set.';\n"
                               "      } else {\n"
                               "        document.getElementById('result_div').innerHTML = 'Failed to set Values.<br/>Reported error code is: ' + xmlHttp.status;\n"
                               "      }\n"
                               "    }\n"
                               "  }\n"
                               "  xmlHttp.open('POST', 'http://";
const String configsitePartB = "/config.plain', true);\n"
                               "  xmlHttp.send(postData);\n"
                               "}\n"
                               "</script>\n"
                               "</head>\n"
                               "<body>\n"
                               "<h2>Configuration</h2>\n"
                               "<dl>\n"
                               "<dt><label for='auth_key'>Auth-Key</label></dt>\n"
                               "<dd><input type='text' id='auth_key' pattern='[^;:]{8,100}' value=''/></dd>\n"
                               "<dt><label for='new_auth_key'>New Auth-Key</label></dt>\n"
                               "<dd><input type='text' id='new_auth_key' pattern='[^;:]{8,100}' value=''/></dd>\n"
                               "<dt><label for='mode'>Mode</label></dt>\n"
                               "<dd>\n"
                               "<select id='mode'>\n"
                               "<option value='0' ";
const String configsitePartC = ">Accesspoint</option>\n"
                               "<option value='1' ";
const String configsitePartD = ">Join WiFi</option>\n"
                               "</select>\n"
                               "</dd>\n"
                               "<dt><label for='wlan_ssid'>WLAN-SSID</label></dt>\n"
                               "<dd><input type='text' id='wlan_ssid' pattern='[^;:]{1,32}' value='";
const String configsitePartE = "'/></dd>\n"
                               "<dt><label for='wlan_password'>WLAN-Password</label></dt>\n"
                               "<dd><input type='text' id='wlan_password' pattern='[^;:]{8,63}' value=''/></dd>\n"
                               "<dt><label for='hostname'>Hostname</label></dt>\n"
                               "<dd><input type='text' id='hostname' pattern='[^;:]{1,253}' value='";
const String configsitePartF = "'/></dd>\n"
                               "</dl>\n"
                               "<button onclick='SendData()' style='min-width: 150px'>Submit</button>\n"
                               "<div id='result_div'>\n"
                               "&nbsp;\n"
                               "</div>\n"
                               "</body>\n"
                               "</html>\n";


void HandleGet(WiFiClient& client, const String & header)
{
  enum {Page, Config, Values, RedValue, GreenValue, BlueValue, TimeValue};
  int contentID = -1;
  String contentType = "text";
  if (header.startsWith("/ ")) {
    contentID = Page;
    contentType = "text/html";
  } else if (header.startsWith("/config.html ")) {
    contentID = Config;
    contentType = "text/html";
  } else if (header.startsWith("/Data.plain ")) {
    contentID = Values;
    contentType = "text/plain";
  } else if (header.startsWith("/Red.plain ")) {
    contentID = RedValue;
    contentType = "text/plain";
  } else if (header.startsWith("/Green.plain ")) {
    contentID = GreenValue;
    contentType = "text/plain";
  } else if (header.startsWith("/Blue.plain ")) {
    contentID = BlueValue;
    contentType = "text/plain";
  } else if (header.startsWith("/Time.plain ")) {
    contentID = TimeValue;
    contentType = "text/plain";
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println();

    client.println("404: Page not Found!");
    client.println();
    return;
  }
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type: " + contentType);
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();

  switch (contentID)
  {
    case Page: // Display the HTML web page
      client.print(websitePartA);
      client.print(configuration.getHostname());
      client.print(websitePartB);
      client.print(configuration.getHostname());
      client.print(websitePartC);
      client.println();
      break;
    case Config:
    {
      bool isBaseStationMode = configuration.getMode() == EEPROM_DATA_Manager::EMODES::MODE_BASE_STATION;
      
      client.print(configsitePartA);
      if(configuration.getHostname().length() > 0) {
        client.print(configuration.getHostname());
      } else if(isBaseStationMode){
        client.print(WiFi.softAPIP()); // transfer the current IP of the hot spot.
      } else {
        client.print(WiFi.localIP()); // transfer the current IP of the node.
      }
      client.print(configsitePartB);
      if(isBaseStationMode) {
        client.print("selected");
      }
      client.print(configsitePartC);
      if(!isBaseStationMode) {
        client.print("selected");
      }
      client.print(configsitePartD);
      client.print(configuration.getSSID());
      client.print(configsitePartE);
      client.print(configuration.getHostname());
      client.print(configsitePartF);
      client.println();
    } break;
    case Values:
      client.println(String(targetRGB[0]));
      client.println(String(targetRGB[1]));
      client.println(String(targetRGB[2]));
      client.println(String(remainingTime));
      break;
    case RedValue:
      client.println(String(targetRGB[0]));
      break;
    case GreenValue:
      client.println(String(targetRGB[1]));
      break;
    case BlueValue:
      client.println(String(targetRGB[2]));
      break;
    case TimeValue:
      client.println(String(remainingTime));
      break;
  }
  // The HTTP response ends with one blank line
  client.println();
}

String ParseValue(const String & sourceString, const String& key) {
  int index = sourceString.indexOf(key);
  if (index < 0) {
    return "";
  }
  index += key.length();
  int endindex = sourceString.indexOf(";", index);
  if (endindex < 0) {
    endindex = sourceString.length();
  }
  return sourceString.substring(index, endindex);
}

void HandlePost(WiFiClient& client, const String & header)
{
  enum {LEDs, Config};
  int contentID = -1;
  String contentType = "text/plain";
  if (header.startsWith("/ ")) {
    contentID = LEDs;
  } else if (header.startsWith("/config.plain ")) {
    contentID = Config;
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println();

    client.println("404: Page not Found!");
    client.println();
    return;
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Content-type: " + contentType);
  client.println("Connection: Keep-Alive");
  client.println("");

  uint32_t ctentLength = header.substring(header.indexOf("Content-Length: ") + 16).toInt();
  String content = "";
  while (client.connected() && content.length () < ctentLength) {
    if (client.available()) content += (char)client.read();
  }

  switch (contentID)
  {
    case LEDs:
      {
        int index = content.indexOf('r');
        if (index >= 0) targetRGB[0] = content.substring(index + 1).toInt();
        index = content.indexOf('g');
        if (index >= 0) targetRGB[1] = content.substring(index + 1).toInt();
        index = content.indexOf('b');
        if (index >= 0) targetRGB[2] = content.substring(index + 1).toInt();
        index = content.indexOf('t');
        if (index >= 0) remainingTime = content.substring(index + 1).toInt();

        client.println("HTTP/1.1 200 OK");
        client.println("Access-Control-Allow-Origin: *");
        client.println("Connection: close");
        client.println();
      } break;
    case Config:
      {
        String oldPassword = ParseValue(content, "auth_key:");
        String newPassword = ParseValue(content, "new_auth_key:");
        String ssid = ParseValue(content, "wlan_ssid:");
        String password = ParseValue(content, "wlan_pwd:");
        String hostname = ParseValue(content, "hostname:");
        EEPROM_DATA_Manager::EMODES mode = EEPROM_DATA_Manager::EMODES (ParseValue(content, "mode:").toInt());
        if (configuration.setNewConfig(oldPassword, newPassword, ssid, password, hostname, mode)) {
          client.println("HTTP/1.1 200 OK");
          client.println("Access-Control-Allow-Origin: *");
          client.println("Connection: close");
          client.println();
          delay(1000);
          ESP.restart();
        } else {
          client.println("HTTP/1.1 409 Conflict");
          client.println("Access-Control-Allow-Origin: *");
          client.println("Connection: close");
          client.println();
        }
      } break;
  }
}

void loop() {
  static unsigned long previousTime = 0;
  updateLedValues();

  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    unsigned long currentTime = millis();
    previousTime = currentTime;
    char previousCharacter = 0;
    Serial.println("New Client.");          // print a message out in the serial port
    String header = "";
    while (client.connected()
           && (currentTime - previousTime) <= timeoutTime) {            // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n' && previousCharacter == '\n') {                    // if the byte is a newline character
          Serial.println("End Of request Found.");
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          int index = 0;
          if ((index = header.indexOf("GET ")) >= 0) {
            HandleGet(client, header.substring(index + 4));
          } else if ((index = header.indexOf("POST ")) >= 0) {
            HandlePost(client, header.substring(index + 5));
          }
          // Break out of the while loop to terminate the connection
          break;
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          previousCharacter = c; // add it to the end of the currentLine
        }
      }
    }
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
