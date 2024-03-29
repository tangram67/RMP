# RMP - The Reference Music Player

![alt text](https://github.com/tangram67/RMP/blob/master/rmp-750x420.jpg?raw=true)

The **Reference Music Player** is a full featured C++ music player software for Linux based operating systems.
The audio interface is focused on exact timing and intelligent content buffering.
An optimized and direct mode of transferring data to the underlying ALSA sound system is used for best sonic performance. The application can be optimized in many ways to deliver best results (process priority and NICE level, CPU affinity, memory usage for buffering, audio output timing, etc.). The software is dedicated to High-End music afficionados and technophile users. Best results will be achieved with multicore desktop processors used in a dedicated streaming hardware solution.

The user interface is web based and build on top of Bootstrap 3. The interface is fully responsive over a wide range of mobile clients.
The UI can be linked to the desktop of mobile devices (see manifest.json handling for Chrome/Chromium based Browsers https://developer.mozilla.org/de/docs/Mozilla/Add-ons/WebExtensions/manifest.json).

All supported codecs (**DSF, DFF, FLAC, WAVE, AIFF, ALAC, MP3**) are using _native decoders_.
**SACD** formats are also supported by using **DoP (DSD over PCM)** on compatible DA converters.

Metadata (MP3) tags are read with exceptional high performance. Libraries with more than 150.000 entries have been tested and confirmed to work faster than any other professionally sold software. Playlist management and export is supported. Also internet readio streaming is supported. Radio stations are managed via the Web UI.

Coverart is extracted from the file metatdata or from a JPEG or PNG file located in the album folder (named cover.jpg, cover.png, folder.jpg or folder.png).
Various library views are available: grouped by artist, grouped by album in list form, grouped by media type (CD, DVD, BD, HD, etc.). Also a full text library search ("Google sytle") is supported.

The core framework used for this application is based on parts of **RAD.web©**. If you are interested in more details, please visit www.dbrinkmeier.de or contact me via e-mail under develop@dbrinkmeier.de

These are only the basic features, there are lots of functions and possibilities that are not mentioned here...

## Setup prerequisites

Change to the folder **"setup"** and execute the following scripts as root:

1. **install_languages.sh** ... Installs the needed system languages (see also the documentation for your Linux distribution)
2. **install_prerequsites.sh** ... Install all necessary libraries (the names or version might differ in your ditribution)
3. **setup_folders.sh \<username\>** ... Setup the needed folders in your system and set the ownership for the given non-root user
  
Install the webroot content from https://github.com/tangram67/RMP-Webroot to a local folder, e.g. in the users home directory. This user must match the username parameter of the **"setup_folders.sh"** script.

## Compile static libraries

1. Change to the folder **"curl"** and execute **./build.sh**
2. Change to the folder **"mpg123"** and execute **./build.sh**

No errors should be shown and a static library file (\*.a) should be found in each folder.

## Compile the application

The build is local, no files will be installed in your system folders by the following commands:

1. Execute **./bootstrap** in root project folder
2. Execute **make -j4**
3. Execute **sudo make install-strip**

The application is build in the ./build folder and installed in ./build/bin folder as "rmp". Please note that the rmp binary file wil loose the capability settings when copied to a different location. The root folder contains a batch file "setcap.sh" to restore all needed capabilities of the binary. Execute **sudo ./setcap.sh /new/path/rmp** as root accordingly to the new path of the binary file.

## Start the application for the first time

1. Execute **./build/bin/rmp -N -P**
2. The application may produce some warnigs and/or error messages, but nevertheless the default configuration will be installed in **/etc/dbApps/rmp/** (if the script **setup_folders.sh \<username\>** was successfully executed and therefore the necessary folders do exist with the correct accress rights for the given usen)
3. Edit **/etc/dbApps/rmp/webserver.conf**
4. Set the webroot entry **"DocumentRoot = /path/to/the/webroot/"** to the real webroot path in your system (by using the path where the content from https://github.com/tangram67/RMP-Webroot is located)

Warning: Do not change any other configuration settings unless you are really sure what you are doing!

## Start the application

1. Execute **./build/bin/rmp -N -P**, parameter **-d** can be used to start daemonized
2. The application should start nomally and the webinterface should be accessible via http://localhost:8099
3. Configure your your DA converter and content folders under **"Settigs/Player"** and do a library scan
4. Your library and the artwork sould show up in the various library views (Media, Artist, Album, ...)

## Installing the mediaplayer

The folder **"setup"** countains an init.d script and a default settings script. Use this helper script as a template to setup your system environment.
Edit the **"defaults"** file to match the username used by the setup scripts as mentioned above. 

1. Copy the startup script **rmp** from **./setup/init.d** to **/etc/init.d/**
2. Copy the defaults file (also named **rmp**) from **./setup/default/** to **/etc/defaults/**
3. Enable the **Reference Music Player** on system startup via **sudo update-rc.d rmp defaults**
4. Start the **Reference Music Player** as a service via **sudo service rmp start**

Have a cup of coffee and enjoy&nbsp;&nbsp;:tea:
