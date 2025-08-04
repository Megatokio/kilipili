# kilipili
*Kio's Little Pico Library*

*A c++ library for video, audio and input devices for the RP2040.*
*If you run into a problem please file a bug report or a merge request.*

## Cheap Summary
Lib *kilipili* provides the tools for using video and audio on a Raspberry Pi Pico or any other board with a RP2040 microcontroller.
Very coarsely it provides:
- a Dispatcher
- a FileSystem: preferences, resource FS in flash, FlashFS, SDCard
- Video output: a multitude of hardware configurations, video formats and options
- Audio output: 3 hardware options (I2S, PWM and SigmaDelta), real-time capable
- USB input: Keyboard and Mouse
- Graphics: TextVDU and drawing primitives
- debugging support


## Latest Additions
- Display true color images up to 600x400 pixels with a *HoldAndModifyVideoPlane* on a RP2040 with 256 kByte RAM. The *ham* images can be created using the RsrcFileWriter, built by desktop_tools/CMakeLists.txt. See Wiki page.
- AY-3-8912 sound chip emulation and *.ym* audio file playback. See Wiki page about *.ymm* files.
- write to internal flash without disrupting video output. 


## Video 
The video engine provides many video modes with indexed or true color up to highest resolution by use of attributes. It provides sprites and a mouse pointer.

- `[done]` Screen resolution 160\*120 up to **1280\*768** in **16 bit true color**.  
- `[done]` Low RAM usage by use of attributes. =\> colorful display in high resolutions.  
  e.g. **1024 x 768** with 8 x 12 pixel **true color** attributes uses **only 132 kB of Ram**, leaving over 100 kB to the application.
- `[done]` Mouse pointer. 
- `[done]` Switch between different video mode and screen resolutions.
- `[done]` video output not affected by flash-lockout for writing to the internal flash. 
- `[done]` Display true color images up to 600x400 pixels with a *HamImageVideoPlane* on a RP2040 with 256 kByte RAM. The *ham* images can be created using the RsrcFileWriter, built by desktop_tools/CMakeLists.txt. See Wiki page.
- `[test]` Sprites.
- `[test]` Tiled background.


## USB Host
The USB host mode currently supports keyboards and pointer devices (aka 'Mouse'). 

- `[done]` USB keyboard support. 
- `[done]` English and German key translation tables
- `[done]` USB mouse support
- `[todo]` more key translation tables


## Graphics
The Graphics engine supports pixmaps with many modes which are supported by the video engine. It provides some graphics primitives, but here is some work to go. Does someone want to help?

- `[done]` **Direct pixel color modes**: 1, 2, 4, 8 and 16 bpp.  
  16 bpp is normally true color while i1 .. i8 are normally indexed color.
- `[done]` **Attribute modes**: The image is composed from a 1 bpp or 2 bpp pixmap and lower resolution color cells.  
  The attributes cells normally match the character cell size which is normally 8 x 12 pixels.  
  By using templates all graphics functions are supported in all modes.  
  Supported attribute modes:
  - 1 or 2 bit per pixel in the pixmap selecting from 2 or 4 colors in the corresponding attribute cell
  - 1, 2, 4 or 8 pixel wide attributes
  - attribute cell height from 1 to 99
  - 4, 8 and 16 bit colors in the attributes, where i4 and i8 are normally indexed colors  
- `[done]` Graphics primitives
- `[done]` Text output
- `[todo]` Graphics still lack some functionality.


## Ansi Terminal
The Graphics engine contains a class which provides a wannabe-ANSI-compliant text terminal. See `Graphics/ANSITerm` for feature list.  
The ANSITerm can be used with any Pixmap size and should be used with colormode_a1w8_rgb for the colors. The number of actual colors displayed depends on your video hardware setup.  
- `[done]` color (xterm compatible)
- `[done]` mouse pointer support
- `[done]` horizontal and vertical scroll region
- `[done]` bold, inverse, italic, underlined, double width and double height 
- `[done]` Latin-1 character set, one Graphics character set
- `[done]` optional utf-8 encoding
- `[done]` US and German keyboard (you are welcomed to add your nationality!)


## Audio
The audio sub system supports no audio and I2S and PWM audio with 1 or 2 channels.
If your audio source has a fixed sample rate which cannot be matched by the hardware (PWM most notably),
then you can wrap it in a SampleRateAdapter.  
Programs can be written indepentent of the actual number of hardware channels by wrapping audio sources in a MakeMonoAdapter or MakeStereoAdapter.  
The latency of the audio system is very low, approx. 5ms with default settings and can be configured down to 1ms.  
Audio adapters add no latency. (the SampleRateAdapter may add up to 2 samples latency.)  
The memory footprint is very low: default 2kB static memory for PWM stereo, 1kB for I2S or mono, can be configured down to 64 frames (à 4 or 8 bytes).  
The adapters allocate no static memory but a small buffer on the stack if needed (64 frames à 4 or 8 bytes). This should be considered for the overall stack usage, especially when audio is filled up using a timer interrupt.  
The `fillBuffer()` function can be called on timer interrupt or manually, e.g. in the applications event loop.
The audio interface automatically adjusts to a changed system clock when switching video modes.  
- `[done]` PWM mono and stereo output
- `[done]` I2S stereo output
- `[test]` Sigma-delta mono and stereo output
- `[done]` no audio and simple beeper
- `[done]` sinus and square wave generators
- `[done]` sample rate adapter, misc. mono <-> stereo adapters
- `[done]` low latency of as little as 1ms
- `[done]` AY-3-8912 sound chip emulation and *.ym* audio file playback. See Wiki page about *.ymm* files.


## Files and Devices
Lib kilipili provides a compressed resource file system with files linked into the binary and written to the program flash. This can be compiled using the RsrcFileWriter in the desktop_tools/ directory. See below. The RsrcFileWriter creates a file `rsrc.cpp` which must be linked into the program. See Wiki page.  
The SDcard Interface accesses the SD card via it's SPI interface.  

- `[done]` Preferences at the end of program flash
- `[done]` Compressed resource file system linked into the binary
- `[done]` Flash file system
- `[done]` Read SDCard in SPI mode 
- `[done]` FAT file system 
- `[done]` BlockDevice for the internal program flash
- `[todo]` Write to SDCard 


## Other
- `[done]` Dispatcher: manages a list of scheduled one-time or repeated events. You can convert interrupts, e.g. timer interrupts into events for the dispatcher to execute them synchronously with much less hazzle to synchronize access to global data.
- `[done]` **malloc** replacement which doesn't fail to return available memory. It also provides many additional tools to support debugging.
- `[done]` cpu load sensor


## Desktop Tools
There are some tools for the desktop (not run on the Pico) which are built by the *CMakeLists.txt* in directory *desktop_tools/*. If you want to convert images to native color depth of your board then make sure to add -DPICO_BOARD=your_board to the cmake command line.  
The following utilities are built by *desktop_tools/CmakeLists.txt*:
- `[done]` UnitTest
- `[done]` RsrcFileWriter: read and convert a directory for use with the resource file system or to write to an SDcard. See Wiki page.


## Resources & restrictions
- Uses CPU core 1, 3 DMA channels and 2 state machines in PIO 1 for video.  
  Up to 2 DMA channels and the other state machines are used for audio.
- Some video modes require excessive high system clock in high screen resolutions.
- Pixel clock restricted to full MHz.
- System clock must be a multiple of the pixel clock.


## Example

*Full working example without the cmake stuff.*

```cpp
// VGA Video 800 x 600 pixel
// with 8x12 pixel true color attributes (this is the character size used in TextVDU)

#include <kilipili.h>
#include <pico/stdlib.h>
#include <stdio.h>

int main()
{
	using namespace kio;
	using namespace kio::USB;
	using namespace kio::Video;
	using namespace kio::Graphics;

	stdio_init_all();
	printf("Hello world\n"); // print to stdio
	initUSBHost();

	auto* pixmap	  = new Pixmap<colormode_a1w8_rgb>(800, 600, attrheight_12px);
	auto* framebuffer = new FrameBuffer(pixmap, nullptr);
	addVideoPlane(framebuffer);
	startVideo(vga_mode_800x600_60);

	TextVDU* textvdu = new TextVDU(pixmap);
	textvdu->cls();
	textvdu->printf("Hello world ... %s mode\n", debug ? "DEBUG" : "RELEASE"); // print on video screen
	textvdu->identify();													   // print on video screen
	pixmap->drawRect(100, 50, 400, 300, red, 1);							   // draw on video screen

	Dispatcher::addHandler(&blinkOnboardLed);
	Dispatcher::addHandler(&pollUSB);

	for (;;)
	{
		Dispatcher::run(1000); // blink the LED
	}
}
```


