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
# This shell script starts/stops the MPS Gateway daemon
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
# MPS Gateway Path
#
APPLICATION_NAME="mpsgateway"
APPLICATION_DIRECTORY_PATH=$ROOT_DIRECTORY_PATH"/bin"
APPLICATION_PATH=$APPLICATION_DIRECTORY_PATH"/"$APPLICATION_NAME


#
# MPS Gateway Parameters
#
MPS_TCP_SOCKET="tcp:9000"
#MPS_UDP_SOCKET="udp:9000"
#HTTP_TCP_SOCKET="tcp:80"
CONFIGURATION_DIRECTORY_PATH=$ROOT_DIRECTORY_PATH"/conf"
CONFIGURATION_FILE_PATH=$CONFIGURATION_DIRECTORY_PATH"/gateway.cf"
CHILDREN_COUNT="50"
SESSIONS_COUNT="200"
STARTUP_INTERVAL="100"
MAX_LOAD="15"
USER="nobody"
PROCESS_ID_FILE_PATH="/var/run/"$APPLICATION_NAME".pid"
LOG_DIRECTORY_PATH=$ROOT_DIRECTORY_PATH"/logs"
LOG_FILE_PATH=$LOG_DIRECTORY_PATH"/mpsgateway.log"



checkconfig() {

    # Fail flag
    FAIL=0

    if [ ! -x $APPLICATION_PATH ]; then
        echo "The MPS Gateway application was not found :"$APPLICATION_PATH
        FAIL=1
    fi

    if [ ! -d $CONFIGURATION_DIRECTORY_PATH ]; then
        echo "The configuration directory was not found :"$CONFIGURATION_DIRECTORY_PATH
        FAIL=1
    fi

    if [ ! -d $CONFIGURATION_FILE_PATH ]; then
        echo "The configuration file was not found :"$CONFIGURATION_FILE_PATH
        FAIL=1
    fi

    # Exit here if we failed
    if [ $FAIL = 1 ]; then
        return 1
    fi
    
}


start() {
    checkconfig || return 1
    echo -n "Starting MPS Gateway"
    daemon $APPLICATION_PATH --socket=$MPS_TCP_SOCKET --configuration-directory=$CONFIGURATION_DIRECTORY_PATH \
            --children=$CHILDREN_COUNT --sessions=$SESSIONS_COUNT --startup-interval=$STARTUP_INTERVAL \
            --max-load=$MAX_LOAD --user=$USER --process-id-file=$PROCESS_ID_FILE_PATH --log=$LOG_FILE_PATH --daemon
    RETVAL=$?
    echo
    return $RETVAL
}


stop() {
    echo -n "Stopping MPS Gateway"
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

