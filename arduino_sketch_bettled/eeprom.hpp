#include <EEPROM.h>
#include <array>

struct EEPROM_DATA {
  uint32_t crc{0};
  uint32_t mode{0};
  std::array<char, 0x40> configPassword;
  std::array<char, 0x40> ssid;
  std::array<char, 0x40> password;
  std::array<char, 0x40> hostname;
  ColorSpace initialColor;
};

namespace {
    template <typename ArrayType, typename use = void>
    uint32_t AddToCRC(uint32_t crc, const ArrayType& data) {
      for (const auto character : data) {
        crc = AddToCRC(crc, character);
      }
      return crc;
    }
    template <>
    uint32_t AddToCRC(uint32_t crc, const char& character) {
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
    template <>
    uint32_t AddToCRC(uint32_t crc, const int& data) {
      for(int i = 24; i >= 0; i -= 8){
        crc = AddToCRC(crc, (char)(data >> i));
      }
      return crc;
    }
}

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

    uint32_t computeCRC() const {
      uint32_t crc = 0xFFffFFff;
      crc = AddToCRC(crc, data.configPassword);
      crc = AddToCRC(crc, data.ssid);
      crc = AddToCRC(crc, data.password);
      crc = AddToCRC(crc, data.hostname);
      crc = AddToCRC(crc, char(data.mode));
      crc = AddToCRC(crc, data.initialColor);
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
      data.initialColor = ColorSpace{0};
    }
  public:
    enum EMODES {
      MODE_BASE_STATION = 0,
      MODE_NODE = 1,
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

    auto getHostname() const -> const String {
      return String(data.hostname.data());
    }

    auto getSSID() const -> const String {
      return String(data.ssid.data());
    }

    auto getPassword() const -> const String {
      return String(data.password.data());
    }

    auto getInitialColor() -> const ColorSpace& {
      return data.initialColor;
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

    void updateInitialColor(const ColorSpace &newColor) {
      data.initialColor = newColor;
      data.crc = computeCRC();

      Serial.println("Assigned.");
      EEPROM.put(0, data);
      EEPROM.commit();
    }
};
static_assert(sizeof(EEPROM_DATA) < 4096, "EEPROM data too large");
