CAvatar
=======

A avatar thing in C.

Requirements
============

* [libevent2](http://libevent.org/)
* [libevhtp](https://github.com/ellzey/libevhtp)
* [imagemagic 6](http://www.imagemagick.org/)
* [(future) hiredis](https://github.com/redis/hiredis)

Workings
========

	Generating a picture

		take a string
		lowercase that
		md5 that
		color is a checksum of the md5
		for each char
			if char&1 == 1
				turn on pixels mirrored in the color
