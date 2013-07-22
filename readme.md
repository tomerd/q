***************************************************************************************

This project is work in progress. if you are interested in contributing or otherwise have input
please touch base via github

***************************************************************************************

### about

q is a queueing toolkit. the idea is to provide a universal application programming interface that can be used throughout the entire
application development lifecycle without the need to commit to a specific queueing technology or to set up complex queueing environments 
where such are not required. you can think of it as an ORM for queueing. 

q runs on multiple back-ends and has bindings to many programing languages. and so, while during development you will most likely run it in-memory and let it clear when the process dies, you may choose a redis back-end on your test environment and running dedicated servers backed by rabbitMQ, amazon SQS or some other enterprise queueing system on production.

##### back-ends support
* lmdb (default): designed for a single machine, can be configured to run non persistent, in memory (useful for development mode) or persistent.
* redis

##### languages support
* [ruby] [1]
* [node.js] [2]
* [java/scala] [3]

  [1]: https://github.com/tomerd/q-ruby-binding        "ruby"
  [2]: https://github.com/tomerd/q-node-binding        "node.js"
  [3]: https://github.com/tomerd/q-java-binding        "java/scala"

##### installing on osx/linux
dependencies: libuuid-devel

	git clone git://github.com/tomerd/q.git
	cd q
	git submodule init
	git submodule update
	
	cd q
	aclocal && autoreconf -i && automake
	./configure
	make && make install

