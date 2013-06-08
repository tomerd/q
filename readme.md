***************************************************************************************

This project is work in progress. if you are interested in contributing or otherwise have input
please touch base via github

***************************************************************************************

### about

q is a queueing framework. the idea is to provide a universal application programming interface that can be used throughout the entire
application development lifecycle without the need to commit to a specific queueing technology or to set up complex queueing environments 
where such are not required. you can think of it as an ORM for queueing. q runs on multiple back-ends and has bindings to many 
programing languages. and so, while during development you will most likely run it in-memory and let it clear when the process dies, 
you may choose a redis back-end on your test environment and running dedicated servers backed by amazon SQS on production. q was designed to 
give you this freedom and to allow you to write the code once and run it anywhere.

##### back-ends support
* in-memory (non persistent, designed for development)
* berkeley db
* redis

##### languages support
* [ruby] [1]
* [node.js] [2]
* [java/scala] [3]

  [1]: https://github.com/tomerd/q-ruby-binding        "ruby"
  [2]: https://github.com/tomerd/q-node-binding        "node.js"
  [3]: https://github.com/tomerd/q-java-binding        "java/scala"

##### installing on linux

git clone git://github.com/tomerd/q.git
cd q/q
aclocal && autoconf -i && automake
./configure
make

