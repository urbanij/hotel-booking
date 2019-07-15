### 2019 Final Project
Concurrent & Distributed Programming class

Project specifications (ita):<br>
[http://vecchio.iet.unipi.it/se/files/2019/05/Progetto-2018_2019-Sistemi-di-Elaborazione.pdf](http://vecchio.iet.unipi.it/se/files/2019/05/Progetto-2018_2019-Sistemi-di-Elaborazione.pdf)

#### abstract:

Client/server application for hotel bookings.<br>
Client and server communicate via TCP sockets. The server is concurrent and the concurrency is implemented with POSIX threads. 
The server indefinitely waits for new incoming connections and every accepted connection is dispatched to a thread – in a pre-allocated pool of threads – in charge of
managing the requests of the client.
The reservations are constrained to the year 2020.

#### demo:

[![demo](https://i.imgur.com/9VQdDrU.png)](https://youtu.be/2S_IpDbXQF8)


#### documentation:
[HTML](https://urbanij.github.io/projects/hotel-booking/docs/html/index.html) | [PDF](https://urbanij.github.io/projects/hotel-booking/docs/latex/refman.pdf)


### Usage<sup>1</sup>:

#### install dependencies:
Linux:
```sh
sudo apt-get install libsqlite3-dev
sudo apt-get install sqlitebrowser          # optional
```
macOS:
```sh
brew install sqlite3                        # optional
brew cask install db-browser-for-sqlite     # optional
```

#### compilation:
```sh
git clone https://github.com/urbanij/hotel-booking.git
cd hotel-booking/hotel-booking
make
```
#### running:
- open a terminal session and run the server: <br>
```sh
./server 127.0.0.1 8888  5
```
- open one or more terminal sessions and run the client(s)<br>
```sh
./client 127.0.0.1 8888
```
where:<br>
`127.0.0.1` is the IP address (in this case _localhost_),<br> 
`8888` is the port number and <br>
`5` the initial total number of available room in the hotel.<br>


#### running with gdb debugger
(may require root privileges on macOS)

make sure to compile with `-DGDB_MODE`, then run `gdb ./server`.<br>
Inside gdb type `r` to run and you're good to go.<br> For an easier debugging experience [this](https://github.com/cyrus-and/gdb-dashboard) is recommended.


<br>

[Francesco Urbani](https://urbanij.github.io/)

---
<sup>1</sup>: tested on Linux Ubuntu 18, Raspbian and macOS.
