// firmware/test2_sweep/test2_sweep.ino
// Arduino Uno R3. TX: Timer2 toggles D2 (module VCC). RX: Timer1 input capture on D8 (ICP1).
// For each TX half-period, measures mean HIGH width, mean LOW width and edges per second.
// RX OUT is active-low: LOW = IR seen, HIGH = no IR.

struct Setting { uint16_t halfUs; uint8_t cs; uint8_t ocr; };

// Half-period = (OCR2A + 1) x tick. Prescaler 32 = 2 us tick, prescaler 8 = 0.5 us tick.
const Setting SETTINGS[] = {
  {500, _BV(CS21) | _BV(CS20), 249},   //  2 kbps
  {200, _BV(CS21) | _BV(CS20),  99},   //  5 kbps
  {100, _BV(CS21),             199},   // 10 kbps
  { 50, _BV(CS21),              99},   // 20 kbps
  { 20, _BV(CS21),              39},   // 50 kbps
  { 10, _BV(CS21),              19},   // 100 kbps
};
const uint8_t N_SETTINGS = sizeof(SETTINGS) / sizeof(SETTINGS[0]);
const uint8_t REPEATS = 3;             // 1-second measurements per setting

volatile uint16_t lastCap = 0;
volatile bool first = true;
volatile uint32_t sumH = 0, sumL = 0;
volatile uint32_t nH = 0, nL = 0, nBad = 0;

ISR(TIMER2_COMPA_vect) { PORTD ^= _BV(PD2); }

ISR(TIMER1_CAPT_vect) {
  uint16_t t = ICR1;
  uint16_t w = t - lastCap;            // unsigned subtraction handles timer wrap
  lastCap = t;
  bool fell = !(TCCR1B & _BV(ICES1));  // were we capturing a falling edge?
  TCCR1B ^= _BV(ICES1);                // now wait for the opposite edge
  TIFR1 |= _BV(ICF1);                  // clear flag set by the edge change
  if (first) { first = false; return; }
  if (w > 10000) { nBad++; return; }   // longer than 5 ms: not a real pulse
  if (fell) { sumH += w; nH++; }       // interval ended on falling edge -> pin was HIGH
  else      { sumL += w; nL++; }
}

void setTx(const Setting &s) {
  TIMSK2 = 0;
  TCCR2A = _BV(WGM21);                 // CTC mode
  TCCR2B = s.cs;
  OCR2A  = s.ocr;
  TCNT2  = 0;
  TIMSK2 = _BV(OCIE2A);
}

void setup() {
  Serial.begin(115200);
  DDRD |= _BV(PD2);                    // D2 output (TX module VCC)
  pinMode(8, INPUT);
  TCCR1A = 0;
  TCCR1B = _BV(CS11);                  // prescaler 8 -> 0.5 us per tick, falling-edge capture
  TIMSK1 = _BV(ICIE1);
  Serial.println(F("half_us,expected_edges_per_s,edges_per_s,meanH_us,meanL_us,bad_pulses"));
}

void loop() {
  for (uint8_t i = 0; i < N_SETTINGS; i++) {
    setTx(SETTINGS[i]);
    delay(200);                        // let the link settle at the new speed
    for (uint8_t r = 0; r < REPEATS; r++) {
      noInterrupts();
      sumH = sumL = 0; nH = nL = nBad = 0;
      first = true;
      TCCR1B &= ~_BV(ICES1);           // start by waiting for a falling edge
      TIFR1 |= _BV(ICF1);
      interrupts();

      delay(1000);

      noInterrupts();
      uint32_t sH = sumH, sL = sumL, cH = nH, cL = nL, bad = nBad;
      interrupts();

      Serial.print(SETTINGS[i].halfUs);               Serial.print(',');
      Serial.print(1000000UL / SETTINGS[i].halfUs);   Serial.print(',');
      Serial.print(cH + cL);                          Serial.print(',');
      Serial.print(cH ? sH * 0.5 / cH : 0.0, 1);      Serial.print(',');
      Serial.print(cL ? sL * 0.5 / cL : 0.0, 1);      Serial.print(',');
      Serial.println(bad);
    }
  }
  TIMSK2 = 0;
  PORTD &= ~_BV(PD2);
  Serial.println(F("done"));
  while (true) {}                      // press reset on the Uno to run again
}