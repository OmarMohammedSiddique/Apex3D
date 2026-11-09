# Starting-light cues

These short interface samples accompany the red lights and lights-out only.
There is no engine loop or race background sound. Cues are muted initially;
the on-screen UNMUTE/MUTE button or `M` controls them on macOS. Other platforms
show NO AUDIO. Screenshot modes never open an audio device.

Source: Kenney, **Interface Sounds 1.0**, CC0.

- Original pack: https://kenney.nl/assets/interface-sounds
- Downloaded WAV mirror: https://github.com/Calinou/kenney-interface-sounds
- Included license: `LICENSE-Kenney.txt`
- `start-light.wav`: `tick_002.wav`, converted from 44.1 kHz to 24 kHz mono PCM16.
- `lights-out.wav`: `confirmation_002.wav`, converted to the same format.
- Conversion: FFmpeg, resampling only. Playback gain is 0.22.

These are application feedback sounds, not an F1 broadcast recording or a
claim that the physical start gantry emits these sounds. The illustrative
6.5-second sequence is separate from the documented race clock.

`python3 tools/generate_race_assets.py` embeds these small WAV samples in a
C++ header, so the built executable works from any working directory.
