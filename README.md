# BMW e90 CAN bus cluster project

This is a work-in-progress BMW e90 CAN bus cluster project to connect it to computer using a microcontroller. Currently _mbed LPC1768_ is used. [Serial CAN bus adapter](https://docs.longan-labs.cc/1030001/) is used between the microcontroller's USART port and the cluster's CAN port.

![Highlight image](./media/highlight.jpg)

## Current capabilities

The code is able to control following things on the cluster

- Speedometer
- RPM
- Indicators
- Backlight
- Light symbols (high beams, fog light)
- Fuel gauge
- Handbrake
- Illuminate symbols such as
    -  Check Engine
    -  Battery issue
    -  Low oil pressure
    -  DTC (stability control) active
    -  DTC disabled
    -  etc.

### Limitations

- Currrently the cluster is not entirely clear of errors
    - Clog wheel with exlamation mark is shown
- Unable to control the fuel consumption needle
    - Apparently this is difficult as the needle cannot be controlled separately but the position is somehow calculated by the cluster
- There is "Service" notification and "SIS calls not possible" warings
    - Code tries to at least suppress those

## Pinout

![Pinout](./external/pinout.jpeg)

[(Source)](https://forum.arduino.cc/t/controlling-bmw-e90-instrument-cluster/670728)

## Serial CAN bus adapter settings

- Serial CAN bus adapter has **persistent** memory for the baud rate and CAN bus speeds. You should only set them once
- Connection should be made RX to TX and TX to RX between the cluster and the Serial CAN bus adapter
- It's best to set the parameters with one-time-use-only code one at the time and have `while(1);` after the set
    - Hook up the USART code so that you can see what the adapter responds to. It should say `OK` after you set a value  
    - The Serial CAN bus should be set to __115200__ baud with `AT+C=4`
    - The CAN bus towards the cluster should be __100 kb/s__ with `AT+C=12`
- There should __NOT__ be 120 Ohm termination in the Serial CAN bus adapter. If it exists, it should be removed
- __The Serial CAN bus adapter can be easily overwhelmed with commands. It seems to work much better having 2 ms between frames. See the main loop how this can be achieved__

## Notes and findings

- There are faint numbers on the cluster port to mark the pin numbers. Look closely!
- Unverified tip: Outside temperature is coming from external sensor between pins 4 and 5. A resistor of 10k Ohm could be tested
- Unverified tip: BC button (Enter, Up, Down) can be controlled by connecting 3 buttons to pin 16. Resistor values should be 1k, 2k, 3k Ohm
- Lights on the cluster (like Check Engine, DTC, Oil Pressure) can be controlled with CAN ID `0x592`. See `canSendErrorLight` and codes in [symbol document](./external/E92%20checkcontrol%20symbols.pdf)
