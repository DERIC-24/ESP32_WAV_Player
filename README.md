# ESP32 WAV Player with LM386 Audio Amplifier

An offline WAV audio player built using the original ESP32 development board. Audio is stored in SPIFFS flash memory and played through the ESP32's built-in DAC into an LM386 amplifier.

## Features

- Offline playback from SPIFFS
- PCM WAV file support
- Analog audio output through GPIO25 (DAC1)
- LM386-based speaker amplification
- PlatformIO development workflow

## Hardware

- ESP32 Dev Module (original ESP32, not ESP32-C3)
- LM386 audio amplifier IC
- 8 Ω speaker
- 10 kΩ potentiometer for volume control
- 10 µF input coupling capacitor
- 470 µF output coupling capacitor
- Breadboard and jumper wires
- 5 V supply suitable for the circuit

## Connections

| ESP32 / Component | Connection |
|---|---|
| ESP32 GPIO25 | Audio signal input through 10 µF coupling capacitor |
| Potentiometer wiper | LM386 pin 3 |
| LM386 pin 2 | GND |
| LM386 pin 4 | GND |
| LM386 pin 6 | +5 V |
| LM386 pin 5 | Output through 470 µF capacitor to speaker |
| Speaker other terminal | GND |
| All circuit grounds | Common GND |

Connect the potentiometer's other outer terminal to GND. The remaining outer terminal receives the audio signal through the input capacitor.

## Audio Format

The WAV file must be:

- Format: PCM, uncompressed
- Channels: Mono
- Sample rate: 22,050 Hz
- Bit depth: 8-bit unsigned

Place the audio file at `data/audio.wav`.

## Software Requirements

- Visual Studio Code
- PlatformIO IDE extension
- ESP32 Arduino framework
- FFmpeg (for audio conversion)

Convert an audio file to the required format with:

```bash
ffmpeg -i input.wav -t 30 -ac 1 -ar 22050 -c:a pcm_u8 audio.wav
```

Place the resulting file in the project's `data` directory.

## Build and Upload

Build the firmware:

```bash
pio run
```

Upload the firmware:

```bash
pio run -t upload
```

Upload the SPIFFS filesystem:

```bash
pio run -t uploadfs
```

Open the serial monitor:

```bash
pio device monitor -b 115200
```

## Project Structure

```text
ESP32_WAV_Player/
├── data/
│   └── audio.wav
├── src/
│   └── main.cpp
├── platformio.ini
├── .gitignore
└── README.md
```

## Limitations

This is a basic DAC playback implementation intended for experimentation. Audio quality and playback timing depend on the software implementation and analog amplifier circuit.

## License

Add a license if you intend to permit others to reuse or modify this project.
