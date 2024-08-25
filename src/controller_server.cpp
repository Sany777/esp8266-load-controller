#include "controller_server.hpp"

#include <ESP8266WebServer.h>
#include <StreamString.h>
#include <FS.h>
#include "ArduinoJson.h"
#include "controoler_tools.h"
#include "page.h"

#include "controoler_tools.h"
#include "ActionsManager.hpp"
#include "stdlib.h"

static void handleInfo();
static void handleGetSettings();
static void handleSetAction();
static void handleStopServer();
static void handleGetHtml();
static void handleGetScript();
static void handleGetStyle();
static void sendResponse(String mes, int code = 200);
static void setRoutes();
static void handleNoFind();
static void handleSetNetwork();
static void handleStatus();

bool server_run;
ESP8266WebServer server(80);

static void setRoutes()
{
	server.on("/close", HTTP_POST, handleStopServer);
	server.on("/", HTTP_GET, handleGetHtml);
	server.on("/info", HTTP_POST, handleInfo);
	server.on("/index.html", HTTP_GET, handleGetHtml);
	server.on("/script.js", HTTP_GET, handleGetScript);
	server.on("/style.css", HTTP_GET, handleGetStyle);
	server.on("/Network", HTTP_POST, handleSetNetwork);
	server.on("/data", HTTP_POST, handleGetSettings);
	server.on("/Actions", HTTP_POST, handleSetAction);
	server.on("/Status", HTTP_POST, handleStatus);
	server.onNotFound(handleNoFind);
}



void startServer()
{
	const int timeout_sec = 200;
	device.startAP();
	delay(ONE_SEC);
	setRoutes();
	server.begin();
	server_run = true;
	delay(ONE_SEC);
	for(unsigned sec_count = 0, i = 0; server_run; ++i){
		delay(1);
		server.handleClient();
		yield();
		if(i > ONE_SEC){
			if(device.getStationNum()>0){
				sec_count = 0;
			}else if(sec_count<timeout_sec){
				++sec_count;
			}else{
				server_run = false;
			}
			i = 0;
		}
	}
	device.saveChanges();
    server.client().stop();
	server.close();
	device.disconnect();
}

static void handleSetNetwork()
{
	bool ssid_seted = false, pwd_seted = false;			
	String data = server.arg("plain");
	JsonDocument doc;
	DeserializationError error = deserializeJson(doc, data);
	if (!error){
		String pwd = doc[NAME_PWD];
		String ssid = doc[NAME_SSID];
		pwd_seted = device.setPWD(pwd.c_str());
		ssid_seted = device.setSSID(ssid.c_str());
	}
	if(pwd_seted || ssid_seted){
		sendResponse("Succes");
	} else {
		sendResponse("Failue", 400);
	}
	yield();
	doc.clear();
}



static void handleGetSettings()
{
	String jsonString = "";
	uint32_t flags = 0;

	flags = BoolTool::setFlag(flags, WIFI_OK, device.isSTAConnection());
	flags = BoolTool::setFlag(flags, TIME_OK, device.isTimeSet());
	flags = BoolTool::setFlag(flags, RELE_STATE, device.getReleState());
	flags = BoolTool::setFlag(flags, TIMER_ACTIVATE, device.timerOn());

	JsonDocument doc;
	doc[NAME_SSID] = device.getSSID();
	doc[NAME_PWD] = device.getPWD();
	doc[NAME_STATUS] = flags;

	String schema = "";
	String actions = "";

	const size_t actions_sum = device.getActionSum();
	if(actions_sum){
		const Action* act_list = device.getActionsList();
		if(act_list){
			for(int day=0; day<WEEK_DAYS_NUM; ++day){
				const uint8_t act_num = device.getActInDay(day);
				schema += CharTool::numToCSTR(act_num, 2);
			}
			for(size_t i=0; i<actions_sum; ++i){
				actions += CharTool::numToCSTR(Action::getHour(act_list[i]), 2);
				actions += CharTool::numToCSTR(Action::getMin(act_list[i]), 2);
				actions += Action::getReleState(act_list[i]) ? '1':'0';
			}
		}
	}

	doc[NAME_ACT_SCHAMA] = schema;
	doc[NAME_ACT_VALUES] = actions;
	serializeJson(doc, jsonString);
	server.send(200, "application/json", jsonString);
	yield();
	doc.clear();
}

static void handleInfo()
{
	const size_t SIZE_BUF = 500;
	char buf[SIZE_BUF];
	snprintf(buf, SIZE_BUF, 
				" System Information:\n"
				"Time: %d:%02d\n"
				"SDK version: %s\n"
				"Chip ID: %d\n"
				"Flash chip size: %d bytes\n"
				"Free sketch space: %d bytes\n",
				device.getHours(),
				device.getMinutes(),
				ESP.getSdkVersion(),
				ESP.getChipId(),
				ESP.getFlashChipSize(),
				ESP.getFreeSketchSpace()
			);
	sendResponse(buf);
}

static void handleSetAction()
{
	Action* act_list = NULL;
	uint8_t schema[WEEK_DAYS_NUM] = {0};
	bool err = false;
	String data = server.arg("plain");
	JsonDocument doc;
	DeserializationError error = deserializeJson(doc, data);
	if (!error){
		String schema_str = doc[NAME_ACT_SCHAMA];
		String values_str = doc[NAME_ACT_VALUES];

		if(schema_str.length() == 14 && values_str.length()%5 == 0){
			const char *values = values_str.c_str();
			const char *schema_p = schema_str.c_str();
			for(unsigned d=0; d<WEEK_DAYS_NUM; ++d, schema_p += 2){
				schema[d] = CharTool::getNumber(schema_p, 2);
			}
			size_t act_sum = ActionsManager::getActionSum(schema);
			if(act_sum){
				act_list = new Action[act_sum];
				if(act_list){
					uint8_t h, m;
					bool rele_state;
					const char *end = values+values_str.length();
					for(unsigned li=0; values<end && !err && li<act_sum; values += 5, ++li){
						h = CharTool::getNumber(values, 2);
						m = CharTool::getNumber(values+2, 2);
						rele_state = *(values+4) == '1';
						if(h>23 || m>59){
							err = true;
						} else {
							act_list[li].time_min = TimeModule::convertToMin(h, m);
							act_list[li].rele_state = rele_state;
						}
					}
				}
			}
		}
	}
	if(err){
		sendResponse("Bad data!", 400);
		if(act_list){
			delete[]act_list;
		}
	} else {
		device.setActionData(act_list, schema);
		device.updateReleState();
		sendResponse("Succes");
	}
	yield();
	doc.clear();
}

static void handleStopServer()
{
	yield();
	if(device.isTimeSet()){
		char buf[50];
		sprintf(buf, "Goodbay!\n\r%02u:%02u\n\rrele: %s", 
					device.getHours(),
					device.getMinutes(),
					device.getReleState()
						? "ON"
						: "OFF");
		sendResponse(buf);
	} else {
		sendResponse("Goodbay");
	}
	server_run = false;
	delay(1000);
}

static void handleGetHtml()
{
	server.send(200, "text/html", HTML_STR);
	yield();
}

static void handleGetScript()
{
	server.send(200, "text/javascript", SCRIPT_STR);
	yield();
}

static void handleGetStyle()
{
	server.send(200, "text/css", STYLE_STR);
	yield();
}

static void handleStatus()
{
	uint32_t flags = server.arg("plain").toInt();
	device.setRele(BoolTool::getFlag(flags, RELE_STATE));
	device.setActivate(BoolTool::getFlag(flags, TIMER_ACTIVATE));
	sendResponse("Ok");
}

static void handleNoFind()
{
	sendResponse("No find", 404);
}




static void sendResponse(String mes, int code)
{
	server.send(code, "text/plain", mes);
}