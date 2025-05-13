#!/bin/bash

count=$(python3 tools/warnanalyzer.py analyzer.txt)

echo "{\"color\": \"yellow\", \"status\": \"$count\", \"subject\": \"Warnings\"}" > warning_count.txt

ffmpeg -f lavfi -i "color=#cccc11:size=120x28" \
 -filter_complex "[v]drawtext=text=\'$count warnings\'
 :fontcolor=black
 :fontsize=14
 :x=(w-text_w)/2
 :y=8[out]" \
 -map '[out]' \
 -frames:v 1 \
 -y warn_badge.png
