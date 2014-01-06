const int clkPin = 0;
const int udPin = 1;

const int pulseDelay = 1; //time in ms between pulsing clkPin on and off
const int period = 1000; //period of sine wave in ms
const int iterations = period/pulseDelay/2;

int counter;
int totalPulses;
int multiplier;

void setup(){
  
  pinMode(clkPin, OUTPUT);
  pinMode(udPin, OUTPUT);
  digitalWrite(clkPin, HIGH);
  digitalWrite(udPin, HIGH);
  reset();
  totalPulses = 0;
}


void loop(){
  Serial.print("\nInitializing...\nPulse Delay: ");
  Serial.print(pulseDelay);
  Serial.print("\nPeriod: ");
  Serial.print(period);
  Serial.print("\nTotal loop cycles: ");
  Serial.print(iterations);
  Serial.print("\n\n"); 
  
  totalPulses = 0;
  
  for (counter=0; counter < iterations; counter++){
    
    increment('u');
  }
  
  Serial.print("\n\nSwitching directions\n\n");
  
  for (counter=0; counter < iterations; counter++){
    
    increment('d');
  }
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

void increment(char dir){
  
  double pulnum;
  
  if (dir == 'u'){
    
    digitalWrite(udPin, HIGH);
    multiplier = 1;
  }
  
  else if (dir == 'd'){
    
    digitalWrite(udPin, LOW);
    multiplier = -1;
  }
  
  
  pulnum = (.51*sin((2*3.14)*((double)counter*2/(double)iterations*2)+4.71)+.51);
  int numPulses = round(pulnum);
  
  Serial.print("\n\nNumber of pulses for this increment: ");
  Serial.print(pulnum);
  Serial.print(", ");
  Serial.print(numPulses);
  //Serial.print("\nDirection: ");
  //Serial.print(dir); 
  //Serial.print("\nPulse Delay: ");
  //Serial.print(pulseDelay);
//  Serial.print("\nPeriod: ");
//  Serial.print(period);
  Serial.print("\nIterations: ");
  Serial.print(iterations);
  Serial.print("\nCounter: ");
  Serial.print(counter);
  Serial.print("\nTotal Pulses: ");
  Serial.print(totalPulses);
  
  for(int i = 0; i < numPulses; i++){
    digitalWrite(clkPin, LOW);
    delay(pulseDelay);
    digitalWrite(clkPin, HIGH);
    totalPulses = totalPulses + multiplier;
  }
}
