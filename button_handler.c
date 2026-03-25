#include <stdbool.h>
#include <string.h>
#include "ui.h" // for lv_event_t type
#include "serial_wrapper.h"
#include "Arduino.h"

int alarmTime = 0;
int timerTime = 0;

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

bool bAlarmAmPm = false;

int lastButtonCheck = 0;
int lastTimerCheck = 0;
int lastBlinkerCheck = 0;

bool bTimerBlanked = false;
bool bTimerRunning = false;
bool bTimerBlinking = false;

bool bAlarmEnabled = false;

void setAlarmTime() {
	if (alarmTime >= (24 * 60 * 60)) {
		alarmTime -= (24 * 60 * 60);
	}
  int hh = alarmTime/60 % 12;
  int mm = alarmTime % 60;

  char am_pm[] = "AM";
  if (alarmTime >= (60 * 12)) {
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

	sendSerial("alarmTime is Now ");
	char cAlarmTime[20];
	sprintf(cAlarmTime, "%d",alarmTime);
	sendSerial(cAlarmTime);
}

void setTimerTime() {
	if (timerTime >= (99 * 60 * 60 * 60)) {
		timerTime == (99 * 60 * 60 * 60);
	}
  int hh = timerTime/3600 % 60;
  int mm = timerTime/60 % 60;
  int ss = timerTime % 60;

	char curTime[10];
  sprintf(curTime, "%02d:%02d:%02d", hh, mm, ss);
  curTime[9] = 0; // terminate it

	lv_label_set_text(ui_Label14,curTime);

	sendSerial("timerTime is Now ");
	char cTimerTime[20];
	sprintf(cTimerTime, "%d",timerTime);
	sendSerial(cTimerTime);
}

void blankTimerTime() {
	sendSerial("Blank Timer Time");
	lv_label_set_text(ui_Label14,"");
}

void incrementTimerHrs() {
	timerTime += (60 * 60);
  setTimerTime();
}

void incrementTimerMinutes() {
	timerTime += 60;
  setTimerTime();
}

void incrementTimerSeconds() {
	timerTime += 1;
  setTimerTime();
}

void resetTimer() {
	timerTime = 0;
  setTimerTime();
}

void timerStartStop() {
	if ((timerTime > 0) && (bTimerRunning == true)) { // pause function
		bTimerRunning = false;
		sendSerial("Timer Paused");
		return;
	}
	if ((timerTime > 0) && (bTimerRunning == false)) { // start function
		bTimerRunning = true;
		sendSerial("Timer Started");
		return;
	}
	if (timerTime <= 0) { // clear function
		sendSerial("CLEAR timer blinking");
		bTimerBlinking = false;
		bTimerRunning = false; // end countdown
		setTimerTime();
		return;
	}
}

void timerTimeOut() {
		sendSerial("Timer TIMEOUT!!!");
		bTimerBlinking = true;
}

//------ Alarm ------- //

void incrementAlarmHrs() {
	alarmTime += 60;
  setAlarmTime();
}

void incrementAlarmMinutes() {
	alarmTime += 1;
  setAlarmTime();
}

void toggleAlarmAmPm() {
	alarmTime += (12 * 60 * 60);
	setAlarmTime();
}

// we need to let cyc_alarm_clock.ino access the variables for the alarm enable/disable and blinking
bool getAlarmEnabled() {
	return bAlarmEnabled;
}

int getAlarmTime() {
	return alarmTime;
}

void alarmEnable() {
	// toggle alarm enabled - to be read by cyc_alarm_clock.ino
	if (bAlarmEnabled == true) {
		bAlarmEnabled = false; 
	} else {
		bAlarmEnabled = true;
	}
}

void pollButtons() {
	if ((millis() - lastButtonCheck) > 200) { // poll every 200 ms
    	lastButtonCheck = millis();

  //   	// which button is down or released
  //   	if ((buttonLastState.alarmHrs == false) && (buttonState.alarmHrs == true)) { // first time button is pressed
  //   	 	buttonLastState.alarmHrs = true;
  //   		alarmTime = alarmTime + (60 * 60); // 3600 seconds
  //       sendSerial("Increment alarmTime to ");
	// 			char cAlarmTime[20];
	// 			sprintf(cAlarmTime, "%d",alarmTime);
  //       sendSerial(cAlarmTime);
	// 			setAlarmTime(alarmTime);
  //   		return;
  //   	}  // only increment when going false to true
  //   	if (buttonState.alarmHrs == false) { // not pressed
  //   		buttonLastState.alarmHrs = false;
  //   	}
	}

	// run timer
	if ((millis() - lastTimerCheck) > 1000) { // poll every 1000 ms
    	lastTimerCheck = millis();
			if (bTimerRunning == false) {
				return;
			}
			if (timerTime == 1) {
				// time out
				timerTimeOut();
			}
			if (timerTime <= 0) {
				return;
			}
			timerTime -= 1;
			setTimerTime();
	}

	// run blinker
	if ((millis() - lastBlinkerCheck) > 500) { // poll every 500 ms
    	lastBlinkerCheck = millis();
			if (bTimerBlinking == false) {
				return;
			}
			if (bTimerBlanked == false) {
				bTimerBlanked = true;
				blankTimerTime();
			} else {
				bTimerBlanked = false;
				setTimerTime();
			}
	}
	
}

void ui_event_global_button_handler(lv_event_t * e) { 
  // any button press besides screen changes is handled here
  lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * target = lv_event_get_target(e);

	// if(code == LV_EVENT_PRESSED) {
	// 	if(target == ui_btnAlarmHrs) {
	// 		buttonState.alarmHrs = true;
  //     sendSerial("Alarm Hrs Button is Pressed");
	// 	}
	// }
	// // ... repeat for other buttons ...

	// if(code == LV_EVENT_RELEASED) {
	// 	if(target == ui_btnAlarmHrs) {
	// 		buttonState.alarmHrs = false;
  //     sendSerial("Alarm Hrs Button is Released");
	// 	}
	// // ... repeat for other buttons ...
	// }

	if(code == LV_EVENT_CLICKED) {
		// Alarm
		if(target == ui_btnAlarmHrs) {
			//buttonState.alarmHrs = true;
      sendSerial("Alarm Hrs Button Clicked");
			incrementAlarmHrs();
		}
		if(target == ui_btnAlarmMin) {
			//buttonState.alarmHrs = true;
      sendSerial("Alarm Minutes Button Clicked");
			incrementAlarmMinutes();
		}
		if(target == ui_btnAlarmAmPm) {
			//buttonState.alarmHrs = true;
      sendSerial("Alarm AM/PM Button Clicked");
			toggleAlarmAmPm();
		}

		if (target == ui_chkAlarm) {
			// TODO
      sendSerial("Alarm ENABLE Clicked");
			alarmEnable();
		}

		// Timer
		if(target == ui_btnTimerHrs) {
      sendSerial("Timer Hrs Button Clicked");
			incrementTimerHrs();
		}

		if(target == ui_btnTimerMin) {
      sendSerial("Timer Minutes Button Clicked");
			incrementTimerMinutes();
		}

		if(target == ui_btnTimerSec) {
      sendSerial("Timer Seconds Button Clicked");
			incrementTimerSeconds();
		}

		if(target == ui_btnTimerReset) {
      sendSerial("Timer Reset Button Clicked");
			resetTimer();
		}

		if(target == ui_btnTimerStartStop) {
      sendSerial("Timer Start/Stop Button Clicked");
			timerStartStop();
		}



	// ... repeat for other buttons ...
	}
}
