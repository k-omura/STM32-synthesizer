# STM32 Digital Synthesizer
Virtual Analog Synthesizer General-MIDI(GM) Sound Source

## Digital synthesizer project with STM32.
The purpose of this project is to make a sound just by creating a simple synthesizer controller.  
The USB-MIDI standard or MIDI Data format over SPI allows to utilize existing equipment and knowledge.  
Hopefully there will be lots of innovative UI ideas for music.  
A binary file is prepared for the controller designer, so download it and write with STM32CubeProg etc.  

## Development environment
- [STM32G431CBU6 Module](https://github.com/WeActStudio/WeActStudio.STM32G431CoreBoard)
- I2S Sound Module. [for example this](https://www.adafruit.com/product/3006)

## Basic features
- Polyphonic (In actual use, a maximum of 12 notes).
- MIDI data format.
- Designed to be operated from the outside via SPI or USB.
- 16ch (GM 128 Instruments, 47 Drum Sounds(10ch)).
- ADSR.
- Pitch bend.
- Envelope
- Filter that can manipulate the cutoff frequency and Q value.
- LFO(Tremolo, Vibrato, WOW).
- Reverb.
- Pan.
- Create custom instruments setting. ((Sine, Saw, SQU) x2 /Channel)
- I2S Stereo output.
- Firmware Update via USB. (Updates are available via [PC Control App for Google Chrome](https://k-omura.github.io/STM32-synthesizer/))

## Future features
- What you want.

## Known issue(s)
- The number of simultaneous notes that can be played depends on the settings, but it is around 12 notes. If this number is exceeded, the unit may stop working.
- Noise occurs at low volume.

## Demo
- [Introduction of each function](https://www.youtube.com/watch?v=JlslueGmlfo)
- [Drum Rhythm Machine - Projects using this synthesizer](https://www.youtube.com/watch?v=DubTo31tJ50)
- [Accordion - Projects using this synthesizer](https://www.youtube.com/watch?v=ZY-Zua1G7Z8)
- Old Version
  - [Waveform creation, low-pass filter demo](https://youtu.be/SDA9uaBMBQ4)
  - [SPI control demo](https://youtu.be/EjWuWOQzq90)
  - [Auto performance (Super Mario)](https://youtu.be/yB0PNu2G10Q)
  - [Introduction of creating sounds](https://youtu.be/cMSFZtEUxo4)

## Hot to use
### I2S Connection
| Signal | STM32 | I2S Module |
| :-: | :-: | :-: |
| SD | PB15 | DIN |
| BCLK | PB13 | BCLK |
| WS(LRC) | PB12 | WS |

### with USB-MIDI
1. Write the binary and connect your controller with STM32CubeProg.
2. Connect I2S Module and Speaker.
3. Connect PC and STM32 via USB and send MIDI data using MIDI software on PC.
4. Adjust parameters, experiment, and play MIDI files. [PC Control App for Google Chrome](https://k-omura.github.io/STM32-synthesizer/).  

#### Note: Because this operates as a USB-MIDI device, you cannot directly connect a USB MIDI keyboard to the STM32, as the USB MIDI keyboard must be connected to a USB-MIDI host.  

### with MIDI over SPI
1. Write the binary and connect your controller with STM32CubeProg.
2. Connect I2S Module and Speaker.
3. Connect Host and STM32 via SPI and send MIDI data over SPI from host.
#### Note: STM32' SPI mode is slave.

#### SPI Connection
| Signal | STM32 |
| :-: | :-: |
| MOSI | PA7 |
| MISO | PA6 |
| SCK | PA5 |
| NSS | PA4 |

#### SPI Data Format(If Message != Control Change)
| Byte | Field |
| :-: | :-- |
| 1 | Cable Number(4bit) / CIndex (4bit)|
| 2 | Message(4bit) / Channel(4bit)|
| 3 | Value1 of Message |
| 4 | Value2 of Message |

#### SPI Data Format(If Message == Control Change)
| Byte | Field |
| :-: | :-- |
| 1 | Cable Number(4bit) / CIndex (4bit)|
| 2 | Message(4bit) / Channel(4bit)|
| 3 | Control Change |
| 4 | Value of Control Change |

#### Note: Commands can be sent using USB and SPI simultaneously.  

## How to edit this project with STM32CubeIDE
1. Create a STM32 project with .ioc.  
2. Add the synthesizer driver and USB-MIDI driver to the tree.
3. Add CMSIS-DSP.  

### Please add the following directories and files using STM32CubeMX etc.
- Driver/CMSIS/
- Driver/CMSIS-DSP/
- Driver/STM32XXxx_HAL_Driver/

#### Note: Can be ported to any microcontroller that has CMSIS DSP available.  

More information will be added.
