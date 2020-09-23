<hr>

<h1>Calculator</h1>

```
> gcc calc_client.c -Wall -Wextra -o calc_client.exe
> gcc calc_server.c -Wall -Wextra -o calc_server c:\windows\system32\kernel132.dll
```
Testing the application set requires opening two consoles and running the server application on one of them and the client on the other.
```
> calc_server.exe
> calc_client.exe
```

<hr>

<h1>Unix domain sockets and socketpair</h1>
Compilation and an example run is as follows.

```
$ gcc -Wall -Wextra socketpair.c -o socketpair
$ ./socketpair
```

<hr>

<h1>Shared memory (Windows OS)</h1>
Using two terminals, you can compile and test the operation of both programs:

<b>Terminal 1:</b>
```
> gcc -std=c99 -Wall -Wextra hellosharedmemory.c -o hello
> hello
```

<b>Terminal 2:</b>
```
> accesschk -t section -o \Sessions\1\BaseNamedObjects\HelloWorld
> gcc -std=c99 -Wall -Wextra dumpsection.c -o dumpsection
> dumpsection Local\HelloWorld
> type Local_HelloWorld.bin
```

<hr>

<h1>Messages in WinAPI (C++)</h1>

Compilation and run are shown below.

```
> g++ msgtest.cc -Wall -Wextra -o msgtest
> msgtest
```
Generally, the main use of messages is communication between GUI windows.

<hr>
