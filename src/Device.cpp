#include "Device.hpp"



ESP8266WiFiClass WiFi;

// ------------------------- Settings Operator

void SettingsOperator::setSchema(const uint8_t *schema)
{
    notSavedActions = notSavedSettings = true;
    memcpy(settings.schema, schema, sizeof(settings.schema)); 
}

void SettingsOperator::setActivate(bool activate)
{
    settings.timer_on = activate;
    notSavedSettings = true;
}

bool SettingsOperator::safetyStrCopy(char *dst, const char *src)
{
    const size_t len = strnlen(src, STR_MAX);
    if(len < STR_MAX){
        memcpy(dst, src, len);
        dst[len] = 0;
        notSavedSettings = true;
        return true;
    }
    return false;
}

void SettingsOperator::saveChanges()
{
    if(notSavedSettings){
        saveSettings();
        notSavedSettings = false;
    }
    if(notSavedActions){
        saveActions();
        notSavedActions = false;
    }
}

// ------------------------- Device

void Device::updateReleState()
{
    if(isTimeSet()){
        setRele(isSignal(getDay(), getCurTimeMin()));
    }
}

bool Device::connectSTA()
{
    bool r = WifiModule::connectSTA(settings.ssid, settings.pwd);
    if(r){
        if(TimeModule::initNTP())disconnect();
    }
    return r;
}

Device::Device()
{
    readSettings();
    readSavedActions();
}

void Device::checkData()
{
    if(settings.pwd[STR_MAX] != 0 
            || settings.ssid[STR_MAX] != 0){
        memset(&settings, 0, sizeof(Settings));
    }
}

void Device::saveActions()
{
    const size_t actions_num = getActionSum();
    if(actions_num){
        const Action* actions_list = getActionsList();
        WriteFLash(ACTIONS_ADDR, (const uint8_t*)actions_list, sizeof(Action)*actions_num);
    }
}

void Device::saveSettings()
{
    WriteFLash(SETTINGS_ADDR, (const uint8_t*)&settings, sizeof(Settings));
}

void Device::readSettings()
{
    ReadFlash(SETTINGS_ADDR, (uint8_t*)&settings, sizeof(Settings));
    checkData();
}

void Device::readSavedActions()
{
    const int actions_sum = ActionsManager::getActionSum(settings.schema);
    if(actions_sum){
        Action* actions_list = new Action[actions_sum];
        ReadFlash(ACTIONS_ADDR, (uint8_t*)actions_list, sizeof(Action)*actions_sum); 
        ActionsManager::setActionData(actions_list, settings.schema);
    }
}

void Device::setActionData(Action *actions_list, uint8_t *actions_in_day)
{
    setSchema(actions_in_day);
    ActionsManager::setActionData(actions_list, settings.schema);
    ActionsManager::sortActions();
}

void Device::flashRelease()
{
    MemoryModule::flashEnd();
}


//------------------------------ WifiModule

void WifiModule::disconnect()
{
    if(WIFI_AP == WiFi.getMode()){
        WiFi.softAPdisconnect();
    } else {
        WiFi.disconnect();
        WiFi.enableSTA(false);
    }
}

void WifiModule::startAP()
{
    if(WiFi.status() == WL_CONNECTED){
		disconnect();
        delay(500);
	}
	WiFi.softAP(AP_NAME, AP_PASS);
	delay(500);
}

bool WifiModule::connectSTA(const char *ssid, const char *pwd)
{
    if(WIFI_STA != WiFi.getMode()){
        if(WiFi.status() == WL_CONNECTED){
            disconnect();
            delay(500);
        }
        WiFi.enableSTA(true);
        WiFi.begin(ssid, pwd);
        int timeout = 0;
        delay(100);
        do{
            delay(1);
            wifi_ok = WiFi.status() == WL_CONNECTED;
            ++timeout;
        }while (!wifi_ok && timeout<WIFI_TIMEOUT_MS);
    }
    return wifi_ok;
}

// ------------------------------ MemoryModule

void MemoryModule::flashEnd()
{
   flash.end();
   buf_size = 0;
}

void MemoryModule::setScopeSize(const size_t data_size)
{
    if(data_size > this->buf_size){
        this->buf_size = data_size+1000;
        flash.begin(this->buf_size);
    }
}

void MemoryModule::ReadFlash(int addr, uint8_t *dst, int data_size)
{
    setScopeSize(data_size);
    while(data_size--)
        *(dst++) = flash.read(addr++);
}

void MemoryModule::WriteFLash(int addr, const uint8_t *src, int data_size)
{
    setScopeSize(data_size);
    while(data_size--){
        flash.write(addr++, *(src++));
    }
    flash.commit();
}


// -------------------------------- TimeModule

TimeModule::TimeModule()
{
    timeClient = new NTPClient(ntpUDP, NTP_SERVER, 0, DELAY_UPDATE_TIME);
}

bool TimeModule::initNTP()
{
    timeClient->begin();
	for (int i = 0; i<10; ++i){
        if(timeClient->forceUpdate()){
            corectionDST();
            break;
        }
    }
    timeClient->end();
	return isTimeSet();
}

int TimeModule::getCurTimeMin()
{
    return convertToMin(getHours(), getMinutes());
}

void TimeModule::corectionDST()
{
    long long epochSeconds = timeClient->getEpochTime();
    long long unixTime = epochSeconds + 10800;
    int dayOfYear = (unixTime / 86400) % 365;
    int year = 1970;
    while (dayOfYear >= 365) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            if (dayOfYear == 365) {
                break;
            }
            dayOfYear--;
        }
        else {
            dayOfYear -= 365;
        }
        year++;
    }

    int month, day, hour, minute;
    const int FEB_IND = 1;
    int FEB_DAYS = 28 + ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    int daysInMonth[] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    daysInMonth[FEB_IND] = FEB_DAYS;
    for (month = 0; month < 12; month++) {
        if (dayOfYear < daysInMonth[month]) {
            break;
        }
        dayOfYear -= daysInMonth[month];
    }
    day = dayOfYear + 1;
    hour = (unixTime / 3600) % 24;
    minute = (unixTime / 60) % 60;

    if ((month > 3 && month < 10) 
            || (month == 3 && day >= 25 && hour >= 3) 
            || (month == 10 && day < 25 && hour < 3)) 
    {
        offset = 3;
    } else {
        offset = 2;
    }
}



