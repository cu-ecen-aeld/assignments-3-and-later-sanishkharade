#!/bin/sh
# Test script used to test assignment 8 char driver implementation

device=/dev/aesdchar

echo "write1" > ${device}
echo "write2" > ${device}
echo "write3" > ${device}
echo "write4" > ${device}
echo "write5" > ${device}
echo "write6" > ${device}
echo "write7" > ${device}
echo "write8" > ${device}
echo "write9" > ${device}
echo "write10" > ${device}

