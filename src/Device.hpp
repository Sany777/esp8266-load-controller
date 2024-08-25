#pragma once

#include "EEPROM.h"
#include "ActionsManager.hpp"
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>



enum{
	ONE_SEC 				= 1000,
	WEEK_DAYS_NUM			= 7,
	STR_MIN					= 1,
	STR_MAX	 				= 32,
	SSID_MIN				= 2,
	PWD_MIN					= 4,
	WIFI_TIMEOUT_MS			= 9000, 

	SERVER_MIN_TIMEOUT_SEC	= 60,
	SERVER_MAX_TIMEOUT_SEC	= 240,

    UPDATE_RELE_STATE_TIMEOUT_SEC = 1,
    UPDATE_TIME_TIMEOUT_SEC = 60*60*5,
    TRY_CONNECT_WIFI_SEC = 240,
    TRY_UPDATE_WIFI_SEC = 300,
};


struct Settings{
    uint8_t timer_on;
    uint8_t offsetHour;
    uint8_t schema[WEEK_DAYS_NUM];
    char pwd[STR_MAX+1];
    char ssid[STR_MAX+1];
};

class SettingsOperator{
    bool notSavedActions;
    bool notSavedSettings;

protected:
    Settings settings;
    
public:
    void setActivate(bool activate);
    bool setPWD(const char *cstr){ return safetyStrCopy(settings.pwd, cstr); }
    bool setSSID(const char *cstr){ return safetyStrCopy(settings.ssid, cstr); }
    bool isChangeSettings()const { return notSavedSettings; }
    bool isChangeActions()const { return notSavedActions; }
    bool timerOn()const { return settings.timer_on; }
    const char* getPWD() const { return settings.pwd; }
    const char* getSSID() const { return settings.ssid; }
    void setSchema(const uint8_t *schema);
    bool safetyStrCopy(char *src, const char *dst);
    void saveChanges();
    virtual void saveActions()=0;
    virtual void saveSettings()=0;
    virtual void flashRelease()=0;
};




class MemoryModule{
    EEPROMClass flash;
    size_t buf_size;
    void setScopeSize(const size_t data_size);

public:
    MemoryModule():buf_size(0){}
    void ReadFlash(int addr, uint8_t *dst, int data_size);
    void WriteFLash(int addr, const uint8_t *src, int data_size);
    void flashEnd();
};



class WifiModule{
    bool wifi_ok;
    static const constexpr char *AP_NAME = "8286";
    static const constexpr char *AP_PASS = "HipiyBilshiy";

public:
    void startAP();
    void disconnect();
    int getStationNum(){ return WiFi.softAPgetStationNum(); }
    bool isSTAConnection()const{ return wifi_ok; };
    bool connectSTA(const char *ssid, const char *pwd);
    
};

class GpioModule{
    static const int RELAY_GPIO = 0;
    bool releState;

public:
    GpioModule(){ pinMode( RELAY_GPIO, OUTPUT ); setRele(false); }
    bool getReleState()const{ return releState; };
    void setRele(bool activate){ releState = activate; digitalWrite(RELAY_GPIO, !releState); };
};



class TimeModule{
    static const constexpr int offsetHour = 2; 
    static const constexpr int HTTP_PORT = 80;
    static const constexpr char *NTP_SERVER = "pool.ntp.org";
    static const constexpr int DELAY_UPDATE_TIME = 600000;
    WiFiUDP ntpUDP;
    NTPClient *timeClient;
    int offset;
    void corectionDST();

public:
    TimeModule();
    static int convertToMin(int hour, int min){ return hour*60 + min; };
    static int convertTimeToMinutes(int min){ return min%60; };
    static int convertTImeToHours(int min){ return min/60; };
    bool initNTP();
    bool isTimeSet()const{ return timeClient->isTimeSet(); };
    void setOffset(const int timeOffset){ timeClient->setTimeOffset(timeOffset); };
    int getCurTimeMin();
    int getMinutes() const { return timeClient->getMinutes(); };
    int getHours() const { return timeClient->getHours()+offset; };
    int getDay() const { return timeClient->getDay(); };
};


class Device : public MemoryModule, 
               public WifiModule, 
               public TimeModule, 
               public SettingsOperator,
               public GpioModule,
               public ActionsManager
{
    static const uint32_t SETTINGS_ADDR = 1;
    static const uint32_t ACTIONS_ADDR = SETTINGS_ADDR+sizeof(Settings);
    void readSavedActions();
    void checkData();
    void flashRelease()override;

public:
    Device();
    void saveSettings()override;
    void saveActions()override;
    void readSettings();
    void setActionData(Action* actions_list, uint8_t *schema);
    bool connectSTA();
    void updateReleState();
};







