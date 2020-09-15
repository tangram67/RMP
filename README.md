# RMP - The Reference Music Player

The **Reference Music Player** is a full featured C++ music player software for Linux based operating systems.
The audio interface is focused on exact timing and intelligent content buffering.
An optimized and direct mode of transferring data to the underlying ALSA sound system is used for best sonic performance. The application can be optimized in many ways to deliver best results (process priority and NICE level, CPU affinity, memory usage for buffering, audio output timing, etc.).

The user interface is web based and build on top of Bootstrap 3. The interface is fully responsive over a wide range of clients.
The UI can be linked to the desktop of mobile devices (see manifest.json handling for Chrome/Chromium based Browsers).

All supported codecs (**DSF, DFF, FLAC, WAVE, AIFF, ALAC, MP3**) are using _native decoders_.
**SACD** formats are also supported by using **DoP (DSD over PCM)** for compatible DA converters.

Metadata (MP3) tags are read with exceptional high performance. Libraries with more than 150.000 entries have been tested and confirmed to work smoothly.
Playlist management and export is supported. Also internet readio streaming is supported and radio stations are managed via the Web UI.

Coverart is extracted from the file metatdat or from a JPEG or PNG file in the album folder (cover.jpg, cover.png, folder.jpg, folder.png).
Various library views are available: grouped by artist, grouped by album in list form, grouped by media type (CD, DVD, BD, HD, etc.). Also a full text libryr search is supported.

These are only the basic features, there are lots of functions and possibilities that are not mentioned here...

## Setup prerequisites

Change to the folder **"setup"** and execute the following scripts as root:

1. **install_languages.sh** ... Installs the needed system languages (see also the documentation of your Linux distribution)
2. **install_prerequsites.sh** ... Install all necessary libraries (the names or version might differ in your ditribution)
3. **setup_folders <username>** ... Setup the needed folders in your system and set the ownership for the given non-root user

## Compile static libraries

1. Change to the folder **"curl"** and execute **./build.sh**
2. Change to the folder **"mpg123"** and execute **./build.sh**

No errors should be shown and the static library file (\*.a) should be in be in each folder.

## Compile the application

The build is local, no files will be installed in your system folders by the following commands:

1. Execute **./bootstrap** in root project folder
2. Execute **make -j4**
3. Execute **sudo make install-strip**

The application is build in the ./build folder and installed in ./build/bin folder as "rmp"

## Start the application for the first time

1. Execute **./build/bin/rmp -N -P**
2. The application may produce some warnigs and/or error messages, but nevertheless the default configuration will be installed in **/etc/dbApps/rmp/**
3. Edit **/etc/dbApps/rmp/webserver.conf**
4. Set the webroot entry **"DocumentRoot = /path/to/the/webroot/"** the real webroot path in your system

## Start the application

1. Execute **./build/bin/rmp -N -P**, parameter **-d** can be used to start daemonized
2. The application should start nomally and the webinterface should be accessible via **http://localhost:8099**
3. Configure your your DA converter and content folders under **"Settigs/Player"** and do a library scan
4. Your library and the artwork sould show up in the various library views (Media, Artist, Album, ...)

## Install the mediaplayer in your system

The folder **"setup"** countains an init.d script and a default setting script. Use this scripts as a template to setup your system.


__Enjoy ;-)__
