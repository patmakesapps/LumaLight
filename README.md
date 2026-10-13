# LumaLight

A proximity nightlight. An ESP32 watches a VL53L1X time-of-flight sensor and lights a 12-LED NeoPixel ring when
someone walks past. The colour shifts from purple to white and the light brightens as you get closer. It stays on
for 2 minutes, then fades out. The sensor uses its own infrared laser, so it works in complete darkness.

The 3D-printed enclosure is in [`cad/`](cad/). See [`cad/LUMALIGHT_SPEC.md`](cad/LUMALIGHT_SPEC.md) for parts,
print orientation and assembly.

## Hardware

| Part | Connection |
|---|---|
| ESP32 dev board (`esp32dev`) | powered over USB |
| VL53L1X distance sensor | SDA → GPIO 21, SCL → GPIO 22, 3.3 V, GND |
| 12-LED NeoPixel ring (GRB) | DIN → GPIO 18, 5 V, GND |

Brightness is capped (`MAX_BRIGHTNESS`) so the ring runs safely from USB power.

## How it decides to turn on

1. When the light goes off, and at power-up, it averages the next 5 readings and saves that as a snapshot of the
   empty room.
2. While it is off, a reading more than 150 mm (`CHANGE_MM`) from the snapshot, closer or farther, for two
   readings in a row, turns it on. A wall right behind the light doesn't matter, because only a change counts.
3. While it is on, distance only sets the colour and brightness. It never extends the 2-minute timer.

If someone is still in front of the light when it goes off, they become part of the snapshot. Walking away then
turns it on for one more cycle.

To tune it, change the constants at the top of [`src/main.cpp`](src/main.cpp). Raise `CHANGE_MM` if it turns on by
itself (steam, mirrors), and lower it if it misses people. `ACTIVE_MS` is the on-time.

## Build and flash

Built with [PlatformIO](https://platformio.org/).

```sh
pio run -t upload          # build and flash over USB
pio device monitor         # serial log, 115200 baud
```

The serial log prints the state, distance, brightness, time left and snapshot once a second. It is USB-only.

## Flashing over Wi-Fi (optional)

After one USB flash with Wi-Fi set up, you can update the light where it sits.

1. Copy `include/secrets.example.h` to `include/secrets.h`. Fill in your Wi-Fi name and password (2.4 GHz only)
   and choose a flashing password.
2. Copy `secrets.example.ini` to `secrets.ini` and put the same flashing password after `--auth=`.
3. Flash once over USB. The serial log shows `[WIFI] lumalight.local at <ip>` when it connects.
4. From then on, flash over Wi-Fi:

   ```sh
   pio run -e ota -t upload
   ```

Both secrets files are git-ignored. Without `include/secrets.h`, the firmware still builds and works, but Wi-Fi
is off and it can only be flashed over USB. The light never waits on Wi-Fi: if the network is down, it keeps
working and retries every 30 seconds.

Troubleshooting:

- **`No response from device` after `Authenticating... OK`.** The light connects back to your computer to download
  the firmware, and a firewall is blocking it. On Windows, allow PlatformIO's Python
  (`%USERPROFILE%\.platformio\python3\python.exe`) on Private networks.
- **`lumalight.local` not found.** Reserve an IP address for the light in your router, and set `upload_port` in the
  `[env:ota]` section of `platformio.ini` to that IP.
