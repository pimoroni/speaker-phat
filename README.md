# Speaker pHAT

To install the software needed to setup your Speaker pHAT, use the one-line installer:

Launch terminal then type:

`curl -sS https://get.pimoroni.com/speakerphat | bash`

Setup will continue automatically.

That's it, enjoy!

# Uninstalling

Back up `/etc/asound.conf` and then delete it. This file includes the configuration for volume control and Pi VU Meteron Speaker pHAT, but your Pi should default to a sane audio configuration without it.

Make sure `dtparam=audio=on` is in `/boot/config.txt` and comment out the lines;

```
dtoverlay=i2s-mmap
dtoverlay=hifiberry-dac
```

If you use the Pixel desktop you might want to re-add the volume control widget- we remove this because it interferes with audio configuration by spontaneously creating an `~/.asoundrc` config file.

And if you want pulseaudio back (if you're using Raspbian < Stretch) you can (if it exists):

```
sudo mv /etc/xdg/autostart/pulseaudio.disabled /etc/xdg/autostart/pulseaudio.desktop
```

# IMPORTANT NOTE


This repository ONLY contains a basic Python library that allows control of the SN3218 LED driver used for the Speaker pHAT VU meter.

The software used to drive the SN3218 as part of the ALSA stack can be found at:

`https://github.com/pimoroni/pivumeter`

Issues not related to the Python library presented in this repository should be reported at the above URL, thanks!
