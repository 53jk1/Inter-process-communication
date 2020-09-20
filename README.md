<h1>calculator</h1>

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
