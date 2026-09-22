# kilipili
*Kio's Little Pico Library*

*A c++ library for video, audio and input devices for the RP2040.*
*If you run into a problem please file a bug report or a merge request.*

## Cheap Summary
Lib *kilipili* provides the tools for using video and audio on a Raspberry Pi Pico or any other board with a RP2040 microcontroller.
Very coarsely it provides:
- a cooperative Dispatcher
- misc. FileSystems: SDCard, Flash FS, preferences, resource FS (compiled into the program)
- Video output: a multitude of hardware configurations, video formats and options
- Audio output: 3 hardware options (I2S, PWM and SigmaDelta), real-time capable, AY and MP3 player
- USB input: Keyboard and Mouse
- Graphics: TextVDU and drawing primitives
- debugging support


## Other Highlights
- Display true color images up to 600x400 pixels with a *HoldAndModifyVideoPlane* on a RP2040 with 256 kByte RAM. The *ham* images can be created using the RsrcFileWriter in *desktop_tools/*.
- write to internal flash without disrupting video output. 
- Mp3Decoder and an asynchronous Mp3Player using libmad


## Latest Additions
- AY-3-8912 sound chip emulation and *.ym* audio file playback. See Wiki page about *.ymm* files.
- Graphics primitives: fill triangle, fill polygon, draw Bezier curve
- Mp3Decoder and an asynchronous Mp3Player 
- support SDCard with any arbitrary pin assignment, e.g. supports the vgaboard


## Video 
The video engine provides many video modes with indexed or true color up to highest resolution by use of color attributes. It provides sprites and a mouse pointer.

- `[done]` Screen resolution 160\*120 up to **1280\*768** with **16 bit true color** attributes.  
- `[done]` Low RAM usage by use of attributes. =\> colorful display in high resolutions.  
  e.g. **1024 x 768** with 8 x 12 pixel **true color** attributes uses **only 132 kB of Ram**, leaving over 100 kB to the application.
- `[done]` Mouse pointer. 
- `[done]` Switch between different video mode and screen resolutions at run-time.
- `[done]` video output not affected by flash-lockout while writing to the internal flash. 
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
The Graphics engine supports pixmaps with many modes which are supported by the video engine. It provides a collection of drawing primitives.

- `[done]` **Direct pixel color modes**: 1, 2, 4, 8 and 16 bpp.  
  16 bpp is normally true color while i1 .. i8 are normally indexed color.
- `[done]` **Attribute modes**: The image is composed from a 1 bpp or 2 bpp pixmap and low resolution true color attribute cells.  
  The attributes cells normally match the character cell size which is normally 8 x 12 pixels.  
  By using templates all graphics functions are supported in all modes.  
- `[done]` Graphics primitives, e.g.: line, circle, polyline, bezier, filled circle, triangle, polygon, flood fill, draw bmp
- `[done]` Text output


## Audio
The audio sub system supports I2S, PWM and SigmaDelta audio with 1 or 2 channels.  
A SampleRateAdapter if your audio source can't match the hardware sample rate directly.  
A MakeMonoAdapter and a MakeStereoAdapter if your audio source has a different number of channels than the hardware.  
The latency of the audio system is very low, approx. 5ms with default settings and can be configured down to 1ms.  
Audio adapters add no latency. (the SampleRateAdapter may add up to 2 samples latency.)  
The memory footprint is very low: default 2kB static memory for PWM stereo, 1kB for I2S or mono, can be configured down to 64 frames (à 4 or 8 bytes).  
The adapters allocate no static memory but a small buffer on the stack if needed (64 frames à 4 or 8 bytes). This should be considered for the overall stack usage, especially when audio is filled up using a timer interrupt.  
The `fillBuffer()` function can be called on timer interrupt or manually, e.g. in the applications event loop.
The audio interface automatically adjusts to a changed system clock when switching video modes.  
- `[done]` PWM mono and stereo output
- `[done]` I2S stereo output
- `[done]` Sigma-delta mono and stereo output
- `[done]` no audio and a simple beeper
- `[done]` sinus and square wave generators
- `[done]` sample rate adapter, mono <-> stereo adapters
- `[done]` low latency of as little as 1ms
- `[done]` AY-3-8912 sound chip emulation and background capable YMMusicPlayer for playing AY sound files. See Wiki page about *.ymm* files
- `[done]` A background capable Mp3Player


## Files and Devices
Lib kilipili provides a compressed resource file system with files linked into the binary and written to the program flash. This can be compiled using the RsrcFileWriter in the desktop_tools/ directory. See below. The RsrcFileWriter creates a file `rsrc.cpp` which must be linked into the program. See Wiki page.  
The SDcard Interface accesses the SD card using the SPI interface.  

- `[done]` Preferences at the end of program flash
- `[done]` Compressed resource file system linked into the binary
- `[done]` Flash file system
- `[done]` Read SDCard in SPI mode, either with normal SPI or PIO
- `[done]` FAT file system 
- `[done]` BlockDevice for the internal program flash
- `[done]` Write to SDCard 


## Other
- `[done]` Dispatcher: manages a list of scheduled one-time or repeated events. You can convert interrupts, e.g. timer interrupts into events for the dispatcher to execute them synchronously with much less hazzle to synchronize access to global data. The YMMusicPlayer and Mp3Player can be run on dispatcher callbacks.
- `[done]` **malloc** replacement which doesn't fail to return available memory. It also provides many additional tools for support debugging.
- `[done]` cpu load sensor


## Desktop Tools
There are some tools for the desktop (not run on the Pico) which are built by the *CMakeLists.txt* in directory *desktop_tools/*. If you want to convert images to native color depth of your board then make sure to add -DPICO_BOARD=your_board to the cmake command line.  
The following utilities are built by *desktop_tools/CmakeLists.txt*:
- `[done]` UnitTest
- `[done]` RsrcFileWriter: read and convert a directory for use with the resource file system or to write to an SDCard. See Wiki page.


## Resources & restrictions
- Uses CPU core 1, 3 DMA channels and 2 state machines in PIO 0 for video.  
  Up to 2 DMA channels and the other state machines are used for audio.
- SDCard with PIO SPI interface uses one pio state machine.  
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
	using namespace kilipili;
	using namespace kilipili::USB;
	using namespace kilipili::Video;
	using namespace kilipili::Graphics;

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
		purge_tempmem();
	}
}
```


