#!/bin/sh


#*****************************************************************************
#       Copyright (C) 1993-2011, FS Consulting LLC. All rights reserved      *
#                                                                            *
#  This notice is intended as a precaution against inadvertent publication   *
#  and does not constitute an admission or acknowledgement that publication  *
#  has occurred or constitute a waiver of confidentiality.                   *
#                                                                            *
#  This software is the proprietary and confidential property                *
#  of FS Consulting LLC.                                                     *
#*****************************************************************************


#--------------------------------------------------------------------------
#
# Author: Francois Schiettecatte
# Creation Date: December 2006
#


#--------------------------------------------------------------------------
#
# Description:
#
# This shell script starts/stops the MPS Server daemon
#


#
# Source function library
#
. /etc/init.d/functions


#
# Paths
#
ROOT_DIRECTORY_PATH="/home/poplar/"


#
# Library path (for ICU and MeCab)
#
LIBRARY_DIRECTORY_PATH=$ROOT_DIRECTORY_PATH"/lib"
export LD_LIBRARY_PATH=$LIBRARY_DIRECTORY_PATH"/icu/lib:"$LIBRARY_DIRECTORY_PATH"/mecab/lib:"


#
# MPS Server Application
#
APPLICATION_NAME="mpsserver"
APPLICATION_DIRECTORY_PATH=$ROOT_DIRECTORY_PATH"/bin"
APPLICATION_PATH=$APPLICATION_DIRECTORY_PATH"/"$APPLICATION_NAME


#
# MPS Server Parameters
#
MPS_TCP_SOCKET="tcp:9001"
#MPS_UDP_SOCKET="udp:9001"
#HTTP_TCP_SOCKET="tcp:80"
INDEX_DIRECTORY_PATH=$ROOT_DIRECTORY_PATH"/index"
CONFIGURATION_DIRECTORY_PATH=$ROOT_DIRECTORY_PATH"/conf"
CONFIGURATION_FILE_PATH=$CONFIGURATION_DIRECTORY_PATH"/search.cf"
CHILDREN_COUNT="50"
SESSIONS_COUNT="200"
MAX_LOAD="10"
MAX_INFORMATION_LOAD="15"
USER="nobody"
PROCESS_ID_FILE_PATH="/var/run/"$APPLICATION_NAME".pid"
LOG_DIRECTORY_PATH=$ROOT_DIRECTORY_PATH"/logs"
LOG_FILE_PATH=$LOG_DIRECTORY_PATH"/mpsserver.log"



checkconfig() {
 
    # Fail flag
    FAIL=0

   if [ ! -x $APPLICATION_PATH ]; then
        echo "The MPS Server application was not found: "$APPLICATION_PATH
        FAIL=1
    fi

    if [ ! -d $INDEX_DIRECTORY_PATH ]; then
        echo "The index directory was not found: "$INDEX_DIRECTORY_PATH
        FAIL=1
    fi

    if [ ! -d $CONFIGURATION_DIRECTORY_PATH ]; then
        echo "The configuration directory was not found: " $CONFIGURATION_DIRECTORY_PATH
        FAIL=1
    fi

    if [ ! -f $CONFIGURATION_FILE_PATH ]; then
        echo "The configuration file was not found: " $CONFIGURATION_FILE_PATH
        FAIL=1
    fi

    # Exit here if we failed
    if [ $FAIL = 1 ]; then
        return 1
    fi
    
}


start() {
    checkconfig || return 1
    echo -n "Starting MPS Server"
    daemon $APPLICATION_PATH --socket=$MPS_TCP_SOCKET --index-directory=$INDEX_DIRECTORY_PATH \
            --configuration-directory=$CONFIGURATION_DIRECTORY_PATH --children=$CHILDREN_COUNT \
            --sessions=$SESSIONS_COUNT --max-load=$MAX_LOAD --max-information-load=$MAX_INFORMATION_LOAD \
            --user=$USER --process-id-file=$PROCESS_ID_FILE_PATH --log=$LOG_FILE_PATH --daemon
    RETVAL=$?
    echo
    return $RETVAL
}


stop() {
    echo -n "Stopping MPS Server"
    killproc $APPLICATION_NAME
    RETVAL=$?
    echo
    return $RETVAL
}


# See how we were called.
case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        stop
        sleep 10
        start
        ;;
    *)
    echo "Usage: $0 {start|stop|restart}"
    exit 1
esac


exit 0

