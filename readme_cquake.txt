cquake release 1
6/11/08

Quake is a registered trademark of id Software
Nintendo DS is a registered trademark of Nintendo

All copyrights property of their respective owners.

Quake source code is released under the GPLv2 - read gnu.txt for details.

This is a modified version of the quake source code that allows it to run on the Nintendo DS.

I am not affiliated with id software or Nintendo in any way either directly or indirectly.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.

In other words use this at your own risk.


How to play:
You will need the PAK files from either the registered or the shareware version of quake. They need to be put in a folder named ID1 in the root directory of your flash card. The cquake.nds file needs to be patched with the appropriate DLDI driver for your card 

If you are not sure how to do this you can refer to the DLDI Wiki page:
http://dldi.drunkencoders.com/index.php?title=Main_Page

After cquake.nds had been patched it needs to be put into the root directory of your flash card.


I have also included a modified version of progs.dat and an autoexec.cfg file. These files can be put in the ID1 directory. The progs.dat file contains no modifications; it has been compiled with an optimizing qcc compiler that reduces the memory used - by about 70k.

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
YOU REALLY NEED TO READ THIS PART
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
in order to increase the read speeds while pulling data from the flash card the libfat library is bypassed. In order for this to work correctly it is required that the pack files be completely defragmented. If the files are not defragmented the game will crash. If you do not already know how to do this then you will need to refer to the documentation for your operating system.

You will probably need to adjust the gamma settings as by default the colors can be very dark. This can be done through the console or through the menu.

The keyboard implementation in this version is horrific, but functional - just barely.
The space key is on the bottom row.


Eric Hobbs
