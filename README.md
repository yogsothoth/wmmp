wmmp
====

This nice little dockapp provides a simple interface to mpd.

Initially, this repository was just here to hold the code I had found somewhere on the internet. Since the code would not compile anymore after quite a few years without being maintained, I decided to make a few changes here and there just to be able to use it.

After being lazy about it for too long, I recently (Jan/Feb 2017) decided to dust it off a little bit and build a proper port for FreeBSD. This involves cleaning up useless files, updating copyrights, the man page, comments, migrating to something simpler to manage than the autotools and removing the embryo of libmpdclient code included with the dockapp, to replace it with a proper external dependency to a modern version of said library.

The code is tested primarily on FreeBSD, and, as time permits, on the other BSDs and Linux as well.


