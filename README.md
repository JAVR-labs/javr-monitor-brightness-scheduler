# Brightness Scheduler

## Requirements
`sudo dnf install qt6-qtbase-devel cmake gcc-c++ kscreen`

## Monitor check
`kscreen-doctor --outputs | sed 's/\x1b\[[0-9;]*m//g' | grep "^Output:" | awk '{print $3}'`

## How to use
1. `cmake -B build`
2. `cmake --build build`
3. `mkdir ~/.local/bin/brightness_scheduler`
4. `cp build/brightness_scheduler ~/.local/bin/brightness_scheduler/brightness_scheduler` 
5. change \<user\> in path "ExecStart=/home/\<user\>/.local/bin/brightness_scheduler/brightness_scheduler" in brightness_sheduler.service 
6. `cp brightness_scheduler.service ~/.config/systemd/user/brightness_scheduler.service`
7. `cp brightness_scheduler.timer ~/.config/systemd/user/brightness_scheduler.timer`
8. create and setup settings.json in ~/.local/bin/brightness_scheduler like settings_example.json (first timestamp must starts with "hour": 0)
9. `systemctl --user daemon-reload`
10. `systemctl --user enable --now brightness_scheduler.service`
11. `systemctl --user enable --now brightness_scheduler.timer`

## Additional
override to next timestamp
`~/.local/bin/brightness_scheduler/brightness_scheduler <value>`

restore default[.gitignore](.gitignore)
`~/.local/bin/brightness_scheduler/brightness_scheduler`