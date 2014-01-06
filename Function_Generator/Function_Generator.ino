const int clkPin = 0;
const int udPin = 1;
const int pulseDelay = 2;
const int pwmPin = 4;

void setup(){
  
  pinMode(clkPin, OUTPUT);
  pinMode(udPin, OUTPUT);
  digitalWrite(clkPin, HIGH);
  digitalWrite(udPin, HIGH);
  reset();
}


void loop(){
  
  for (int i=0; i <= 128; i++){
    
    increment('u');
  }
  
  for (int i=0; i <= 128; i++){
    
    increment('d');
  }
}


void increment(char dir){
  
  if (dir == 'u'){
    
    digitalWrite(udPin, HIGH);
  }
  
  else if (dir == 'd'){
    
    digitalWrite(udPin, LOW);
  }
  
  digitalWrite(clkPin, LOW);
  delay(pulseDelay);
  digitalWrite(clkPin, HIGH);
}


void reset(){
  
  digitalWrite(udPin, LOW);
  for(int i=0;i<255;i++){
    
    digitalWrite(clkPin, LOW);
    delay(pulseDelay);
    digitalWrite(clkPin, HIGH);
  }
  
  digitalWrite(udPin, HIGH);
  for(int i=0;i<64;i++){
    
    digitalWrite(clkPin, LOW);
    delay(pulseDelay);
    digitalWrite(clkPin, HIGH);
  }
}
