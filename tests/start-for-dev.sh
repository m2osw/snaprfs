#!/bin/sh -e
#
# Start the snaprfs locally for a developer

# Make sure that the Debug binary exists
BINARY_DIR=`cd ../../BUILD/Debug/contrib/snaprfs && pwd`
SNAPRFS=${BINARY_DIR}/daemon/snaprfs
if test ! -x "${SNAPRFS}"
then
	echo "error: could not find the snaprfs binary, was it compiled?"
	exit 1
fi
TMP_DIR=${BINARY_DIR}/tmp

REMOTE_LISTEN="192.168.2.1"
SECURE_LISTEN="192.168.3.1"
GDB=""
while test -n "$1"
do
	case "$1" in
	"--debug"|"-d")
		shift
		GDB='gdb -ex "catch throw" -ex run --args'
		;;

	"--help"|"-h")
		echo "Usage: $0 [-opts]"
		echo "where -opts are:"
		echo "   --secure-listen | -s       define the secure-listen IP address (public network)"
		echo "   --remote-listen | -r       define the remote-listen IP address (private network)"
		echo
		exit 1
		;;

	"--remote-listen"|"-r")
		shift
		REMOTE_LISTEN="${1}"
		shift
		;;

	"--secure-listen"|"-s")
		shift
		SECURE_LISTEN="${1}"
		shift
		;;

	*)
		echo "error: unrecognized option $1"
		exit 1
		;;

	esac
done

mkdir -p ${TMP_DIR}

if test ! -f ${TMP_DIR}/priv.key \
     -o ! -f ${TMP_DIR}/cert.crt
then
	openssl req -newkey rsa:2048 \
		-nodes -keyout ${TMP_DIR}/priv.key \
		-x509 -days 3650 -out ${TMP_DIR}/cert.crt \
		-subj "/C=US/ST=California/L=Sacramento/O=Snap/OU=Website/CN=snap.website"
fi

OPTIONS=

if test -n "${REMOTE_LISTEN}"
then
	OPTIONS="${OPTIONS} --listen rfs://${REMOTE_LISTEN}:4044"
fi
if test -n "${SECURE_LISTEN}"
then
	#OPTIONS="${OPTIONS} --secure-listen rfss://admin:password1@${SECURE_LISTEN}:4045"
	OPTIONS="${OPTIONS} --secure-listen rfss://admin@${SECURE_LISTEN}:4045"
	OPTIONS="${OPTIONS} --certificate ${TMP_DIR}/cert.crt"
	OPTIONS="${OPTIONS} --private-key ${TMP_DIR}/priv.key"
fi

${GDB} ${SNAPRFS} ${OPTIONS}

