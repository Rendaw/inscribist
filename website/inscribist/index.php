<?xml version="1.0" encoding="UTF-8"?>
<?php

// Start up the database interface
require_once "../zarbonet.php";
$ZarboMS = new Zarbonet;

// Get our dataZ
$Project = $ZarboMS->GetRow('projects', array('serial' => 3));

?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<!-- Copyright 2009 Zarbosoft -->
<!-- Awesome that you listen music to browsing while we recommend this site. -->
<html xmlns="http://www.w3.org/1999/xhtml">

<head>
	<title>Inscribist</title>
	<link rel="stylesheet" type="text/css" href="style.css" />
</head>
<body>
<div id="body">
	<table id="header"><tr>
		<td><img src="logo.png" alt="inscribblenauts" /></td>
		<td><h1><?php echo $Project['name']; ?></h1></td>
	</tr></table>
	
	<div id="showcase">
		<img src="rendaw_fishing.jpg" alt="Rendaw - Fear Fishing"/>
		<span>Fear Fishing by <a href="/rendaw">Rendaw</a></span>
	</div>
	<ul id="links">
		<li><a href="<?php echo $Project['source']; ?>">Source</a></li>
		<li><a href="<?php echo $Project['win32']; ?>">Win32</a></li>
		<li><a href="http://zarbosoft.46.forumer.com/viewforum.php?f=3">Forums</a></li>
	</ul>
	
	<table id="content"><tr>
		<td>
			<a class="screenshot" href="screenshot1.jpg"><img src="screenshot1-thumb.jpg" alt="The main window." /></a>
			<a class="screenshot" href="screenshot2.jpg"><img src="screenshot2-thumb.jpg" alt="The non-main window." /></a>
		</td>
		<td>
			<p><strong>Inscribist</strong> is a flexible two-color drawing program.  Even on older hardware, it can smoothly handle huge images (10000x10000 pixels and more) and brushes (1000 pixel radius) with no latency.  Inscribist also features simple, efficient, and intuitive user interface.</p>	
			<h2>Concepts</h2>
			<ul>
				<li><strong>Colors</strong>: Each image has a "foreground" and "background" color.  Because the pixels in the image are encoded as run-lengths of the foreground and background color, you can easily swap in new colors at any time.  Inscribist also allows you to specify different colors for exporting and drawing.</li>
				<li><strong>Brushes</strong>: Inscribist has ten configurable brushes, bound to the number keys on the keyboard.  Each brush has a color (either the foreground or background color) and a radius.</li>
				<li><strong>Devices</strong>: Each input device (mouse, stylus, trackpad, etc) is associated with a brush.  When you select one with the number keys, Inscribist associates the device you're currently using with the new brush.</li>
				<li><strong>Over sampling</strong>: In order to get smoothed images, exporting and display must downsample the image.  If you view or export the image at 1:1 scale, you will be able to see hard pixel edges in the resultant file.  Although downsampling reduces the image size, Inscribist can handle immense image sizes easily, so just make your images larger.</li>
			</ul>
			<h2>Keyboard</h2>
			<ul>
				<li><strong>0-9</strong>: Select brush 0-9.</li>
				<li><strong>Tab</strong>: Switch the current brush between background and foreground color.</li>
				<li><strong>h</strong>: Flip the image horizontally.</li>
				<li><strong>v</strong>: Flip the image vertically.</li>
				<li><strong>Ctrl + s</strong>: Save (or save as, if you haven't yet saved).</li>
				<li><strong>Ctrl + Shift + s</strong>: Save as.</li>
				<li><strong>Ctrl + z, Ctrl + Shift + z</strong>: Undo, redo.</li>
				<li><strong>[, ]</strong>: Zoom in, zoom out (although I don't know which one's which).</li>
			</ul>
			<br />
			<p>Inscribist can both save and load .inscribble files (BZ2 compressed, run length encoded) and export .png files.</p>
			<br />
			<p>Inscribist requires Lua, GTK+ 2 and bzip2. Inscribist was inspired by gsumi.</p>
			<br />
			<p>The current revision is <?php echo $Project['version']; ?>, released <?php echo $Project['lastdate']; ?>.</p>
		</td>
	</tr></table>
	
	<div class="copyright clear">&copy; 2010 Zarbosoft</div>
	<div><a href="..">Back to Zarbosoft</a></div>
</div>
</body>

</html>
