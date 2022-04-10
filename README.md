# midi-asdf

A toy tool that allows a MIDI keyboard (ALSA)  to be used as a computer keyboard (/dev/uinput).

# Usage
![MIDI to ASDF key mapping](./midi-asdf.svg)

```
./midi-asdf                    # start the tool, "sudo" may be necessary instead, depending on permissions
aconnect aconnect 20:0 128:0   # connect your MIDI keyboard to midi-asdf
```
