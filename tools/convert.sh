#!/usr/bin/env bash
file="ratio"
for i in {0..2}
do
    python ./tools/to_txt.py ./resources/assets/${file}_${i}.bmp ./resources/assets/${file}_${i}.txt
    rm ./resources/assets/${file}_${i}.bmp
    python ./tools/to_bmp.py ./resources/assets/${file}_${i}.txt ./resources/assets/${file}_${i}.bmp
    rm ./resources/assets/${file}_${i}.txt
done
