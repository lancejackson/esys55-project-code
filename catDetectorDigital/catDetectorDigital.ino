const int sensorPin = 13;
int sensorValue;

void setup() {
  // put your setup code here, to run once:
  pinMode(sensorPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  sensorValue = digitalRead(sensorPin);
  /*
  float voltage = sensorValue * 5.0;
  voltage /= 1024.0;

  Serial.print(voltage);
  Serial.println(" volts");*/
  Serial.println(sensorValue);
  delay(1000);
}
