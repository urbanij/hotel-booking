2019 Final Project
Concurrent & Distributed Programming class

Project specifications (ita):<br>
[http://vecchio.iet.unipi.it/se/files/2019/05/Progetto-2018_2019-Sistemi-di-Elaborazione.pdf](http://vecchio.iet.unipi.it/se/files/2019/05/Progetto-2018_2019-Sistemi-di-Elaborazione.pdf)


Usage<sup>1</sup>:

```sh
git clone https://github.com/urbanij/hotel-booking.git
cd hotel-booking
make
```
then:
- open a terminal session and run the server: <br>
```sh
./server 127.0.0.1 8888  15
```
- open one or more terminal sessions and run the client(s)<br>
```sh
./client 127.0.0.1 8888
```
where:<br>
`127.0.0.1` is the ip address (in this case _localhost_),<br> 
`8888` is the port number and 
`15` the number of room the hotel has.<br>
Both the ip address and port number can be changed. The number of room as well of course: being an argument was simply a project directive.


<br>
[Francesco Urbani](https://urbanij.github.io/)

---
<sup>1</sup>: tested on Linux Ubuntu and macOS