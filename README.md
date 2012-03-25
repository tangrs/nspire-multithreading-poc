# NSpire multi-threading proof-of-concept

This is a proof of concept for multithreading on the calculator. More information about threading can be found in Wikipedia (http://en.wikipedia.org/wiki/Thread_%28computing%29).

This library will most likely appeal to developers of Ndless who can take advantage of this to build more advanced programs.

Unfortunately, this is a quick and dirty solution. It doesn't perform as optimally as it could have. Atomic reads/writes are achieved by turning interrupts on and off. I'll refine it as I need it.

## Features

This threading POC does basic context switches between each thread giving them all equal priority.

There are also thread sleeping options since you obviously can't use the built in sleep() function.

## What can't be used in a threaded environment?

Some functions that either rely on the timers (i.e. Ndless's sleep function) or rely on the absense of timers (i.e. printf) won't work.

Hopefully system calls are reentrant. If not, they can't be used either. I haven't tested them yet. You may need to disable interrupts when making system calls.

## How to use

TODO.

Refer to ```example.c``` for now. All functions exposed by ```atomic.h``` and ```threading.h``` are callable.

## Licence

See LICENCE