Unix Timestamp Converter for Geany
==================================


Unix Timestamp Converter is a plugin for Geany used to convert unix
epoch timestamps to a human readable string. The supported timestamp
formats include integers in seconds, milliseconds or microseconds like
"1433141615", "1433141615123", "1433141615123456" as well as floating
point like "1433141615.123" (milliseconds) or "1433141615.123456"
(microseconds). Microseconds or lesser values will not be shown in the
final conversion result!

This repository represents an independent project whose results could
be manually integrated with Geany.

* Supported platforms: Linux
* License: MIT

Features:

* Conversion to time string in GMT and user's local time zone.
* Shows results in 'Messages' tab or both in 'Messages' tab and in
a popup window.
* Use only the current selected text as a timestamp source or use the
current selected text with first priority or the clipboard with second
priority.
* Turn on/off error messages.
* Turn on/off the support for timestamps in milli/microseconds.

Dependencies:

* geany
* geany-devel or geany-common  (depending on the distro)
* gtk+3.0-dev(el) or gtk+2.0-dev(el)  (depending on the distro)
* make
* pkg-config

Compilation
-----------

To compile run: `make`

To install (you may need root privileges) run: `make install`

To uninstall (you may need root privileges) run: `make uninstall`

Local to the current account installation
-----------------------------------------

This is an alternative to globally install the plugin for all users.

To install for the current account run: `make localinstall`

To uninstall for the current account run: `make localuninstall`

Other notes
-----------

Attention MacOS users - this plugin will work with the manually
installed and compiled Geany editor from source code. It will not work
with the version installed from dmg files.

Other Useful Plugins
--------------------
* [Geany JSON Prettifier](https://github.com/zhgzhg/Geany-JSON-Prettifier)
* [Geany Generic SQL Formatter](https://github.com/zhgzhg/Geany-Generic-SQL-Formatter)
