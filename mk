#!/bin/sh
#
# Sample script to run make without having to retype the long path each time
# This will work if you built the environment using our ~/bin/build-snap script

PROCESSORS=`nproc`
PROJECT_PATH=../../BUILD/contrib/snaprfs

case $1 in
"-l")
	make -C ${PROJECT_PATH} 2>&1 | less -SR
	;;

"-d")
	rm -rf ${PROJECT_PATH}/doc/snaprfs-doc-1.0.tar.gz
	make -C ${PROJECT_PATH}
	;;

"-i")
	make -j${PROCESSORS} -C ${PROJECT_PATH} install
	;;

"-t")
	(
		if make -j${PROCESSORS} -C ${PROJECT_PATH}
		then
			shift
			${PROJECT_PATH}/tests/unittest --progress $*
		fi
	) 2>&1 | less -SR
	;;

"")
	make -j${PROCESSORS} -C ${PROJECT_PATH}
	;;

*)
	echo "error: unknown command line option \"$1\""
	;;

esac
