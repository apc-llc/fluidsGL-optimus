## Compiling

## How to compile Linux server and Linux client

Linux server and client are built by default with the project `Makefile`:

```
$ cd 5_Simulations/fluidsGL/
$ make
```

### How to compile Android client

Android client compilation requires Android SDK and NDK. Using the provided `Makefile`, client could be configured, compiled, installed and executed on connected Android device:

```
$ make configure
$ make
```

Note the Android device must have development mode enabled, and have the host PC authorized for using it.

## Running

### How to run Linux server and Linux client 

By default, the server and client are configured to run on the same host, using loopback interface, for testing purposes. In this configuration, client and server could be started without arguments:

```
$ ../../bin/x86_64/linux/release/fluidsGL
$ ../../bin/x86_64/linux/release/fluidsGL_client
```

Assuming the **server** address is `192.168.0.61`, the **client** address is `192.168.0.10`, and the *port* 9097 is free and not firewalled, the following commands have to be executed:

* On server (broadcast to client subnet consisting of one host from server's address within this network):
```
$ ../../bin/x86_64/linux/release/fluidsGL 192.168.0.10:9097 192.168.0.61
```

* On client (listen to the broadcast incoming to client's address within this network, on the specified port):
```
$ ../../bin/x86_64/linux/release/fluidsGL_client 192.168.0.10:9097
```

