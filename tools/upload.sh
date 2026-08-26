#!/usr/bin/env bash
python ./tools/generate_graphics.py ./resources/$1.json ./include/graphics.hpp
pio run -t upload
