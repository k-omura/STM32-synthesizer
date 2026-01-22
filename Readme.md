# STM32 Digital Synthesizer (Virtual Analog Sound Source)

## Digital synthesizer project with STM32.
The purpose of this project is to make a sound just by creating a simple synthesizer controller.  
The USB-MIDI standard or MIDI Data format over SPI allows to utilize existing equipment and knowledge.  
Hopefully there will be lots of innovative UI ideas for music.  
A binary file is prepared for the controller designer, so download it and write with STM32CubeProg etc.  

## Development environment
- [STM32G431CBU6 (Double-row pin type)](https://github.com/WeActStudio/WeActStudio.STM32G431CoreBoard)
- I2S Sound Module. [for example this](https://www.adafruit.com/product/3006)

## Basic features
- Designed to be operated from the outside via SPI or USB.
- General-MIDI(GM) data format.
- 16ch (GM 128 Instruments, 47 Drum Sounds(10ch)).
- Create custom instruments setting. ((Sine, Saw, SQU) x2 /Channel)
- ADSR.
- Pitch bend.
- Filter that can manipulate the cutoff frequency and Q value.
- LFO(Tremolo, Vibrato, WOW).
- Reverb.
- Pan.
- I2S Stereo output.

## Future features
- What you want.

## Known issue(s)
- XXXX

## Demo
- [Introduction of each function](https://www.youtube.com/watch?v=JlslueGmlfo)
- [Accordion - Projects using this synthesizer](https://www.youtube.com/watch?v=DubTo31tJ50)
- [Rhythm Machine - Projects using this synthesizer](https://www.youtube.com/watch?v=ZY-Zua1G7Z8)
- Old Version
  - [Waveform creation, low-pass filter demo](https://youtu.be/SDA9uaBMBQ4)
  - [SPI control demo](https://youtu.be/EjWuWOQzq90)
  - [Auto performance (Super Mario)](https://youtu.be/yB0PNu2G10Q)
  - [Introduction of creating sounds](https://youtu.be/cMSFZtEUxo4)

## Hot to use
### with USB-MIDI
1. Write the binary and connect your controller with STM32CubeProg.
2. Connect I2S Module and Speaker.
3. Connect PC and STM32 via USB and send MIDI data using MIDI software on PC.
4. Adjust parameters, experiment, and play MIDI files. [PC Control App for Google Chrome](https://k-omura.github.io/STM32-synthesizer/). 

### with MIDI over SPI
underconstration

| Signal | STM32 |
| ---- | ---- |
| MOSI | PA7 |
| MISO | PA6 |
| SCK | PA5 |
| NSS | PA4 |

### I2S Connection
| Signal | STM32 | I2S Module |
| ---- | ---- | ---- |
| SD | PB15 | DIN |
| BCLK | PB13 | BCLK |
| WS(LRC) | PB12 | WS |

## How to edit this project with STM32CubeIDE
Create a STM32 project with .ioc

### Please add the following directories and files using STM32CubeMX etc.
- Driver/CMSIS/
- Driver/CMSIS-DSP/
- Driver/STM32XXxx_HAL_Driver/

More information will be added.
