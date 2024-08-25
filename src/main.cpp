#include "main.hpp"


#include "controller_server.hpp"



Device device;






void setup()
{
    device.connectSTA();
    device.updateReleState();

    do{
        delay(ONE_SEC);
        startServer();
        device.connectSTA();
        device.updateReleState();
        if(device.timerOn())break;
    } while(1);
}


void loop() 
{
    static int tc;
    delay(1000);
    if(device.isTimeSet()){
        device.updateReleState();
    } else if(tc>TRY_CONNECT_WIFI_SEC){
        device.connectSTA();
        tc = 0;
    } else {
        ++tc;
    }
}
