#include "Clock.h"
#include <map>

/**
 * コンストラクタ
 */
Clock::Clock():yearMatrix(YEAR_CLK, YEAR_DIO),
               dateMatrix(DATE_CLK, DATE_DIO), 
               timeMatrix(TIME_CLK, TIME_DIO){
                
  this->setBrightness(BRIGHT_HIGH);
}

/**
 * 初期化を行う
 */
bool Clock::begin(){
  //RTCの初期化
  return rtc.begin();
}

/**
 * 時計を表示する
 */
void Clock::showTime(DateTime clock){

  //年を設定    
  yearMatrix.showNumberDec(clock.year(), 0x00, true);

  //月日を設定
  int date = clock.month() * 100;
  date += clock.day();
  dateMatrix.showNumberDec(date, 0x40, true);  

  //時分を設定
  int time = clock.hour() * 100;
  time += clock.minute();    
  timeMatrix.showNumberDec(time, 0x40, true);  
    
}

/**
 * モードの表示を行う
 */
void Clock::showSetting(mode_config mode, float volume){
  uint8_t SEG_DONE[] = {SEG_G, SEG_G, SEG_G, SEG_G};
  
  yearMatrix.showString("MODE");

  String mode_str[] ={"SLOW", "FAST", "MIX"};

  //モードの文字を表示
  dateMatrix.showString(mode_str[mode].c_str());

  int time = (int)volume * 100;
  time +=  (volume - (int)volume) * 60;

  //時間を表示
  if(time == 0){
    //0の場合だけ、Segmentsで表示
    uint8_t data[] = {  0x00, 0xbf, 0x3f, 0x3f   };
    timeMatrix.setSegments(data);
  }
  else{
    timeMatrix.showNumber(time/100, 2);
    //timeMtrix->showNumberDec(time, 0x40, true);
  }
}


/**
 * 明るさの制御
 */
void Clock::setBrightness(uint8_t brightness, bool on){

  yearMatrix.setBrightness(brightness, on);
  dateMatrix.setBrightness(brightness, on);
  timeMatrix.setBrightness(brightness, on);

}

/**
 * RTCに時間を設定する
 */
void Clock::setRTC(DateTime time){

  this->rtc.adjust(time);
}

/**
 * Volumeを取得する
 */
void Clock::setVolume(uint16_t vol){

  //ローパスフィルターをかけておく
  this->vol_val = vol * 0.9f +vol * 0.1f;

  int scale = (int)(((ANALOG_MAX-this->offset) - vol_val) / ((ANALOG_MAX-this->offset) / this->separated));   

  //キャッシュを超えた場合は、先頭を削除する
  if(this->cache.size() >= VOL_CACHE_MAX){
    this->cache.erase(this->cache.begin());
  }

  this->cache.push_back(scale);

}


/**
 * Volumeの値
 */
int Clock::getVolume() {
  
  std::map<int, int> map_table;

  //出現回数を集計する
  for(auto itr =  this->cache.begin(); itr !=  this->cache.end(); ++itr) {

    auto key = map_table.find(*itr); 
    if(key == map_table.end()){
      map_table[*itr] = 0;      
    }
    map_table[*itr] ++;
  }

  //出現回数の多いものを抽出する
  int key = 0;
  int count = 0;
  for(auto itr = map_table.begin(); itr != map_table.end(); ++itr) {
      if(count < itr->second){
        key = itr->first;
        count = itr->second;
      }
  }
  
  return key;
}

/**
 * モード切り替え
 */
void Clock::changeMode(){

  int next_mode = ((int)mode + 1) % 4;
  mode =(mode_config)next_mode;

  switch(mode){
  case MODE_SLOW:    //遅くするモード
    break;
  case MODE_FAST:    //早くするモード
    break;
  case MODE_HOUR_FAST: //各時間を早くするモード
    break;
  }
}

/**
 * 時計の実行
 */
void Clock::clock(){

  //ボリュームの値を取得する
  this->setVolume(analogRead(35));

  if(this->is_setting){

    return;
  }
  DateTime now =  this->rtc.now();      
  //DateTime now = DateTime(2021,3,14, 22,30,0);
  //while(1){
  //  now = now + TimeSpan(0, 0, 1, 0);
    this->run(now); 
  //}
}

/**
 * 時計の表示を行う
 */
void Clock::run(DateTime now){

  //現在時間を取得
  //6〜22時は基本普通の時計とする
  if(now.hour() >= end_hour && now.hour() <start_hour){
    run_clockmode = false;
    this->showTime(now);
    return;
  }

  //動作開始時間に動作状態を設定する 
  if(now.hour() == start_hour && now.minute() == 0 && !run_clockmode){
    Serial.println("target_check========================");
    
    run_clockmode = true;

    //動作の開始時間を取得する
    startTime = DateTime(now.year(),now.month(), now.day(), start_hour, 0, 0);
     
    //動作の終了時間を設定する
    TimeSpan endspan = TimeSpan(0, (24 - start_hour + end_hour), 0, 0);

    //終了時間を設定する
    endTime = startTime + endspan;
    
    //前借り、前貸し時間を取得する
    //time_scale = this->getVolume();
    time_scale = 30;
    //各時間の初期化
    switch(mode){
      case MODE_MIX:
        time_scale *= ((float)VOLUME_HOUR / this->separated);
        mixClockInit();
        break;
      case MODE_SLOW:      //遅くするモード
        slowClockInit();
        break;
      
      case MODE_FAST:      //早くするモード
        fastClockInit();
        break;
        
      case MODE_HOUR_FAST: //各時間を早くするモード
        hourFastClockInit();      
        break;
    }
  }

  DateTime show = now;
  switch(mode){
    case MODE_MIX:
      show = mixClockRun(now);
      break;

    case MODE_SLOW:      //遅くするモード
      show = slowClockRun(now);
      break;
      
    case MODE_FAST:      //早くするモード
      show = fastClockRun(now);
      break;      
    
    case MODE_HOUR_FAST: //各時間を早くするモード
      show = hourFastClockRun(now);
      break;
  }

  this->showTime(show);
}

/**
 * 
 */
void Clock::mixClockInit(){
  
  //前借り
  if(time_scale > 0){
    slowClockInit();
  } 
  
  //前貸し
  if(time_scale < 0){
    fastClockInit();
  }

}

/**
 * 
 */
DateTime Clock::mixClockRun(DateTime now){
  
  if(time_scale > 0){
    return slowClockRun(now);
  }
  
  //前貸し動作
  if(time_scale < 0){
    return fastClockRun(now);
  }

  return now;
}

/**
 * 前貸
 * 仕組みとしては、5時を時間で経過させるかで前貸を実現している
 * 前借りが23時で実施するのと逆の動作で実現を行っている
 */
void Clock::fastClockInit(){
  
  //調整開始時間を取得する（5時）
  this->adjustStartTime = this->startTime + TimeSpan(0, 6, 0, 0);

  //この時間までに5時に移行する
  this->adjustEndTime = this->endTime - TimeSpan((abs(this->time_scale) + 1) * HOUR_SECOND); 

  //遅延時間（秒）を取得する
  this->adjustSecond = (this->adjustEndTime - this->startTime).totalseconds();
}

/**
 * 
 */
DateTime Clock::fastClockRun(DateTime now){
  //遅延開始からの経過時間を求める
  TimeSpan elapsedTime = now - startTime;

  //前貸し時の動作
  if(elapsedTime.totalseconds() < adjustSecond.totalseconds()){
      
    int32_t fastSecond = ((float)(6 * HOUR_SECOND) / adjustSecond.totalseconds()) * elapsedTime.totalseconds();
      
    return startTime +  TimeSpan(fastSecond);        
  }

  //調整時の動作
  elapsedTime = now - adjustEndTime;

  TimeSpan diffAdjustSecond =  endTime-adjustEndTime;
  int32_t elapsedSecond =  ((float)elapsedTime.totalseconds() / diffAdjustSecond.totalseconds()) * HOUR_SECOND;
    
  return adjustStartTime + TimeSpan(elapsedSecond);
}

/**
 * 前借
 */
void Clock::slowClockInit(){
      
  //調整開始時間を取得する(24時)
  this->adjustStartTime = this->startTime + TimeSpan(0, 1, 0, 0);
            
  //遅延時間（23時〜24時までの１時間＋ボリュームでの設定時間）の秒数を取得する
  this->adjustSecond = TimeSpan((abs(this->time_scale) + 1) * HOUR_SECOND);

  //0時時点の時間を取得する
  this->adjustEndTime =  this->startTime + this->adjustSecond;  
}


/**
 * 遅延時計
 */
DateTime Clock::slowClockRun(DateTime now){
  
  //遅延開始からの経過時間を求める
  TimeSpan elapsedTime = now - startTime;
        
  //前借り時の動作
  if( elapsedTime.totalseconds() < adjustSecond.totalseconds()){

    //現在の経過時間 / 23~24時までの遅延総時間 から比率で時間を算出
    int32_t elapsedSecond =  ((float)elapsedTime.totalseconds() / adjustSecond.totalseconds()) * HOUR_SECOND;

    //開始時間＋経過時間を表示
    this->showTime(startTime + TimeSpan(elapsedSecond));

  }

  //調整時の動作
  //遅延後からの残りの時間を求める
  elapsedTime = now - adjustEndTime;

  ///遅延した時間から6時までの秒数を求める
  TimeSpan diffAdjustSecond = endTime - adjustEndTime;
  //残りの秒数で6時間分を進める
  int32_t fastSecond = ((float)((24 - start_hour + end_hour-1) * HOUR_SECOND) / diffAdjustSecond.totalseconds()) * elapsedTime.totalseconds();
      
  return adjustStartTime +  TimeSpan(fastSecond);
}

/**
 * 時間指定の前貸初期化
 */
void Clock::hourFastClockInit(){

  Serial.println("hourSlowClockInit");
  
  //調整開始時間を取得する(5時)
  this->adjustStartTime = this->startTime + TimeSpan(0, 6, 0, 0);
            
  //この時間までに5時に移行する
  //各時間＋遅延時間（0 ~ 60)が5時までにかかる秒数を求める
  this->adjustEndTime = this->startTime + TimeSpan((60 - this->time_scale) * 60 * (24 - start_hour + end_hour -1)); 

  //遅延時間（秒）を取得する
  this->adjustSecond = (this->adjustEndTime - this->startTime).totalseconds();  
}

/**
 * 時間指定の前貸 
 */
DateTime Clock::hourFastClockRun(DateTime now){
  
  //遅延開始からの経過時間を求める
  TimeSpan elapsedTime = now - startTime;

  //遅延動作
  if(elapsedTime.totalseconds() < adjustSecond.totalseconds()){
      
    int32_t fastSecond = ((float)((24 - start_hour + end_hour-1) * HOUR_SECOND) / adjustSecond.totalseconds()) * elapsedTime.totalseconds();
      
    return startTime +  TimeSpan(fastSecond);        
  }

  //調整時の動作
  elapsedTime = now - adjustEndTime;

  TimeSpan diffAdjustSecond =  endTime-adjustEndTime;
  int32_t elapsedSecond =  ((float)elapsedTime.totalseconds() / diffAdjustSecond.totalseconds()) * HOUR_SECOND;
    
  return adjustStartTime + TimeSpan(elapsedSecond);
    
}


/**
 * 時間を出力する（Debug）
 */
void Clock::printDateTime(DateTime time){
  Serial.printf("NOW TIME: %04d/%02d/%02d %02d:%02d:%02d\n",
                    time.year(), time.month(), time.day(),
                    time.hour(), time.minute(), time.second());
}

/**
 * RTCの設定
 */
void Clock::setRTC(){
  
  WiFi.begin(WIFI_SSID, WIFI_PW);
  Serial.print("WiFi connecting");
  
  while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(100);
  }
  
  
  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
  
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  
  DateTime time = DateTime(timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday,
                              timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec); 
  printDateTime(time);                                        
  rtc.adjust(time);  
}
