### 2019 Final Project
Concurrent & Distributed Programming class

Project specifications (ita):<br>
[http://vecchio.iet.unipi.it/se/files/2019/05/Progetto-2018_2019-Sistemi-di-Elaborazione.pdf](http://vecchio.iet.unipi.it/se/files/2019/05/Progetto-2018_2019-Sistemi-di-Elaborazione.pdf)


### Usage<sup>1</sup>:

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
`5` the total number of room in the hotel.<br>


#### running with gdb debugger
(may require root privileges on macOS)

make sure to compile with `-DGDB_MODE`, then run `gdb ./server`.<br>
Inside gdb type `r` to run and you're good to go.<br> For an easier debugging experience [this](https://github.com/cyrus-and/gdb-dashboard) is recommended.


<br>

[Francesco Urbani](https://urbanij.github.io/)

---
<sup>1</sup>: tested on Linux Ubuntu 18, Raspbian and macOS