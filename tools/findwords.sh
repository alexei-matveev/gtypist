#!/bin/bash

 case "$1" in
   "")
	echo "You must specify the letters you want"
	echo "as the first argument. Try --help."
	;;
   "--help")
	echo "Usage: ./findwords.sh letters [combination]"
	echo
	echo "This is a simple script to help you find words"
	echo "which contain only specified letters and/or have"
	echo "some special combination of letters in them."
	echo 
	echo "You can use '.' to match all the letters"
	;;
   ".")
	aspell dump master | grep -v [\'] | grep "$2"
	;;
   *)
	aspell dump master | grep -w ^[${1}]* | grep -v [\'] | grep "$2" 
	;;
 esac

