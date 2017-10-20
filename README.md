# Chip8 Virtual Machine Emulator

Supports all opcodes in the [chip8 instruction set](https://en.wikipedia.org/wiki/CHIP-8#Opcode_table)

Emulates all chip8 features, including:
+ 16 8-bit CPU registers (16th is used as carry flag)
+ 4096 bytes of RAM
+ A call stack (max 16 stack frames)
+ 64x32 pixel black and white graphics buffer


## Dependencies
+ SDL2 (for graphics and input)
+ C++14 compliant compiler ('14 for binary literals)

It has only been tested on OS X, but it should also compile and run perfectly on Linux. I don't think anything besides the makefile is platform-dependent, so if you can build it on Windows it should run there too


## FAQ

**How do I use it?**  
Run `make` to build, then run `./main.out path/to/chip8_rom` to run a chip8 executable!


**What do you even run on chip8? Isn't that from the 70s?**  
Some fun retro chip8 games can be found [here](http://www.pong-story.com/chip8/) courtesy of David Winter - my favorites are blitz and blinky


**Why do some of those games flicker?**  
It's supposed to; chip8 programs have no way to control when the display is refreshed so it must be refreshed every time there is a change made to the graphics buffer


**It's running too fast/slow**  
Somewhat oddly, there's no standard for how many instructions the chip8 virtual machine executes per second, so I found a number that made the games listed above run at a reasonable speed (500 instructions per second). If you want to mess with it, change the `hz` variable defined in main.cpp


**How do I press buttons?**  
chip8 is designed to be used with a 4x4 keypad, which I've mapped to the keyboard (as shown below). However, there's no standard defining what each keypad button does, so you're going to have to hit them all in each application you run to figure it out

```
Keypad                 Keyboard
1|2|3|C       -->      1|2|3|4
4|5|6|D       -->      Q|W|E|R
7|8|9|E       -->      A|S|D|F
A|0|B|F       -->      Z|X|C|V
```
