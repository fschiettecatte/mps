#!/usr/bin/env perl


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
# Author: Francois Schiettecatte (FS Consulting LLC.)
# Creation Date: ???
#


#--------------------------------------------------------------------------
#
# Description:
#
# This script generates an spi or lwps script file from 
# an mpsserver/mpsgateway log file
#


#--------------------------------------------------------------------------
#
# Pragmatic Modules
#

use strict;
use warnings;


#--------------------------------------------------------------------------
#
# Package Constants
#

BEGIN {

    # Set the locale to utf8
    $ENV{'LC_ALL'} = 'en_US.UTF-8';
    $ENV{'LANGUAGE'} = 'en_US.UTF-8';
    $ENV{'LANG'} = 'en_US.UTF-8';


    # Formats
    $main::formatSpiScript = 'spi';
    $main::formatLwpsScript = 'lwps';


    # Base directory path
    $main::baseDirectoryPath = '/home/poplar';

    # Index directory path
    $main::indexDirectoryPath = sprintf("%s/index", $main::baseDirectoryPath);

    # Configuration directory path
    $main::configurationDirectoryPath = sprintf("%s/conf", $main::baseDirectoryPath);


    # Protocols
    $main::protocolTcp = 'tcp';
    $main::protocolUdp = 'udp';

    # Protocol
    $main::protocol = $main::protocolTcp;

    # Host
    $main::host = 'localhost';

    # Port
    $main::port = 9001;


    # Language
    $main::language = 'en';

    # Index
    $main::index = 'items';

    # Start search offset
    $main::startSearchOffset = 0;

    # End search offset
    $main::endSearchOffset = 9;

}



#--------------------------------------------------------------------------
#
#    Function:      main()
#
#    Purpose:       main
#
#    Called by:     
#
#    Parameters:    
#
#    Globals:    
#
#    Returns:       void
#


# Autoflush STDOUT
$| = 1;


# Set the log file path
my $logFilePath = undef;

# Set the script format
my $format = $main::formatSpiScript;

# Set the script defaults
my $indexDirectoryPath = $main::indexDirectoryPath;
my $configurationDirectoryPath = $main::configurationDirectoryPath;

my $protocol = $main::protocol;
my $host = $main::host;
my $port = $main::port;

my $language = $main::language;
my $index = $main::index;
my $startSearchOffset = $main::startSearchOffset;
my $endSearchOffset = $main::endSearchOffset;


# Check for command parameters
for ( my $Argc = 0; $Argc <= $#ARGV; $Argc++ ) {

    my $option = $ARGV[$Argc];

    if ( $option =~ /^--log=(.*)$/i ) {
        $logFilePath = $1;
    }
    elsif ( $option =~ /^--format=(.*)$/i ) {
        $format = $1;
    }
    elsif ( $option =~ /^--index-directory=(.*)$/i ) {
        $indexDirectoryPath = $1;
    }
    elsif ( $option =~ /^--configuration-directory=(.*)$/i ) {
        $configurationDirectoryPath = $1;
    }
    elsif ( $option =~ /^--protocol=(.*)$/i ) {
        $protocol = $1;
    }
    elsif ( $option =~ /^--host=(.*)$/i ) {
        $host = $1;
    }
    elsif ( $option =~ /^--port=([\d+])$/i ) {
        $port = $1;
    }
    elsif ( $option =~ /^--language=(.*)$/i ) {
        $language = $1;
    }
    elsif ( $option =~ /^--index=(.*)$/i ) {
        $index = $1;
    }
    elsif ( $option =~ /^--start-search-offset=([\d+])$/i ) {
        $startSearchOffset = $1;
    }
    elsif ( $option =~ /^--end-search-offset=([\d+])$/i ) {
        $endSearchOffset = $1;
    }

    elsif ( $option =~ /^(--help|\-?)$/ ) {
        printf("Usage:\n\n");
        printf("\t[--help|-?] print out this message.\n");
        printf("\n");
        printf("\t[--log=name] log file path, defaults to 'stdin'.\n");
        printf("\n");
        printf("\t[--format=name] format, defaults to '%s' ('%s' or '%s').\n", $format, $main::formatSpiScript, $main::formatLwspScript);
        printf("\n");
        printf("\t[--index-directory=path] index directory, defaults to '%s'.\n", $indexDirectoryPath);
        printf("\t[--configuration-directory=path] configuration directory, defaults to '%s'.\n", $configurationDirectoryPath);
        printf("\n");
        printf("\t[--protocol=name] protocol, defaults to '%s' ('%s' or '%s').\n", $protocol, $main::protocolTcp, $main::protocolUdp);
        printf("\t[--host=name] host, defaults to '%s'.\n", $host);
        printf("\t[--port=#] port, defaults to %d.\n", $port);
        printf("\n");
        printf("\t[--language=name] language, defaults to '%s'.\n", $language);
        printf("\t[--index=name] index, defaults to '%s'.\n", $index);
        printf("\t[--start-search-offset=#] start search offset, defaults to %d.\n", $startSearchOffset);
        printf("\t[--end-search-offset=#] end search offset, defaults to %d.\n", $endSearchOffset);
        printf("\n");
        exit (0);
    }
    else {
        printf(STDERR "Invalid action: '%s', type '%s  --help' for help.\n\n", $option, $0);
        exit (-1);
    }
}



# Check that the file is there and accessible if it was specified
if ( defined($logFilePath) ) {
    
    if ( ! -f $logFilePath ) {
        printf(STDERR "Failed to find the log file: '%s', exiting.\n", $logFilePath);
        exit (-1);
    }
    elsif ( ! -r $logFilePath ) {
        printf(STDERR "Failed to access the log file: '%s', exiting.\n", $logFilePath);
        exit (-1);
    }
}


# Check the format
if ( ($format ne $main::formatSpiScript) && ($format ne $main::formatLwspScript) ) {
    printf(STDERR "Invalid format: '%s', exiting.\n", $format);
    exit (-1);
}


# Check the protocol, only if the format is LWPS
if ( $format eq $main::formatLwpsScript  ) {
    if ( ($protocol ne $main::protocolTcp) && ($protocol ne $main::protocolUdp) ) {
        printf(STDERR "Invalid protocol: '%s', exiting.\n", $protocol);
        exit (-1);
    }
}



# The log file
my $LOG_FILE;

# Open the log file
if ( defined($logFilePath) ) {
    if ( ! open($LOG_FILE, $logFilePath) ) {
        printf(STDERR "Failed to open the log file: '%s', exiting.\n", $logFilePath);
        exit (-1);
    }
}
else {
    $LOG_FILE = *STDIN;
}



# Output the header
if ( $format eq $main::formatSpiScript  ) {
    printf("initializeServer %s %s\n\n", $indexDirectoryPath, $configurationDirectoryPath);

}
elsif ( $format eq $main::formatLwpsScript  ) {
    printf("openConnection %s %s %d\n\n", $protocol, $host, $port);
}

printf("language %s\n\n", $language);
printf("openIndex %s\n\n", $index);
printf("searchOffsets %d %d\n\n", $startSearchOffset, $endSearchOffset);


# Process the log file
while (<$LOG_FILE>) {

    # 6664: 16: 25/Jan/2006:08:03:03 -0800: INFO: Search, index: 'items', search: 'zipcodes {sort:relevance:desc} {debug:enable} \
    # {tag:SearchAPI.php}', found: 587 documents and returned: 31 (+3 search reports), in: 0.579306 seconds.
    if ( $_ =~ /^\d+.*?Search, (index|indices): '.*?', search: '(.*?)', found: .*$/ ) {
        printf("search %s\n", $1);
    }

}


# Output the footer
print("\n\n");
printf("closeIndex %s\n\n", $index);

if ( $format eq $main::formatSpiScript  ) {
    printf("shutdownServer\n\n");
}
elsif ( $format eq $main::formatLwpsScript  ) {
    printf("closeConnection\n\n");
}

print("exit\n");



# Close the log file
if ( defined($logFilePath) ) {
    close($LOG_FILE);
}



exit (0);

