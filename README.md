# adaptive-ir-optical-link
Characterizing and adapting the bit rate of an IR optical link on Arduino Uno (ATmega328P) with FC-51 modules. Timer1 input capture, BER measurement.

#
# Adaptive-Rate IR Optical Link (Arduino Uno + FC-51)

Characterizing how fast an infrared optical link built from two off-the-shelf FC-51 modules can switch, with the goal of building a link that adapts its own bit rate.

**Status:** Test 2 (speed sweep) complete. BER measurement, adaptive control and encoding comparison not started yet.

---

## Goal

1. Measure the usable bandwidth of an IR link made from two FC-51 modules, using hardware timer input capture on an ATmega328P.
2. Measure bit error rate (BER) against bit rate.
3. Build a controller that changes the bit rate based on link quality.
4. Compare on-off keying against Manchester encoding.

## Hardware

| Part | Role |
|---|---|
| Arduino Uno R3 (ATmega328P, 16 MHz) | TX timing, RX capture, serial logging |
| FC-51 IR module #1 | Transmitter |
| FC-51 IR module #2 | Receiver |
| Plastic bottles | Mounts to hold the modules facing each other |

The FC-51 is an obstacle-avoidance sensor, not a data modem. Its IR emitter is wired to be always on, so the transmitter is modulated by switching the module's **VCC pin from a digital output**. The receiver side is photodiode → LM393 comparator → digital OUT (active-low: LOW means IR detected).

### Wiring

| Module | Pin | Uno |
|---|---|---|
| TX | VCC | D2 |
| TX | GND | GND |
| TX | OUT | not connected |
| RX | VCC | 5V |
| RX | GND | GND |
| RX | OUT | D8 (Timer1 input capture, ICP1) |

Notes: the RX module's own emitter is covered with tape; TX emitter faces the RX photodiode; measured indoors away from direct sunlight. Driving the module from D2 draws about 20 mA, near the pin's recommended limit.

*Photo of the setup: `docs/setup.jpg`*

## Method

**Transmit:** Timer2 in CTC mode toggles D2 from an interrupt, producing a square wave with a chosen half-period. Because the interrupt runs independently of the main loop, the transmitter keeps going while the receiver is measured.

**Receive:** Timer1 runs at 16 MHz / 8 = 2 MHz (0.5 µs per tick). Input capture on D8 stores the timer value in hardware on every edge, so pulse widths are measured to 0.5 µs without software delay. The edge-select bit is flipped after each capture so that HIGH and LOW widths are measured separately. Counts and widths are averaged over one second per reading.

**Sweep:** half-periods of 200, 100, 50, 20 and 10 µs (square wave, one bit per half-period, i.e. 5, 10, 20, 50 and 100 kbps equivalent), several one-second readings each.

## Results: Test 2 (speed sweep)

Conditions: distance ___ cm (LED to LED), RX pot at ________, lighting ________, TX driven directly from D2.

| Half-period | Equivalent rate | Expected edges/s | Measured edges/s | Mean HIGH (µs) | Mean LOW (µs) |
|---|---|---|---|---|---|
| 200 µs | 5 kbps | 5000 | 5000, 4998 | 138.0 | 262.0 |
| 100 µs | 10 kbps | 10000 | 9998 (x3) | 37.6 | 162.4 |
| 50 µs | 20 kbps | 20000 | 0 (x3) | none | none |
| 20 µs | 50 kbps | 50000 | 0 (x3) | none | none |
| 10 µs | 100 kbps | 100000 | 0 (x3) | none | none |

Raw output: `data/test2_sweep.csv`.

### What the data shows

- Edges arrive essentially intact at 5 and 10 kbps (99.96-100% delivered) and do not arrive at all at 20 kbps and above.
- The total period is preserved (HIGH + LOW = 2x the half-period), but the duty cycle is heavily distorted.
- The distortion is the same size at both working rates: HIGH is about 62 µs shorter than nominal and LOW about 62 µs longer. That is consistent with a fixed turn-on versus turn-off delay mismatch of roughly 62 µs in the link, rather than random noise.

### Hypothesis (not yet tested)

If the skew is a fixed ~62 µs, the shortened HIGH pulse disappears once the half-period gets close to that value, which would put the cutoff near 60-65 µs half-period. The cause of the skew is not yet known; candidates are slow turn-off of the transmitter emitter because of supply decoupling on the module, photodiode/comparator response, and the pot threshold setting. The plan is to test the cutoff prediction with a fine sweep before drawing conclusions.

## Limitations so far

- Square-wave test only; no data frames or BER yet.
- A single pair of modules, one distance and one pot setting so far.
- Only 2 readings at the 200 µs setting versus 3 elsewhere.
- Last column of the raw CSV is always 0 and its meaning needs to be documented from the firmware.

## Repository layout

```
firmware/
  test1_link_check/     TX via Timer2 ISR, RX via pulseIn (slow-rate sanity check)
  test2_sweep/          TX via Timer2, RX via Timer1 input capture
data/
  test2_sweep.csv       raw serial output
docs/
  lab_log.md            dated run log with conditions
  setup.jpg             photo of the bottle-mounted setup
README.md
```

## Next steps

1. Fine sweep at 90, 80, 70, 65, 60 and 55 µs half-period to test the ~62 µs cutoff prediction.
2. Repeat at multiple fixed distances and pot settings.
3. Try a transistor-switched transmitter to see whether the skew changes.
4. Frame format with preamble and sync word; BER versus bit rate.
5. Adaptive rate controller, then Manchester versus on-off keying comparison.

## License

MIT
