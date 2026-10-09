const int RX = 8;

void setup() {
  Serial.begin(115200);
  DDRD |= _BV(PD2);              // D2 output (TX module VCC)
  pinMode(RX, INPUT);

  // Timer2 CTC: prescaler 32 -> 2 us per tick, OCR2A=249 -> 500 us half-period
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS21) | _BV(CS20);
  OCR2A  = 249;
  TIMSK2 = _BV(OCIE2A);
}

ISR(TIMER2_COMPA_vect) { PORTD ^= _BV(PD2); }

void loop() {
  unsigned long lowW  = pulseIn(RX, LOW,  5000);
  unsigned long highW = pulseIn(RX, HIGH, 5000);
  Serial.print("low="); Serial.print(lowW);
  Serial.print(" us  high="); Serial.print(highW);
  Serial.println(" us");
  delay(300);
}