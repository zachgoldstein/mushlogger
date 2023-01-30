# Mushroom Datalogger

A environmental datalogger and local-network data viewer
Originally designed to monitor a mushroom fruiting chamber

This repo contains firmware meant to run on the system
Built for Raspi Pico W and RP2040
Built with platformIO

## Main functions

- Read sensors for environmental data
- Write data to file on a SD card
- Connect to local wifi network
- Start a webserver and make it available on network
- Expose simple JSON API to retrieve JSON data 
- Host a static webpage to chart data over time