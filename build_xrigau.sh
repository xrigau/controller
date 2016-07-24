#!/usr/local/bin/fish
cd Keyboards/
rm -rf xrigau_led_layout
bash xrigau_led.bash -f -o xrigau_led_layout
cd xrigau_led_layout/
dfu-util -D kiibohd.dfu.bin
cd ../..

