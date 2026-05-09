# Framebuffer capture

Captures the LVGL draw buffer from the device and converts it to a PNG.

## 1. Get the buffer address

Build and flash with `CONFIG_CHIP_APP_LOG_LEVEL=LOG_LEVEL_DBG`. On boot, the log emits:

```
LVGL draw buf (stride 25):
savebin framebuf.bin 0x<addr> 0x<size>
exit
```

Copy the two `savebin`/`exit` lines into `framedump.jlink`. The buffer is statically
allocated so the address never changes between resets.

## 2. Dump with JLink and convert to PNG

Take the dump after the display has been updated with content. Then:

```sh
./framedump
```

This writes `framebuf.bin` in the current directory and then converts it to `framebuf.png`.

Note: the Zephyr LVGL mono flush callback converts the buffer in-place from LVGL's I1
format to MONO10 before sending to the SSD16xx, so the dump captures MONO10 data
(bit=1 = black pixel). The script accounts for this.
