<?php
require "settings.php" // Generated with updated filenames and stuff.
?><!doctype html>
<html lang="en">
<head>
	<!-- Copyright 2012 Zarbosoft -->
	<!-- Awesome that you listen music to browsing while we recommend this site. -->
	<meta charset="utf-8">
	<title>Inscribist</title>
	<link rel="stylesheet" type="text/css" href="style.css" />
</head>
<body>
<div id="body">
	<table id="header"><tr>
		<td><img src="logo.png" alt="inscribblenauts" /></td>
		<td><h1>Inscribist</h1></td>
	</tr></table>
	
	<div id="showcase">
		<img src="rendaw_fishing.jpg" alt="Rendaw - Fear Fishing"/>
		<span>Fear Fishing by <a href="/rendaw">Rendaw</a></span>
	</div>
	<ul id="links">
		<li><a href="<?php echo $SourcePackageFilename; ?>">Source</a></li>
		<li><a href="inscribist-4-win32.zip">Win32</a></li>
		<li><a href="http://http://zarbosoft.com/forum/index.php?board=8.0">Forums</a></li>
	</ul>
	
	<table id="content"><tr>
		<td>
			<a class="screenshot" href="screenshot1.jpg"><img src="screenshot1-thumb.jpg" alt="The main window." /></a>
			<a class="screenshot" href="screenshot2.jpg"><img src="screenshot2-thumb.jpg" alt="The non-main window." /></a>
		</td>
		<td>
			<p><strong>Inscribist</strong> is an efficient two-color drawing program.  Even on older hardware, it can smoothly handle huge images (10000x10000 pixels and more) and brushes (1000 pixel radius) with unnoticable latency.</p>	
			<h2>Concepts</h2>
			<ul>
				<li><strong>Colors</strong>: Each image has a "foreground" and "background" color.  Because the pixels in the image are encoded as run-lengths of the foreground and background color, you can easily replace the colors at any time.  Inscribist also allows you to specify different color settings for exporting and drawing.</li>
				<li><strong>Over sampling</strong>: Because Inscribist only uses two colors, it can't antialias your strokes.  In order to get smoothed images, exporting and display downsample the image by a configurable amount.  The resultant resolution will be smaller than your working resolution, so plan ahead and work at a much larger resolution than you think you'll need.</li>
				<li><strong>Brushes</strong>: Brushes use either the foreground or background color and have a radius.  Inscribist remembers ten brush configurations.</li>
				<li><strong>Devices</strong>: Inscribist recognizes a number of input devices, including mice, drawing styluses, trackpads, etc.  Associate a brush with your current drawing device by pressing the brush number on your keyboard.</li>
			</ul>
			<h2>Keyboard</h2>
			<ul>
				<li><strong>0-9</strong>: Select brush 0-9.</li>
				<li><strong>Tab</strong>: Switch between background and foreground color.</li>
				<li><strong>h</strong>: Flip the image horizontally.</li>
				<li><strong>v</strong>: Flip the image vertically.</li>
				<li><strong>Ctrl + s</strong>: Save (or save as, if you haven't yet saved).</li>
				<li><strong>Ctrl + Shift + s</strong>: Save as.</li>
				<li><strong>Ctrl + z, Ctrl + Shift + z</strong>: Undo, redo.</li>
				<li><strong>[, ]</strong>: Zoom in, zoom out (or maybe that's backwards).</li>
			</ul>
			<br />
			<p>Inscribist can both save and load .inscribble files (BZ2 compressed, run length encoded) and export .png files.</p>
			<br />
			<p>Inscribist requires Lua, GTK+ 2 and bzip2. Inscribist was inspired by <a href="http://fishsoup.net/software/gsumi/">gsumi</a>.</p>
			<br />
			<p>The current revision is dated <?php echo $SourcePackageDate; ?>.</p>
		</td>
	</tr></table>
	
	<div class="copyright clear">&copy; 2012 Zarbosoft</div>
	<div><a href="..">Back to Zarbosoft</a></div>
</div>
</body>

</html>
