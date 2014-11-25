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
# Author: Francois Schiettecatte
# Creation Date: December, 2006
#


#--------------------------------------------------------------------------
#
# Description:
#
# This is a script to expire a file hierarchy. All options need 
# to be specified on the command line, so it is completely generic
#
#


#--------------------------------------------------------------------------
#
# Pragmatic modules
#

use strict;
use warnings;


#--------------------------------------------------------------------------
#
# Other Packages Needed
#

# Add paths for the location of modules
use lib '/home/poplar/lib/perl5';


#
# Perl native modules
#
use Time::Local;


#
# Perl extra modules
#
use Proc::PID::File;


#--------------------------------------------------------------------------
#
# Package Constants
#

BEGIN {
    
    # Set the locale to utf8
    $ENV{'LC_ALL'} = 'en_US.UTF-8';
    $ENV{'LANGUAGE'} = 'en_US.UTF-8';
    $ENV{'LANG'} = 'en_US.UTF-8';


    # Sleep trigger (in number of files)
    $main::sleepTrigger = 100;
    
    # Sleep interval (in seconds, 5 seconds)
    $main::sleepInterval = 5;


    # Check if the process is already running, bail if it is already running,
    # needs write access to '/var/run/MpsFileExpirer.pid'
    if ( Proc::PID::File->running(dir => '/var/run', name => 'MpsFileExpirer', verify => 1) ) { 
        printf(STDERR "The MPS file expirer process is already running.\n");
        exit (-1);
    }

}


#--------------------------------------------------------------------------
#
#    Function:      _expireFiles()
#
#    Purpose:       Expires the files in a specified directory, with a specified
#                   extension and of a specified age
#
#    Called by:     main()
#
#    Parameters:    $directoryPath          the directory (required)
#                   $fileNameExtension      the extension (optional)
#                   $expiration             the expiration in minutes (optional)
#                   $sleepTrigger           sleep trigger (defaults to $main::sleepTrigger)
#                   $sleepInterval          sleep interval (defaults to $main::sleepInterval)
#                   $continuous             set to 1 to run continuously
#
#    Globals:    
#
#    Returns:    0 on success, -1 on error
#
sub _expireFiles {

    my $directoryPath = shift;
    my $fileNameExtension = shift;
    my $expiration = shift;
    my $sleepTrigger = shift;
    my $sleepInterval = shift;
    my $continuous = shift;


    # Check the parameters
    if ( ! $directoryPath ) {
        printf(STDERR "The directory path is undefined.\n");
        return (-1);
    }
    
    if ( ! -d $directoryPath ) {
        printf(STDERR "The directory path: '%s' does not exist.\n", $directoryPath);
        return (-1);
    }
    
    if ( ! -x $directoryPath ) {
        printf(STDERR "The directory path: '%s' is inaccessible.\n", $directoryPath);
        return (-1);
    }


    # Set sleep trigger default
    if ( ! $sleepTrigger ) {
        $sleepTrigger = $main::sleepTrigger;
    }

    # Set sleep interval default
    if ( ! $sleepInterval ) {
        $sleepInterval = $main::sleepInterval;
    }


    # Create the 'find' command
    my $command = "/usr/bin/find " . $directoryPath . " -follow -type f ";
    
    # Add the extention if defined
    if ( $fileNameExtension ) {
        $command .= " -name \"*" . $fileNameExtension . "\"";
    }
    
    # Add the expiration if defined
    if ( $expiration ) {
        $command .= " \\( -amin +" . $expiration . " -or -empty \\)";
    }
        

    # Loop
    do {

        # Counters
        my $checkedCount = 0;
        my $statFailedCount = 0;
        my $deleteFailedCount = 0;
        my $deletedCount = 0;
    
#         printf("\$command: '%s'.\n", $command);
    
        # Run a find over the directory
        if ( !open(FIND_FILE, '-|', $command) ) {
            printf(STDERR "Failed to read the directory: '%s'.\n", $directoryPath);
            return (-1);
        }
        else {
            
            # Loop over each file
            while (<FIND_FILE>) {
            
                my $filePath = $_;
                chomp $filePath;
            
                $checkedCount++;
            
                # Delete the file
                if ( ! unlink($filePath) ) {
                    printf("Failed to delete the file: '%s'.\n", $filePath);
                    $deleteFailedCount++;
                }
                else {
#                     printf("\$filePath: '%s'.\n", $filePath);
                    $deletedCount++;
                }
                
                # Sleep if we have reached the sleep trigger 
                if ( ($checkedCount % $sleepTrigger) == 0 ) {
                    sleep($sleepInterval)
                }
            }
        }
    
        # Log
        printf("Checked: %d, stat failed: %d, delete failed: %d, deleted: %d.\n", 
                $checkedCount, $statFailedCount, $deleteFailedCount, $deletedCount);

        # Close the find file        
        close (FIND_FILE);

    } while ( $continuous == 1 );
    

    return (0);

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


# Set the defaults
my $directoryPath = undef;
my $fileNameExtension = undef;
my $expiration = undef;
my $sleepTrigger = $main::sleepTrigger;
my $sleepInterval = $main::sleepInterval;
my $continuous = 0;

# Check for command parameters
for ( my $argc = 0; $argc <= $#ARGV; $argc++ ) {

    my $option = $ARGV[$argc];

    if ( $option =~ /^--directory-path=(.*)$/i ) {
        $directoryPath = $1;
    }
    elsif ( $option =~ /^--file-name-extension=(.*)$/i ) {
        $fileNameExtension = $1;
    }
    elsif ( $option =~ /^--expiration=(.*)$/i ) {
        $expiration = $1;
    }
    elsif ( $option =~ /^--sleep-trigger=(\d+)$/i ) {
        $sleepTrigger = $1;
    }
    elsif ( $option =~ /^--sleep-interval=(\d+)$/i ) {
        $sleepInterval = $1;
    }
    elsif ( $option =~ /^--continuous$/i ) {
        $continuous = 1;
    }
    elsif ( $option =~ /^(--help|\-?)$/ ) {
        printf("Usage:\n\n");
        printf("\t[--help|-?] print out this message.\n");
        printf("\n");
        printf("\t[--directory-path=name] directory path (required, no default.) \n");
        printf("\t[--file-name-extension=name] file name extension (optional, no default.) \n");
        printf("\t[--expiration=#] expiration in minutes, '1' or '1m' = one minute, '1h' = one hour, '1d' = one day (required, no default)\n");
        printf("\t[--sleep-trigger=#] sleep trigger in number of files (default: %d files.)\n", $sleepTrigger);
        printf("\t[--sleep-interval=#] sleep interval in seconds (default: %d seconds.)\n", $sleepInterval);
        printf("\t[--continuous] run continuously.\n");
        exit (0);
    }
    else {
        printf(STDERR "Invalid action: '%s', type '%s  --help' for help.\n\n", $option, $0);
        exit (-1);
    }
}


# Check that the directory path is defined
if ( ! $directoryPath ) {
    printf(STDERR "The directory path is undefined.\n");
    exit (-1);
}

# Check that the directory path exists
if ( ! -d $directoryPath ) {
    printf(STDERR "The directory path: '%s' does not exist.\n", $directoryPath);
    exit (-1);
}

# Check that the directory path is accessible
if ( ! -x $directoryPath ) {
    printf(STDERR "The directory path: '%s' is inaccessible.\n", $directoryPath);
    exit (-1);
}


# Check that the expiration path was defined
if ( ! $expiration ) {
    printf(STDERR "The expiration was undefined.\n");
    exit (-1);
}

# Set the expiration, minutes
if ( $expiration =~ /(\d+)m/ ) {
    $expiration = $1;
}
# Hours
elsif ( $expiration =~ /(\d+)h/ ) {
    $expiration = $1 * 60;
}
# Days
elsif ( $expiration =~ /(\d+)d/ ) {
    $expiration = $1 * 60 * 24;
}
# Minutes again
elsif ( $expiration =~ /(\d+)/ ) {
    $expiration = $1;
}


# Expire the files
my $Status = &_expireFiles($directoryPath, $fileNameExtension, $expiration, $sleepTrigger, $sleepInterval, $continuous);    


exit (0);

