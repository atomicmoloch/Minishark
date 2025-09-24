# Minishark
### Identify devices transmitting around you

Minishark intercepts 802.11 wifi packets and attempts to identify the transmitting and receiving devices, based on MAC organizational unique identifiers.

Inspired by (and a more generalized version of) [Loukanikos](https://github.com/sudo-nano/loukanikos) by [sudo-nano](https://github.com/sudo-nano), which attempts to identify defense contractor surveillence hardware using MAC address scanning.

Every time the esp32 is restarted, it advances to the next wifi channel in its scan, between the minimum and maximum configured.

This is a project for the TTGO T-Camera Plus ESP32 devboard, revision 20190214. However, it should work with any devboard with at least 4MB flash memory and a ST7789 display controller.

## Updating the database

The MAC address database is up to date as of 2025/09/22.
I'm including a python script that I used to generate mac_database.c, convert_c.py. It takes a csv of MAC address OUI prefixes and manufacturer names and generates a new mac_database.c.
