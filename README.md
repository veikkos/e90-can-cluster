# BMW e90 CAN bus cluster project

__Projects like these are floating all around the internet. This project focuses on documenting things that were hard to come by. Please feel free to contribute!__

This is a work-in-progress BMW e90 CAN bus cluster project to connect a real car instrument cluster to a computer using a microcontroller. Currently I'm using _mbed LPC1768_. [Serial CAN bus adapter](https://docs.longan-labs.cc/1030001/) is used between the microcontroller's UART port and the cluster's CAN port.

_The cluster will set "tampering dot" to the bottom part of the screen when played around with. It's not adviceable to do this project on a cluster which you still plan to use in a car!_

The cluster I have is a km/h model from a car __with automatic gearbox__.

![Highlight image](./media/highlight.jpg)

## Current capabilities

The code is able to control following things on the cluster

- Speedometer
    - ODO / trip (automatic based on speed)
- RPM
- Indicators
- Backlight
- Light symbols (high beams, fog lights front/back)
- Fuel gauge
    - Low fuel warning is automatic based on the level
- Handbrake
- Gear selection (automatic gearbox cluster!)
    - Current selection (P, R, N, D)
    - Manual mode (M1, M2...)
    - "Sport" mode
- Illuminate symbols such as
    -  Check Engine
    -  Battery issue
    -  Low oil pressure
    -  DTC (stability control) active
    -  DTC disabled
    -  etc.
- Time

### Limitations

- Unable to control the fuel consumption needle
    - Apparently this is difficult as the needle cannot be controlled separately but the position is somehow calculated by the cluster
- There is "Service" notification and "SOS calls not possible" warnings
    - Code tries to at least suppress those
- There is a warning triangle on the lower screen part

## Pinout

__Tip:__ There are faint numbers on the cluster port marking the pin numbers. Look closely!

![Pinout](./external/pinout.jpeg)

[(Source)](https://forum.arduino.cc/t/controlling-bmw-e90-instrument-cluster/670728)

### Other inputs

- Outside temperature is coming from external sensor between pins 4 and 5. A resistor of 10k Ohm can be used to have approximately 10'C which removes the cold weather warning
- BC buttons (Enter, Up, Down) can be controlled by connecting 3 buttons to pin 16. Resistor values should be 1k, 2k, 3k Ohm to ground

## Serial CAN bus adapter settings

- Serial CAN bus adapter has **persistent** memory for the baud rate and CAN bus speeds. You should only set them once
- Connection should be made RX to TX and TX to RX between the cluster and the Serial CAN bus adapter
- It's best to set the parameters with one-time-use-only code one at the time and have `while(1);` after the set
    - Hook up the UART code so that you can see what the adapter responds. It should say `OK` after you set a value
    - Send `+++` to go to configuration mode
    - The CAN bus towards the cluster should be set to __100 kb/s__ with `AT+C=12`
    - The serial port speed between the microcontroller and the adapter should be set to __115200__ baud with `AT+S=4`. This is the highest speed possible and is needed to be able to send CAN messages fast enough
- There should __NOT__ be 120 Ohm termination in the Serial CAN bus adapter. If it exists, it should be removed
- __The Serial CAN bus adapter can be easily overwhelmed with commands. It seems to work much better having 2 ms between frames. See the main loop how this can be achieved__

## Notes and findings

- There's a Discord community around hacking the clusters with lots of knowledge and information
    - [Arduino-Tacho Gang](https://discord.gg/UQFsS9D6kq)
- Lights on the cluster (like Check Engine, DTC, Oil Pressure) can be controlled with CAN ID `0x592`. See `canSendErrorLight` and codes in [symbol document](./external/E92%20checkcontrol%20symbols.pdf)
