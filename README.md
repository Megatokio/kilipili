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
- Audio output: 3 hardware options, real-time capable, AY and MP3 player
- USB input: Keyboard and Mouse
- Graphics: Printer and drawing primitives
- debugging support


## Other Highlights
- display true color images up to 600x400 pixels with a *HoldAndModifyVideoPlane* on a RP2040 with 256 kByte RAM. The *ham* images can be created using the RsrcFileWriter in *desktop_tools/*.
- write to internal flash without disrupting video output. 
- Mp3Decoder and an asynchronous Mp3Player using libmad


## Latest Additions
- AY-3-8912 sound chip emulation and *.ym* audio file playback. See Wiki page about *.ymm* files.
- graphics primitives: fill triangle, fill polygon, draw Bezier curve
- Mp3Decoder and an asynchronous Mp3Player 
- support SDCard with any arbitrary pin assignment, e.g. supports the vgaboard
- character-based FrameBuffers


## Video 
The video engine provides many pixel-based video modes with indexed or true color up to highest resolution by use of color attributes as well as character-based modes. It provides sprites and a mouse pointer.

- `[done]` pixel-based screen resolution 160\*120 up to **1280\*768** with **true color** attributes.  
- `[done]` Low RAM usage by use of attributes. =\> colorful display in high resolutions.  
  e.g. **1024 x 768** with 8 x 12 pixel **true color** attributes uses **only 132 kB of Ram** (with 16 bit hardware Color), leaving over 100 kB to the application.
- `[done]` a simple monochrome character mode and character modes with per-character attributes configurable by template parameters.
- `[done]` Mouse pointer. 
- `[done]` Switch between different video mode and screen resolutions at run-time.
- `[done]` video output not affected by flash-lockout while writing to the internal flash. 
- `[done]` Display true color images up to 600x400 pixels with a *HamImageVideoPlane* on a RP2040 with 256 kByte RAM. The *ham* images can be created using the RsrcFileWriter, built by *desktop_tools/CMakeLists.txt.* See Wiki page.
- `[test]` Sprites.
- `[test]` Tiled background.


## USB Host
The USB host mode currently supports keyboards and pointer devices (aka 'Mouse'). 

- `[done]` **USB keyboard** support. 
- `[done]` English and German **key translation tables**
- `[done]` **USB mouse** support
- `[todo]` more key translation tables


## Graphics
The Graphics engine supports _Pixmaps_ with many modes which are supported by the video engine. It provides a collection of drawing primitives. class _Printer_ can print to _CharMaps_ and _Pixmaps_.

- `[done]` **Direct pixel color modes**: 1, 2, 4, 8 indexed color and true color rgb.  
- `[done]` **Attribute modes**: The image is composed from a 1 bpp or 2 bpp Pixmap and low resolution true color attribute cells.  
  The attributes cells normally match the character cell size which is normally 8 x 12 pixels.  
  By using templates all graphics functions are supported in all modes.  
- `[done]` **Graphics primitives**, e.g.: line, circle, polyline, bezier, filled circle, triangle, polygon, flood fill, draw bmp.
- `[done]` **Text output** via class Printer.


## Audio
The audio sub system supports _I2S_, _PWM_ and _SigmaDelta_ audio with 1 or 2 channels.  
The _latency_ of the audio system is very low, approx. 5ms with default settings and can be configured down to 1ms.  
Various AudioAdapters are provided, e.g. a _SampleRateAdapter_ if your audio source can't match the hardware sample rate directly or _MakeMonoAdapter_ and _MakeStereoAdapter_ to convert between Mono and Stereo. Audio adapters add no latency.  
The _memory footprint_ is very low: default 2kB static memory for PWM stereo, 1kB for I2S or mono, can be configured down to 64 frames (à 4 or 8 bytes).  
The _fillAudioBuffer()_ function can be called on timer interrupt or directly in the applications event loop.  
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
Lib _kilipili_ provides a compressed resource file system with files linked into the binary and written to the program flash. This can be compiled using the _RsrcFileWriter_ in 'desktop_tools/'. See below. The RsrcFileWriter creates a file `rsrc.cpp` which must be linked into the program. See Wiki page.  
The SDcard Interface accesses the SD card using the SPI interface or, if needed, using a PIO emulation.
- `[done]` Preferences at the end of program flash
- `[done]` Compressed resource file system linked into the binary
- `[done]` Flash file system
- `[done]` Read SDCard in SPI mode, either with normal SPI or PIO
- `[done]` FAT file system 
- `[done]` BlockDevice for the internal program flash
- `[done]` Write to SDCard 


## Other
- `[done]` **Dispatcher**: manages a list of scheduled one-time or repeated events. You can convert interrupts, e.g. timer interrupts into events for the dispatcher to execute them synchronously with much less hazzle to synchronize access to global data. The YMMusicPlayer and Mp3Player can be run on dispatcher callbacks.
- `[done]` **malloc** replacement which doesn't fail to return available memory. It also provides many additional tools for support debugging, e.g. hexdump the heap.
- `[done]` cpu **load sensor**


## Desktop Tools
There are some tools for the desktop (not run on the Pico) which are built by the *CMakeLists.txt* in directory *desktop_tools/*. If you want to convert images to native color depth of your board then make sure to add `-DPICO_BOARD=your_board` to the cmake command line (or set via cmake variable).  
The following utilities are built by *desktop_tools/CmakeLists.txt*:
- `[done]` **UnitTest**
- `[done]` **RsrcFileWriter**: read and convert a directory for use with the resource file system or to write to an SDCard. See Wiki page.


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
#include <kilipili.h>
#include <stdio.h>

// VGA Video 800 x 600 pixel
// with 8x12 pixel true color attributes (the size of characters)

int main()
{
	using namespace kilipili;

	stdio_init_all();
	printf("Hello world\n"); // print to stdio
	USB::initUSBHost();

	RCPtr pixmap	  = new Pixmap_a1w8(800, 600);
	RCPtr framebuffer = new FrameBuffer(pixmap);
	RCPtr printer	  = new Printer(pixmap);

	addVideoPlane(framebuffer);
	startVideo(vga_mode_800x600_60);

	printer->cls();
	printer->printf("Hello world ... %s mode\n", debug ? "DEBUG" : "RELEASE"); // print on video screen
	printer->printf("%s\n", printer->identification());                        // print on video screen
	pixmap->drawRect(100, 50, 400, 300, red);                                  // draw on video screen

	Dispatcher::addHandler(&blinkOnboardLed);
	Dispatcher::addHandler(&USB::pollUSB);

	for (;;)
	{
		int c = stdio_getchar_timeout_us(0); // get input from stdin (UART)
		if (c < 0) c = USB::getChar();       // get input from usb keyboard
		if (c == '\r') c = '\n';
		if (c == uchar(c)) printer->putc(char(c));

		Dispatcher::run(1000); // run the registered Dispatcher handlers
		purge_tempmem();       // house keeping
	}
}
```


