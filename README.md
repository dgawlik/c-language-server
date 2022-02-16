# c-language-server

Project is simple implementation of code navigation server for C language. It's not meant to cover everything but to cover a range of most popular use cases. Originally it was inspired by Stack Graphs talk from Github but during the development it departed slightly from the ideas presented.


### Design

It contains 2 parts. First there is VSCode plugin in node.js that contributes 3 commands: index, find definition and find usages. It spawns a process of language sever written in C++. The communication between parent-child processes is via stdin and stdout. 

C++ language server is ahead of time indexer. It uses Tree Sitter to parse C files into Concrete Syntax Tree and simplifies it into another tree. Then in second stage the trees are crosslinked together to form jumps to type definitions. I optimized away all bottlenecks in code so it is as fast Tree Sitter parser itself.

Performance is good for small and medium sized projects. Indexing time is around 50 files/second and crosslinking is of similar performance. Full scan of kernel source code takes around 3-4 minutes on my PC.

### Usage

Build language server:

```
mkdir build && cd build
cmake ..
make
./tst
```

Running VSCode with plugin:

* Open *vscode-plugin* folder in VSCode
* Modify configuration defaults in package.json
    * c-lang-navigation.rootIndexPath - path of project to index
    * c-lang-navigation.serverPath - path to previously built indexer
    * c-lang-navigation.excludePatterns - optionally exclude directories to make it faster
* Run extensions.js - ctrl + F5
* In new window open folder of indexed project

Key bindings:

* ctrl+alt+i - index, wait for 2 messages indexing_done and crosslinking_done
* ctrl+alt+r - find reference
* ctrl+alt+u - find usages