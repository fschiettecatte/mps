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
# This shell script rolls over the mpsserver log or the mpsgateway log 
# and produces a report from the log
#



#--------------------------------------------------------------------------
#
# Paths
#

ROOT_DIRECTORY_PATH="/home/poplar"

SCRIPT_DIRECTORY_PATH=$ROOT_DIRECTORY_PATH"/scripts"
REPORT_SCRIPT_PATH=$SCRIPT_DIRECTORY_PATH"/MpsLogReport.pl"

LOG_DIRECTORY_PATH=$ROOT_DIRECTORY_PATH"/logs"
MPS_SERVER_LOG_FILE_PATH=$LOG_DIRECTORY_PATH"/mpsserver.log"
MPS_GATEWAY_LOG_FILE_PATH=$LOG_DIRECTORY_PATH"/mpsgateway.log"

ARCHIVES_DIRECTORY_PATH=$LOG_DIRECTORY_PATH"/archives"
DATE=`/bin/date +%F`
MPS_SERVER_ARCHIVE_LOG_FILE_PATH=$ARCHIVES_DIRECTORY_PATH"/mpsserver-"$DATE".log"
MPS_GATEWAY_ARCHIVE_LOG_FILE_PATH=$ARCHIVES_DIRECTORY_PATH"/mpsgateway"$DATE".log"



#--------------------------------------------------------------------------
#
# Report script parameters
# 

EMAIL_FROM_ADDRESS="fschiettecatte@gmail.com"
EMAIL_TO_ADDRESSES="fschiettecatte@gmail.com"
MPS_SERVER_EMAIL_SUBJECT="MPS Server log"
MPS_GATEWAY_EMAIL_SUBJECT="MPS Gateway log"



#--------------------------------------------------------------------------
#
# See how we were called
#

case "$1" in
    server)
        LOG_FILE_PATH=$MPS_SERVER_LOG_FILE_PATH
        ARCHIVE_LOG_FILE_PATH=$MPS_SERVER_ARCHIVE_LOG_FILE_PATH
        EMAIL_SUBJECT=$MPS_SERVER_EMAIL_SUBJECT
        ;;
    gateway)
        LOG_FILE_PATH=$MPS_GATEWAY_LOG_FILE_PATH
        ARCHIVE_LOG_FILE_PATH=$MPS_GATEWAY_ARCHIVE_LOG_FILE_PATH
        EMAIL_SUBJECT=$MPS_GATEWAY_EMAIL_SUBJECT
        ;;
    *)
        echo "Usage: $0 {server|gateway}"
        exit 1
esac



#--------------------------------------------------------------------------
#
# Check the paths
#

# Fail flag
FAIL=0


if [ ! -x $REPORT_SCRIPT_PATH ]; then
    echo "The report script was not found: "$REPORT_SCRIPT_PATH
    FAIL=1
fi


if [ ! -d $LOG_DIRECTORY_PATH ]; then
    echo "The log directory was not found: "$LOG_DIRECTORY_PATH
    FAIL=1
fi


if [ ! -w $LOG_DIRECTORY_PATH ]; then
    echo "The log directory cannot be accessed: " $LOG_DIRECTORY_PATH
    FAIL=1
fi


if [ ! -f $LOG_FILE_PATH ]; then
    echo "The log file was not found: "$LOG_FILE_PATH
    FAIL=1
fi


if [ ! -d $ARCHIVES_DIRECTORY_PATH ]; then
    echo "The archives directory was not found: "$ARCHIVES_DIRECTORY_PATH
    FAIL=1
fi

if [ ! -w $ARCHIVES_DIRECTORY_PATH ]; then
    echo "The archives directory cannot be accessed: "$ARCHIVES_DIRECTORY_PATH
    FAIL=1
fi


if [ ! -d $SCRIPT_DIRECTORY_PATH ]; then
    echo "The script directory was not found: "$SCRIPT_DIRECTORY_PATH
    FAIL=1
fi


# Exit here if we failed
if [ $FAIL = 1 ]; then
    exit 1
fi



#--------------------------------------------------------------------------
#
#
#

# Move the log file to the archive
/bin/mv $LOG_FILE_PATH $ARCHIVE_LOG_FILE_PATH


# Touch a new file
/bin/touch $LOG_FILE_PATH


# Run the report script
$REPORT_SCRIPT_PATH --log=$ARCHIVE_LOG_FILE_PATH --totals --totals-per-index --totals-per-client \
    --processes-per-hour --searches-per-hour --search-timings --slow-search-time=5 --slow-searches=100 \
    --top-searches=100 --format=text --destination=email --email-from-address="$EMAIL_FROM_ADDRESS" \
    --email-to-addresses="$EMAIL_TO_ADDRESSES" --subject="$EMAIL_SUBJECT" \
    --disposition=inline --cleanup


exit 0

