# MOVE Prosthetics — Stage 1 Envelope
## Engineering Decision Report

**In response to:** *MOVE Prosthetics — Stage 1 Envelope: Decision Brief*
**Scope:** Design decisions required before implementing `src/envelope.c`
**Deliberately excluded:** the `envelope.c` implementation itself, per the brief's instruction

---

### Tagging convention

Every claim in this report carries one of three tags, as the brief requires:

| Tag | Meaning |
|---|---|
| **[FACT]** | Specified by MOVE project material or the ADS1299 datasheet |
| **[ASSUMPTION]** | Engineering assumption — must be verified |
| **[REC]** | My recommendation — a judgement call, open to challenge |

---

### Summary of decisions

| Question | Decision | Section |
|---|---|---|
| Sampling frequency | **1000 SPS** | 3 |
| High-pass filter | **First-order IIR** | 4 |
| Coefficient α | **0.8884** (at f_s=1000, f_c=20) | 5 |
| Filter state | **2 variables** + coefficient + prime flag | 6 |
| Smoothing | **Moving average, N = 100** (100 ms) | 7 |
| Numeric type | **`float` internal, `int32_t` at API** | 8 |
| API change | **Parameterised `envelope_init()`** | 10 |

---

## 1. Known parameters

Established by MOVE project material — not decisions, constraints.

| Parameter | Value | Source |
|---|---|---|
| AFE | ADS1299-4PAGR, 24-bit, 4-channel | **[FACT]** PCB project definition |
| Channels in use | 2 differential (flexor, extensor) + 1 driven bias | **[FACT]** PCB project definition |
| Clock | Internal oscillator (2.048 MHz) | **[FACT]** PCB project definition |
| Voltage reference | Internal 4.5 V | **[FACT]** PCB project definition |
| Host link | SPI, sample timing driven by DRDY interrupt | **[FACT]** PCB project definition |
| Sample format | 24-bit two's complement, MSB first | **[FACT]** ADS1299 datasheet |
| PGA gain options | 1, 2, 4, 6, 8, 12, 24 | **[FACT]** ADS1299 datasheet |
| Available data rates | 250 SPS – 16 kSPS | **[FACT]** ADS1299 datasheet |
| EMG band of interest | ~20–500 Hz; dominant energy 50–150 Hz | **[FACT]** MOVE project material |
| Mains frequency | 50 Hz (Germany) | **[FACT]** MOVE project material |
| Ring buffer | `WINDOW_SIZE 256`, `int32_t` storage, Stage 0 complete | **[FACT]** MOVE project material |

---

## 2. Unresolved parameters

The brief asked me not to pretend hardware parameters are known when they are not. These four are genuinely **not established** anywhere in MOVE's material:

| # | Unresolved | Consequence | How to resolve |
|---|---|---|---|
| 1 | **Output data rate** (CONFIG1 `DR[2:0]`) | Determines α and the real duration of the averaging window | Decide it (§3), then configure the board to match — or read CONFIG1 back at startup |
| 2 | **PGA gain setting** | Scales every sample, therefore every Stage 2 threshold | Choose during board bring-up; check for clipping on real electrodes |
| 3 | **Real EMG amplitude in ADC counts** | All thresholds are provisional without it | Measure once the board acquires real signals |
| 4 | **Acceptable end-to-end latency budget** | Constrains the smoothing window length | MOVE has not defined one; §7 proposes a working target |

Items 2–4 do **not** block Stage 1 implementation. Item 1 does, and is resolved next.

---

## 3. Recommended sampling frequency

### The governing constraint

**[FACT]** The ADS1299's on-chip sinc³ decimation filter sets the usable analog bandwidth, and that bandwidth scales with the output data rate. At 250 SPS the −3 dB bandwidth is 65.5 Hz; at 1000 SPS it is four times wider (~262 Hz). The ratio is therefore approximately **0.262 × f_DR**.

This single fact eliminates the lower data rates:

| Data rate | −3 dB bandwidth | Covers 50–150 Hz? | Verdict |
|---|---|---|---|
| 250 SPS | ~66 Hz | ✗ | Rejected — cuts into the dominant EMG band |
| 500 SPS | ~131 Hz | Partially | Rejected — clips the top of the dominant band |
| **1000 SPS** | **~262 Hz** | ✓ | **[REC] Recommended** |
| 2000 SPS | ~524 Hz | ✓ (full 20–500 Hz) | Viable alternative |
| 4000+ SPS | ≥1 kHz | ✓ | Rejected — no benefit, more noise and data |

### **[REC] Recommendation: 1000 SPS**

Justification:

1. **Bandwidth is sufficient.** ~262 Hz fully covers the 50–150 Hz range where EMG power actually lives.
2. **[FACT] Lower data rate means lower noise.** The datasheet states that reducing the data rate increases internal averaging and correspondingly reduces noise. 1000 SPS is therefore *quieter* than 2000 SPS — a real advantage for a microvolt-level signal.
3. **Δt = 1 ms exactly.** Every filter and window calculation becomes trivially checkable by hand, which matters in a learning project.
4. **Trivial CPU load.** A 1 kHz DRDY interrupt is nothing for a 480 MHz STM32H7.
5. **The envelope is a slow quantity.** Content above 262 Hz contributes little to a smoothed activation estimate.

### Documented alternative

**2000 SPS** if MOVE later wants the complete 20–500 Hz band — for example, median-frequency fatigue analysis, which needs the upper band. Costs: double the data rate and slightly higher noise.

> **Critical coupling:** α (§5) and the real-world duration of N (§7) are *both* functions of f_s. Changing the data rate silently invalidates both. This is the primary reason for the API change proposed in §10.

---

## 4. Recommended high-pass filter

### Purpose

**[FACT]** The ADS1299 is DC-coupled and has no input high-pass. Electrode half-cell potentials and motion artifact therefore arrive at the ADC unattenuated. The high-pass exists to remove **DC offset and slow baseline drift** — nothing else.

### Evaluation against the brief's criteria

| Criterion | FIR | 2nd-order IIR (biquad) | **1st-order IIR** |
|---|---|---|---|
| Adequate for ~20 Hz cutoff | Yes, sharp | Yes, −12 dB/oct | **Yes, −6 dB/oct — sufficient, see below** |
| Computational complexity | ~100–200 MAC/sample | ~5 MAC/sample | **3 operations/sample** |
| Required filter state | N-sample delay line (~100–200 floats) | 2–4 floats | **2 floats** |
| Numerical stability | Unconditionally stable (no feedback) | Stable, but poles sit close to z=1 at f_c/f_s = 0.02 → coefficient-sensitive | **Pole at z = α = 0.888, comfortably inside the unit circle** |
| Ease of understanding | Coefficient design needs a tool | Requires bilinear transform | **Derivable by hand from an RC circuit** |
| Portability to STM32 | Fine (H7 has FPU/DSP), but largest memory | Fine | **Fine — smallest footprint** |
| Real-time sample-by-sample | O(N), bounded | O(1) | **O(1), constant time** |
| **Verdict** | Rejected | Deferred | **[REC] RECOMMENDED** |

### Why first-order is genuinely sufficient — not a beginner compromise

A sharp filter is only necessary when wanted and unwanted content sit **close together in frequency**. Here they do not:

```
   drift        (wanted signal)
   ~0–2 Hz         50–150 Hz
      │                │
   ───┴────────────────┴────────►  frequency
       ↑ cutoff 20 Hz
       └── a whole decade of separation
```

A −6 dB/octave rolloff from 20 Hz attenuates 2 Hz drift by roughly 20 dB and 0.2 Hz drift by roughly 40 dB. That is ample, because there is no wanted signal just below 20 Hz that needs preserving.

There is also a **stability argument in favour of first-order** at this cutoff ratio: with f_c/f_s = 0.02, a 2nd-order Butterworth high-pass places its poles very close to the unit circle, making it noticeably sensitive to coefficient quantisation — which becomes a real problem at Stage 3 when converting to fixed-point. The first-order filter has no such fragility.

### ⚠ What this filter does *not* do

**A 20 Hz high-pass does not remove 50 Hz mains hum.** 50 Hz is *above* the cutoff and passes through essentially unattenuated.

Hum is handled elsewhere in the system: **[FACT]** by the driven-bias (DRL) electrode in hardware, and by differential measurement / CMRR. A dedicated notch filter remains an option later.

**Consequence for Stage 2:** after rectification, residual 50 Hz hum smooths into a **DC pedestal that raises the resting envelope baseline**. Thresholds must be set against the *measured* resting level, not against zero.

---

## 5. Difference equation

```
y[n] = α · ( y[n−1] + x[n] − x[n−1] )
```

Coefficient, derived from the analog RC prototype:

```
α = RC / (RC + Δt)  =  1 / (1 + 2π · f_c · Δt)
```

**Worked value** at f_s = 1000 Hz (Δt = 0.001 s) and f_c = 20 Hz:

```
RC = 1 / (2π × 20)             = 0.0079577 s
α  = 0.0079577 / 0.0089577     = 0.8884
```

Cross-check via the second form: `1 / (1 + 2π × 20 × 0.001) = 1 / 1.125664 = 0.8884` ✓

**[REC]** Compute α at init from `f_s` and `f_c`. Do **not** hardcode `0.8884` — see §3's coupling warning.

---

## 6. Required filter state

State that must persist between calls to `envelope_update()`:

| State | Type | Role | Changes every sample? |
|---|---|---|---|
| `prev_input` | `float` | x[n−1] in the difference equation | **Yes** |
| `prev_output` | `float` | y[n−1] in the difference equation | **Yes** |
| `hp_alpha` | `float` | Coefficient α | No — set at init |
| `primed` | `uint8_t` | First-sample guard | Once, on first call |

The high-pass therefore needs **two true state variables**, plus one constant and one flag.

### Why `primed` is necessary

On the very first call, `prev_input` has no meaningful value. Initialising it to zero injects an artificial step equal to the full first sample, producing a spurious envelope spike that decays over several time constants — long enough to look like a real contraction.

**[REC]** On the first sample, seed `prev_input = x[0]` and `prev_output = 0`, then set `primed = 1`.

### Smoothing state

The moving average additionally requires the **rectified sample history**, held in the existing `ring_buffer_t` (§9). No redesign of the Stage 0 buffer is required.

---

## 7. Recommended smoothing method and window

### **[REC] Method: moving average of the rectified signal**

This matches the project specification, and it reuses the Stage 0 ring buffer as intended. A single-pole IIR low-pass is a valid alternative (1 state variable, no buffer needed) and is the natural Stage-1b refinement — but it would leave the ring buffer unused, so the moving average is the better first implementation.

### **[REC] Window: N = 100 samples (100 ms at 1000 SPS)**

A moving average has a group delay of approximately **(N − 1) / 2 samples**:

| N | Window @1000 SPS | Group delay | Assessment |
|---|---|---|---|
| 50 | 50 ms | ~25 ms | Responsive, but visibly rippling |
| **100** | **100 ms** | **~50 ms** | **[REC] Balanced** |
| 200 | 200 ms | ~100 ms | Very smooth, sluggish |
| 256 | 256 ms | ~128 ms | Consumes most of the latency budget alone |

### The tradeoff

```
   short window  ←──────────────────────────→  long window
   fast response                              smooth, stable envelope
   noisy, rippling envelope                   laggy, feels disconnected
   risk: false triggers in Stage 2            risk: unresponsive prosthesis
```

Rectification leaves ripple at roughly twice the input frequency; a 100 ms window averages several cycles of the dominant band and about five cycles of any residual 50 Hz hum, which suppresses that ripple well.

**[ASSUMPTION]** Myoelectric-control literature commonly cites controller delay below roughly 100–125 ms as desirable, with usability degrading well before 300 ms. Treat this as a working target to verify against a source MOVE trusts, not as an established requirement. Because motor spin-up and mechanical tendon travel add further delay downstream, holding the DSP contribution near 50 ms leaves genuine budget for the rest of the chain.

### On reusing `WINDOW_SIZE 256`

The brief was right to question this. They are **two different quantities**:

- `WINDOW_SIZE` (256) = ring buffer **capacity** — how much history is stored
- `N` (100) = **averaging window** — how much of that history is used

The requirement is only `N ≤ WINDOW_SIZE`. **[REC]** Keep capacity at 256: it costs 1 KB and lets you experiment with N anywhere up to 256 without resizing or re-testing the completed Stage 0 buffer.

### Implementation note — O(N) vs O(1)

A naive re-sum of N samples per update is O(N): at 1000 Hz × 100 that is 100,000 float additions per second, which is negligible on an STM32H7.

**[REC] Start with the simple O(N) version.** It is obviously correct and easy to verify against hand calculation. The O(1) running-sum optimisation (add newest, subtract oldest via `rb_get(rb, N-1)`) is a documented later improvement — note that running sums accumulate floating-point drift over long runtimes and need periodic recomputation.

---

## 8. Recommended numeric representation

### **[REC] `float` internally; `int32_t` at the API boundary**

Justification:

1. **α = 0.8884 has no integer representation** without a scaling scheme.
2. **The high-pass has feedback.** In integer arithmetic, per-sample rounding errors accumulate *through* that feedback path and can produce DC bias or limit cycles — a subtle numerical bug that would be very hard to diagnose while simultaneously learning C.
3. **The project plan already defers this.** Stage 3 explicitly covers float-to-fixed-point conversion. Doing it now inverts the intended learning sequence.
4. **The STM32H743 has a hardware FPU**, so single-precision float is fast on the real target. Use `float`, not `double` — it matches the FPU's native width and halves memory.

### Precision — an exact and reassuring result

IEEE-754 single precision has a 24-bit mantissa, so it represents every integer exactly up to 2²⁴ = 16,777,216. A 24-bit signed ADC sample never exceeds 2²³ = 8,388,608.

**Therefore every possible ADS1299 sample converts to `float` with zero precision loss.**

### ⚠ Overflow hazard to avoid

If a moving-average sum were accumulated in `int32_t`:

```
256 samples × 2²³ (full scale)  = 2³¹ = 2,147,483,648
INT32_MAX                              = 2,147,483,647
```

A full-scale 256-sample sum overflows `int32_t` by **exactly one**. With N = 100 the sum reaches ~8.4 × 10⁸ — about 39% of the range, safe — but the margin depends on N and on signal amplitude.

**[REC] Accumulate in `float`** (or `int64_t`) and remove the hazard entirely regardless of N.

### Boundary conversions

| Point | Type | Note |
|---|---|---|
| Input | `int32_t` → `float` | Exact (see above) |
| Rectified value → ring buffer | `float` → `int32_t` | Rounded; discarded fraction < 1 ADC count, far below the ADS1299 noise floor |
| Envelope output | `float` → `int32_t` | Keeps the API consistent with Stage 0 and Stage 2 |

---

## 9. Final `envelope_t` design

```c
typedef struct {
    /* --- high-pass filter state --- */
    float    hp_alpha;      /* coefficient α, computed at init      */
    float    prev_input;    /* x[n-1]                               */
    float    prev_output;   /* y[n-1]                               */

    /* --- smoothing --- */
    ring_buffer_t rect_buf; /* rectified history (reuses Stage 0)   */
    uint16_t window;        /* N; must be <= WINDOW_SIZE            */

    /* --- housekeeping --- */
    uint8_t  primed;        /* first-sample guard                   */
} envelope_t;
```

### Justification per member

| Member | Why it must exist | Changes every sample? |
|---|---|---|
| `hp_alpha` | Required by the difference equation. Stored rather than `#define`d so each channel is independently configurable and so it tracks `f_s`. | No |
| `prev_input` | x[n−1] — required by the difference equation | **Yes** |
| `prev_output` | y[n−1] — required by the difference equation | **Yes** |
| `rect_buf` | Holds rectified history for the moving average | **Yes** |
| `window` | The averaging length N, deliberately distinct from buffer capacity | No |
| `primed` | Prevents the first-sample transient described in §6 | Once |

Nothing else earns a place. Notably absent: no sample counter (the ring buffer's `filled` flag covers warm-up), no cached output (the caller receives it by return value), no configuration copies of `f_s` or `f_c` (they are needed only to compute α at init).

### Incorporating the existing `ring_buffer_t`

The buffer is embedded **by value**, not by pointer. This means:

- Each `envelope_t` owns its own history — two channels cannot alias one another.
- An `envelope_t` can be declared statically: **no dynamic allocation anywhere.**
- The Stage 0 code is used **unmodified**, as the brief required.

**The one type mismatch, and its resolution:** `ring_buffer_t` stores `int32_t`, but the rectified high-pass output is a `float`. Rather than redesign the ring buffer (explicitly out of scope), round the rectified value to `int32_t` before pushing. The lost fraction is below one ADC count — well under the converter's own noise floor — so it has no practical effect on the envelope.

---

## 10. Proposed `envelope.h` API

```c
/* src/envelope.h */
#ifndef ENVELOPE_H
#define ENVELOPE_H

#include <stdint.h>
#include "ringbuf.h"

typedef struct {
    float         hp_alpha;
    float         prev_input;
    float         prev_output;
    ring_buffer_t rect_buf;
    uint16_t      window;
    uint8_t       primed;
} envelope_t;

/* Configure one channel.
 * Computes hp_alpha from sample_rate_hz and cutoff_hz.
 * window must be <= WINDOW_SIZE. */
void    envelope_init(envelope_t *e,
                      float    sample_rate_hz,
                      float    cutoff_hz,
                      uint16_t window);

/* Feed one raw ADC sample; returns the current envelope value. */
int32_t envelope_update(envelope_t *e, int32_t raw_sample);

/* Clear filter state without re-configuring. Useful for tests. */
void    envelope_reset(envelope_t *e);

#endif /* ENVELOPE_H */
```

Example use:

```c
static envelope_t flexor, extensor;

envelope_init(&flexor,   1000.0f, 20.0f, 100);
envelope_init(&extensor, 1000.0f, 20.0f, 100);

int32_t f_env = envelope_update(&flexor,   flexor_sample);
int32_t e_env = envelope_update(&extensor, extensor_sample);
```

### Deviation from the brief's proposed API — justification

The brief proposed `void envelope_init(envelope_t *envelope);` and asked that it be changed only for a strong engineering reason. I believe there is one. A no-argument init forces `f_s`, `f_c`, and `N` to be compile-time constants, which causes four concrete problems:

1. **Tuning becomes painful.** You will sweep `f_c` and `N` experimentally. Recompiling with different `#define` values for every trial is far worse than passing arguments.
2. **The test harness cannot compare configurations.** Several parameter sets in one binary is essential for the golden-file testing in §11 — impossible with compile-time constants.
3. **Channels cannot differ.** Flexor and extensor may need different settings during tuning.
4. **It removes the §3 coupling hazard.** If the ADS1299 is reconfigured from 1000 to 2000 SPS, a parameterised init recomputes α automatically. A hardcoded α would silently become wrong, with no compiler warning and no runtime error — only a subtly incorrect cutoff.

`envelope_reset()` is a minor addition that makes deterministic unit tests straightforward.

### File split

| File | Contents |
|---|---|
| `src/envelope.h` | `envelope_t`, three prototypes |
| `src/envelope.c` | `envelope_init`, `envelope_update`, `envelope_reset` |

Both are **platform-independent**: no hardware headers, no `malloc`, no I/O, no STM32 references. They compile identically on the PC and on the target.

---

## 11. Test strategy

Test the high-pass and the smoothing **separately before testing them together.** Combined-only testing makes it impossible to tell which stage is misbehaving.

### A. High-pass unit tests

| # | Input | Expected | Proves |
|---|---|---|---|
| 1 | Constant DC | Output decays toward 0 | DC rejection works |
| 2 | Step | Transient, then decay | Correct first-order behaviour |
| 3 | 0.5 Hz sine (drift proxy) | Heavily attenuated | Drift rejection |
| 4 | **50 Hz sine** | **Passes largely unattenuated** | Makes the hum behaviour explicit rather than a later surprise |

Test 4 exists specifically to prevent a false bug report at integration time.

### B. Smoothing unit tests

| # | Input | Expected | Proves |
|---|---|---|---|
| 5 | Constant amplitude | Envelope settles to a stable value | Averaging is correct |
| 6 | Step change in amplitude | Rise time ≈ window length (~100 ms) | Latency matches design |

### C. End-to-end tests with synthetic signals

| # | Input | Expected | Proves |
|---|---|---|---|
| 7 | Quiet segment (low-amplitude noise) | Low, stable envelope | Baseline behaviour |
| 8 | Contraction segment (larger noise) | Envelope rises clearly and settles | Core function |
| 9 | Large DC offset added throughout | Envelope **unchanged** | High-pass is doing its job |
| 10 | Slow ramp (drift) added | Envelope **unchanged** | Drift rejection end-to-end |
| 11 | 50 Hz hum added | Measure the rise in resting baseline | **Feeds directly into Stage 2 thresholds** |

Test 11 produces a number MOVE actually needs, not just a pass/fail.

### D. Method

- Generate synthetic signals in C or Python; write input and output to CSV.
- Plot both together to inspect behaviour visually.
- Keep known-good outputs as **golden files** and re-run the full set after every change.

This regression suite is precisely what the PC-first architecture was designed to produce, and it remains valid after porting to the STM32 because `src/` is unchanged by the port.

---

## 12. Risks / things to verify against the actual ADS1299 configuration

| # | Risk | Why it matters | Mitigation |
|---|---|---|---|
| 1 | **Firmware `f_s` ≠ configured `DR`** | α and the window's real duration both scale with `f_s`. A mismatch silently yields a wrong cutoff and wrong latency — no error, no warning, just subtly wrong output. | Read CONFIG1 back over SPI at startup, derive `f_s`, pass it to `envelope_init()`. |
| 2 | **Sinc³ bandwidth assumption** (~0.262 × f_DR) | The entire sampling-rate argument in §3 rests on it. | Confirm against the filter-response figures in SBAS499 before finalising. |
| 3 | **24-bit → 32-bit sign extension** | **[FACT]** Output is 24-bit two's complement, MSB first. Assembling three bytes without extending the sign turns every negative sample into a large positive one. The envelope will still look plausible — that is what makes this dangerous. | Unit-test the byte-assembly routine with known negative values before trusting any data. |
| 4 | **PGA gain vs electrode offset** | **[FACT]** The ADS1299 is DC-coupled with no input high-pass. Electrode half-cell offsets at high gain can saturate the PGA — and no amount of downstream filtering recovers a saturated input. | Start at a modest gain; check for clipping with real electrodes; raise gain only once headroom is confirmed. |
| 5 | **Unknown real signal amplitude** | Every Stage 2 threshold depends on it. | Treat all thresholds as provisional until measured on a real arm. |
| 6 | **Residual 50 Hz raising the envelope baseline** | Shifts the resting level that Stage 2 compares against (see §4). | Measure the resting baseline with electrodes attached; add a notch filter only if it proves significant. |
| 7 | **Latency target is assumed, not specified** | The N = 100 choice rests on it (§7). | Confirm an acceptable end-to-end delay for MOVE, then re-check N. |

---

## Open questions for the team

1. Is **1000 SPS** acceptable, or does MOVE want the full 20–500 Hz band (implying 2000 SPS)?
2. Is the **parameterised `envelope_init()`** (§10) accepted, or should the no-argument signature be preserved?
3. Is there a **latency budget** MOVE wants to commit to, so N can be justified against a real requirement rather than an assumption?

---

**Status:** decisions made and justified; `envelope.c` implementation deliberately not written, per the brief.
