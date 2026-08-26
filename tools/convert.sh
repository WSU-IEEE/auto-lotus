#!/usr/bin/env bash
for i in {0..7}
do
    python ./tools/to_txt.py ./resources/assets/waves_$i.bmp ./resources/assets/waves_$i.txt
    rm ./resources/assets/waves_$i.bmp
    python ./tools/to_bmp.py ./resources/assets/waves_$i.txt ./resources/assets/waves_$i.bmp
    rm ./resources/assets/waves_$i.txt
done
