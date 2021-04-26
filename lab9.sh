#!/bin/bash
g++ -c server.cpp
g++ -c client.cpp
g++ -o server server.o -lpthread
g++ -o client client.o -lpthread
