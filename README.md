# kilipili
Kio's Little Pico Library

A c++ library for video, audio and input devices for the RP2040.

**This is work in progress**   
Anybody who runs into a bug is welcome to file a bug report or a merge request.

## Video 
- Video output fully functional and tested.
- start-and-forget.
- The video engine can display **up to 1280 x 768 pixel** in **16 bit true color**.  
- Low RAM usage by use of attributes. =\> colorful display in high resolutions.  
  e.g. **1024 x 768** with 8 x 12 pixel **true color** attributes uses **only 132 kB of Ram**, leaving over 100 kB to the application.
- runs entirely in RAM: stable video while writing to the internal flash possible.
- can be restarted in different video mode.
- automatically sets the required system clock.  
- Sprites **WIP**
- Tiled background mode **TODO**

## USB Host
- USB keyboard support. 
- USB mouse support. 

## Graphics
- **Direct pixel color modes**: 1, 2, 4, 8 and 16 bpp.  
  16 bpp is normally true color while i1 .. i8 are normally indexed color.
- **Attribute modes**: The image is composed from a 1 bpp or 2 bpp pixmap and lower resolution color cells.  
  The attributes cells normally match the character cell size which is normally 8 x 12 pixels.  
  By using templates all graphics functions are supported in all modes.  
  Supported attribute modes:
  - 1 or 2 bit per pixel in the pixmap selecting from 2 or 4 colors in the corresponding attribute cell
  - 1, 2, 4 or 8 pixel wide attributes
  - attribute cell height from 1 to 99
  - 4, 8 and 16 bit colors in the attributes, where i4 and i8 are normally indexed colors  
- Graphics primitives
- Text output
- Graphics still lack some functionality.

## SDCard support
- Access SC card in SPI mode
- FAT file system support **WIP**

## Other
- cpu load sensor
- **malloc** replacement which doesn't fail to return available memory
- Audio: TODO

## Resources & restrictions
- Uses CPU core 1, 3 DMA channels and 2 state machines in PIO 1.  
  (The 2 other state machines will probably be used by audio)
- Some video modes require excessive high system clock in high screen resolutions.
- Pixel clock restricted to full MHz.
- System clock must be a multiple of the pixel clock.

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


