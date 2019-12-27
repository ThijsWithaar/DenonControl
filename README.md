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
