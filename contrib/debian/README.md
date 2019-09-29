
Debian
====================
This directory contains files used to package tscd/tsc-qt
for Debian-based Linux systems. If you compile tscd/tsc-qt yourself, there are some useful files here.

## tsc: URI support ##


tsc-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install tsc-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your tsc-qt binary to `/usr/bin`
and the `../../share/pixmaps/bitcoin128.png` to `/usr/share/pixmaps`

tsc-qt.protocol (KDE)

