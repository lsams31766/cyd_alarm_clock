
#ifdef __cplusplus
extern "C" {
#endif

void ui_event_global_button_handler(lv_event_t * e);
void pollButtons();

bool getAlarmEnabled();
int getAlarmTime();

#ifdef __cplusplus
} /*extern "C"*/
#endif
