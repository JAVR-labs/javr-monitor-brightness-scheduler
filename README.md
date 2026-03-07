# Monitor Brightness Scheduler

## Requirements
`sudo dnf install qt6-qtbase-devel cmake gcc-c++ kscreen`

## Checking monitor names
`kscreen-doctor --outputs | sed 's/\x1b\[[0-9;]*m//g' | grep "^Output:" | awk '{print $3}'`

## How to use
1. `cmake -B build`
2. `cmake --build build`
3. `mkdir ~/.local/bin/brightness_scheduler`
4. `cp build/brightness_scheduler ~/.local/bin/brightness_scheduler/brightness_scheduler` 
5. change \<user\> in path "ExecStart=/home/\<user\>/.local/bin/brightness_scheduler/brightness_scheduler" in brightness_sheduler.service 
6. `cp brightness_scheduler.service ~/.config/systemd/user/brightness_scheduler.service`
7. `cp brightness_scheduler.timer ~/.config/systemd/user/brightness_scheduler.timer`
8. create and setup settings.json in ~/.local/bin/brightness_scheduler like example_settings.json ()
9. `systemctl --user daemon-reload`
10. `systemctl --user enable --now brightness_scheduler.service`
11. `systemctl --user enable --now brightness_scheduler.timer`

## settings.json
### Monitors
"monitors" must be an array of strings containing values from the "Checking monitor names" command.

### Timestamps
The first timestamp must starts with "hour": 0. \
\
Each timestamp must be in format: \
(same brightness on all monitors) \
{\
    "hour": \<int value\>, \
    "brightness": \<int value\>  \
}\
\
OR \
\
(different brightness on different monitors) \
{\
"hour": \<int value\>, \
"brightness": \<array\<int\>\[number of monitors\]\>  \
}\

## Additional
override to next timestamp
`~/.local/bin/brightness_scheduler/brightness_scheduler <value>`

restore default
`~/.local/bin/brightness_scheduler/brightness_scheduler`