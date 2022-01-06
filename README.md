[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](LICENSE)
[![Appveyor build status](https://ci.appveyor.com/api/projects/status/grn3rjfng0dg5wba/branch/master?svg=true)](https://ci.appveyor.com/project/ThijsWithaar/denoncontrol/branch/master)
[![Cirrus build status](https://api.cirrus-ci.com/github/ThijsWithaar/DenonControl.svg)](https://cirrus-ci.com/github/ThijsWithaar/DenonControl/master)
[![Gitub actions](https://github.com/ThijsWithaar/DenonControl/actions/workflows/cmake.yml/badge.svg)](https://github.com/ThijsWithaar/DenonControl/actions)
[![Github all releases](https://img.shields.io/github/downloads/ThijsWithaar/DenonControl/total.svg)](https://GitHub.com/ThijsWithaar/DenonControl/releases/)

# DenonControl

A Library and UI for controlling [Denon Receivers](https://www.denon.com), for linux and windows:

<img src="https://github.com/ThijsWithaar/DenonControl/blob/master/doc/Screenshot_KDE.png" width="400"> <img src="https://github.com/ThijsWithaar/DenonControl/blob/master/doc/screenshot-windows.png" width="400">


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

Denon receivers have two ways on communicating. On is the legacy [serial protocol](https://usa.denon.com/us/product/hometheater/receivers/avr3808ci?docname=AVR-3808CISerialProtocol_Ver520a.pdf) which is reachable over telnet (port 23). The other is a combination of HTTP based protocols. An overview is given in the table below:


| Port | Protocol | Path | description |
| ---- | -------- | ---- | ------------|
|   27 | telnet   |      | serial [commands](lib/include/Denon/serial.h)/[events](lib/include/Denon/denon.h) |
| 8080 | http     | goform/Deviceinfo.xml | [Android API](lib/include/Denon/appInterface.h) |
| 8080 | http     | goform/AppCommand.xml | [Android API](lib/include/Denon/appInterface.h) |
| 239.255.255.250:1900 | ssdp | | [SSDP Device discovery](lib/include/Denon/ssdp.h) |
| 60006 | scdp | upnp/desc/aios_device/aios_device.xml | [UPnP Event description](lib/include/Denon/upnpEvent.h) |
| 60006 | http | upnp/scpd/renderer_dvc/RenderingControl.xml | [SCDP control](lib/include/Denon/upnpControl.h) |
| (listen) | http | upnp/event/renderer_dvc/RenderingControl | [Volume change events](lib/include/Denon/upnpEvent.h) |


Although both the Android and UPnP protocols does seem to have more features, they are also a whole lot more complex with xml-wrapped-in-xml constructions. As far as I know, no documentation exists. Since most of the protocol is xml-based, using [wireshark](https://www.wireshark.org/) and Denon's [Android app](https://play.google.com/store/apps/details?id=com.dmholdings.DenonAVRRemote) make it relatively easy to figure things out.
This does mean that the parsing code in this archive is far from complete.
