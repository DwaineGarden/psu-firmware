/*
 * EEZ PSU Firmware
 * Copyright (C) 2015-present, Envox d.o.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#pragma once

namespace eez {
namespace psu {

class Channel;

namespace profile {

struct Parameters;

}
}
} // namespace eez::psu::profile

namespace eez {
namespace psu {
/// Store/restore of persistent configuration data (device configuration, calibration parameters, profiles) using external EEPROM.
namespace persist_conf {

/// Header of the every block stored in EEPROM. It contains checksum and version.
struct BlockHeader {
    uint32_t checksum;
    uint16_t version;
};

/// Device binary flags stored in DeviceConfiguration.
struct DeviceFlags {
    unsigned isSoundEnabled : 1;
    unsigned dateValid : 1;
    unsigned timeValid : 1;
    unsigned profileAutoRecallEnabled : 1;
    unsigned dst : 1;
    unsigned channelsViewMode : 3;
    unsigned ethernetEnabled : 1;
    unsigned outputProtectionCouple : 1;
    unsigned shutdownWhenProtectionTripped : 1;
    unsigned forceDisablingAllOutputsOnPowerUp : 1;
    unsigned isFrontPanelLocked: 1;
    unsigned isClickSoundEnabled : 1;
    unsigned ch1CalEnabled: 1;
    unsigned ch2CalEnabled: 1;
    unsigned reserved: 16;
};

/// Device configuration block.
struct DeviceConfiguration {
    BlockHeader header;
    char serialNumber[7 + 1];
    char calibration_password[PASSWORD_MAX_LENGTH + 1];
    DeviceFlags flags;
    uint8_t date_year;
    uint8_t date_month;
    uint8_t date_day;
    uint8_t time_hour;
    uint8_t time_minute;
    uint8_t time_second;
	int16_t time_zone;
    int8_t profile_auto_recall_location;
    int8_t touch_screen_cal_orientation;
    int16_t touch_screen_cal_tlx;
    int16_t touch_screen_cal_tly;
    int16_t touch_screen_cal_brx;
    int16_t touch_screen_cal_bry;
    int16_t touch_screen_cal_trx;
    int16_t touch_screen_cal_try;
#ifdef EEZ_PSU_SIMULATOR
    bool gui_opened;
#endif // EEZ_PSU_SIMULATOR
};

/// Device binary flags stored in DeviceConfiguration.
struct DeviceFlags2 {
    unsigned encoderConfirmationMode : 1;
    unsigned displayState: 1;
    unsigned triggerContinuousInitializationEnabled: 1;
    unsigned skipChannelCalibrations: 1;
    unsigned skipDateTimeSetup: 1;
    unsigned skipSerialSetup: 1;
    unsigned skipEthernetSetup: 1;
    unsigned serialEnabled: 1;
    unsigned ethernetDhcpEnabled: 1;
    unsigned ntpEnabled: 1;
    unsigned sdLocked: 1;
};

struct IOPin {
    unsigned polarity: 1;
    unsigned function: 7;
};

struct SerialConf {
    unsigned bits: 2;
    unsigned parity: 2;
    unsigned sbits: 1;
};

struct DeviceConfiguration2 {
    BlockHeader header;
    char systemPassword[PASSWORD_MAX_LENGTH + 1];
    DeviceFlags2 flags;
    uint8_t encoderMovingSpeedDown;
    uint8_t encoderMovingSpeedUp;
    uint8_t displayBrightness;
    uint8_t triggerSource;
    float triggerDelay;
    IOPin ioPins[3];
    uint8_t serialBaud;
    uint8_t serialParity;
    uint32_t ethernetIpAddress;
    uint32_t ethernetDns;
    uint32_t ethernetGateway;
    uint32_t ethernetSubnetMask;
    uint16_t ethernetScpiPort;
    char ntpServer[32+1];
    uint8_t dstRule;
    uint8_t ethernetMacAddress[6];
    uint8_t displayBackgroundLuminosityStep;
    uint8_t reserverd[24];
};

static const uint16_t PROFILE_VERSION = 8;

bool check_block(const BlockHeader *block, uint16_t size, uint16_t version);
uint32_t calc_checksum(const BlockHeader *block, uint16_t size);

extern DeviceConfiguration devConf;
extern DeviceConfiguration2 devConf2;

void loadDevice();
bool saveDevice();

void loadDevice2();
bool saveDevice2();

bool isSystemPasswordValid(const char *new_password, size_t new_password_len, int16_t &err);
bool changeSystemPassword(const char *new_password, size_t new_password_len);

bool isCalibrationPasswordValid(const char *new_password, size_t new_password_len, int16_t &err);
bool changeCalibrationPassword(const char *new_password, size_t new_password_len);

bool changeSerial(const char *newSerialNumber, size_t newSerialNumberLength);

bool enableSound(bool enable);
bool isSoundEnabled();

bool enableClickSound(bool enable);
bool isClickSoundEnabled();

bool enableEthernet(bool enable);
bool isEthernetEnabled();

bool readSystemDate(uint8_t &year, uint8_t &month, uint8_t &day);
void writeSystemDate(uint8_t year, uint8_t month, uint8_t day, unsigned dst);

bool readSystemTime(uint8_t &hour, uint8_t &minute, uint8_t &second);
void writeSystemTime(uint8_t hour, uint8_t minute, uint8_t second, unsigned dst);

void writeSystemDateTime(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, unsigned dst);

bool enableProfileAutoRecall(bool enable);
bool isProfileAutoRecallEnabled();
bool setProfileAutoRecallLocation(int location);
int getProfileAutoRecallLocation();

void toggleChannelsViewMode();
void setChannelsViewMode(unsigned int viewMode);

void loadChannelCalibration(Channel &channel);
bool saveChannelCalibration(Channel &channel);

void saveCalibrationEnabledFlag(Channel &channel, bool enabled);

bool loadProfile(int location, profile::Parameters *profile);
bool saveProfile(int location, profile::Parameters *profile);

uint32_t readTotalOnTime(int type);
bool writeTotalOnTime(int type, uint32_t time);

bool enableOutputProtectionCouple(bool enable);
bool isOutputProtectionCoupleEnabled();

bool enableShutdownWhenProtectionTripped(bool enable);
bool isShutdownWhenProtectionTrippedEnabled();

bool enableForceDisablingAllOutputsOnPowerUp(bool enable);
bool isForceDisablingAllOutputsOnPowerUpEnabled();

bool lockFrontPanel(bool lock);

bool setEncoderSettings(uint8_t confirmationMode, uint8_t movingSpeedDown, uint8_t movingSpeedUp);

bool setDisplayState(unsigned state);
bool setDisplayBrightness(uint8_t displayBrightness);
bool setDisplayBackgroundLuminosityStep(uint8_t displayBackgroundLuminosityStep);

bool enableSerial(bool enable);
bool isSerialEnabled();
int getIndexFromBaud(long baud);
long getBaudFromIndex(int index);
int getSerialBaudIndex();
bool setSerialBaudIndex(int baudIndex);
int getSerialParity();
bool setSerialParity(int parity);
bool setSerialSettings(bool enable, int baudIndex, int parity);

bool enableEthernetDhcp(bool enable);
bool isEthernetDhcpEnabled();
bool setEthernetMacAddress(uint8_t macAddress[]);
bool setEthernetIpAddress(uint32_t ipAddress);
bool setEthernetDns(uint32_t dns);
bool setEthernetGateway(uint32_t gateway);
bool setEthernetSubnetMask(uint32_t subnetMask);
bool setEthernetScpiPort(uint16_t scpiPort);
bool setEthernetSettings(bool enable, bool dhcpEnable, uint32_t ipAddress, uint32_t dns, uint32_t gateway, uint32_t subnetMask, uint16_t scpiPort, uint8_t *macAddress);

bool enableNtp(bool enable);
bool isNtpEnabled();
bool setNtpServer(const char *ntpServer, size_t ntpServerLength);
bool setNtpSettings(bool enable, const char *ntpServer);

bool setSdLocked(bool sdLocked);
bool isSdLocked();

}
}
} // namespace eez::psu::persist_conf
