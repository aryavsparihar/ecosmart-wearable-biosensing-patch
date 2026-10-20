# Design notes

Why the firmware and the ML pipeline look the way they do. Each section names
the decision, the constraint that forced it, and what it costs.

> Research prototype. Not a medical device, and all model results come from
> synthetic data. See the disclaimer in the [README](../README.md).

---

## 1. Constraints

| Resource | ATmega328P | Consequence |
|---|---|---|
| SRAM | 2,048 B, shared by globals, stack and Serial buffers | Static allocation only, no heap, windows of int16 |
| Flash | 30,720 B usable | Model, floors and strings in `PROGMEM` |
| CPU | 8-bit AVR, 16 MHz, no FPU | Integer and fixed-point maths, no `float` in the signal path |
| `int` | **16 bits** | Every product that can exceed ±32,767 is explicitly widened to `int32_t` / `int64_t` |

Budget at 1 Hz: one window every 8 s. Feature extraction, normalisation and
tree inference take well under a millisecond, so compute is never the
bottleneck. Memory is.

## 2. Fixed-point representation

Every channel is an `int16_t` in **hundredths** of its unit: pH × 100,
mS/cm × 100, °C × 100. Two decimal places are below the sensor resolution
(the 10-bit ADC is about 0.03 pH per LSB, and the DS18B20 is 0.0625 °C per
LSB), and values clamp to `[0, 10000]` before they reach the pipeline. That
clamp is what makes the overflow analysis in `window_features.h` hold:

- `Σ(x − mean)²` over 32 samples is ≤ 32 × 10⁸ = 3.2 × 10⁹, which fits in
  `uint32_t`;
- `6 · Σ wᵢ xᵢ` with `wᵢ = 2i − 31` is ≤ about 3.1 × 10⁷, which fits in
  `int32_t`;
- baseline sums of squares use `int64_t` (8 windows × features up to
  2.5 × 10⁷).

All divisions use one rounding rule (`fx::divRound`: half away from zero),
and the Python mirror implements the same rule. Mixing C's truncation with
Python's floor division would break parity on negative slopes.

## 3. Windowing and features

32 samples (32 s) with a hop of 8: long enough to estimate a slope over sensor
noise, short enough that an anomaly is flagged within about half a minute. The
features are the five requested per channel:

| Feature | Why it helps |
|---|---|
| mean | level shifts: pH excursions, conductivity surges, temperature rises |
| variance | intermittent contact and noise bursts |
| slope × window length | onsets and recoveries, before the mean has moved much |
| min / max | short dips and spikes that the mean dilutes |

The slope uses the closed form for equally spaced x, so no x arrays and no
divisions inside the loop: `round(6 · Σ(2i − (n−1))·xᵢ / (n² − 1))` is the
least-squares slope multiplied by n.

## 4. Personal baseline, and the floor problem

Anomalies must be relative to the wearer, so the first 8 windows (88 samples)
freeze a per-feature mean and scale, and every later window becomes
`z = (f − mean) / scale`, quantised to 1/16 σ in an `int8_t` (saturating at
about ±7.9 σ).

The first version used the calibration standard deviation as the scale. Test
F1 was **0.31**. The diagnosis: 88 seconds cannot see slow physiological
drift, so ordinary drift later in the session looked like a 10 σ event. 59 %
of *normal* windows saturated `temp_mean` at ±127, exactly as often as true
anomalies did.

The fix keeps the **mean personal** and takes a **population floor** for the
scale. The floor is the pooled robust spread (1.4826 × MAD) of normal training
windows around each wearer's own calibration mean, and the scale is
`max(personal std, floor)`. Test F1 rose to **0.907**. The floors are
learnt in `train.py`, exported into `model_weights.h`, and read by
`Baseline::finalize()` from flash.

A relative (multiplicative) floor for conductivity was also tried. It gained
about 0.02 F1 but needed extra state and another parity surface, so it was
left out.

**Memory trick**: once frozen, the 240 B of `int64` accumulators are dead.
`Baseline` overlays them with the `{mean, scale}` pairs in a union. Each
`stats[i]` occupies exactly the bytes of `sum[i]`, which `finalize()` has
already consumed when it writes them. Saving: 120 B, with no extra stack.

## 5. Why a decision tree

The requirement was a *tiny* model: logistic regression or a shallow tree.

1. **Two-sided deviations.** Sweat pH is abnormal both when it is too acidic
   *and* when it is too alkaline. A logistic regression on signed z-scores is
   monotone in each feature, so it cannot express "far from 0 either way"
   without hand-crafted |z| features. On the same inputs, test F1 was **0.274
   for logistic regression versus 0.907 for the tree**.
2. **Integer-exact inference.** The inputs are integers, so sklearn's split
   `z ≤ t.5` equals `z ≤ ⌊t⌋`. The deployed tree makes *exactly* the
   decisions of the trained one: agreement on the training set is 1.0, and
   the parity test checks every node visit.
3. **Cost.** At most 5 `int8` comparisons per window, no multiplications. 21
   nodes × 4 B in flash, read with `memcpy_P`.
4. **Explainability.** `model_weights.h` prints every node as a rule
   (`temp_mean <= +3.88 sigma ? 1 : 22`).

Depth is chosen by grouped 5-fold CV on the training wearers: the smallest
depth within 0.01 F1 of the best, which is 5. Redundant splits (both subtrees
predict the same label) are collapsed, taking the tree from 35 to 21 nodes
with identical decisions.

## 6. Saturation guard

Trees extrapolate badly. Random garbage input (every z pinned at ±127) landed
in a *normal* leaf, because no training window ever had mean +127 together
with min −127. Windows with ≥ 6 saturated z-scores are therefore flagged
outright (`cfg::kSaturationGuard`). In the synthetic data, no realistic
window, normal or anomalous, saturates 8 or more features, while garbage
saturates about 15. A threshold of 6 trips on only 0.08 % of normal windows.
The guard therefore catches inputs the model has no business classifying
(shorted probe, broken wire). The parity vectors include such a burst.

## 7. Alert hysteresis

Window decisions overlap by 75 %, so one noisy window could otherwise flicker
the LED. The alert turns on after 2 consecutive anomalous windows and off
after 2 consecutive normal ones. On the synthetic test set this gives 0.3
false alerts per hour and a median detection latency of 23.5 s.

## 8. Hardware abstraction

```
src/            portable core: compiles for AVR and for the host
src/hal/        Arduino-only: analogRead, OneWire, digitalWrite, Serial
include/platform.h   PROGMEM / memcpy_P / pgm_read_* shims for the host
```

The `native` environment builds everything except `src/hal/` and `main.cpp`,
so the exact code that runs on the Nano is what the Unity tests exercise.
The DS18B20 driver is a non-blocking state machine: request a conversion,
return, and read the result ≥ 750 ms later. A 1 Hz loop therefore never
stalls for the 12-bit conversion time.

**Lesson learnt:** the header was originally called `features.h`. On Linux,
glibc's `<stdio.h>` includes its own `<features.h>`, and with `include/` on
the search path the native build picked up ours. It passed on macOS and failed
in CI. It is now `window_features.h`.

## 9. Testing without hardware

| Layer | Tool | Catches |
|---|---|---|
| Unit | Unity on the host (`pio test -e native`) | Logic errors in the portable core |
| Cross-language | Golden vectors from Python, replayed in C++ | Any drift between training maths and firmware maths |
| End-to-end | `SIMULATION_MODE` generator → real pipeline → real model (host) | Model or threshold changes that break the demo |
| Binary | Real `firmware.elf` on **simavr** in CI | AVR-specific bugs: 16-bit `int`, PROGMEM, startup, serial |
| Circuit | Wokwi (`diagram.json`) | Wiring, OneWire timing, LED behaviour |
| Toolchain | `arduino-cli` compile of the exported sketch | Arduino IDE compatibility |

The parity contract is enforced from both sides. `pytest` fails if the
committed vectors are stale, and the C++ test checks a model fingerprint
before comparing outputs. A deliberate mutation (dropping one rounding term in
the variance) fails the parity test on the very first window.

## 10. Synthetic data design

The generator's job is to make the problem *honest*, not easy:
- each wearer has their own baseline, so global thresholds don't work;
- benign exertion bumps (+8-30 % conductivity, slightly warmer skin) are
  labelled normal, so magnitude alone isn't enough;
- anomalies ramp in and out, so window labels near boundaries are genuinely
  ambiguous;
- values are quantised exactly as the firmware sees them.

The split is by wearer (90 train / 30 test), and nothing about the test
wearers is used for depth selection or floors. None of this makes the data
real. See *Limitations* in the README.
