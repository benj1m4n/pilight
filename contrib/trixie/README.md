# pilight on Debian 13 "trixie" / Raspberry Pi OS trixie

Unofficial fork of [pilight](https://github.com/pilight/pilight) (based on the
`staging` branch) that builds and runs on current systems. The packages from
apt.pilight.org do not work there: they are 32-bit only and depend on
libraries that no longer exist (e.g. libmbedtls10).

Not affiliated with the pilight project.

## What is different from upstream `staging`

- Builds with GCC 14 and without `-fcommon`, against mbedtls 3.6 (2.28 still works)
- Several crash fixes (malformed HTTP requests, pilight-send, failed GPIO interrupts, …)
- MQTT broker supports **MQTT 5** in addition to 3.1 and 3.1.1, including
  several topic filters per SUBSCRIBE, UNSUBSCRIBE and No Local, plus
  robustness fixes (malformed packets, use-after-free on disconnect)
- Unknown device names in MQTT control topics are logged instead of ignored

Details are in the commit messages of the `trixie` and `mqtt5` branches.

## Raspberry Pi 4: wiringX patch

Current Raspberry Pi kernels number the on-board GPIOs in sysfs from 512
(`pinctrl-bcm2711` → `gpiochip512`). wiringX already handles this for the
Pi 2/3 (BCM2836); `wiringx-pi4-sysfs.patch` does the same for the Pi 4.
Without it, setting up the receiver interrupt fails. `build.sh` applies it
automatically.

## Build and install

```sh
git clone -b mqtt5 https://github.com/benj1m4n/pilight.git
cd pilight
contrib/trixie/build.sh
```

The script installs the build dependencies, builds wiringX (fixed commit +
patch) and pilight, installs everything to `/usr/local`, keeps an existing
`/etc/pilight/config.json` and installs a systemd drop-in that runs pilight
in the foreground (`pilight-foreground.conf`).

Afterwards:

```sh
sudo systemctl stop pilight              # edit the config only while stopped
sudoedit /etc/pilight/config.json
sudo pilight-daemon -F -D                # test in the foreground
sudo systemctl enable --now pilight
```

Example settings for 433 MHz on a Raspberry Pi 4 (wiringX numbering:
0 = GPIO17 / pin 11, 1 = GPIO18 / pin 12):

```json
"settings": { "gpio-platform": "raspberrypi4", ... },
"hardware": { "433gpio": { "sender": 0, "receiver": 1 } }
```

## MQTT / Home Assistant

pilight's broker listens on port 1883 (setting `mqtt-port`).

- switch: publish `on` / `off` to `pilight/dev/ctrl/<device>/state`
- state: pilight publishes to `pilight/dev/stat/<device>/state` (not retained)

`<device>` is the key in the `devices` section of `config.json`, not the
name shown in the web interface. It is case sensitive.

Home Assistant example (`configuration.yaml`):

```yaml
mqtt:
  switch:
    - name: "Living room lamp"
      unique_id: pilight_livingroom_lamp
      command_topic: "pilight/dev/ctrl/LivingroomLamp/state"
      state_topic: "pilight/dev/stat/LivingroomLamp/state"
      payload_on: "on"
      payload_off: "off"
      state_on: "on"
      state_off: "off"
      optimistic: true
```

pilight's broker has no authentication and no persistent sessions. If you
need those, run Mosquitto on 1883 and bridge to pilight on another port, see
`mosquitto-pilight.conf`.

## Security notes

- The bundled `pilight.pem` (HTTPS webserver) is public; its private key is
  in this repository. Do not expose the web interface or the MQTT broker to
  the internet.
- The 433gpio hardware needs direct GPIO access, so pilight runs as root.

## Files

| File | Purpose |
|------|---------|
| `build.sh` | build and install script |
| `wiringx-pi4-sysfs.patch` | wiringX fix for the Raspberry Pi 4 |
| `pilight-foreground.conf` | systemd drop-in (`pilight.service.d/override.conf`) |
| `mosquitto-pilight.conf` | optional Mosquitto bridge configuration |
