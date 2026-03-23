#include <stdbool.h>
#include <string.h>
#include "ui.h" // for lv_event_t type
#include "serial_wrapper.h"
#include "Arduino.h"

int alarmTime = 0;

struct buttonStateStr { // current state of button press
	bool alarmHrs;
	bool alarmMin;
	bool alarmAmPm;
	bool timerHrs;
	bool timerMin;
	bool timerSec;
	bool timerReset;
	bool timerStartStop;
};
struct buttonStateStr buttonState = {0};

struct buttonLastStateStr { // last state of button press - for scrolling values
	bool alarmHrs;
	bool alarmMin;
	bool alarmAmPm;
	bool timerHrs;
	bool timerMin;
	bool timerSec;
	bool timerReset;
	bool timerStartStop;
};
struct buttonLastStateStr buttonLastState = {0};


int lastButtonCheck = 0;

void ui_event_global_button_handler(lv_event_t * e) { 
  // any button press besides screen changes is handled here
  lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * target = lv_event_get_target(e);

	if(code == LV_EVENT_PRESSED) {
		if(target == ui_btnAlarmHrs) {
			buttonState.alarmHrs = true;
      sendSerial("Alarm Hrs Button is Pressed");
		}
	}
	// ... repeat for other buttons ...

	if(code == LV_EVENT_RELEASED) {
		if(target == ui_btnAlarmHrs) {
			buttonState.alarmHrs = false;
      sendSerial("Alarm Hrs Button is Released");
		}
	// ... repeat for other buttons ...
	}

}

void setAlarmTime(int value) {
	// convert total_minutes to display string
	value = value / 60; // seoconds to minutes
  int hh = value/60 % 12;
  int mm = value % 60;

  char am_pm[] = "AM";
  if (value >= (60 * 12)) {
      strcpy(am_pm,"PM");
  }
  // THIS option shows seconds
  //sprintf(curTime, "%02d:%02d:%02d %s", hh, mm, totalSeconds, am_pm);
  //curTime[11] = 0; // terminate it
  // THIS option does not show seconds
  if (hh == 0) {
    hh = 12;
  }
	char curTime[10];
  sprintf(curTime, "%02d:%02d %s", hh, mm, am_pm);
  curTime[9] = 0; // terminate it

	lv_label_set_text(ui_lblAlarmTime,curTime);
}

void pollButtons() {
	if ((millis() - lastButtonCheck) > 200) { // poll every 200 ms
    	lastButtonCheck = millis();

    	// which button is down or released
    	if ((buttonLastState.alarmHrs == false) && (buttonState.alarmHrs == true)) { // first time button is pressed
    	 	buttonLastState.alarmHrs = true;
    		alarmTime = alarmTime + (60 * 60); // 3600 seconds
        sendSerial("Increment alarmTime to ");
				char cAlarmTime[20];
				sprintf(cAlarmTime, "%d",alarmTime);
        sendSerial(cAlarmTime);
				setAlarmTime(alarmTime);
    		return;
    	}  // only increment when going false to true
    	if (buttonState.alarmHrs == false) { // not pressed
    		buttonLastState.alarmHrs = false;
    	}
	}
}