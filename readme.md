***************************************************************************************

This project is work in progress. if you are interested in contributing or otherwise have input
please touch base via github

***************************************************************************************

### about

q is a queueing framework. the idea behind it is to provide a universal application interface that can be used across all
development phases and scaling requirements. q runs on multiple back-ends and has binding to many programing languages. and so
while during development you probably want to run it using an in-memory back-end that clears with the process, you may choose 
to use redis on your test environment and amazon SQS on production.

##### back-ends support
* in-memory (non persistent, designed for development)
* berkeley db
* redis

##### languages support
* [node.js] [1]
* [java] [2]
* [scala] [2]
	
	[1] https://github.com/tomerd/q-node-binding
	[2] https://github.com/tomerd/q-java-binding
