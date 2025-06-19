# BMW e90 CAN bus cluster project

This is a work-in-progress BMW e90 CAN bus project to connect a real car instrument cluster to a personal computer using a microcontroller. Currently I'm using _mbed LPC1768_ but the code can be ported to other platforms. [Serial CAN bus adapter](https://docs.longan-labs.cc/1030001/) is used between the microcontroller's UART port and the cluster's CAN port.

__Projects like these are floating all around the internet. This project focuses on documenting things [mostly in code] that were hard to come by. It also goes a bit further than many similar projects. Please feel free to contribute!__

__NOTE:__ _The cluster will set "tampering dot" to the bottom part of the screen when played around with. It's not adviceable to do this project on a cluster which you still plan to use in a car!_

![Highlight image](./media/highlight.jpg)

The cluster I have is a km/h model from a car __with an automatic gearbox__. It is handy because it can show the gear selection (P, R, N, D) and manual mode (M1, M2...) as well as a "Sport" mode.

## Current capabilities

The code is able to control following things on the cluster

- Speedometer
    - ODO / trip (automatic based on speed)
    - This needed to be "calibrated", see the code
- RPM
- Indicators
- Backlight
- Light symbols (high beams, fog lights front/back)
- Fuel gauge
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
    - ... and more can (and will) be added
- Water temperature
    - Visible in debug menu only ([instructions](https://www.youtube.com/watch?v=7exeRgWtkt4&ab_channel=BossM5))
- Oil level
    - Fixed value for now, but could be made dynamic
- Time and date
- Suppress warning symbols
    - ABS
    - SOS call system
    - Steering wheel
    - Airbag

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
- __BC buttons__: Cluster menus can be navigated by connecting pin 16 to ground with 3 buttons via resistors. The values should be 1k (Enter), 2k (Up), 3k (Down) Ohm to ground

Remember to connect the cluster GND, microcontroller GND and Serial CAN bus adapter GND together.

### Power

The cluster needs 12V power supply. 12V wall adapter can be used, but you need to make sure it's regulated. If you are unsure you should check that the voltage is roughly 12V with a multimeter when there is no load to avoid overvoltage.

## The API

The cluster can be controlled over the virtual serial port using a simple text-based API. Please note that the example has spaces for readability, but the actual messages should not have spaces. The baud rate is __921600__.

`S 20250619164530 02350 0853 3 095 0734 TFFTFTFTFFT 0087 0000T M TFTF 0680 1 \n`

Breakdown:

    Start of the message: S

    Timestamp: 2025-06-19 16:45:30

    RPM: 2350

    Speed: 0853 = 85.3 km/h

    Gear: 3 (2nd gear), Reverse is 0, Neutral is 1

    Engine Temp: 95°C

    Fuel: 0734 = 73.4% (0-1000)

    Lights: TFFTFTFTFFT
        - T = on, F = off
        - 1st light: Shift indicator
        - 2nd light: High beams
        - 3rd light: Handbrake
        - 4th light: Traction control
        - 5th light: Left turn signal
        - 6th light: Right turn signal
        - 7th light: Oil warning
        - 8th light: Battery warning
        - 9th light: ABS
        - 10th light: High engine temperature, yellow
        - 11th light: High clutch temperature, red

    Fuek injection: 0087 = 8.7ul per 100 ms

    Custom light: 0004T (Show symbol #4, 0000F to disable)

    Gear extension: M (M = semi-automatic, S = sport mode, P = park, A = automatic, C = common

    Extra lights: TFTF
        - 1st: Low beam headlights (backlight)
        - 2nd: ESC (Electronic Stability Control)
        - 3rd: Check engine light
        - 4th: Clutch temperature warning

    Cruise speed: 0680 = 68.0 km/h

    Cruise mode: 1 (on), 0 (off)

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

## Notes and findings

- There's a Discord community around hacking the clusters with lots of knowledge and information
    - [Arduino-Tacho Gang](https://discord.gg/UQFsS9D6kq)
- Lights on the cluster (like Check Engine, DTC, Oil Pressure) can be controlled with CAN ID `0x592`. See `canSendErrorLight` and codes in [symbol document](./external/E92%20checkcontrol%20symbols.pdf)
