#!/bin/bash

pkill final
fuser -k 888/tcp
./final -p 888 -h 10.0.3.154 -d /www/dev/htdocs/stepic-final-http-server/www
