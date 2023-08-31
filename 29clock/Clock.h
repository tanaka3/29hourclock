#ifndef Clock_H_
#define Clock_H_

#include <TM1637TinyDisplay.h>
#include <RTClib.h>
#include <vector>
#include <WiFi.h>


#define YEAR_CLK  33
#define YEAR_DIO  32
#define DATE_CLK  26
#define DATE_DIO  25
#define TIME_CLK  14
#define TIME_DIO  27

#define HOUR_SECOND 3600          //1時間の秒数

#define ANALOG_MAX 4095           //Vol最大値
#define VOL_CACHE_MAX 10          //キャッシュの量

#define VOLUME_HOUR 5            //時間調整可能な時間

#define WIFI_SSID ""
#define WIFI_PW ""

typedef enum{
  MODE_MIX = -1,     //混合モード
  MODE_SLOW = 0,    //遅くするモード
  MODE_FAST = 1,    //早くするモード
  MODE_HOUR_FAST = 2, //各時間を早くするモード
} mode_config;


/**
 * 29時間時計
 */
class Clock{
  public:
  
    Clock();

    bool begin();

    void setBrightness(uint8_t brightness, bool on = true);

    void clock();
    void setRTC(DateTime time);

  private:
    TM1637TinyDisplay yearMatrix;    //年表示用
    TM1637TinyDisplay dateMatrix;    //日付表示用
    TM1637TinyDisplay timeMatrix;    //時間表示用

    RTC_DS3231 rtc;                   //RTC操作用

    float vol_val = 0;                //ボリューム管理用 
    uint16_t separated = 20;          //分解能
    uint16_t offset = ANALOG_MAX / 2; //値のオフセット値
    std::vector<int> cache;           //ボリューム値の保持用

    mode_config mode = MODE_HOUR_FAST;      //現在のモード

    boolean is_setting = false;       //設定モードかどうか
    boolean run_clockmode = false;     //時計モード動作ちゅうかどうか
    
    DateTime startTime;               //今日の動作開始時間
    DateTime endTime;                 //今日の動作終了時間
    DateTime adjustStartTime;         //調整開始時間
    DateTime adjustEndTime;           //調整終了時間
    TimeSpan adjustSecond;            //調整秒数
    float time_scale = 0;             //遅延時間
    uint8_t start_hour = 23;          //遅延開始時間
    uint8_t end_hour = 6;             //遅延終了時間

    //表示系の処理
    void showTime(DateTime time);
    void showSetting(mode_config mode, float volume);

    //Mode切り替え
    void changeMode();

    //ボリューム関連
    void setVolume(uint16_t vol);
    int getVolume();

    ///時計の処理
    void run(DateTime now);
    void mixClockInit();
    DateTime mixClockRun(DateTime now);

    
    void fastClockInit();
    DateTime fastClockRun(DateTime now);

    void slowClockInit();
    DateTime slowClockRun(DateTime now);

    void hourFastClockInit();
    DateTime hourFastClockRun(DateTime now);

    //Debug用
    void printDateTime(DateTime time);
    void setRTC();
};

#endif
