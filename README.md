DreamDesktop. Animated wallpaper for KDE 4 environment
======================================================

About DreamDesktop
------------------

DreamDesktop is an animated wallpaper for KDE 4 environment Has been tested in KDE 4.9.3. It should work quite well also in the KDE 4.8.x / 4.9.x.

Home Page: http://www.jarzebski.pl/projekty/dreamdesktop.html

Dependencies
------------

 * FFmpeg (http://ffmpeg.org/)

How To Build
------------

```bash
# cd plasma-wallpaper-dreamdesktop
# mkdir build
# cd build
# cmake .. -DCMAKE_INSTALL_PREFIX=`kde4-config --prefix`
# make
# sudo make install
```

Perhaps it will be necessary to restart KDE.

Where can you find animations
-----------------------------

Animation can be any video output from FFMPEG. Beautiful animations can be found at: http://www.dreamscene.org/

How can you help?
-----------------

 * First, you can report bugs to me at the address blog@jarzebski.pl.
 * Second, if you  have programming experience more from me you can send your comments or patches.
 * If in any way you want to support the development of this project, you can give for this purpose any donation. The appropriate form can be found on the project website.
 