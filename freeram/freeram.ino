void setup(){
    Serial.begin(115200);
}

void loop(){
    Serial.print("available memory=");
    Serial.println(freeRam(), DEC);\
    delay(1000);
}    


int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
