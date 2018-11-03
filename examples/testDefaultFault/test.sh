#!/bin/bash
g++ testDefaultFault.cpp -o t `pkg-config --cflags --libs opencv` -L. -lwiringPi -lGPIO
sudo ./t