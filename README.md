Unix Timestamp Converter for Geany
==================================


Unix Timestamp Converter is a plugin for Geany used to convert unix
epoch timestamps to a human readable string. The supported format of the
timestamp is an integer one for e.g. "1433141615". Timestamps akin to
this type "1433141615.123" will be internally trimmed to "1433141615".

This repository represents an independent project whose results could
be manually integrated with Geany.

* Supported platforms: Linux
* License: GPLv2 or later

Dependencies: 

* geany, geany-dev(el)
* gtk+2.0 or later
* glib

Features
--------

Depending on the settings the user have chosen the plugin can:

* Show results in 'Messages' tab or both in 'Messages' tab and in
a popup window.
* Turn on/off error messages.
* Use only the current selected text as a timestamp source or use the
current selected text with first priority or the clipboard with second
priority.


Compilation
-----------

To compile run: `make`

To install (you may need root privileges) run: `make install`

To uninstall (you may need root privileges) run: `make uninstall`
