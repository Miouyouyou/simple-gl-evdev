# Tipping

[![Pledgie !](https://pledgie.com/campaigns/32702.png)](https://pledgie.com/campaigns/32702)
[![Tip with Altcoins](https://raw.githubusercontent.com/Miouyouyou/Shapeshift-Tip-button/9e13666e9d0ecc68982fdfdf3625cd24dd2fb789/Tip-with-altcoin.png)](https://shapeshift.io/shifty.html?destination=16zwQUkG29D49G6C7pzch18HjfJqMXFNrW&output=BTC)

# About

A not-so-small example showing how to use DRM (KMS), OpenGL ES 2 and 
Evdev to control a software cursor. 

The point is to show how to capture input with Evdev, and show the 
results in an DRM-driven OpenGL application.

Software cursors are inherently bad, as they're limited by the current
application refresh rate, but are still nice when it comes to show 
input feedback.

# Requirements

- CMake
- DRM (kernel drivers, libraries and development headers)
- Evdev (kernel drivers, libraries and development headers)
- GBM (libraries and development headers)
- OpenGL ES 2.x (drivers, libraries and development headers)
- A user that has the rights to read raw input data from Mouse input 
  node.

# Build

To build this project, do something like :
```bash
cd /tmp
git clone https://gitlab.com/Miouyouyou/simple-gl-evdev
mkdir build-glev
cmake ../simple-gl-evdev
make
```

You can then run `./Program` to run the program. If the program 
complains about a missing mouse, check that a mouse is clearly plugged
in and that your user can read raw input data, from the /dev/input/ node 
representing your mouse.
You can also run the program as root, but this is ill-advised.

# Thanks to

- @Robclark for [kmscube](https://github.com/robclark/kmscube)

# License

MIT
