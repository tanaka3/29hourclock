#include "Clock.h"
#include <Metro.h>
Clock clock29;
Metro timer = Metro(100);

void setup() {
  Serial.begin(115200);
  
  //RTCのチェック（ここでエラーになったら何も手が打てない）
  if(!clock29.begin()){
    Serial.println("error");
  }
}

void loop() {
  if (timer.check() == 0) {
    return;
  }
  clock29.clock();
}
