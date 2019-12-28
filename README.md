[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](LICENSE)
[![Appveyor build status](https://ci.appveyor.com/api/projects/status/grn3rjfng0dg5wba/branch/master?svg=true)](https://ci.appveyor.com/project/ThijsWithaar/denoncontrol/branch/master)
[![Github all releases](https://img.shields.io/github/downloads/ThijsWithaar/DenonControl/total.svg)](https://GitHub.com/ThijsWithaar/CD-Grab/releases/)

# DenonControl

A Library and UI for controlling [Denon Receivers](https://www.denon.com).


# Components

- C++ library for parsing the protocols:
    - Telnet
    - SSDP (UPnP) protocol, for automatic discovery of receivers
    - Ipod / http protocol, for control
    - UPnp events, for updates on status changes
- SSDP bridge, to discover Denon receivers which are on a different subnet.
  This also allows the official Denon Android app to find a receiver which is on a different subnet.
- Qt-based UI for basic control and status: power, volume, equalizer.


# Denon Protocols

Denon receivers have two ways on communicating. On is the legacy [serial protocol](https://usa.denon.com/us/product/hometheater/receivers/avr3808ci?docname=AVR-3808CISerialProtocol_Ver520a.pdf) which is reachable over telnet (port 23). The other is a combination of HTTP based protocols:

- SSDP for discovering the device (as long as it's on the same subnet)
- Upnp Events for device updates
- Http-Get based commands

Although the latter does seems to have more features, it is also a whole lot more complex. As far as I know, no documentation exists. Since most of the protocol is xml-based, using [wireshark](https://www.wireshark.org/) and Denon's [Android app](https://play.google.com/store/apps/details?id=com.dmholdings.DenonAVRRemote) make it relatively easy to figure things out.
This does mean that the parsing code in this archive is far from complete.
