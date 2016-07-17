#!/bin/bash

pkill final
fuser -k 80/tcp
./final -p 80 -h 127.0.0.1 -d /home/mihail/stepic-final-http-server/www
