Inscribist revision 4

Inscribist is a flexible two-color drawing program.  Even on older hardware, it can smoothly handle huge images (10000x10000 pixels and more) and brushes (1000 pixel radius) with no latency.  It also has a simple, efficient, and intuitive user interface.

Concepts
	* Colors: Each image has a "foreground" and "background" color.  Because the pixels in the image are encoded as run-lengths of each the foreground and background color, you can switch colors at any time.  Inscribist also allows you to specify different colors for exporting and working.  Because there are only two colors, images take up little storage space.
	* Brushes: Inscribist has ten configurable brushes, bound to the number keys on the keyboard.  Each brush has a color (either the foreground or background color) and a radius.
	* Devices: Each input device (mouse, stylus, trackpad, etc) is associated with a brush.  When you select a brush with the number keys, it associates the device you're currently using with the new brush.
	* Over sampling: In order to get smoothed images, exporting and display must downsample time image.  If you view or export the image at 1:1 scale, you will be able to see hard pixel edges in the resultant file.  Although downscaling reduces the image size, Inscribist can handle immense image sizes easily, so just make your images larger.

Inscribist requires Lua, GTK+ 2 and bzip2. Inscribist was inspired by gsumi.

Installation
1. Tweak variables in the Makefile (the makefile defaults to installing into /usr/local/bin)
2. Run "make" to compile the application.
3. Run "sudo make install" to copy the files into system directories.
4. Assuming the install location is in your path, you can now start Inscribist by typing "inscribist" on the command line.

URL: www.zarbosoft.com/inscribist
Use the forums for support, bug reports, etc.
