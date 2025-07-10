# BMW e90 cluster Arduino project

This is a BMW e90 CAN bus project to connect a real car instrument cluster to a personal computer using an Arduino microcontroller. The cluster can then be used in driving games. It is demonstrably easy to port the code to other similar platforms (see notes about `mbed`). [Serial CAN bus adapter](https://docs.longan-labs.cc/1030001/) is used between the microcontroller's UART port and the cluster's CAN port.

__Projects like these are floating all around the internet. This project focuses on documenting things [mostly in code] that were hard to come by. It also goes a bit further than many similar projects. Please feel free to contribute!__

__NOTE:__ _The cluster will set a "tampering dot" to the bottom part of the screen when played around with. It's not adviceable to do this project on a cluster which you still plan to use in a car!_

![Highlight image](./media/highlight.jpg)

The cluster I have is a km/h model from a car __with an automatic gearbox__. It is handy because it can show the gear selection (P, R, N, D) and manual mode (M1, M2...) as well as a "Sport" mode.

## The setup

The setup is a bit convoluted but currently it consists of the following parts:
- [BMW e90 CAN bus BeamNG protocol](https://github.com/veikkos/e90-can-cluster-beamng-protocol)
    - This BeamNG plugin provides the game telemetry via UDP socket
- [BMW e90 CAN bus cluster proxy](https://github.com/veikkos/e90-can-cluster-proxy)
    - The Node.js proxy receives the telemetry and sends it to the microcontroller over a (virtual USB) serial port
    - The proxy supports **BeamNG**, **Euro Truck Simulator 2** and **American Truck Simulator**
    - It could be possible to get rid of this proxy and send the telemetry directly to the microcontroller if the microcontroller supports networking. This is considered in the future.
- This repository is the microcontroller Arduino firmware that receives the telemetry from the proxy and sends it to the cluster over CAN bus

## Current capabilities

The code is able to control following things on the cluster

- Speedometer
    - This needed to be "calibrated", see the code
- RPM
- Indicators
- Backlight
- Indirectly controlled by the cluster
    - Odometer (mileage)
    - Trip meter
    - Average speed
    - Average fuel consumption
    - Range
- Light symbols (high beams, fog lights front/back)
- Fuel gauge
    - Relatively well calibrated
    - Low fuel warning is automatic based on the level
- Current fuel consumption needle
- Handbrake
- Gear selection (automatic gearbox cluster!)
    - Current selection (P, R, N, D)
    - Manual mode (M1, M2...)
    - "Sport" mode
- Cruise control
- Warnings ([see also](#notes-and-findings))
    - Check engine
    - Battery light
    - Low oil pressure
    - Traction control
    - Stability control
    - High or very high engine temperature
    - High clutch temperature
    - High brake temperature
    - Tire deflated (individual, all)
    - Radiator warning
    - Doors open while on gear
    - Tailgate open
    - ... and more can be added
- Water temperature
    - Visible in debug menu only ([instructions](https://www.youtube.com/watch?v=7exeRgWtkt4&ab_channel=BossM5))
- Oil level
    - Fixed value for now, but could be made dynamic
- Time and date
- Suppress warning symbols
    - ABS
    - SOS call system
    - Steering wheel
    - Airbags

## Pinout

```
               +----------+
               |  1 | 10  |
               |  2 | 11  |
               |  3 | 12  |
 Temperature E |  4 | 13  |
 Temperature M |  5 | 14  |
         CAN H |  6 | 15  |
         CAN L |  7 | 16  | BC buttons
               |  8 | 17  |
           12V |  9 | 18  | GND
               +----------+
```

__Tip:__ There are faint numbers on the cluster port marking the pin numbers. Look closely!

- __Temperature__: Outside temperature is measured with an external sensor of resistive type between pins 4 and 5. A resistor of 10k Ohm can be used to show approximately 10'C which removes the cold weather warning
- __BC buttons__: Cluster menus can be navigated by connecting pin 16 to ground with 3 buttons via resistors. The values should be 1k (Enter), 2k (Up), 3k (Down) Ohm

Remember to connect the cluster GND, microcontroller GND and Serial CAN bus adapter GND together.

### Power

The cluster needs 12V power supply. 12V wall adapter can be used, but you need to make sure it's regulated. If you are unsure you should check that the voltage is roughly 12V with a multimeter when there is no load to avoid overvoltage.

## The API

The cluster is controlled over a virtual serial port using a compact **binary protocol**. The baud rate is **921600**. Framed starts with (`'S'`) and ends with (`'E'`) marker bytes.

### Frame Structure (Little Endian)

| Offset | Size | Field               | Description                          |
|--------|------|---------------------|--------------------------------------|
| 0      | 1    | `'S'`               | Start marker                         |
| 1      | 1    | `year`              | e.g., 25 = 2025                      |
| 2      | 1    | `month`             | 1–12                                 |
| 3      | 1    | `day`               | 1–31                                 |
| 4      | 1    | `hour`              | 0–23                                 |
| 5      | 1    | `minute`            | 0–59                                 |
| 6      | 1    | `second`            | 0–59                                 |
| 7      | 2    | `rpm`               | 0–65535                              |
| 9      | 2    | `speed`             | km/h × 10 (e.g. 853 = 85.3 km/h)     |
| 11     | 1    | `gear`              | 0 = R, 1 = N, 2+ = forward gears     |
| 12     | 1    | `engine temp`       | °C                                   |
| 13     | 2    | `fuel`              | 0–1000 (% × 10)                      |
| 15     | 4    | `showlights`        | Bitfield of all light states (see table below) |
| 19     | 2    | `fuel injection`    | microliters per 100 ms               |
| 21     | 2    | `custom light`      | Symbol ID (0–65535)                  |
| 23     | 1    | `custom light on`   | 1 = show, 0 = off                    |
| 24     | 1    | `gear extension`    | ASCII char: M = semi-automatic, S = sport mode, P = park, A = automatic, N = none |
| 25     | 2    | `cruise speed`      | km/h × 10                            |
| 27     | 1    | `cruise enabled`    | 1 = on, 0 = off                      |
| 28     | 1    | `ignition`          | 3 = starter, 2 = on, 1 = accessory only, 0 = off |
| 29     | 1    | `engine running`    | 1 = on, 0 = off                      |
| 30     | 1    | `checksum`          | Additive checksum of all previous bytes excluding start marker |

### `showlights` Breakdown

```
Bit  0 : DL_SHIFT         (Shift light) UNUSED
Bit  1 : DL_FULLBEAM      (Full beam headlights)
Bit  2 : DL_HANDBRAKE     (Handbrake engaged)
Bit  4 : DL_TC            (Traction control active)
Bit  5 : DL_SIGNAL_L      (Left turn signal)
Bit  6 : DL_SIGNAL_R      (Right turn signal)
Bit  8 : DL_OILWARN       (Oil pressure warning)
Bit  9 : DL_BATTERY       (Battery warning) UNUSED
Bit 10 : DL_ABS           (ABS active/disabled) UNUSED
Bit 11 : DL_BEACON        (Truck beacon light is on)
Bit 12 : DL_LOWBEAM       (Low beam headlights)
Bit 13 : DL_ESC           (ESC active)
Bit 14 : DL_CHECKENGINE   (Check engine light)
Bit 15 : DL_CLUTCHTEMP    (Clutch temp warning)
Bit 16 : DL_FOGLIGHTS     (Fog lights on)
Bit 17 : DL_BRAKETEMP     (High brake temperature)
Bit 18 : DL_TIREFLAT_FL   (Front left tire deflated)
Bit 19 : DL_TIREFLAT_FR   (Front right tire deflated)
Bit 20 : DL_TIREFLAT_RL   (Rear left tire deflated)
Bit 21 : DL_TIREFLAT_RR   (Rear right tire deflated)
Bit 22 : DL_RADIATOR      (Radiator warning)
Bit 23 : DL_ENGINETEMP_Y  (Engine temp yellow)
Bit 24 : DL_ENGINETEMP_R  (Engine temp red)
Bit 25 : DL_DOOROPEN_FL   (Front left door open)
Bit 26 : DL_DOOROPEN_FR   (Front right door open)
Bit 27 : DL_DOOROPEN_RL   (Rear left door open)
Bit 28 : DL_DOOROPEN_RR   (Rear right door open)
Bit 29 : DL_TAILGATEOPEN  (Tailgate open)
Bit 30 : DL_TC_DISABLED   (Traction control disabled)
Bit 31 : DL_ESC_DISABLED  (ESC disabled)
```

## Serial CAN bus adapter settings

- Serial CAN bus adapter has **persistent** memory for the baud rate and CAN bus speeds. You should only set them once
- Connection should be made RX to TX and TX to RX between the cluster and the Serial CAN bus adapter
- It's best to set the parameters with one-time-use-only code one at the time and have `while(1);` after the set
    - Hook up the UART code so that you can see what the adapter responds. It should say `OK` after you set a value
    - Send `+++` to go to configuration mode
    - The CAN bus towards the cluster should be set to __100 kb/s__ with `AT+C=12`
    - The serial port speed between the microcontroller and the adapter should be set to __115200__ baud with `AT+S=4`. This is the highest speed possible and is needed to be able to send CAN messages fast enough
- There should __NOT__ be 120 Ohm termination in the Serial CAN bus adapter. If it exists, it should be removed
- __The Serial CAN bus adapter can be easily overwhelmed with commands. It seems to work much better having 3 ms between sending frames. See the main loop how this can be achieved without blocking__
- The adapter is picky about the baud rate. Smallest error AT90USB has is +2.1% 115200 and it did not work. When changed to the second closest error -3.5% it started working

## Notes and findings

- There's a Discord community around hacking the clusters with lots of knowledge and information
    - [Arduino-Tacho Gang](https://discord.gg/UQFsS9D6kq)
- Lights on the cluster (like Check Engine, DTC, Oil Pressure) can be controlled with CAN ID `0x592`. See `canSendErrorLight` and codes in [symbol document](./external/E92%20checkcontrol%20symbols.pdf)
- The code was originally implemented for _mbed LPC1768_. You can find the old code from the history with a tag `mbed_last`
- Special credits for material or help to
    - beanseater420
    - peter black
       - [Log](./external/e64_dump_peter_black.trc)
       - [Automatic gear reverse engineering](./external/automatic_gear_peter_black.webp)
    - [HeinrichG-V12](https://github.com/HeinrichG-V12/E65_ReverseEngineering)
    - [Trevor Cook](https://www.loopybunny.co.uk/CarPC/k_can.html)
    - [Marcin648](https://github.com/Marcin648/e90-dashboard-ets2)
