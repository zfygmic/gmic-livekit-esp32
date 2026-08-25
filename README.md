# LiveKit on the GMIC HA-TOYMD (ESP32-S3)

A working LiveKit voice client on a production audio module: it joins a room,
publishes its microphone, and plays what it receives through its own speaker.

Everything needed to reproduce that is in this repository — the project builds
as-is against ESP-IDF v5.4.4, and the pin map is documented because the vendor
never published one.

---

## Why this exists

LiveKit ships an ESP32 client SDK, and the SDK ships an example. The example
targets a Waveshare development board: touchscreen, two audio chips, a separate
power-management IC. That board is fine for trying the SDK and wrong for
shipping a product.

So the question a lot of people hit is: *what do I actually build this on?*

This repository is one answer. The HA-TOYMD is a module we manufacture — a bare
ESP32-S3 audio board, one codec, microphone and speaker, no screen. We brought
LiveKit up on it ourselves and wrote down what it took, including the parts that
did not work on the first try.

If you are evaluating hardware for a voice product, the useful thing here is
probably not the code. It is the pin map and the limits section — the two things
that cost us the most time and that a datasheet would not have told us.

---

## What is verified

Measured on the hardware, not inferred from a datasheet:

| | Result |
|---|---|
| Wi-Fi association and internet access | Works (2.4 GHz, WPA2) |
| Audio codec brought up over I2C | Single device on the bus at 0x18 |
| Microphone capture | Live audio, verified against room noise |
| Speaker playback | Verified by ear |
| LiveKit signalling and room join | Participant reaches ACTIVE state |
| Microphone track published | Track live and unmuted |
| **Uplink audio intelligible at the far end** | **The published track was recorded off the room and transcribed; speech in the room came back as clean text** |
| Remote audio subscribed and played | A tone published from a PC was heard from the module's speaker |
| Session stability | 90 s of subscribed audio with no gap; one session held over five minutes |
| Flash headroom | App is 1.9 MB in a 3 MB partition |

Two rows carry the weight. Audio flows in **both** directions over LiveKit on
this hardware, and what the microphone sends is **intelligible at the other
end** — the uplink was checked by decoding and transcribing the received track,
not by trusting a track that reports itself as live.

---

## Hardware

ESP32-S3 (QFN56, rev v0.2) · 4 MB flash · 2 MB quad PSRAM @ 40 MHz ·
ES8311 audio codec · Wi-Fi b/g/n **2.4 GHz only** · native USB serial/JTAG over
the USB-C connector (VID 303A / PID 1001).

### Pin map

There is no vendor pin map for this module. These values were recovered by
flashing the factory firmware back onto a board, letting it run, and reading the
GPIO matrix routing registers out of the live chip over its built-in USB-JTAG
port. They were then confirmed by bringing the codec up independently from
scratch.

| Signal | GPIO |
|---|---|
| I2C SDA | 17 |
| I2C SCL | 18 |
| I2S MCLK | 16 |
| I2S BCLK | 9 |
| I2S WS (word clock) | 45 |
| I2S DOUT (ESP32 → codec, playback) | 8 |
| I2S DIN (codec → ESP32, microphone) | 10 |
| Speaker amplifier enable | 48 |
| Buttons | 12, 13, 14 (observed, not vendor-confirmed) |

Codec address is `0x18`. Audio format is 16-bit Philips I2S at 16 kHz with a
256× master clock — the combination this hardware was verified with.

If you are bringing up a different ESP32 board and cannot find its pin map
either: brute-forcing the combinations does not work (we tried roughly seventy
thousand and got nothing), and neither did disassembling the firmware. Reading
the GPIO routing registers off the running chip took ten minutes. Go straight to
the debug port.

---

## Quick start

```
idf.py set-target esp32s3
idf.py menuconfig      # LiveKit Example → Wi-Fi SSID/password, and either a
                       # LiveKit sandbox ID or a pre-generated server URL + token
idf.py build flash monitor
```

Built and verified with **ESP-IDF v5.4.4**. Component versions are pinned in
`dependencies.lock`.

Rebuilding this project unmodified reproduces the firmware we run on hardware:
identical image size (2,006,512 bytes), differing only in the embedded build
timestamp and ELF hash — under 100 bytes out of two million.

Powering a board up without configuring it is harmless — it prints what is
missing, then completes hardware init, so you can confirm the codec is alive
before setting anything up.

Pre-built binaries are attached to the
[latest release](../../releases/latest) if you want to confirm a board before
installing a toolchain.

---

## What was changed from the stock LiveKit example

The stock example does not run on this module. Worth knowing if you are porting
it to a board of your own:

- **`main/board.c` was rewritten.** Upstream targets a board with a touchscreen,
  two audio chips, and a PMIC. This module has one codec and none of the rest,
  on different pins.
- **Capture no longer goes through the on-device AEC front end.** That path
  expects a 4-channel TDM stream with a hardware reference channel. This module
  has a single microphone on a 2-channel bus; feeding the AEC front end from it
  overflowed its ring buffer and tore the connection down after ~27 seconds.
  Capture is now taken straight from the codec, and echo cancellation is left to
  the application side.
- **A startup banner was added** so an unconfigured board explains itself.

Full attribution and the upstream license are in [NOTICE](NOTICE).

---

## Known limits

Stated plainly rather than discovered later:

- **2.4 GHz and WPA2 only.** The radio does not do 5 GHz, and WPA3-only networks
  fail in a way that looks like "network not found" — the error line even prints
  the wrong SSID. Worth knowing before you debug the wrong layer.
- **CPU is tight.** With Opus encode and decode both running, the idle task gets
  starved and the task watchdog complains. Audio was continuous and clean by ear
  throughout, but there is little headroom left for extra processing on-device.
- **No hardware echo cancellation in this configuration**, for the reason above.
  In a speakerphone arrangement, echo control has to happen application-side.
- **Button-to-GPIO mapping is observed, not vendor-confirmed.**
- **The RGB status LED is not driven by this firmware.** Its control pin has not
  been identified.
- **One reconnect was seen shortly after a cold-boot join** (~45 s in, once). The
  session that replaced it ran over five minutes uninterrupted, and subscribed
  audio across that window had no gaps. Cause not identified; noted because you
  may see it once on first bring-up.

### A note on IPv4

Not a limit of this module, but it cost us an afternoon and it will cost you one
too: a LiveKit server will happily hand an ESP32 an IPv6 candidate address, and
the ESP32 will not use it. If the device joins the room, publishes a track, and
then no media ever flows, pin the server to your IPv4 address
(`rtc.ips.includes`) before looking anywhere else.

---

## Getting the hardware

<!-- TODO(Fanyao): one or two sentences. How does a reader actually obtain a
     module? No prices, no MOQ. Everything below this line is placeholder. -->

_Contact us and we will point you at the right part._

---

## Questions

Pin-level and firmware questions we can answer directly — we brought this up
ourselves. Other form factors, what the factory can change, availability: same
contact.

Open an issue, or reach GMIC AI Inc. at <fanyao@gmic.ai>.
