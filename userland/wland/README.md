# wland: Wi-Fi from userland (WCN3990 through the modem)

The OnePlus 7 Pro's Wi-Fi is the SM8150's integrated WCN3990 (`qcom,icnss`):
its firmware runs on the modem DSP. `wland` does the whole bring-up from
userland over `/dev/pmem` (kernel 0138, boot-arg `sweetpmem=1`): RPMh votes
for the WLAN rails, PAS boot of the modem through TrustZone, SMP2P, GLINK +
QRTR, the rmtfs / pd-mapper / TFTP services the modem needs, the WLFW QMI
service (board data, mission mode), then the radio: copy engines, WMI, HTT,
scans, WPA2-PSK, DHCP over a utun.

```
wland up SECS      everything from a fresh phone boot, for SECS seconds (0 = forever)
wland smem | smp2p | boot | shutdown | smmu | ...   single steps (usage in wland.c)
```

Files on the phone (from your own phone's partitions; never in git):

| Path | From |
| --- | --- |
| `/var/local/fw/modem/modem.mdt`, `modem.bNN`, `modemr.jsn`, `modemuw.jsn`, `wlanmdsp.mbn`, `bdwlan.*` | `modem_b` (FAT, `image/`), names in lower case |
| `/var/local/fw/rmtfs/{modemst1,modemst2,fsg,fsc}.img` | those partitions (the modem's EFS; it writes to these copies) |
| `/var/local/wlan.conf` (`ssid=`, `psk=`) | optional: join a network |

## Origin and licence

Taken from [NWylynko/ios-flame](https://github.com/NWylynko/ios-flame)
`phone/wifi` (commit 745c434), which runs it on the Pixel 4: the
same SM8150 and WCN3990. guacamole changes are marked `guacamole:` in the
source (modem region size, board id and board file as the stock cnss-daemon
picks them, MAC address from WMI READY, the BSS peer before VDEV_START, the
firmware log over WLFW INI, jetsam limit in `up`; docs/guacamole.md, Wi-Fi). ios-flame's CREDITS: `radio.c` is a
userland port of ios-topaz's TopazModem; the protocol code follows Linux
ath10k (ISC) and the Qualcomm SMEM, SMP2P, GLINK, QRTR and remoteproc
drivers, and Bjorn Andersson's rmtfs, pd-mapper and tqftpserv.

```
MIT License

Copyright (c) 2026 dipa-studio

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
