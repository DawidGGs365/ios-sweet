# ios-sweet: guacamole fork

**Apple's iOS 16.7.16 userland on a OnePlus 7 Pro (`guacamole`, GM1913, Snapdragon 855 / SM8150).**

This fork carries the XNU kernel side of the OnePlus 7 Pro port. It started from [dipa-studio/ios-sweet](https://github.com/dipa-studio/ios-sweet), which ports Apple's open-source XNU (xnu-8796, the iOS 16 generation) to the Redmi Note 10 Pro (`sweet`, SM7150). The same `SWEET` kernel configuration now runs on both phones. Drivers pick the board at runtime from the device tree (`sweet_is_guacamole()`), so the upstream `sweet` support still builds. Its instructions are in [`docs/SWEET.md`](docs/SWEET.md).

The boot chain, payload and board files for guacamole live in the companion repository **unixnu-guacamole**: Project Aloha UEFI → UniXNU → DarwinBoot → this kernel.

> Experimental bring-up, not a usable phone OS. Apple's security model is mostly switched off, and the iOS root lives in the `userdata` partition, so Android's user data is erased. Restore Android with `fastboot -w`.

## Status (2026-11-06)

| Area | State on guacamole |
| --- | --- |
| Boot | `fastboot boot` of a UniXNU boot image; nothing is flashed to the boot partitions |
| CPU | All 8 cores (SMP through PSCI) |
| Storage | Own UFS host driver; iOS root is journaled HFS+ in `userdata`, read/write |
| GUI | backboardd, SpringBoard, lock and home screen reach the panel. CoreAnimation renders on the CPU |
| Display | iPhone X geometry (1125×2436) stretched to the 1440×3120 OLED; panel on/off works |
| Brightness | DSI DCS 0x51 through the DSI0 command FIFO plus a `backlight-control` service (0104); waiting for a hardware test |
| Power key | Works (PM8150 PON) |
| Volume keys | Work (PM8150 GPIO6 / GPIO7, 0102); the volume HUD reacts slowly |
| Alert slider | Ring/silent switch from TLMM gpio27/134/125 (0107); not tested yet |
| Touch | Samsung S6SY761 on QUP SE17. SE17 is FIFO-locked by TrustZone, so it needs GPI DMA through an SMMU context bank. Not working yet; 0105 adds the S1 translation Linux uses |
| GPU | Adreno 640 GMU firmware starts (0094 onwards) but aborted with 0xBADDFACE on the old HFI layout; 0105 fixes the layout. The HFI handshake (0106) and perf/bandwidth tables (0108) are not tested on hardware yet. No acceleration yet |
| Battery | bq27541 fuel gauge over I²C SE8 (0103); reports to the IOPMPowerSource service |
| USB | Device mode with a CDC-ECM live log to the PC (`tools/sweet-usbnet.sh`) |
| Haptics | Read-only probe of the PM8150B haptics block (0107); the AW8697 path needs working GPI first |
| Missing | Wi-Fi, Bluetooth, cellular, audio, camera, sensors, SEP / Face ID, fingerprint |

The engineering log with every test boot and its result is `NOTES.md` in unixnu-guacamole.

## Building

The kernel builds on GitHub's macOS runners (`.github/workflows/xnu.yml`, [blacktop/darwin-xnu-build](https://github.com/blacktop/darwin-xnu-build)). The workflow fetches xnu-8796 and applies `xnu/patches/` in order.

- Pushing does **not** start a build: dispatch with `gh workflow run xnu.yml -R DawidGGs365/ios-sweet --ref main`.
- Only one run at a time: the workflow uses `cancel-in-progress`, so a new dispatch cancels the running one.
- In unixnu-guacamole, `scripts/build-test.sh sNNNN` dispatches the run, waits for it, installs the kernel and packs `out/guacamole/boot/boot-sNNNN-all.img`.

Before spending a CI run, syntax-check single files against stub headers, or replay a previous run's compile commands with `tools/xnu-cc.py`.

## Testing a build

```sh
sh tools/sweet-usbnet.sh ../logs/usb-sNNNN.log    # start the USB live-log listener first
fastboot boot ../unixnu-guacamole/out/guacamole/boot/boot-sNNNN-all.img
```

Driver lines start with `SWEET <DRIVER>:` (TOUCH, GPU, KEYS, SLIDER, DSI, BACKLIGHT, BATTERY, RPMH, USB, HAPTICS).

## Boot-args of the guacamole drivers

| Boot-arg | Effect |
| --- | --- |
| `sweettouch=0/1`, `sweettouchdelay=S` | Touch driver off/on; start S seconds after boot |
| `sweetgpibcm=0` | Skip the QUP0/SN11 bus votes before GPI init |
| `sweetgpu=N`, `sweetgpudelay=S` | GPU: 1 read state, 2 power CX, 3 boot the GMU and do the HFI handshake, 4 also send the perf/BW tables and CORE_FW_START |
| `sweetbl=N`, `sweetbldelay=S` | Set brightness N (1..1023) once, S seconds after boot (path test without the GUI) |
| `sweetdsi=0` | No DSI writes at all |
| `sweetslider=0/2` | Alert slider off / inverted polarity |
| `sweetkeys=0` | Button driver off |
| `sweetusb=2`, `sweetusbrecap=1` | USB live log; recap thread for the last log lines |
| `sweetfb_noconsole=1`, `sweetfb_hz=N`, `sweetfb_stretch=0` | No kernel text over the GUI; panel copy rate; native 1:1 instead of stretching |

## Kernel patches

0001–0081 are the upstream `sweet` series. The table in [`docs/SWEET.md`](docs/SWEET.md) covers 0001–0069. 0070–0081 add USB device mode with CDC-ECM, 8-core SMP and display power.

| # | guacamole / shared |
| --- | --- |
| 0082, 0086 | Present the iPhone X screen on larger panels; stretch it to the panel |
| 0083, 0087, 0093 | USB on SM8150: GCC offsets, GDSC/clocks/SNPS femto PHY power-up, log recap |
| 0084–0085, 0090, 0092 | S6SY761 touch: GCC/TLMM/rail setup, QUP DFS level, HW clock-gating halt check |
| 0095–0097, 0099–0100, 0105 | Touch over GPI DMA: GPII setup, IRQs, diagnostics, QUP2 bus votes, SMMU S1 translation |
| 0089, 0091, 0094, 0098, 0101 | Adreno 640: CX power, SMMU state, GMU firmware boot, stock-KGSL clocks |
| 0105, 0106, 0108 | GMU HFI: queue layout, v2 transport and FW_VER, perf/BW tables, CORE_FW_START |
| 0088 | Display off: blank once without reading the panel back |
| 0102 | Volume keys (PM8150 GPIO6 / GPIO7) |
| 0103 | bq27541 fuel gauge |
| 0104 | Brightness: DSI DCS 0x51 and the `backlight-control` service |
| 0107 | Alert slider as the ring/silent switch; read-only haptics probe |
| 0109 | One cached board check for all drivers; panel stretch without read-back |

## guacamole hardware facts

- RPMh BCM votes go through cmd-db (QUP0 `0x50080`, SN11 `0x5006c`). The GPU perf table uses the `gfx.lvl`, `cx.lvl` and `mx.lvl` ARC tables.
- Touch: S6SY761 at I²C `0x48` on SE17 (`0xc80000`), GPI DMA on `0xc00000` gpii 1, apps-SMMU SID `0x7b6`, IOVA `0x100000`.
- Fuel gauge: bq27541 at `0x55` on SE8 (`0xa80000`), gpio88/89.
- Panel: Samsung S6E3HC2 on DSI0 (`0xae94000`), command mode with DSC, brightness 1..1023.
- GPU: GMU at `0x2c6a000`, GPU CC at `0x2c90000`, SMMU at `0x2ca0000`. The speed-bin fuse is at qfprom `0x784130` (bits 31:29).
- Keys: PM8150 GPIO6 (volume up) and GPIO7 (volume down), power on PON KPDPWR. Alert slider on TLMM gpio27/134/125.

Register-level detail, with the stock OOS 12 driver sources each value came from, is in unixnu-guacamole `NOTES.md`.

## Credits

Built on [dipa-studio/ios-sweet](https://github.com/dipa-studio/ios-sweet) and Apple's open-source releases (xnu, IOHIDFamily, libpthread, hfs), [BearSSL](https://bearssl.org), [blacktop/darwin-xnu-build](https://github.com/blacktop/darwin-xnu-build), [UniXNU](https://github.com/UniXNU/UniXNU), [Project Aloha](https://github.com/Project-Aloha/mu_aloha_platforms), and OnePlus's published SM8150 kernel source for the hardware details. iOS itself is Apple's and is not distributed here.
