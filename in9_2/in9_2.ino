const int analogInPinL = A2;  
const int analogInPinR = A0;  
const int analogOutPinL = 9;
const int analogOutPinR = 11; 


const float outShift = 15; //520
const float outRatio = 0.4 ;
const float outFade = 0.07 ;


float val_intL, val_intR; 
float valo_intL, valo_intR; 



void setup() {
  pinMode(analogInPinL, INPUT);
  pinMode(analogInPinR, INPUT);

}

void loop() {

  val_intL = analogRead(analogInPinL);
  val_intR = analogRead(analogInPinR);   
  //val_intR = val_intL;



  //set zero offset
  val_intL =   val_intL - 512;
  val_intR =   val_intR - 512;
  if (val_intL < 0){ 
    val_intL = 0; 
  }
  if (val_intR < 0){ 
    val_intR = 0; 
  }


  //fade
  if(val_intL < valo_intL){ 
    val_intL = valo_intL - outFade;    
  }
  valo_intL = val_intL;
  if(val_intR < valo_intR){ 
    val_intR = valo_intR - outFade;    
  }
  valo_intR = val_intR;



  analogWrite(analogOutPinL, val_intL * outRatio + outShift);
  analogWrite(analogOutPinR, val_intR * outRatio + outShift);

}






