
// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <array>

#include "color_space_conversions.hpp"
using ColorSpace = RGBW_t;

#include "eeprom.hpp"
decltype(EEPROM_DATA_Manager::modes) EEPROM_DATA_Manager::modes{0xAA55CC33, 0xE1D2C3B4};

#include "page.h"


EEPROM_DATA_Manager configuration;

// Set web server port number to 80
WiFiServer server(80);

// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 20000;

const std::array<int,4>pinsRGBW{14/*R*/, 12/*G*/, 16/*B*/, 13/*W*/};
// Setting PWM bit resolution
const int PwmPeriod = 1024;
const std::array<int,0> buttons;

// timeout after which the leds should be turned off
int remainingTime = -1;
// color to set once the remaining time has run to <0
ColorSpace targetRGB {0,0,0,0};

void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");

  pinMode(0, INPUT);
  // configure LED PWM resolution/range and set pins to LOW
  analogWriteRange(PwmPeriod);
  for(const auto & pin : pinsRGBW){
    analogWrite(pin, 0);  
  }
  
  configuration.initialize();
  targetRGB = configuration.getInitialColor();

  wifi_station_set_hostname(configuration.getHostname().c_str());
  Serial.println("");
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
        int counter = 0;
        for (uint i = 0 ; i < targetRGB.size() ; ++i) {
            analogWrite(pinsRGBW[i], targetRGB[i]);
        }
        while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
          if(++counter > 8 && !digitalRead(0)) {
            configuration.resetToBaseMode();
            delay(1000);
            ESP.restart();
          }
        }
        // Print local IP address and start web server
        Serial.println("");
        Serial.println("WiFi connected.");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.println("hostname: ");
        Serial.println(configuration.getHostname().c_str());
      } break;
  }

  server.begin();
}

void updateLedValues() {
  static auto currentRGB = targetRGB;
  static int lastUpdate = millis();
  static int lastTargetSet = millis();

  int now = millis();
  int diff_t = now - lastUpdate;
  if (diff_t > 10) {
    lastUpdate = now;

    if (remainingTime > 0) {
      remainingTime -= diff_t;
      if (remainingTime > 0) return;
    }

    for (uint i = 0 ; i < targetRGB.size() ; ++i) {
      int dRGB = targetRGB[i] - currentRGB[i];
      const int diff = 20;
      if (dRGB >= diff || dRGB <= -diff)
        dRGB /= diff;
      else if (dRGB >= 1)
        dRGB = 1;
      else if (dRGB <= -1)
        dRGB = -1;

      if (dRGB != 0) {
        currentRGB[i] += dRGB;
        analogWrite(pinsRGBW[i], currentRGB[i]);
        Serial.print(i);
        Serial.print(" ");
        Serial.println(currentRGB[i]);
        lastTargetSet = now;
      }
    }
    if (targetRGB == currentRGB && now - lastTargetSet > 5000 && targetRGB != configuration.getInitialColor()) {
      configuration.updateInitialColor(targetRGB);
    }
  }
}


void handleButtons() {
  static int btnPressed[2] = {0, 0};
  static int previousTime = 0;
  int currentTime = millis();
  if(currentTime <= previousTime + 10) return;
  previousTime = currentTime;
  
  for(uint i = 0 ; i < buttons.size(); ++i){
    const int t0 = 35;
    if(digitalRead(buttons[i])){
      if(++btnPressed[i] > t0) {
        int updateValue = btnPressed[i] - t0;
        remainingTime  = -1;
        if(updateValue >= (PwmPeriod * 2))
          btnPressed[i] = t0;
        if(updateValue >= PwmPeriod) {
          for(auto & value : targetRGB)
            value = (PwmPeriod * 2) - updateValue;
        } else if (updateValue >= (PwmPeriod / 2)) {
          for(auto & value : targetRGB)
            value = updateValue;
        } else {
          for(auto & value : targetRGB)
            value = 0;
          targetRGB[0] = updateValue;
        }
      }
    } else if (btnPressed[i] < t0) {
      if(btnPressed[i] > 7) {
        for(auto & value : targetRGB)
            value = 0;
      }
      btnPressed[i] = 0;
    } else {
      btnPressed[i] = 0;
    }
  }
}

template<typename ColorSpace>
void PrintTargetColor(const ColorSpace & target){
  Serial.print("The Target Color is: ");
  Serial.print(String(target[0]));
  Serial.print(", ");
  Serial.print(String(target[1]));
  Serial.print(", ");
  Serial.print(String(target[2]));
  Serial.print(", ");
  Serial.println(String(target[3]));

}

void HandleGet(WiFiClient& client, const String & header)
{
  Serial.println("Get:");
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
    case Values:{
      const auto color = convert<ColorSpace, RGB_t>(targetRGB);
      client.println(String(color[0]*256/PwmPeriod));
      client.println(String(color[1]*256/PwmPeriod));
      client.println(String(color[2]*256/PwmPeriod));
      client.println(String(remainingTime));
      PrintTargetColor<RGBW_t>(targetRGB);
    } break;
    case RedValue:{
      const auto color = convert<ColorSpace, RGB_t>(targetRGB);
      client.println(String(color[0]*256/PwmPeriod));
    } break;
    case GreenValue:{
      const auto color = convert<ColorSpace, RGB_t>(targetRGB);
      client.println(String(color[1]*256/PwmPeriod));
    } break;
    case BlueValue:{
      const auto color = convert<ColorSpace, RGB_t>(targetRGB);
      client.println(String(color[2]*256/PwmPeriod));
    } break;
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
  Serial.println("Post:");
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
        auto color = convert<ColorSpace, RGB_t>(targetRGB);
        int index = content.indexOf('r');
        if (index >= 0) color[0] = content.substring(index + 1).toInt();
        index = content.indexOf('g');
        if (index >= 0) color[1] = content.substring(index + 1).toInt();
        index = content.indexOf('b');
        if (index >= 0) color[2] = content.substring(index + 1).toInt();
        index = content.indexOf('t');
        if (index >= 0) remainingTime = content.substring(index + 1).toInt();
        targetRGB = convert<RGB_t, ColorSpace>(color);
        for(auto& value : targetRGB)
        {
          value = value * PwmPeriod / 256;
        }
        PrintTargetColor<RGBW_t>(targetRGB);
        
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
  static std::array<char, 6> buffer{0};
  static int buffer_offset{0};
  int c = Serial.read();
  if(c > 0 && c < 'z')
  {
    buffer[buffer_offset % buffer.size()] = c;
    if(buffer == decltype(buffer){'R','e','s','e','t','!'}) configuration.resetToBaseMode();
    if(c ==10  || c ==13) buffer_offset = 0;
  }
  
  handleButtons();
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
