# MOVE Prosthetics — Embedded C Firmware Project Update
## Consolidated changes, decisions, and current state for Claude

**Purpose:** This document consolidates the known changes and decisions made while developing the MOVE Prosthetics embedded C firmware, so Claude can update the project documentation/code plan without losing prior decisions.

**Project:** MOVE Prosthetics  
**Firmware:** `move-emg-firmware` / local folder `CCA01 Firmware`  
**Git remote:** `https://github.com/antosalra/move-emg-firmware.git`

---

# 1. Overall firmware architecture

The project is being developed as a real embedded C firmware stack for a myoelectric prosthetic hand.

The development strategy is:

1. Build and test platform-independent signal-processing code on a Mac/PC.
2. Keep the algorithm layer independent of STM32 hardware.
3. Make the code embedded-safe.
4. Port the hardware-facing layer to STM32.
5. Interface the STM32 with the ADS1299 EMG analog front end.
6. Add motor control and hardware safety logic.

Current conceptual data path:

```text
EMG electrodes
      ↓
ADS1299-4
      ↓ SPI
STM32H753
      ↓
signal processing
      ↓
intent detection
      ↓
motor control
      ↓
prosthetic hand
```

The important architectural boundary is:

```text
src/
    platform-independent algorithms

platform/stm32/
    STM32/ADS1299 hardware-specific code
```

The algorithm code should ultimately be portable to STM32 with minimal or no changes.

---

# 2. Repository structure

Current intended structure:

```text
CCA01 Firmware/
├── platform/
│   ├── pc/
│   └── stm32/
│       ├── hal.c
│       ├── hal.h
│       ├── ads1299.c
│       └── ads1299.h
├── tests/
├── ring_buffer.c
├── ring_buffer.h
├── envelope.c
├── envelope.h
├── intent.c
├── intent.h
├── calibration.c
├── calibration.h
├── main.c
└── ...
```

Important correction already made:

- `stm32/` belongs directly under `platform/`.
- It must **not** be `platform/pc/stm32/`.

---

# 3. Stage 1 — Signal processing

Stage 1 has been implemented and tested.

## Sampling assumptions

- Sampling rate: **1 kHz**
- `dt = 1 ms`

## Signal-processing chain

The intended chain is:

```text
raw EMG
   ↓
high-pass filter
   ↓
rectification
   ↓
moving average / envelope
```

### High-pass filter

Implemented as:

```c
y[n] = alpha * (y[n-1] + x[n] - x[n-1])
```

- Approximate cutoff: **20 Hz**
- `alpha ≈ 0.8884`

### Rectification

The project uses:

```c
sqrtf(y * y)
```

rather than a simple absolute-value operation, because this was an explicit design preference.

### Moving average

- Window: **100 samples**
- At 1 kHz this corresponds to approximately **100 ms**
- Approximate group delay: **50 ms**

### ADS1299 digital filtering context

The intended ADS1299 configuration uses the sinc³ digital filter.

Approximate:

- `-3 dB` point: **262 Hz**

This is compatible with the intended EMG processing bandwidth.

## Ring buffer

- Size: **256 samples**

---

# 4. Stage 2 — Intent detection

Stage 2 design and implementation are complete.

The intent layer receives two envelope values:

```text
flexor_env
extensor_env
```

Both are represented as:

```c
int32_t
```

The intent layer produces a hand-state decision.

Conceptual states:

```text
IDLE
CLOSING
HOLDING
OPENING
```

## Calibration

Calibration is performed separately for each channel.

Approximate procedure:

1. ~3 seconds relaxed
2. Calculate baseline mean
3. ~3 seconds firm contraction
4. Calculate maximum voluntary contraction (MVC) mean
5. Compute threshold

Threshold:

```text
threshold = baseline + k * (mcv - baseline)
```

Current constants:

```text
k_enter = 0.30
k_leave = 0.15
```

The calibration code was separated into:

```text
calibration.c
calibration.h
```

Current validation rule:

- Reject calibration only when `mcv <= baseline`.
- No additional initial minimum/floor was selected.

## Hysteresis

The intent detector uses two thresholds:

```text
HI → enter active state
LO → leave active state
```

This prevents rapid switching around a single threshold.

## Co-contraction

If flexor and extensor are both HIGH:

```text
do not change the current hand state
```

This is treated as intentional co-contraction rather than as an open/close command.

## Intent behavior

### Closing

Active flexor contraction causes:

```text
IDLE → CLOSING
```

When flexor activity relaxes:

```text
CLOSING → HOLDING
```

### Opening

Opening requires an **active extensor contraction**.

Relaxing the flexor does **not** automatically open the hand.

### Direct reversal

A sufficiently active opposite muscle can directly reverse the current intent where allowed by the state machine.

## Dwell time

A minimum dwell time of:

```text
100 ms = 100 samples at 1 kHz
```

is enforced.

Important behavior:

- Dwell time only blocks **leaving the current state**.
- The first response to a new command is not blocked.

---

# 5. Stage 3 — Embedded-safe C

Stage 3 is complete.

The goal was to remove unnecessary assumptions that are problematic on a microcontroller and make execution bounded and deterministic.

## Important ring-buffer API change

The ring-buffer read API is now:

```c
int rb_get(const struct ring_buffer *rb,
           uint16_t age,
           int32_t *sample);
```

The API uses an output pointer for the retrieved sample.

## IMPORTANT — do not change this index calculation

The user explicitly requested that this calculation remain unchanged:

```c
uint16_t index = (rb->head + WINDOW_SIZE - 1 - age) % WINDOW_SIZE;
```

Do not alter the calculation unless the user explicitly asks for it.

## Ring-buffer push

The old modulo-based head increment was replaced with explicit increment + wrap:

```c
rb->head++;

if (rb->head >= WINDOW_SIZE)
    rb->head = 0;
```

This was done as part of the embedded-safety cleanup.

## Envelope changes

The envelope implementation now:

- uses `sqrtf`
- performs explicit float conversions where needed
- has explicit startup state
- uses a `primed` flag
- protects against a zero window

Zero-window behavior:

```c
if (window == 0)
    window = 1;
```

## General Stage 3 goals

The code was reviewed for:

- dynamic memory assumptions
- bounded loops
- integer/float conversions
- explicit state
- deterministic behavior
- clean interfaces
- compiler warnings

---

# 6. Current Stage 3 test status

All tests currently pass.

Current result:

```text
19/19 tests passed
```

Strict compilation command used:

```bash
gcc -Wall -Wextra -Wconversion -Wshadow -I. main.c ring_buffer.c envelope.c intent.c calibration.c tests/test_grip.c -o test
```

Then:

```bash
./test
```

Result:

```text
All 19 passed.
```

Tests cover, among other things:

- grip persistence
- full open/close cycle
- hysteresis
- chatter rejection
- co-contraction
- direct reversal
- dwell enforcement
- bad calibration
- asymmetric channels
- calibration
- calibration-to-intent behavior
- zero window
- empty ring-buffer read
- ring-buffer wraparound
- envelope reset
- envelope startup
- empty calibration
- invalid ring-buffer age
- invalid calibration order

Strict compiler warnings were clean.

---

# 7. Git checkpoint

Stage 3 was committed and pushed.

Commit:

```text
2f5c070
```

Commit message:

```text
Adjustment for Embedded Hardware
```

The commit was pushed successfully to `main`.

Remote:

```text
https://github.com/antosalra/move-emg-firmware.git
```

---

# 8. Stage 4 — STM32 hardware layer

Stage 4 has started.

The purpose is to move from platform-independent firmware to actual hardware.

The first hardware target is:

```text
STM32H753
```

Development board:

```text
NUCLEO-H753ZI
```

The user is new to STM32 development, so explanations should be explicit rather than assuming familiarity with STM32 peripherals, HAL, SPI, registers, interrupts, or ADS1299 commands.

---

# 9. HAL decision

A Hardware Abstraction Layer was introduced.

Files:

```text
platform/stm32/hal.h
platform/stm32/hal.c
```

Current starter interface:

```c
#ifndef HAL_H
#define HAL_H

#include <stdint.h>

void hal_init(void);
void hal_delay_ms(uint32_t ms);

#endif
```

Current implementation is intentionally a placeholder:

```c
#include "hal.h"

void hal_init(void)
{
    /* STM32 hardware initialization will go here. */
}

void hal_delay_ms(uint32_t ms)
{
    /* STM32 delay implementation will go here. */
    (void)ms;
}
```

The HAL exists because:

```text
intent.c
```

should decide what the hand wants to do, while:

```text
hal.c
```

should handle interaction with physical STM32 hardware.

---

# 10. ADS1299 driver files

Created:

```text
platform/stm32/ads1299.h
platform/stm32/ads1299.c
```

Current header:

```c
#ifndef ADS1299_H
#define ADS1299_H

#include <stdint.h>

void ads1299_init(void);
uint8_t ads1299_read_id(void);

#endif
```

Current source:

```c
#include "ads1299.h"

void ads1299_init(void)
{
    /* ADS1299 initialization will go here. */
}

uint8_t ads1299_read_id(void)
{
    /* SPI communication will go here. */
    return 0;
}
```

These are placeholders only. Actual SPI implementation has not yet been completed.

---

# 11. ADS1299 architecture decisions

The selected AFE is:

```text
ADS1299-4
```

Purchased part:

```text
ADS1299-4PAGR
```

This is a **bare TQFP-64 IC**, not a development module.

Relevant architecture:

- 4 channels
- 24-bit ADC
- biopotential acquisition
- SPI interface
- programmable gain
- integrated bias-drive functionality
- intended for ECG/EEG/EMG-type biopotential acquisition

For the current prosthetic design:

- Two differential EMG channels are planned:
  - flexor
  - extensor
- A driven bias electrode is planned.

The ADS1299 therefore provides more channels than the immediate two-channel intent detector requires.

---

# 12. ADS1299 ↔ STM32 communication

The selected digital interface is SPI.

Core SPI signals:

```text
STM32 → ADS1299
SCLK
MOSI
CS

ADS1299 → STM32
MISO
DRDY
```

`DRDY` is a separate data-ready signal from the ADS1299 to the STM32.

The intended architecture is:

```text
ADS1299
    │
    ├── SPI → STM32
    │
    └── DRDY → STM32 interrupt/input
```

---

# 13. First ADS1299 hardware milestone

Do **not** begin with EMG signal acquisition.

First prove that the STM32 can communicate with the ADS1299.

The first milestone is:

```text
Read ADS1299 device ID register successfully.
```

Conceptual flow:

```text
STM32
  │
  │ SPI command
  ↓
ADS1299
  │
  │ ID register
  ↓
STM32
```

Only after this works should we proceed to configuration and actual EMG acquisition.

---

# 14. ADS1299 register/command decision

The ADS1299 uses internal registers for configuration and status.

The read-register command (`RREG`) is a multi-byte SPI command.

For reading register 0:

```text
first command byte = 0x20
second command byte = 0x00
```

The second byte specifies that one register is being read.

Important:

- CS should remain asserted for the entire multi-byte transaction.
- If the ADS1299 is in RDATAC mode, `SDATAC` must be issued before register access.

Exact SPI timing and STM32 peripheral configuration are to be implemented later with the real Nucleo hardware.

---

# 15. Bitwise C learning requirement

Before implementing the ADS1299 command construction, the user wants to learn the C bitwise operations involved.

The immediate next lesson/task should explain:

```text
0x20
```

and how it represents:

```text
0010 0000
```

and how ADS1299 command bytes can be constructed with operations such as:

```c
<<
>>
&
|
```

The user is learning embedded C and has not previously worked with STM32/ADS1299, so do not simply provide unexplained SPI driver code.

Teach the operation first, then implement it incrementally.

---

# 16. Planned ADS1299 firmware architecture

The planned progression is:

### Step 1
STM32 project / HAL initialization.

### Step 2
Configure STM32 SPI.

### Step 3
Control ADS1299 CS.

### Step 4
Send ADS1299 commands.

### Step 5
Read ID register.

### Step 6
Configure ADS1299 registers.

### Step 7
Start conversion.

### Step 8
Use `DRDY` to detect new samples.

### Step 9
Read sample data over SPI.

### Step 10
Push samples into the ring buffer.

### Step 11
Run the existing signal-processing pipeline.

---

# 17. DRDY interrupt architecture decision

The intended real-time architecture is:

```text
ADS1299 DRDY
      ↓
STM32 interrupt
      ↓
read sample
      ↓
push into ring buffer
      ↓
set flag
      ↓
return from ISR
```

The interrupt service routine should remain minimal.

Signal processing and intent decisions should occur outside the ISR, in the main loop or appropriate task context.

Reason:

- keep interrupt latency short
- avoid long computations in ISR
- preserve deterministic real-time behavior
- separate acquisition from processing

---

# 18. Hardware procurement decisions

## STM32 board

Selected:

```text
ST NUCLEO-H753ZI
```

Mouser part:

```text
511-NUCLEO-H753ZI
```

Quantity:

```text
1
```

The user has ordered this board.

Expected arrival was approximately three days from the time of the decision.

---

## ADS1299

Purchased:

```text
2 × ADS1299-4PAGR
```

Mouser price at the time of purchase:

```text
~€38.68 each
```

One is enough for the design; the second is a spare.

Important:

These are **bare ICs**.

They cannot simply be plugged into a breadboard.

---

## LDO

Purchased:

```text
2 × TPS7A4901DGNT
```

Mouser part:

```text
595-TPS7A4901DGNT
```

This is intended as the low-noise regulator for the ADS1299 analog supply.

Conceptual power architecture:

```text
battery
   ↓
low-noise regulator
   ↓
~5 V analog supply
   ↓
ADS1299 analog supply
```

The TPS7A4901 is a bare IC and requires its external capacitors/resistors according to the final regulator design.

Do not assume the LDO is a ready-made power module.

---

## AP2112K

Purchased:

```text
2 × AP2112K-3.3TRG1
```

This is a 3.3 V LDO.

Current decision:

**Do not automatically use it in the final design.**

The Nucleo already provides 3.3 V, so the need for a separate digital regulator must be determined when the complete power architecture is designed.

Treat it as optional/deferred.

---

# 19. IMPORTANT latest hardware decision — ADS1299 breakout board

The user initially realized that the bare ADS1299 IC requires a practical prototyping interface.

After considering commercial options, the decision is:

## Do NOT buy an expensive ADS1299 evaluation kit/breakout yet.

Specifically:

- Do not purchase the ~€200+ TI ADS1299 evaluation kit merely for initial development.
- Do not rely on an expensive OpenBCI-style board because it introduces its own MCU/system architecture.
- Do not buy an unverified random ADS1299 module simply because it is available.

Instead:

## Design a small ADS1299 carrier/prototype PCB ourselves.

The purpose is to provide:

- ADS1299 footprint
- power connections
- decoupling
- reference circuitry
- required support components
- SPI header/interface
- DRDY
- reset/control connections as required
- electrode connectors
- appropriate analog routing

This will allow:

```text
ADS1299 carrier
       ↕ SPI
NUCLEO-H753ZI
```

and later the same knowledge can be incorporated into MOVE's custom PCB.

---

# 20. IMPORTANT — do not buy more components yet

Before ordering additional hardware:

1. Derive the exact ADS1299 schematic from the datasheet.
2. Derive the TPS7A4901 power circuit.
3. Determine the exact AP2112K requirement.
4. Determine all required capacitors/resistors.
5. Determine connectors.
6. Determine STM32/Nucleo header/interface wiring.
7. Determine SPI and DRDY pin assignments.
8. Then generate the exact BOM.

The goal is to avoid buying components blindly.

---

# 21. Passive-component decisions

The user has discussed/bought assorted passives.

Available resistor assortment:

- 0603 SMD
- approximately 45 values
- approximately 1/10 W
- approximately 75 V
- values include common pull-up, series, bias and divider values
- ±1% tolerance is preferred/expected

A separate set of:

```text
33 Ω ±1%
1/2 W
through-hole
```

was also purchased.

Decision:

- 33 Ω is electrically suitable for SPI series damping if needed.
- It is unnecessarily large physically for the eventual PCB.
- The 0603 assortment already contains 33 Ω, so the SMD part is preferred for the final PCB.
- The through-hole 33 Ω resistors can still be useful for prototyping.
- For a normal 3.3 V indicator LED, ~330 Ω is a more appropriate starting point than 33 Ω.

Exact passive values for the ADS1299 board must be determined from the final schematic rather than assumed.

---

# 22. LED decision

A prewired 0603 LED pack was considered.

Decision:

- Optional.
- Not necessary for first hardware communication testing.
- The Nucleo already has status LEDs.
- If external LEDs are used, verify whether the purchased prewired LEDs already contain current-limiting resistors.

---

# 23. Skin preparation decision

A standardized skin-preparation process is required for EMG testing.

The user found:

```text
OneStep EEG-Gel
```

with fine pumice particles and cleaning/roughening properties.

Decision:

- This is a plausible inexpensive substitute for a specialized abrasive skin-preparation product, provided the manufacturer's intended use does not restrict it away from EMG/EKG-type electrode preparation.
- A conductive electrode gel such as TensCare Go Gel is **not** the same thing as an abrasive skin-preparation product.

For testing, use a consistent skin-preparation protocol and record it.

---

# 24. ESD and cleaning

The user's eLab provides:

- ESD grounding/wristband
- cleaning supplies / IPA

Decision:

- No separate ESD mat is required if the lab provides the required ESD setup.
- Do not improvise protective-earth connections at home.
- ESD ground and circuit ground are conceptually separate.

---

# 25. Mechanical prototype decisions

The motor selected for the prosthetic prototype is:

```text
Pololu #4785
15.25:1 Micro Metal Gearmotor HP 6V
extended shaft
```

Relevant motor characteristics discussed:

- 3 mm D-shaped output shaft
- ~2000 rpm no-load
- ~100 mA no-load
- ~1.6 A stall current
- ~0.30 kg·cm stall torque
- 10 × 12 × 25 mm motor body
- extended rear shaft is for encoder mounting

The mechanical element previously called a shaft collar was clarified to actually be a:

```text
tendon spool / cable drum / capstan
```

The spool must be designed around the motor's:

```text
3 mm D-shaped shaft
```

Force relationship:

```text
F = torque / radius
```

Smaller spool radius gives higher tendon force.

---

# 26. Motor encoder decision

Pololu magnetic encoder kits were considered.

Current candidates:

```text
Pololu #4760 — top-entry
Pololu #4761 — side-entry
```

The encoder kits provide approximately:

```text
12 CPR
```

and are intended for the corresponding Micro Metal Gearmotor geometry.

One encoder per motor is planned.

---

# 27. Motor driver caution

The selected motor can reach approximately:

```text
1.6 A stall current
```

The motor-driver choice therefore must be checked against actual stall/current requirements.

Do not assume a small motor driver is automatically adequate.

The DRV8838 was discussed, but its suitability for the actual motor current requirements must be verified before committing to it.

---

# 28. Tendon and Bowden tube

Tendon candidate:

```text
Dingbear braided fishing line
PE/Dyneema-type microfibers
```

A ~40 lb / ~0.28 mm line was considered suitable.

PTFE Bowden tube:

```text
~2 mm ID × 4 mm OD
```

Approximately 4 m was considered a useful amount for prototyping.

Fusion/CAD starting dimensions:

- ~4.1 mm printed hole for a 4 mm OD PTFE tube
- approximately 1 mm hole as a starting point if the tendon itself passes directly through a printed feature

These dimensions should be treated as starting points and adjusted for actual print tolerances.

---

# 29. Springs

Do not buy only four springs and assume that is sufficient.

Preferred approach:

```text
multiple candidate spring rates
```

For example:

```text
4 springs × 4 different rates = 16 springs
```

Then empirically determine the appropriate rate.

---

# 30. Fasteners / printed mechanics

M2/M3 screws and heat-set inserts are appropriate for:

- printed fingers
- joints
- motor mounts
- structural printed parts

Large quantities are acceptable if inexpensive, but exact requirements depend on the final mechanical design.

---

# 31. Wiring decisions

For early prototyping:

```text
22 AWG silicone stranded wire
```

can be used for general wiring and motor power.

For the final PCB:

- thinner wires are preferable where appropriate.
- SPI wiring does not need 22 AWG.
- Electrode leads should use suitable low-noise electrode cable/wiring.

A 28 AWG purchase was considered optional and can be omitted for simplicity.

---

# 32. Connectors / parts explicitly deferred

The following were discussed and postponed because they are not necessary for the first prototype or the interface is not yet finalized:

- DIN 42802-1 1.5 mm touchproof panel sockets
- generic test-point loops
- IDC ribbon cable
- 2×10 IDC sockets

Do not add these to the BOM until the actual interface requires them.

Electrical tape is not considered a substitute for an electrical connector.

---

# 33. Shipping decision

Mouser shipment is via:

```text
FedEx International Priority
```

Therefore:

- Do not address the order to a DHL Postfiliale.
- Use FedEx Delivery Manager once tracking is available.
- If eligible, redirect to a FedEx pickup/hold location.

This is logistics information and does not affect firmware architecture.

---

# 34. 3D printing / CAD decisions

The user is using:

```text
Fusion 360
Cura
```

For exporting multiple Fusion components:

- Use Save as Mesh / appropriate mesh export.
- Separate files can be generated for separate components.

Cura:

If Cura warns about:

```text
Unused Extruder(s), Extruder 2
```

for a single-extruder printer:

```text
Disable unused extruder(s)
```

---

# 35. Current project state

At the moment:

### Firmware

```text
Stage 1 — COMPLETE
Stage 2 — COMPLETE
Stage 3 — COMPLETE
Stage 4A — STARTED
```

### Tests

```text
19/19 PASS
```

### STM32

```text
NUCLEO-H753ZI ordered
```

### ADS1299

```text
2 × ADS1299-4PAGR ordered
```

### Power

```text
2 × TPS7A4901DGNT ordered
2 × AP2112K-3.3TRG1 ordered
```

### ADS1299 carrier

```text
NOT YET DESIGNED
```

### Next immediate firmware learning step

```text
C bitwise operations
        ↓
ADS1299 RREG command construction
        ↓
STM32 SPI implementation
        ↓
read ADS1299 ID
```

---

# 36. Immediate next steps

Do these in this order.

## Step A — Learn bitwise operations

Explain:

```c
uint8_t command = 0x20;
```

and:

```text
0x20 = 0010 0000
```

Then explain:

```c
<<
>>
&
|
```

using simple binary examples.

Then construct the ADS1299 RREG command.

## Step B — STM32 SPI fundamentals

Explain:

- what SPI is
- master/slave
- SCLK
- MOSI
- MISO
- CS
- SPI modes
- why ADS1299 timing matters

## Step C — Configure NUCLEO-H753ZI

Once the board arrives:

- create/configure STM32 project
- configure SPI peripheral
- configure GPIO for CS
- configure GPIO/input or interrupt for DRDY
- configure reset/control pins as required

## Step D — Implement ADS1299 register access

Build:

```c
ads1299_write_register(...)
ads1299_read_register(...)
```

and related command functions.

## Step E — Read ID

First hardware proof:

```text
STM32 successfully reads ADS1299 ID.
```

## Step F — Configure acquisition

Only after ID communication works:

- reset
- stop continuous data mode when necessary
- configure channels
- configure PGA
- configure data rate
- configure bias
- configure reference
- start conversion

## Step G — Acquire EMG samples

Use:

```text
DRDY interrupt
    ↓
minimal sample acquisition
    ↓
ring buffer
    ↓
signal processing
```

---

# 37. Rules for future project changes

When modifying the firmware:

1. Preserve the separation between algorithm code and hardware-specific code.
2. Keep `src`/algorithm behavior platform-independent.
3. Keep STM32-specific code under `platform/stm32/`.
4. Do not put STM32-specific logic into `intent.c`.
5. Do not change the explicitly protected ring-buffer index calculation without asking.
6. Keep interrupt routines minimal.
7. Prefer deterministic, bounded execution.
8. Avoid dynamic memory unless there is a specific reason.
9. Keep strict compiler warnings clean.
10. Add tests when changing algorithm behavior.
11. Explain STM32/ADS1299 concepts because the user is learning them for the first time.
12. Do not dump large unexplained hardware-driver implementations before teaching the underlying concepts.
13. Do not order additional hardware until the ADS1299 schematic/BOM has been derived.
14. Distinguish clearly between:
    - implemented
    - planned
    - optional
    - deferred
    - experimentally chosen

---

# 38. One-sentence project status

**MOVE's PC-tested, embedded-safe EMG signal-processing and intent-detection firmware is complete through Stage 3 with 19/19 tests passing; Stage 4A has begun with a NUCLEO-H753ZI and bare ADS1299-4PAGR parts ordered, and the next step is to learn bitwise C and implement STM32 SPI communication, while designing a small ADS1299 carrier PCB rather than buying an expensive commercial evaluation board.**
