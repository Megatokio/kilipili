# kilipili
Kio's Little Pico Library

A library for video, audio and input devices for the RP2040.

**This is work in progress**  
**current state:**  
- Video output, graphics and USB keyboard and mouse work as far as tested.
- The video engine displays up to 1280x768 pixel in 16 bit true color¹.  
  e.g. 1024x768 with true color attribute mode uses only 132kB of Ram, leaving over 100kB for the application.
- I am currently fixing bugs. Please checkout the current `work` branch. (`work_2023_11` as of writing)  

Anybody who runs into a bug is welcome to file a bug report or a merge request.

## A C++ library for:
- graphics
	- true color and indexed color
	- direct color and attribute modes for colorful graphics with little memory
	- drawing primitives
	- PicoTerm terminal	
- video
	- originally based on Scanvideo
	- runs entirely on core 1
	- cpu usage varies widely
	- runs entirely in RAM (at least it should. to be tested)
	- can be stopped and restarted in different video mode
	- screen size 160\*120 up to 1280\*768 pixel (or what you define)
	- true color and indexed color 
	- direct color and attribute modes 
	- tiled modes **TODO**
	- **sprites** and mouse pointer in separate plane (no longer supported)
	- framebuffer, sprites and mouse pointer rendered into single scanline buffer **TODO**
	- pixel clock and display enable signals supported (untested)
- audio
	- **TODO**
- USB host mode
	- uses usb tiny host 
	- Keyboard
	- Mouse (API not yet fixed)
	- game pad **TODO**
	- usb stick **TODO**
- and more
	- set system clock
	- cpu load sensor
	- **malloc** replacement which doesn't fail to return available memory
	- SDcard **TODO**

¹) of course there is a catch :-)

## Advantages (VGA Video)

- start-and-forget.
- low RAM usage.
- colorful display in high resolutions.
- runs in RAM: no need to lockout core 1 while writing to the internal flash.
- can be stopped and restarted in different video mode.

## Disadvantages

- this is work in progress
- uses CPU core 1, 3 DMA channels and 2 state machines in PIO 1.
- some video modes require excessive high system clock in higher screen resolutions.
- pixel clocks restricted to full MHz.
- system clock must be a multiple of the pixel clock.

## Example

*Full working example without the cmake stuff.*

```cpp
main()
{
// VGA Video 800 x 600 pixel
// with 8x12 pixel true color attributes (character size used in PicoTerm)

#include "kilipili/Graphics/PicoTerm.h"
#include "kilipili/Graphics/Pixmap_wAttr.h"
#include "kilipili/Video/FrameBuffer.h"
#include "kilipili/Video/VideoController.h"
#include "pico/stdio.h"
#include <tusb.h>

int main()
{
    using namespace kio::Video;

    stdio_init_all();
    tuh_init(BOARD_TUH_RHPORT);

    auto* pixmap          = new Pixmap<colormode_a1w8_rgb>(800, 600, attrheight_12px);
    auto* framebuffer     = new FrameBuffer<colormode_a1w8_rgb>(*pixmap, nullptr);
    auto& videocontroller = VideoController::getRef();
    videocontroller.addPlane(framebuffer);
    videocontroller.startVideo(vga_mode_800x600_60);

    PicoTerm* picoterm = new PicoTerm(*pixmap);
    picoterm->cls();
    picoterm->printf("%s\n", picoterm->identify());

    for (;;) {} // your main action
}
```


