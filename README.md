[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](LICENSE)
[![Appveyor build status](https://ci.appveyor.com/api/projects/status/grn3rjfng0dg5wba/branch/master?svg=true)](https://ci.appveyor.com/project/ThijsWithaar/cd-grab/branch/master)
[![Github all releases](https://img.shields.io/github/downloads/ThijsWithaar/DenonControl/total.svg)](https://GitHub.com/ThijsWithaar/CD-Grab/releases/)

# DenonControl

A Library and UI for controlling [Denon Receivers](https://www.denon.com).


# Components

- C++ library for parsing the protocols:
    - Telnet
    - Ipod / http protocol
    - SSDP (UPnP) protocol, for automatic discovery of receivers
- SSDP bridge, to discover Denon receivers which are on a different subnet.
  This also allows the official Denon Android app to find a receiver which is on a different subnet.
- Qt-based UI for basic control and status: power, volume, equalizer.
