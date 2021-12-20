# AudioOutAppCpp

App I wrote to toggle/cycle between windows audio outputs.
I have a particularly annoying issue with my Dell U3419W screen, which emits some static noise from it's speakers even when I'm using another audio output.
A workaround is to mute the screen's audio device in windows before switching to another output.
Now I connect this app to a keyboard shortcut using AHK.

Whenever called, this application cycles through a list of available audio devices (specified in ```config.json```).
Before switching to the next audio device, it'll store the current audio level in a LUT in the config file, mute it, and unmute the next audio device with the last volume stored in the LUT.

### TODO List
- Generate default config
- cmd line options for different behaviors
