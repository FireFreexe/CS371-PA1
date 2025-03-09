# CS371-PA1
Repository for CS 371, Project 1

This is the submission by Casey Howard and Claire Caldwell for CS 371 PA1.

pa1_skeleton.c is our working C file.

it is compiled with gcc -o pa1_skeleton pa1_skeleton.c -pthread

and can be ran with

./pa1_skeleton server 127.0.0.1 12345 &

to run the server in the backround, followed by

./pa1_skeleton client 127.0.0.1 12345 4 <X>

where <X> is replaced by an integer for however many messages you want sent, such as

./pa1_skeleton client 127.0.0.1 12345 4 1000


