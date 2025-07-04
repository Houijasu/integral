#!/bin/bash
echo "position startpos" | ./integral
echo "go depth 10" | ./integral &
sleep 10
echo "quit" | ./integral