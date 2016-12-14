#! /bin/bash
if [ "$1" == "-j" -a "${JLINK}x" != "x" ]; then
  JLINK=`which JLinkExe`
  if [ "$JLINK}"x == "x" ]; then
    echo "JLinkExe not found, please ensure its in the PATH"
    exit 1
  else
    ${JLINK} -if SWD -device MK82FN256xxx15 -speed 4000 ./flash.jlink
  fi
else
  HOST=`uname`
  BLBASE=`dirname $0`/blhost
  BLHOST=$([ "$HOST" == "Darwin" ] && echo "$BLBASE/mac/blhost" || echo "$BASE/linux/blhost")
  ${BLHOST} -u -- flash-erase-all
  [ $? -eq 0 ] && ${BLHOST} -u -- write-memory 0x0 ./BUILD/UBIRCH1/GCC_ARM/mbed-src-program.bin
  [ $? -eq 0 ] && ${BLHOST} -u -- reset
fi