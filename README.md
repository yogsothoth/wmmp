wmmp
====

This nice little dockapp provides a simple interface to mpd.

The code in this repository is basically the same as that of the original distribution, and only differs on two things:

- the way the scroll wheel works has been changed. In the original code, the scroll wheel would change the volume by calculating offsets; the code now sends a straight target volume to the server;
- the network structure has been changed, as the original one would not compile anymore.

Note that this has only been tested on NetBSD >= 5.0.



