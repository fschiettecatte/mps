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
# This script downloads and indexes the hci-bib data.
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
use HTTP::Request;
use LWP::UserAgent;


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
    $main::indexName = 'hci-bib';


    # User agent
    $main::userAgent = 'HCI-BIB Updater/1.0; FS Consulting LLC.';
    
    # Email address
    $main::emailAddress = 'fschiettecatte@gmail.com';
    
    # Base url
    $main::baseUrl = 'http://www.hcibib.org';

    # List url
    $main::listUrl = sprintf("%s/listdir.cgi", $main::baseUrl);

    # List url
    $main::downloadUrl = sprintf("%s/bibdata", $main::baseUrl);

    # Retries
    $main::retries = 5;
    
    # Sleep
    $main::sleep = 2;


    # Download directory path
    $main::downloadDirectoryPath = sprintf("%s/%s", $main::temporaryDirectoryPath, $main::indexName);
    

    # Base directory path
    $main::baseDirectoryPath = '/var/lib/mps';

    # Root data directory path
    $main::rootDataDirectoryPath = sprintf("%s/data", $main::baseDirectoryPath);

    # Data directory path
    $main::dataDirectoryPath = sprintf("%s/%s", $main::rootDataDirectoryPath, $main::indexName);


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
#    Function:    main()
#
#    Purpose:    main
#
#    Called by:    
#
#    Parameters:    
#
#    Globals:    
#
#    Returns:    void
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
print("Downloading file list from: '$main::listUrl'.\n");

# Get the page which lists all the files we can download
my $header = HTTP::Headers->new(Content_Type => 'text/html', User_Agent => $main::userAgent, From => $main::emailAddress);
my $userAgent = LWP::UserAgent->new();
my $request = HTTP::Request->new('GET', $main::listUrl, $header);
my $response = $userAgent->request($request);

# Check the request response
if ( ! $response->is_success ) {
    die("Failed to get the file list page, error: '" . $response->error_as_HTML . "'\n");
}
# else {
#     print("Response->content [%s]\n", $response->content);
# }


# Get the list of files names from the page, file names end in '.tmp' and '.bib'
# <a href=/ftp/DL03.bib>file</a>
my @fileNames = ($response->content =~ /<a href='\/bibdata\/([^.]*\.bib|[^.]*\.tmp)'.*?>file<\/a>/gis);

# map(print("[%s]\n", $_), @fileNames);



# Log
printf("Downloading %d files from: '$main::downloadUrl'.\n", scalar(@fileNames));

# Download the files
foreach my $fileName ( sort(@fileNames) ) {

    # Set the success flag
    my $success = 0;

    # Set the requests counter
    my $requests = 0;

    # Put together the download url
    my $downloadUrl = sprintf("%s/%s", $main::downloadUrl, $fileName);

    # Create a download file path
    my $downloadFilePath = sprintf("%s/%s", $main::downloadDirectoryPath, $fileName);

    # Log
    print("Downloading: '$downloadUrl', to: '$downloadFilePath'.\n");
    
    # Try to get the file
    while ( $requests < $main::retries ) {

        # Sleep between requests
        sleep($main::sleep);
        
        # Put together the request
        my $header = HTTP::Headers->new(Content_Type => 'application/x-www-form-urlencoded');
        my $userAgent = LWP::UserAgent->new();
        my $request = HTTP::Request->new('GET', $downloadUrl, $header);

        # Make the request
        my $response = $userAgent->request($request);
        
        # Check the request response
        if ( $response->is_success ) {
    
            # Save the content to the local file
            if ( open(FILE, '>', $downloadFilePath) ) {
                print(FILE "%s", $response->content);
                close (FILE);
            }
            else {
                die "Failed to save '$downloadUrl', to: '$downloadFilePath'.\n";
            }
            
            # Set the success flag and exit the while loop
            $success = 1;
            last;
        }
        
        print("Retrying: '%s'...\n", $downloadUrl);

        # Increment the requests counter by one
        $requests++;
    }

    # Check the success flag, give up if we failed to download the file
    if ( $success == 0 ) {
        die "Failed to download: '" . $downloadUrl . "'.\n";
    }
}



# Log
print("Moving files from: '$main::downloadDirectoryPath', to: '$main::dataDirectoryPath'.\n");

# Delete old files
if ( system("/bin/rm -f $main::dataDirectoryPath/*") != 0 ) {
    die "Failed to delete the old files.\n";
}

# Move new files into place
if ( system("/bin/mv -f $main::downloadDirectoryPath/* $main::dataDirectoryPath") != 0 ) {
    die "Failed to move the downloaded files into place.\n";
}

# Delete download directory
if ( system("/bin/rm -rf $main::downloadDirectoryPath") != 0 ) {
    die "Failed to delete the download directory: '$main::downloadDirectoryPath'.\n";
}

# Rename the typos file
if ( -f "$main::dataDirectoryPath/TYPOS.bib" ) {
    if ( system("/bin/mv -f $main::dataDirectoryPath/TYPOS.bib $main::dataDirectoryPath/TYPOS.bib-back") != 0 ) {
        die "Failed to rename the typos file.\n";
    }
}



# Log
print("Parsing: '$main::dataDirectoryPath', to: '$main::parseFilePath'.\n");

# Parse command line
my $parseCommandLine = "$main::parserApplicationPath --configuration-directory=$main::configurationDirectoryPath --type=refer --item=abstract --tokenizer=fsclt-2 --autokey $main::dataDirectoryPath/*.bib > $main::parseFilePath";
# print("\$parseCommandLine: [$parseCommandLine].\n");

# Index
if ( system($parseCommandLine) != 0 ) {
    die "Failed to parse: '$main::dataDirectoryPath', to: '$main::parseFilePath'.\n";
}


# Log
print("Indexing: '$main::parseFilePath', to: '$main::indexDirectoryPath', index: '$main::indexName'.\n");

# Index command line
my $indexCommandLine = "/bin/cat $main::parseFilePath | $main::indexerApplicationPath --configuration-directory=$main::configurationDirectoryPath --index-directory=$main::indexDirectoryPath --index=$main::indexName --stemmer=plural --stoplist=none --maximum-memory=512 --suppress --log=stdout";
# print("\$indexCommandLine: [$indexCommandLine].\n");

# Index
if ( system($indexCommandLine) != 0 ) {
    die "Failed to create the index: '$main::indexName'.\n";
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

