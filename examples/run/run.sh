#!/bin/bash
g++ merge.cpp -o run1 `pkg-config --cflags --libs opencv` -L. -lwiringPi -lGPIO –lpthread
g++ runBack.cpp -o run2 `pkg-config --cflags --libs opencv` -L. -lwiringPi -lGPIO –lpthread