#!/bin/sh

curl --socks5 127.0.0.1:1080  http://detectportal.firefox.com/success.txt | hexdump -C
