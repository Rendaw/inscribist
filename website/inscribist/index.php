<?php
require "settings.php" // Generated with updated filenames and stuff.
?><!doctype html>
<html lang="en">
<head>
	<!-- Copyright 2013 Zarbosoft -->
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
		<li><a href="#binaries">Binaries</a></li>
		<li><a href="http://http://zarbosoft.com/forum/index.php?board=8.0">Forums</a></li>
	</ul>
	
	<table id="content"><tr>
		<td>
			<a class="screenshot" href="screenshot1.jpg"><img src="screenshot1-thumb.jpg" alt="The main window." /></a>
			<a class="screenshot" href="screenshot2.jpg"><img src="screenshot2-thumb.jpg" alt="The non-main window." /></a>
		</td>
		<td>
			<p><strong>Inscribist</strong> is a simple, two-color drawing program.  It is efficient for very large images (10000x10000 pixels and higher) and is quite usable even on older hardware.</p>

			<h2>Concepts</h2>
			<ul>
				<li>All images are composed of exactly two <strong>colors</strong>.  You can change the colors at any time.  A separate set of colors are used when exporting.</li>
				<li>When you export images or zoom out, the image is smoothed with a resampling algorithm.  For this reason, you should work at a high resolution and use <strong>downsampling</strong> when exporting.</li>
				<li>Inscribist remembers 10 <strong>brush</strong> configurations.  Each input <strong>device</strong> is associated with 1 brush.</li>
				<li>All drawing operations have one or more <strong>keyboard shortcuts</strong>.  If you are pressed for desk space the most common operations have numeric keypad shortcuts so you don't have to use a full sized keyboard.</li>
			</ul>

			<h2>Keyboard</h2>
			<ul>
				<li><strong>0-9</strong>: Select brush 0-9.</li>
				<li><strong>Tab, Enter</strong>: Switch between background and foreground color.</li>
				<li><strong>[, ], +, -</strong>: Zoom in, zoom out.</li>
				<li><strong>h, /</strong>: Flip the image horizontally.</li>
				<li><strong>v, *</strong>: Flip the image vertically.</li>
				<li><strong>Ctrl + s</strong>: Save (or save as, if you haven't yet saved).</li>
				<li><strong>Ctrl + Shift + s</strong>: Save as.</li>
				<li><strong>Ctrl + z, Ctrl + Shift + z</strong>: Undo, redo.</li>
				<li><strong>Left, Right, Up, Down</strong>: Roll the image.</li>
				<li><strong>Ctrl + Left, Right, Up, Down</strong>: Roll the image slightly.</li>
			</ul>

			<h2>Brush and Device Settings</h2>
			<ul>
				<li><strong>Heavy diameter</strong>: The diameter (in percentage of smallest image dimension) of the brush when full pressure is applied.</li>
				<li><strong>Light diameter</strong>: The diameter (in percentage of smallest image dimension) of the brush when minimum pressure is applied.</li>
				<li><strong>Damping</strong>: The modifier for the pressure curve.  At higher values, the brush size will vary more at high pressures.  At lower values, the brush size will vary more at low pressures.  At 1, the brush size will vary evenly at every pressure.</li>
			</ul>
			<br />
			<p>Inscribist can both save and load .inscribble files and export .png files.</p>
			<br />
			<p>Inscribist requires Lua, GTK+ 2 and bzip2. Inscribist was inspired by <a href="http://fishsoup.net/software/gsumi/">gsumi</a>.</p>
			
			<h2>Binaries</h2>
			<ul>
				<li><a href="install_inscribist.msi">Windows XP/7</a></li>
				<li><a href="inscribist-6.tar.gz">Arch</a></li>
				<li><a href="inscribist-6.deb">Ubuntu 12.04</a></li>
			</ul>
			<br />
			<p>The current revision is dated "sometime after the 1st century A.D."</p>
		</td>
	</tr></table>
	
	<div class="copyright clear">&copy; 2013 Zarbosoft</div>
	<div><a href="..">Back to Zarbosoft</a></div>
</div>
</body>

</html>
