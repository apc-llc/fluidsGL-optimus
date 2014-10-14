### How to run Linux server and Linux client 

By default, the server and client are configured to run on the same host, using loopback interface, for testing purposes. In this configuration, client and server could be started without arguments:

```
$ ../../bin/x86_64/linux/release/fluidsGL
$ ../../bin/x86_64/linux/release/fluidsGL_client
```

Assuming the **server** address is `192.168.0.61`, **client** address is `192.168.0.10`, and port 9097 is free and not firewalled, the following commands have to be executed:

* On server (broadcast to client subnet consisting of one host from server's address within this network):
```
$ ../../bin/x86_64/linux/release/fluidsGL 192.168.0.10:9097 192.168.0.61
```

* On client (listen to the broadcast incoming to client's address within this network, on the specified port):
```
$ ../../bin/x86_64/linux/release/fluidsGL_client 192.168.0.10:9097
```

