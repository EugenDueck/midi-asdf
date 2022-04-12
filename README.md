# midi-asdf

A toy tool that allows a MIDI keyboard (ALSA)  to be used as a computer keyboard (/dev/uinput).

# Usage
![MIDI to ASDF key mapping](./midi-asdf.svg)

```
# Just start the tool:
./midi-asdf

# OR, if it does not work due to permission issues with /dev/uinput, sudo:
sudo ./midi-asdf

# Start the tool and connect it to port 0 of the MIDI sequencer "Roland Digital Piano"
./midi-asdf "Roland Digital Piano"
```
