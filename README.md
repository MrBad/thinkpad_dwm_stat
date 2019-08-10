# thinkpad_dwm_stat
A simple status line for my thinkpand and dwm window manager

Tested on thinkpad X220, Debian Jessie, librebooted T60 with Debian Stretch.
It displays CPU temperature, fan RPM, battery / AC - discharging, idle or charging percent and remainig time and date.

To run it, add a line to your ~/.xinitrc like this
[full_path_to]/tkstat &
before executing dwm
