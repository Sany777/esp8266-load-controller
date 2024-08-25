#ifndef CONTROLLER_SERVER_HPP
#define CONTROLLER_SERVER_HPP

#include "main.hpp"

// http://192.168.4.1/

// JSON field name 
#define NAME_SSID "SSID"
#define NAME_PWD "PWD"
#define NAME_STATUS "Status"
#define NAME_ACT_SCHAMA "schema"
#define NAME_ACT_VALUES "values"
#define PATH_INFO "info"


enum IndxFlags{
	WIFI_OK,
	TIME_OK,
	RELE_STATE,
	TIMER_ACTIVATE,
};

void startServer();



#endif