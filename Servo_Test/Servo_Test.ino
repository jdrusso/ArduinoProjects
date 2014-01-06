const int pwmPin = 4;
const int pulseDelay = 20;

void setup(){
  
  pinMode(pwmPin, OUTPUT);
}

void loop(){
  
  for(int i=0;i<100;i++){
    
    pwmWrite(pwmPin, i);
    delay(pulseDelay);
    Serial.print(i);
    Serial.print("\n");
  }
  for(int i=100;i>0;i--){
    
    pwmWrite(pwmPin, i);
    delay(pulseDelay);
    Serial.print(i);
    Serial.print("\n");
  }
}

//duty is 0-100, length of duty cycle
void pwmWrite(int pin, int duty){

  //Servo works on a 0.75-2.25 ms high pulse, so 750 - 2250 us
  //Pulses are expected at 20 ms intervals.
  //Duty cycle of 0% is a 750us pulse every 20ms
  //Pulse length = 750 + (1500)*(duty/100)
  double pulseLength = (750 + 1500*((double)duty/100));
  Serial.print(duty);
  Serial.print(", ");
  Serial.print(pulseLength);
  Serial.print("\n");
  digitalWrite(pwmPin, HIGH);
  delayMicroseconds(pulseLength);
  digitalWrite(pwmPin, LOW);
}
