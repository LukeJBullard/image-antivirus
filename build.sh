#!/bin/bash
rm build/*
g++ -c -o build/image-antivirus.o src/image-antivirus.cpp
g++ -o build/image-antivirus build/image-antivirus.o -lspng
