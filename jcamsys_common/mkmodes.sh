#!/bin/bash

# type enum and #define are not the same thing, I wanted it like this ...

echo "CUSTOM" >/tmp/m
# 640x480 must always be 1
echo "640x480" >>/tmp/m
echo "160x90" >>/tmp/m
echo "160x120" >>/tmp/m
echo "320x180" >>/tmp/m
echo "320x200" >>/tmp/m
echo "320x240" >>/tmp/m
echo "352x288" >>/tmp/m
echo "432x240" >>/tmp/m
echo "640x360" >>/tmp/m
echo "720x576" >>/tmp/m
echo "768x576" >>/tmp/m
echo "720x480" >>/tmp/m
echo "800x448" >>/tmp/m
echo "800x600" >>/tmp/m
echo "864x480" >>/tmp/m
echo "960x720" >>/tmp/m
echo "1024x576" >>/tmp/m
echo "1280x720" >>/tmp/m
echo "1280x1024" >>/tmp/m
echo "1600x896" >>/tmp/m
echo "1920x1080" >>/tmp/m
echo "2304x1296" >>/tmp/m
echo "2304x1536" >>/tmp/m
echo "3840x2160" >>/tmp/m


X="0"
echo "" >/tmp/x
while read p; do
  echo -e "#define JC_MODE_$p\t\t$X" >>/tmp/x
  X=$[$X + 1]
done </tmp/m
echo -e "#define JC_MAX_MODES\t\t$X" >>/tmp/x


# Optionally add space around the x
sed -i -e "s/x/ x /g" /tmp/m


DEST="jcamsys_modes.h"
echo "// jc_modes.h  - made with mkmodes.sh" >$DEST
echo "// Camera capture and common display resolutions" >>$DEST
echo "" >>$DEST
column -t -s $'\t' -n /tmp/x >>$DEST

echo "" >>$DEST
echo "" >>$DEST
echo "static const char* jc_modes[] __attribute__((unused))= { " >>$DEST

while read p; do
  	echo -e "\t\t\"$p\"," >>$DEST
done </tmp/m
echo -e "\t};" >>$DEST
echo "" >>$DEST

cat $DEST


