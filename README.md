# BMW e90 CAN bus cluster project

This is a work-in-progress BMW e90 CAN bus cluster project to connect it to computer using a microcontroller. Currently _mbed LPC1768_ is used. [Serial CAN bus adapter](https://docs.longan-labs.cc/1030001/) is used between the microcontroller's USART port and the cluster's CAN port.

## Pinout

![Pinout](./external/pinout.jpeg)

[(Source)](https://forum.arduino.cc/t/controlling-bmw-e90-instrument-cluster/670728)

## Notes and findings

- There are faint numbers on the cluster port to mark the pin numbers. Look closely!
- There should __NOT__ be 120 Ohm termination in the Serial CAN bus adapter. If it exists, it should be removed
- The Serial CAN bus adapter can be easily overwhelmed with commands. It seems to work much better if changing the baud rate to maximum of 115200 and have 2 ms between frames. See the main loop how this can be achieved
- Unverified tip: Outside temperature is coming from external sensor between pins 4 and 5. A resistor of 10k Ohm could be tested
- Unverified tip: BC button (Enter, Up, Down) can be controlled by connecting 3 buttons to pin 16. Resistor values should be 1k, 2k, 3k Ohm
- Lights on the cluster (like Check Engine, DTC, Oil Pressure) can be controlled with CAN ID `0x592`. See `canSendErrorLight` and codes in [symbol document](./external/E92%20checkcontrol%20symbols.pdf)
