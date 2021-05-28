# craftos-pico
A ComputerCraft 1.8 emulator built for the Raspberry Pi Pico. Uses VGA or serial for screen output. Based on [craftos-native](https://github.com/MCJack123/craftos-native).

## Requirements
* Raspberry Pi Pico
* USB OTG adapter for keyboard and/or storage
* One of:
  * A serial adapter and wires to connect to the UART port
  * [A VGA demo board or compatible setup](https://datasheets.raspberrypi.org/rp2040/hardware-design-with-rp2040.pdf#_the_vga_sd_card_audio_demo_board_for_raspberry_pi_pico) ([Pimoroni](https://shop.pimoroni.com/products/pimoroni-pico-vga-demo-base) has a pre-built version)
* If not using a demo board or USB storage, an SD card breakout connected over SPI

## Building
To build, you must have the [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk), including cmake.

*TODO: Figure out configuration options for VGA/Serial, USB/SD, etc.*

```sh
patch -p1 craftos2-lua < craftos2-lua.patch
mkdir build
cd build
cmake -DPICO_SDK_PATH=<path to pico-sdk> ..
cmake --build .
```

## Running
Simply plug in the Pico while holding BOOTSEL, drop the built UF2 file onto the new USB drive, and the Pico will reboot into CraftOS automatically.
