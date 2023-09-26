# kilipili
Kio's Little Pico / RC2040 Library

A library for video, audio and input devices.

**This is work in progress**  
**current state:**  
Video output, graphics and USB keyboard and mouse work as far as tested.  
I am currently fixing bugs. please checkout the current `work` branch.  
Anybody who runs into a bug is welcome to file a bug report or a merge request.

## A C++ library for:
- graphics
	- true color and indexed color
	- direct color and attribute modes for colorful graphics with little memory
	- drawing primitives
	- PicoTerm terminal	
- video
	- based on Scanvideo
	- runs on core 1
	- cpu usage varies widely
	- runs in RAM - no need to lockout core 1 when writing to internal flash
	- can be stopped and restarted in different mode
	- screen size 320\*240 to 1024\*768
	- all true color and indexed color 
	- all direct color and attribute modes 
	- tiled modes TODO
	- 'hardware' **sprites** and mouse pointer in separate plane
	- sprites and mouse pointer in same plane as framebuffer TODO
- audio
	- TODO
- USB host mode
	- uses usb tiny host
	- Keyboard
	- Mouse
	- game pad TODO
	- usb stick TODO
- and more
	- set system clock
	- cpu load sensor
	- **malloc** replacement which doesn't fail to return available memory
	- SDcard TODO

## Example

```cpp
main()
{
   // 800 x 600 pixel VGA video
   // using 8x12 pixel true color attributes (best for PicoTerm with 8x12 pixel characters)
   
   stdio_init_all();
   setSystemClock(200 MHz); 
   loadsensor.start();
   tusb_init();
   auto* pixmap = new Pixmap<colormode_a1w8_rgb>(800,600,attrheight_12px);
   auto* framebuffer = new FrameBuffer(pixmap, nullptr);
   auto& videocontroller = VideoController::getRef();
   videocontroller.setup(vga_mode[vga_mode_800x600_60], vga_timing[vga_mode_800x600_60]);
   videocontroller.addPlane(framebuffer);
   videocontroller.startVideo();
   for(;;); // your main action
}
```


