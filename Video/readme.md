# Directory **Video**

contains a VGA video driver.
It is based on the original pico-playground/scanvideo project.

It can display the 3 basic VGA resolutions and the half-sizes thereof.

It can display all *Pixmap*s and *Pixmap_wAttr*s, though some modes are heavily 
cpu intensive and are only possible with the lower resolutions. Of course direct color modes 
other than 1 or 2bpp need a lot of ram and are thereby limited to low resolutions as well.

It can 3 planes, as the scanvideo driver did, and supports 'hardware' sprites and a mouse pointer which use one plane. 
These will sometime be complemented by 'software' sprites, which need no extra plane, but can't mix with tiled screen modes.

Tiled screen modes are not yet supported (there is no scanline render function defined yet), but tiled mode will come. 
The FrameBuffer for tiled modes cannot be based on Pixmap/_wAttr. How exactly it fits in we will see.

I am currently also trying hard to display 1024*768 with attributes (a1w8 rgb) to get a colorful 1k display. 
But even if overclocking the pico to 4 times the pixel clock of 1024*768, which is 65MHz, 
i need to find a way to cramp pixel fetch, lookup and store into 4 clock cycles per pixel. This is still a challenge to me.






