CAvatar
=======

An avatar thing in C. Includes a WebUI for viewing and (in the future) editing avatars.

ToDo
====

* save custom avatars in redis (char[7])
   * first 4 chars represent the 32 pattern positions before mirroring
   * the latter 3 chars represent the RGB value
* some authentication, you can claim your email hash only or something
* avatar editor in WebUI

Requirements
============

* [CMake](http://www.cmake.org/)
* [OpenSSL](https://www.openssl.org/) (only for MD5 currently)
* [libevent2](http://libevent.org/)
* [libevhtp](https://github.com/ellzey/libevhtp)
* [libpng 1.6](http://www.libpng.org/pub/png/libpng.html)
* [(future) hiredis](https://github.com/redis/hiredis)

**or**

* [Docker](http://www.docker.com/)

Building
========

	cd build
	cmake ..
	make

Running
=======

	cd build
	./cavatar

Using Docker
============

Build using

	$ docker build -t "cavatar" .

Run using

	$ docker run -p 3002:3002 -d --name "cavatar" cavatar

Stop using

	$ docker stop cavatar
	
Remove using
	
	$ docker remove cavatar
	
See [this reference page](http://docs.docker.com/reference/) (and [this](https://docs.docker.com/reference/run/)!) for more info.

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
