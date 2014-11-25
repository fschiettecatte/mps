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
# Creation Date: ????
#


#--------------------------------------------------------------------------
#
# Description:
#
# This script downloads and indexes the omim data.
#


#--------------------------------------------------------------------------
#
# Pragmatic modules
#

use strict;
use warnings;


#--------------------------------------------------------------------------
#
# Required packages
#

#
# Perl extra modules
#
use Net::FTP;


#--------------------------------------------------------------------------
#
# Package Constants
#

BEGIN {

    # Set the locale to utf8
    $ENV{'LC_ALL'} = 'en_US.UTF-8';
    $ENV{'LANGUAGE'} = 'en_US.UTF-8';
    $ENV{'LANG'} = 'en_US.UTF-8';


    # Additional LD library paths to add
    @main::libraryDirectoryPaths = ('/home/poplar/lib/icu/lib', '/home/poplar/lib/mecab/lib');

    # Add the LD library paths to the LD_LIBRARY_PATH environment variable if they are not there
    foreach my $libraryDirectoryPath ( @main::libraryDirectoryPaths ) {
    if ( !$ENV{'LD_LIBRARY_PATH'} || (index($ENV{'LD_LIBRARY_PATH'}, $libraryDirectoryPath) == -1) ) {
            $ENV{'LD_LIBRARY_PATH'} .= ':' if $ENV{'LD_LIBRARY_PATH'};
            $ENV{'LD_LIBRARY_PATH'} .= $libraryDirectoryPath;
        }
    }



    # Temporary directory path
    $main::temporaryDirectoryPath = '/var/tmp';


    # Index name
    $main::indexName = 'omim';


    # Ftp server name
    $main::ftpServerName = 'grcf.jhmi.edu';
    
    # Ftp user name
    $main::ftpUserName = 'anonymous';
    
    # Ftp user password
    $main::ftpUserPassword = 'fschiettecatte@gmail.com';
    
    # Ftp file name
    $main::ftpFileName = 'omim.txt.Z';
    
    # Ftp file path
    $main::ftpFilePath = sprintf("/OMIM/%s", $main::ftpFileName);


    # Download directory path
    $main::downloadDirectoryPath = sprintf("%s/%s", $main::temporaryDirectoryPath, $main::indexName);
    
    # Download file path
    $main::downloadFilePath = sprintf("%s/%s", $main::downloadDirectoryPath, $main::ftpFileName);


    # Base directory path
    $main::baseDirectoryPath = '/var/lib/mps';

    # Root data directory path
    $main::rootDataDirectoryPath = sprintf("%s/data", $main::baseDirectoryPath);

    # Data directory path
    $main::dataDirectoryPath = sprintf("%s/%s", $main::rootDataDirectoryPath, $main::indexName);

    # Data file name
    $main::dataFileName = sprintf("%s.txt", $main::indexName);

    # Data file path
    $main::dataFilePath = sprintf("%s/%s", $main::dataDirectoryPath, $main::dataFileName);


    # Parse directory path
    $main::parseDirectoryPath = sprintf("%s/parse", $main::baseDirectoryPath);

    # Parse file name
    $main::parseFileName = sprintf("%s.prs", $main::indexName);

    # Parse file path
    $main::parseFilePath = sprintf("%s/%s", $main::parseDirectoryPath, $main::parseFileName);


    # Index directory path
    $main::indexDirectoryPath = sprintf("%s/index", $main::baseDirectoryPath);


    # Archive directory path
    $main::archiveDirectoryPath = '/home/francois';

    # Archive file name
    $main::archiveFileName = sprintf("%s.tgz", $main::indexName);

    # Archive file path
    $main::archiveFilePath = sprintf("%s/%s", $main::archiveDirectoryPath, $main::archiveFileName);


    # Application path
    $main::applicationPath = '/home/poplar/bin';

    # Parser application path
    $main::parserApplicationPath = sprintf("%s/mpsparser", $main::applicationPath);

    # Indexer application path
    $main::indexerApplicationPath = sprintf("%s/mpsindexer", $main::applicationPath);

    # Configuration directory path
    $main::configurationDirectoryPath = sprintf("%s/conf", $main::baseDirectoryPath);

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


# Redirect stderr to stdout
*STDERR = *STDOUT;

# Autoflush STDOUT
$| = 1;


# Log
print("Preparing download directory: '$main::downloadDirectoryPath'.\n");

# Delete download directory if it is there
if ( -f $main::downloadDirectoryPath || -d $main::downloadDirectoryPath ) {
    if ( system("/bin/rm -rf $main::downloadDirectoryPath") != 0 ) {
        die "Failed to delete the download directory.\n";
    }
}

# Make download directory
if ( ! mkdir ($main::downloadDirectoryPath, 0700) ) {
    die "Failed to create the download directory.\n";
}



# Log
print("Downloading: '$main::ftpServerName:/$main::ftpFilePath', to: '$main::downloadFilePath'.\n");

# Open the ftp connection
my $ftp = Net::FTP->new($main::ftpServerName, Timeout=>120, Passive=>1, Debug => 0);
if ( ! defined($ftp) ) {
    die "Failed to open a connection to the ftp server, error: '$@'.\n";
}

# Log in
if ( ! $ftp->login($main::ftpUserName, $main::ftpUserPassword) ) {
    die "Failed to log into the ftp server.\n";
}

# Set to binary
if ( ! $ftp->binary ) {
    die "Failed to set the transfer mode to binary.\n";
}

# Set to passive
if ( ! $ftp->pasv ) {
    die "Failed to set the transfer mode to passive.\n";
}

# Download the file
if ( ! defined($ftp->get($main::ftpFilePath, $main::downloadFilePath)) ) {
    die "Failed to download the file from the ftp server.\n";
}

# Close the ftp connection
$ftp->quit;



# Log
print("Decompressing: '$main::downloadFilePath', to: '$main::dataFilePath'.\n");

# Decompress downloaded file
if ( system("/bin/gunzip -c $main::downloadFilePath > $main::dataFilePath") != 0 ) {
    die "Failed to decompress the file: '$main::downloadFilePath'.\n";
}

# Delete download directory
if ( system("/bin/rm -rf $main::downloadDirectoryPath") != 0 ) {
    die "Failed to delete the download directory: '$main::downloadDirectoryPath'.\n";
}



# Log
print("Parsing: '$main::dataFilePath', to: '$main::parseFilePath'.\n");

# Parse command line
my $parseCommandLine = "$main::parserApplicationPath --configuration-directory=$main::configurationDirectoryPath --type=omim --tokenizer=fsclt-2 $main::dataFilePath > $main::parseFilePath";
# print("\$parseCommandLine: [$parseCommandLine].\n");

# Parse
if ( system($parseCommandLine) != 0 ) {
    die "Failed to parse: '$main::dataFilePath', to: '$main::parseFilePath'.\n";
}



# Log
print("Indexing: '$main::parseFilePath', to: '$main::indexDirectoryPath', index: '$main::indexName'.\n");

# Index command line
my $indexCommandLine = "/bin/cat $main::parseFilePath | $main::indexerApplicationPath --configuration-directory=$main::configurationDirectoryPath --index-directory=$main::indexDirectoryPath --index=$main::indexName --stemmer=plural --stoplist=none --maximum-memory=512 --suppress --log=stdout";
# print("\$indexCommandLine: [$indexCommandLine].\n");

# Index
if ( system($indexCommandLine) != 0 ) {
    die "Failed to index: '$main::parseFilePath', to: '$main::indexDirectoryPath', index: '$main::indexName'.\n";
}



# Log
print("Archiving: '$main::dataDirectoryPath', to: '$main::archiveFilePath'.\n");

# Archive
if ( system("cd $main::rootDataDirectoryPath && /bin/tar zcf $main::archiveFilePath $main::indexName") != 0 ) {
    die "Failed to archive the data directory: '$main::dataDirectoryPath'.\n";
}



# Log
print("Finished.\n");


exit (0);

