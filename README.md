# Unknown
Unknown is my personal codebase written from scratch in C using a minimal amount of libraries.

## Building 
On **Linux**, you will need [clang](https://clang.llvm.org/get_started.html) or [gcc](https://gcc.gnu.org/).

```sh
./code/build.sh
```

On **Windows**, you will need [msvc](https://visualstudio.microsoft.com/downloads/) (I recommend installing the toolchain using this [gist](https://gist.github.com/mmozeiko/7f3162ec2988e81e56d5c4e22cde9977)).

```bat
.\code\build.bat
```

**Example**: To build `Muze` on Linux you would do `./code/build.sh -Muze`. This will create an executable `./build/muze` which you can then run.

# Applications
All applications can be built by passing a flag to the build script.  Run the script with no flags to see available targets.

## Muze (-Muze)
A GUI for music composition and practice.

It runs on Linux and Windows (OpenGL) with nice platform debug features such as code profiling, hot reloading, loop editing, saving/loading replays and state to/from disk. 

## Editor (-Editor) 
A POC GUI text editor.

## LSD (-LSD)
My attempt at making a Serial Downloader for the ADuC832 board.  Achieves 2x speed improvements over WSD.

For now it only supports the ADuC832, but I could easily add support for your board.  Just send me a request at `luca@keyfried.com`..

## WSDCOM (-WSD)
Windows only!

My C port of the wsdcom utility.

## Cling
The self-hosting C build script and metaprogram for all projects. 
