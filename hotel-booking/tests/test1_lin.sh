#!bin/bash


IP_SERVER=127.0.0.1
PORT_SERVER=9999
HOTEL_AVAILABLE_ROOMS=3

echo "Test 1"

cd ../

./server_lin $IP_SERVER $PORT_SERVER $HOTEL_AVAILABLE_ROOMS &
sleep 1

./client_lin $IP_SERVER $PORT_SERVER              # doesnt work as intended...
