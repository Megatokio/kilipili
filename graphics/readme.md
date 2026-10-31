#Directory **Graphics/**

contains hardware independent graphics classes and functions.

Class *Canvas* defines an interface for subclasses on which we can draw on.
It provides (slow) default implementations for most of it's virtual functions.

Class *Canvas* provides graphics primitives for drawing lines, rectangles, circles and polygons, 
pixmaps, bitmaps, and character glyphs.

Template class *Pixmap* defines drawables which use a pixel buffer with direct pixels. 
Direct pixels are either true color rgb or indexed color. ' direct' means 
that each pixel in the pixel buffer directly defines the pixel's color. 
'indexed color' is not viewed as 'indirect' because the Pixmap knows nothing about colors. 
There are just pixels with, depending on template parameter 'ColorDepth', 
are 1, 2, 4, 8 or 16 bits in size.

Template class *Pixmap_wAttr* defines drawables which use a pixel buffer and color attributes.
Attribute cells are lower resolution than pixels, e.g. 8*8 pixels, and may match character width and size of text used.
The pixels in the pixel buffer are 1 or 2 bits which selects the color from the attribute cell which the pixel is in.
Thus each attribute has 2 or 4 colors.

The advantage of this complicated aproach is that you can have colorful graphics with lower memory usage. 
The disadvantage is the limitation of the number of colors per attribute cell and the additional work.
Who is old enough to know the Sinclair ZX Spectrum knows exactly how it works, and all about 'color clash'.

*BitBlit* header and cpp files define functions and templates to do most of the raw drawing.

Class *PicoTerm* is a simple terminal program for drawing text on a *Cancas* and is for used by the application. 
It uses a 8*12 pixel font by default and if used with a *Pixmap_wAttr* the attribute cell size should be chosen accordingly.



