/* Using LVGL with Arduino requires some extra steps...
 *  
 * Be sure to read the docs here: https://docs.lvgl.io/master/integration/framework/arduino.html
 * but note you should use the lv_conf.h from the repo as it is pre-edited to work.
 * 
 * You can always edit your own lv_conf.h later and exclude the example options once the build environment is working.
 * 
 * Note you MUST move the 'examples' and 'demos' folders into the 'src' folder inside the lvgl library folder 
 * otherwise this will not compile, please see README.md in the repo.
 * 
 */
#include <lvgl.h>

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// for time
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>


// ---- squareline Studio exported UI headers ----
#include "ui.h"


// A library for interfacing with the touch screen
//
// Can be installed from the library manager (Search for "XPT2046")
//https://github.com/PaulStoffregen/XPT2046_Touchscreen
// ----------------------------
// Touch Screen pins
// ----------------------------

// The CYD touch uses some non default
// SPI pins

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
SPIClass touchscreenSpi = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700, touchScreenMinimumY = 240,touchScreenMaximumY = 3800;

/*Set to your screen resolution*/
//#define TFT_HOR_RES   320
//#define TFT_VER_RES   240
#define TFT_HOR_RES   240
#define TFT_VER_RES   320

// for getting time from the web
#include <WiFi.h>
#include <time.h>
const char* ssid     = "Verizon_4WZFSW";
const char* password = "broke-iris4-zoo";
// NTP
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -4 * 3600;      // UTC-4
const int  daylightOffset_sec = 0;


/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

#if LV_USE_LOG != 0
void my_print( lv_log_level_t level, const char * buf )
{
    LV_UNUSED(level);
    Serial.println(buf);
    Serial.flush();
}
#endif

// to get time from the internet
String getTimeString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "NO NTP SYNC";
  }
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  std::string curTime = buf;
  // convert this to c string, only year month day, am or pm
  std::string curTimeFixed;
  size_t spacePos = curTime.find(' ');
  if (spacePos == std::string::npos) {
     curTimeFixed = "";
     return "BAD TIME FMT";
  }  
  curTimeFixed =  curTime.substr(spacePos + 1);
  //printf("\nFixed time is ");
  //printf(curTimeFixed.c_str());
  //printf("\n");
    
  // make it am or pm 
  int hh = std::stoi(curTimeFixed.substr(0, 2));
  std::string Meridien;
  if (hh < 12) {
      Meridien = "AM";
  } else {
      Meridien = "PM";
  }
  hh %= 12;
  if (hh == 0) {
        hh = 12;
    }

  // Use stringstream to format the output nicely (e.g., "05" instead of "5")
  std::ostringstream os;
  os << std::setfill('0') << std::setw(2) << hh << curTimeFixed.substr(2, 6) << " " << Meridien;
  std::string const cppString = os.str(); 
  char const* cString = cppString.c_str(); 
  // printf("\nAM PM Fixed time is ");
  // printf(cString);
  // printf("\n");
  //return String(buf);
  return cString;
}

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
{
    /*Call it to tell LVGL you are ready*/
    lv_disp_flush_ready(disp);
}
/*Read the touchpad*/
void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data )
{
  if(touchscreen.touched())
  {
    TS_Point p = touchscreen.getPoint();
    //Some very basic auto calibration so it doesn't go out of range
    if(p.x < touchScreenMinimumX) touchScreenMinimumX = p.x;
    if(p.x > touchScreenMaximumX) touchScreenMaximumX = p.x;
    if(p.y < touchScreenMinimumY) touchScreenMinimumY = p.y;
    if(p.y > touchScreenMaximumY) touchScreenMaximumY = p.y;
    //Map this to the pixel position
    data->point.x = map(p.x,touchScreenMinimumX,touchScreenMaximumX,1,TFT_HOR_RES); /* Touchscreen X calibration */
    data->point.y = map(p.y,touchScreenMinimumY,touchScreenMaximumY,1,TFT_VER_RES); /* Touchscreen Y calibration */
    data->state = LV_INDEV_STATE_PRESSED;
    /*
    Serial.print("Touch x ");
    Serial.print(data->point.x);
    Serial.print(" y ");
    Serial.println(data->point.y);
    */
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

lv_indev_t * indev; //Touchscreen input device
uint8_t* draw_buf;  //draw_buf is allocated on heap otherwise the static area is too big on ESP32 at compile
uint32_t lastTick = 0;  //Used to track the tick timer

void setup()
{
  //Some basic info on the Serial console
  String LVGL_Arduino = "LVGL demo ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);
    
  //Initialise the touchscreen
  touchscreenSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS); /* Start second SPI bus for touchscreen */
  touchscreen.begin(touchscreenSpi); /* Touchscreen init */
  touchscreen.setRotation(2); /* Inverted landscape orientation to match screen */

  //Initialise LVGL
  lv_init();
  draw_buf = new uint8_t[DRAW_BUF_SIZE];
  lv_display_t * disp;
  disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);
  lv_disp_set_rotation(disp, LV_DISP_ROTATION_90);

  //Initialize the XPT2046 input device driver
  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);  
  lv_indev_set_read_cb(indev, my_touchpad_read);

  // ---- Initialize squareline UI ----
  ui_init(); // Create all squareline screens

  // for time
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());
    // NTP time init
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  //Done
  Serial.println( "Setup done" );
}

int lastTimeCheck = 0;
void loop()
{   
    lv_tick_inc(millis() - lastTick); //Update the tick timer. Tick is new for LVGL 9
    lastTick = millis();
    lv_timer_handler();               //Update the UI
    delay(5);

    // update time every 500 ms
    if ((millis() - lastTimeCheck) > 500) {
      lastTimeCheck = millis();
      String curTime = getTimeString();
      Serial.print("Time is ");
      Serial.println(curTime.c_str());
      lv_label_set_text(ui_lblCurrentTime,curTime.c_str());
    }
    
}