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
# Creation Date: October 2004
#


#--------------------------------------------------------------------------
#
# Description:
#
# This script produces a report from a mpsserver/mpsgateway log. 
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
use Encode;
use MIME::Base64;
use Sys::Hostname;


#
# Perl extra modules
#
use Mail::Sendmail;
use Spreadsheet::WriteExcel;


#--------------------------------------------------------------------------
#
# Package Constants
#

BEGIN {
    
    # Set the locale to utf8
    $ENV{'LC_ALL'} = 'en_US.UTF-8';
    $ENV{'LANGUAGE'} = 'en_US.UTF-8';
    $ENV{'LANG'} = 'en_US.UTF-8';


    # Get the current date
    my (undef, undef, undef, $day, $month, $year, undef, undef, undef) = localtime();
    $main::date = sprintf("%d-%02d-%02d", $year + 1900, $month + 1, $day);


    # Temporary directory path
    $main::temporaryDirectoryPath = '/var/tmp';



    # Slow search time
    $main::slowSearchTime = 5;
    
    # Slow search count
    $main::slowSearchCount = 100;
    
    # Top search count
    $main::topSearchCount = 100;
    
    # Top search frequencies count
    $main::topSearchFrequenciesCount = 100;
    

    # Sleep line count
    $main::sleepLineCount = 10000;
    
    # Sleep interval
    $main::sleepInterval = 2;
    


    # Destinations
    $main::destinationStdout = 'stdout';
    $main::destinationFile = 'file';
    $main::destinationEmail = 'email';


    # Format
    $main::formatText = 'text';
    $main::formatExcel = 'excel';


    # Email disposition
    $main::emailDispositionAttachment = 'attachment';
    $main::emailDispositionInline = 'inline';


    # File names
    $main::fileNameSearchLogReport = sprintf("SearchLogReport-%s", $main::date);



    # Month abbreviation to number mapping
    %main::monthAbbreviations = (
        'Jan',            1,
        'Feb',            2,
        'Mar',            3,
        'Apr',            4,
        'May',            5,
        'Jun',            6,
        'Jul',            7,
        'Aug',            8,
        'Sep',            9,
        'Oct',            10,
        'Nov',            11,
        'Dec',            12,
    );

}



#--------------------------------------------------------------------------
#
#    Function:      _createMailAttachment()
#
#    Purpose:       Create and return a mail attachement
#
#    Called by:    
#
#    Parameters:    $boundary        Attachement boundary
#                   $fileName        File name in attachement
#                   $filePath        File path for attachement
#                   $compress        Compress the file
#
#    Globals:    
#
#    Returns:       mail attachement
#
sub _createMailAttachment {

    my $boundary = shift;    
    my $fileName = shift;    
    my $filePath = shift;    
    my $compress = shift;    


    # The file content
    my $fileContent = undef;

    # Compress the file if requested
    if ( $compress ) {

        # Create the command line to compress the file
        my $commandLine = sprintf("gzip -c %s", $filePath);
    
        # Read in the file
        if ( ! open (FILE, '-|', $commandLine) ) {
            printf("Failed to open the file: '%s', exiting.\n", $filePath);
            exit (-1);
        }
        binmode FILE; 
        undef $/;
        my $fileContent = encode_base64(<FILE>);
        close (FILE);

        # Create and return the attachement 
        return(sprintf("--%s\nContent-Transfer-Encoding: base64\nContent-Type: text/plain; name=\"%s.gz\"\nContent-Disposition: attachment; filename=%s.gz\n\n%s\n\n", 
                $boundary, $fileName, $fileName, $fileContent));

    }
    else {

        # Read in the file
        if ( ! open (FILE, $filePath) ) {
            printf("Failed to open the file: '%s', exiting.\n", $filePath);
            exit (-1);
        }
        binmode FILE; 
        undef $/;
        my $fileContent = encode_base64(<FILE>);
        close (FILE);

        # Create and return the attachement 
        return(sprintf("--%s\nContent-Transfer-Encoding: base64\nContent-Type: application/octet-stream; name=\"%s\"\nContent-Disposition: attachment; filename=%s\n\n%s\n\n", 
                $boundary, $fileName, $fileName, $fileContent));
    }

}



#--------------------------------------------------------------------------
#
#    Function:      _createMailInline()
#
#    Purpose:       Create and return mail content
#
#    Called by:    
#
#    Parameters:    $boundary        Attachement boundary
#                   $name            Name
#                   $fileName        File name in attachement
#                   $filePath        File path for attachement
#
#    Globals:    
#
#    Returns:    mail content
#
sub _createMailInline {

    my $boundary = shift;    
    my $name = shift;    
    my $fileName = shift;    
    my $filePath = shift;    


    # Read in the counts file
    if ( ! open (FILE, $filePath) ) {
        printf("Failed to open the file: '%s', exiting.\n", $filePath);
        exit (-1);
    }
    binmode FILE; 
    undef $/;
    my $fileContent = <FILE>;
    close (FILE);
    
    # Create and return the content
    return(sprintf("--%s\nContent-Transfer-Encoding: 7bit\nContent-Type: text/plain; name=\"%s\"\n\n%s\n\n%s\n\n\n\n", 
            $boundary, $fileName, $name, $fileContent));

}



#--------------------------------------------------------------------------
#
#    Function:      _cleanSearch()
#
#    Purpose:       Clean the search
#
#    Called by:    
#
#    Parameters:    $search        search
#
#    Globals:    
#
#    Returns:       the cleaned search
#
sub _cleanSearch {

    my $search = shift;    

    
    # Catch invalid searches
    if ( !defined($search) ) {
        return ($search);
    }


    # Search engine modifiers
    $search =~ s/{search_results:return}//gi;
    $search =~ s/{search_results:suppress}//gi;
    $search =~ s/{sr:r}//gi;
    $search =~ s/{sr:s}//gi;

    $search =~ s/{search_report:enable}//gi;
    $search =~ s/{search_report:disable}//gi;
    $search =~ s/{sr:e}//gi;
    $search =~ s/{sr:d}//gi;

    $search =~ s/{search_cache:enable}//gi;
    $search =~ s/{search_cache:disable}//gi;
    $search =~ s/{sc:e}//gi;
    $search =~ s/{sc:d}//gi;

    $search =~ s/{debug:enable}//gi;
    $search =~ s/{debug:disable}//gi;
    $search =~ s/{d:e}//gi;
    $search =~ s/{d:d}//gi;

    $search =~ s/{boolean_operator:or}//gi;
    $search =~ s/{boolean_operator:ior}//gi;
    $search =~ s/{boolean_operator:xor}//gi;
    $search =~ s/{boolean_operator:and}//gi;
    $search =~ s/{boolean_operator:adj}//gi;
    $search =~ s/{boolean_operator:near}//gi;
    $search =~ s/{bo:or}//gi;
    $search =~ s/{bo:ior}//gi;
    $search =~ s/{bo:xor}//gi;
    $search =~ s/{bo:and}//gi;
    $search =~ s/{bo:adj}//gi;
    $search =~ s/{bo:near}//gi;

    $search =~ s/{boolean_operation:relaxed}//gi;
    $search =~ s/{boolean_operation:strict}//gi;
    $search =~ s/{bo:r}//gi;
    $search =~ s/{bo:s}//gi;

    $search =~ s/{operator_case:any}//gi;
    $search =~ s/{operator_case:upper}//gi;
    $search =~ s/{operator_case:lower}//gi;
    $search =~ s/{oc:a}//gi;
    $search =~ s/{oc:u}//gi;
    $search =~ s/{oc:l}//gi;

    $search =~ s/{term_case:drop}//gi;
    $search =~ s/{term_case:keep}//gi;
    $search =~ s/{tc:d}//gi;
    $search =~ s/{tc:k}//gi;

    $search =~ s/{frequent_terms:drop}//gi;
    $search =~ s/{frequent_terms:keep}//gi;
    $search =~ s/{ft:d}//gi;
    $search =~ s/{ft:k}//gi;

    $search =~ s/{search_type:boolean}//gi;
    $search =~ s/{search_type:freetext}//gi;
    $search =~ s/{st:b}//gi;
    $search =~ s/{st:f}//gi;

    $search =~ s/{date:.*?}//gi;
    $search =~ s/{d:.*?}//gi;

    $search =~ s/{sort:default}//gi;
    $search =~ s/{sort:none}//gi;
    $search =~ s/{sort:.*?}//gi;
    $search =~ s/{s:d}//gi;
    $search =~ s/{s:n}//gi;
    $search =~ s/{s:.*?}//gi;

    $search =~ s/{early_completion:enable}//gi;
    $search =~ s/{early_completion:disable}//gi;
    $search =~ s/{ec:e}//gi;
    $search =~ s/{ec:d}//gi;

    $search =~ s/{term_weight:.*?}//gi;
    $search =~ s/{feedback_term_weight:.*?}//gi;
    $search =~ s/{tw:.*?}//gi;
    $search =~ s/{ftw:.*?}//gi;

    $search =~ s/{frequent_term_coverage_threshold:.*?}//gi;
    $search =~ s/{ftct:.*?}//gi;

    $search =~ s/{feedback_minimum_term_count:.*?}//gi;
    $search =~ s/{feedback_maximum_term_percentage:.*?}//gi;
    $search =~ s/{feedback_maximum_term_coverage_threshold:.*?}//gi;
    $search =~ s/{fmtc:.*?}//gi;
    $search =~ s/{fmtp:.*?}//gi;
    $search =~ s/{fmtct:.*?}//gi;

    $search =~ s/{connection_timeout:.*?}//gi;
    $search =~ s/{search_timeout:.*?}//gi;
    $search =~ s/{retrieval_timeout:.*?}//gi;
    $search =~ s/{information_timeout:.*?}//gi;
    $search =~ s/{ct:.*?}//gi;
    $search =~ s/{st:.*?}//gi;
    $search =~ s/{rt:.*?}//gi;
    $search =~ s/{it:.*?}//gi;

    $search =~ s/{segments_searched_maximum:.*?}//gi;
    $search =~ s/{segments_searched_minimum:.*?}//gi;
    $search =~ s/{ssmx:.*?}//gi;
    $search =~ s/{ssmn:.*?}//gi;

    $search =~ s/{exclusion_filter:.*?}//gi;
    $search =~ s/{inclusion_filter:.*?}//gi;
    $search =~ s/{ef:.*?}//gi;
    $search =~ s/{if:.*?}//gi;

    $search =~ s/{exclusion_list_filter:.*?}//gi;
    $search =~ s/{inclusion_list_filter:.*?}//gi;
    $search =~ s/{elf:.*?}//gi;
    $search =~ s/{ilf:.*?}//gi;

    $search =~ s/{language:.*?}//gi;
    $search =~ s/{l:.*?}//gi;

    $search =~ s/{tag:.*?}//gi;
    $search =~ s/{t:.*?}//gi;


    # Gateway modifiers
    $search =~ s/{gtwy_early_completion:enable}//gi;
    $search =~ s/{gtwy_early_completion:disable}//gi;
    $search =~ s/{gec:e}//gi;
    $search =~ s/{gec:d}//gi;

    $search =~ s/{gtwy_connection_timeout:.*?}//gi;
    $search =~ s/{gtwy_search_timeout:.*?}//gi;
    $search =~ s/{gtwy_retrieval_timeout:.*?}//gi;
    $search =~ s/{gtwy_information_timeout:.*?}//gi;
    $search =~ s/{gct:.*?}//gi;
    $search =~ s/{gst:.*?}//gi;
    $search =~ s/{grt:.*?}//gi;
    $search =~ s/{git:.*?}//gi;

    $search =~ s/{gtwy_segments_searched_maximum:.*?}//gi;
    $search =~ s/{gtwy_segments_searched_minimum:.*?}//gi;
    $search =~ s/{gssmx:.*?}//gi;
    $search =~ s/{gssmn:.*?}//gi;

    $search =~ s/{gtwy_mirror_affinity:.*?}//gi;
    $search =~ s/{gma:.*?}//gi;


    # Trim leading and trailing spaces
    $search =~ s/^\s+//;
    $search =~ s/\s+$//;


    return ($search);

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


# Set the default temporary directory path
my $temporaryDirectoryPath = $main::temporaryDirectoryPath;


# Set the log file path
my $logFilePath = undef;

# Set the search filter
my $searchFilter = undef;

# Set the title
my $reportTitle = sprintf("MPS Log Report: %s", Sys::Hostname::hostname());


# Set the limits
my $slowSearchTime = $main::slowSearchTime;
my $slowSearchCount = $main::slowSearchCount;
my $topSearchCount = $main::topSearchCount;
my $topSearchFrequenciesCount = $main::topSearchFrequenciesCount;


# Set the flags
my $totalsFlag = 0;
my $totalsPerIndexFlag = 0;
my $totalsPerClientFlag = 0;
my $maximumProcessesFlag = 0;
my $processesPerHourFlag = 0;
my $processesPerDayFlag = 0;
my $searchesPerHourFlag = 0;
my $searchesPerDayFlag = 0;
my $searchTimingsFlag = 0;
my $slowSearchesFlag = 0;
my $topSearchesFlag = 0;
my $searchFrequenciesFlag = 0;

my $searchErrorsFlag = 0;
my $retrievalErrorsFlag = 0;
my $warningsFlag = 0;
my $errorsFlag = 0;
my $fatalsFlag = 0;
my $debugsFlag = 0;

my $slow = 0;


# Set the default format
my $format = $main::formatText;

# Set the default destination
my $destination = $main::destinationStdout;

# Set the default email disposition
my $emailDisposition = $main::emailDispositionAttachment;

# The email addresses
my $emailFromAddress;
my $emailToAddresses;

# Set the subject
my $emailSubject = sprintf("MPS Log Report: %s", Sys::Hostname::hostname());


# Clean up flag
my $cleanupFlag = 0;




# Check for command parameters
for ( my $Argc = 0; $Argc <= $#ARGV; $Argc++ ) {

    my $option = $ARGV[$Argc];

    if ( $option =~ /^--title=(.*)$/i ) {
        $reportTitle = $1;
    }
    elsif ( $option =~ /^--log=(.*)$/i ) {
        $logFilePath = $1;
    }
    elsif ( $option =~ /^--search-filter=(.*)$/i ) {
        $searchFilter = $1;
    }
    elsif ( $option =~ /^--totals$/i ) {
        $totalsFlag = 1;
    }
    elsif ( $option =~ /^--totals-per-index$/i ) {
        $totalsPerIndexFlag = 1;
    }
    elsif ( $option =~ /^--totals-per-client$/i ) {
        $totalsPerClientFlag = 1;
    }
    elsif ( $option =~ /^--maximum-processes$/i ) {
        $maximumProcessesFlag = 1;
    }
    elsif ( $option =~ /^--processes-per-hour$/i ) {
        $processesPerHourFlag = 1;
    }
    elsif ( $option =~ /^--processes-per-day$/i ) {
        $processesPerDayFlag = 1;
    }
    elsif ( $option =~ /^--searches-per-hour$/i ) {
        $searchesPerHourFlag = 1;
    }
    elsif ( $option =~ /^--searches-per-day$/i ) {
        $searchesPerDayFlag = 1;
    }
    elsif ( $option =~ /^--search-timings$/i ) {
        $searchTimingsFlag = 1;
    }
    elsif ( $option =~ /^--slow-search-time=([\d+\.])$/i ) {
        $slowSearchTime = $1;
    }
    elsif ( $option =~ /^--slow-searches=(\d+)$/i ) {
        $slowSearchesFlag = 1;
        $slowSearchCount = $1;
    }
    elsif ( $option =~ /^--slow-searches$/i ) {
        $slowSearchesFlag = 1;
    }
    elsif ( $option =~ /^--top-searches=(\d+)$/i ) {
        $topSearchesFlag = 1;
        $topSearchCount = $1;
    }
    elsif ( $option =~ /^--top-searches$/i ) {
        $topSearchesFlag = 1;
    }
    elsif ( $option =~ /^--search-frequencies=(\d+)$/i ) {
        $searchFrequenciesFlag = 1;
        $topSearchFrequenciesCount = $1;
    }
    elsif ( $option =~ /^--search-frequencies$/i ) {
        $searchFrequenciesFlag = 1;
    }
    elsif ( $option =~ /^--search-errors$/i ) {
        $searchErrorsFlag = 1;
    }
    elsif ( $option =~ /^--retrieval-errors$/i ) {
        $retrievalErrorsFlag = 1;
    }
    elsif ( $option =~ /^--warnings$/i ) {
        $warningsFlag = 1;
    }
    elsif ( $option =~ /^--errors$/i ) {
        $errorsFlag = 1;
    }
    elsif ( $option =~ /^--fatals$/i ) {
        $fatalsFlag = 1;
    }
    elsif ( $option =~ /^--debugs$/i ) {
        $debugsFlag = 1;
    }
    elsif ( $option =~ /^--slow$/i ) {
        $slow = 1;
    }
    elsif ( $option =~ /^--format=(.*)$/i ) {
        $format = $1;
    }
    elsif ( $option =~ /^--destination=(.*)$/i ) {
        $destination = $1;
    }
    elsif ( $option =~ /^--email-from-address=(.*)$/i ) {
        $emailFromAddress = $1;
    }
    elsif ( $option =~ /^--email-to-addresses=(.*)$/i ) {
        $emailToAddresses = $1;
    }
    elsif ( $option =~ /^--subject=(.*)$/i ) {
        $emailSubject = $1;
    }
    elsif ( $option =~ /^--disposition=(.*)$/i ) {
        $emailDisposition = $1;
    }
    elsif ( $option =~ /^--cleanup$/i ) {
        $cleanupFlag = 1;
    }
    elsif ( $option =~ /^(--help|\-?)$/ ) {
        printf("Usage:\n\n");
        printf("\t[--help|-?] print out this message.\n");
        printf("\n");
        printf("\t[--log=name] log file path, defaults to 'stdin'.\n");
        printf("\n");
        printf("\t[--search-filter=filter] filter searches against the filter.\n");
        printf("\n");
        printf("\t[--title=name] title, defaults to '%s'.\n", $reportTitle);
        printf("\n");
        printf("\t[--totals] list totals.\n");
        printf("\t[--totals-per-index] list totals per index.\n");
        printf("\t[--totals-per-client] list totals per client.\n");
        printf("\t[--maximum-processes] list maximum processes.\n");
        printf("\t[--processes-per-hour] list processes per hour.\n");
        printf("\t[--processes-per-day] list processes per day.\n");
        printf("\t[--searches-per-hour] list searches per hour.\n");
        printf("\t[--searches-per-day] list searches per day.\n");
        printf("\t[--search-timings] list searches timings.\n");
        printf("\t[--slow-search-time=#] slow search time, defaults to: %d seconds.\n", $slowSearchTime);
        printf("\t[--slow-searches[=#]] list to slow searches, defaults to: %d (0 to list all).\n", $slowSearchCount);
        printf("\t[--top-searches[=#]] list top searches, defaults to: %d (0 to list all).\n", $topSearchCount);
        printf("\t[--search-frequencies[=#]] list search frequencies, defaults to: %d (0 to list all).\n", $topSearchFrequenciesCount);
        printf("\n");
        printf("\t[--search-errors] list search errors.\n");
        printf("\t[--retrieval-errors] list retrieval errors.\n");
        printf("\t[--warnings] list warnings.\n");
        printf("\t[--errors] list errors.\n");
        printf("\t[--fatals] list fatals.\n");
        printf("\t[--debugs] list debugs.\n");
        printf("\n");
        printf("\t[--slow] slow processing so as not to overwhelm the CPU.\n");
        printf("\n");
        printf("\t[--format=[text|excel]] format, defaults to: '%s'.\n", $format);
        printf("\t[--destination=[stdout|file|email]] report destination, defaults to: '%s'.\n", $destination);
        printf("\t[--email-from-address=name, from email address.\n");
        printf("\t[--email-to-addresses=name[,name]] email addresses, comma delimited.\n");
        printf("\t[--subject=name], email subject, defaults to: '%s'.\n", $emailSubject);
        printf("\t[--disposition=[inline|attachment]] email disposition, defaults to: '%s'.\n", $emailDisposition);
        printf("\n");
        printf("\t[--cleanup] clean up files after processing.\n");
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


# Check the destination
if ( ($destination ne $main::destinationStdout) && ($destination ne $main::destinationFile) && ($destination ne $main::destinationEmail) ) {
    printf(STDERR "Invalid destination: '%s', exiting.\n", $destination);
    exit (-1);
}


# Check the format
if ( ($format ne $main::formatText) && ($format ne $main::formatExcel) ) {
    printf(STDERR "Invalid format: '%s', exiting.\n", $format);
    exit (-1);
}


# Check the disposition
if ( ($emailDisposition ne $main::emailDispositionAttachment) && ($emailDisposition ne $main::emailDispositionInline) ) {
    printf(STDERR "Invalid disposition: '%s', exiting.\n", $emailDisposition);
    exit (-1);
}


# Check that we have email addresses
if ( $destination eq $main::destinationEmail ) {
    if ( !$emailFromAddress ) {
        printf(STDERR "Invalid 'from' email address, exiting.\n");
        exit (-1);
    }
    if ( !$emailToAddresses ) {
        printf(STDERR "Invalid 'to' email address, exiting.\n");
        exit (-1);
    }
}


# Override the destination if the format is excel and the destination is stdout
if ( ($format eq $main::formatExcel) && ($destination eq $main::destinationStdout) ) {
    printf("Setting the destination to: '%s', because the format: '%s', was specified.\n", $main::destinationFile, $main::formatExcel);
    $destination = $main::destinationFile;
}



# Override the destination if an email address was specified
if ( $emailFromAddress && $emailToAddresses && ($destination ne $main::destinationEmail) ) {
    printf("Setting the destination to: '%s', because email addresses were specified, from: '%s', to: '%s'.\n", 
            $main::destinationEmail, $emailFromAddress, $emailToAddresses);
    $destination = $main::destinationEmail;
}


# Override the email disposition if the format is excel and the destination is email
if ( ($format eq $main::formatExcel) && ($destination eq $main::destinationEmail) && ($emailDisposition ne $main::emailDispositionAttachment) ) {
    printf("Setting the email disposition to: '%s', because the format: '%s', was specified.\n", $main::emailDispositionAttachment, $main::formatExcel);
    $emailDisposition = $main::emailDispositionAttachment;
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


my ($connectCount, $closeCount, $searchCount, $feedbackCount, $retrievalCount, $infoCount, $warningCount, $errorCount, $fatalCount, $debugCount) = (0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
my ($searchErrorCount, $retrievalErrorCount) = (0, 0);

my (%searchPerHourCounts, %searchPerDayCounts);
my ($searchPerHourCount, $searchPerDayCount) = (0, 0);
my ($maxSearchTime, $minSearchTime) = (-1, 10000000000);

my %currentProcesses;
my (%maxProcessCounts, %maxProcessPerHourCounts, %maxProcessPerDayCounts);
my ($maxProcessCount, $maxProcessPerHourCount, $maxProcessPerDayCount) = (0, 0, 0);

my (%searches, %searchFrequencies);

my (%searchClientCounts, %searchClientErrorCounts);
my (%retrievalClientCounts, %retrievalClientErrorCounts);

my (%searchIndexCounts, %searchIndexErrorCounts);
my (%retrievalIndexCounts, %retrievalIndexErrorCounts);

my ($totalSearchTime, $totalSearchSquaredTime) = (0, 0);
my @searchTimes;

my (%slowSearchTimes, @slowSearchTimes, @searchErrors, @retrievalErrors, @warnings, @errors, @fatals, @debugs);



# Loop over each line in the log file
while (<$LOG_FILE>) {

    chop $_;

    my $logLine = $_;



    # Slow processing down
    if ( $slow > 0 ) {
        
        $slow++;
        
        if ( $slow > $main::sleepLineCount ) {
            sleep ($main::sleepInterval);
            $slow = 1;
        }
    }


    my ($processID, $entry, $date, $day, $month, $year, $hours, $minutes, $seconds, $offset, $type, $message);


    # Parse the line, extract all the fields
    #
    # 30243: 14: 12/Oct/2004:09:32:31 -0500: INFO: ...
    if ( $logLine =~ /^(\d+): (\d+): ((\d+)\/(\w+)\/(\d+):(\d+):(\d+):(\d+)) ([\-\d]+): ([\w\-]+): (.*)$/ ) {
        
        $processID = $1;
        $entry = $2;
        $date = $3;
        $day = $4;
        $month = $main::monthAbbreviations{$5};
        $year = $6;
        $hours = $7;
        $minutes = $8;
        $seconds = $9;
        $offset = $10;
        $type = $11;
        $message = $12;
    }
    # 30243: 3012565936: 14: 12/Oct/2004:09:32:31 -0500: INFO: ...
    elsif ( $logLine =~ /^(\d+: \d+): (\d+): ((\d+)\/(\w+)\/(\d+):(\d+):(\d+):(\d+)) ([\-\d]+): ([\w]+): (.*)$/ ) {
        
        $processID = $1;
        $entry = $2;
        $date = $3;
        $day = $4;
        $month = $main::monthAbbreviations{$5};
        $year = $6;
        $hours = $7;
        $minutes = $8;
        $seconds = $9;
        $offset = $10;
        $type = $11;
        $message = $12;
    }
    else {
        printf("Failed to parse line: '%s' (ii).\n", $logLine);
        next;
    }


    
    # Process each line type
    if ( $type eq 'INFO' ) {

        # 30243: 14: 12/Oct/2004:09:32:31 -0500: INFO: Accepted connection from a client on: '192.168.0.2' [192.168.0.2].
        # 30243: 14: 12/Oct/2004:09:32:31 -0500: INFO: Accepted connection from a client on: [192.168.0.2].
        if ( $message =~ /^Accepted connection from a client on: ('(.*?)' \[(.*?)\]|\[(.*?)\])\.$/ ) {
            
            $connectCount++;
            
            $currentProcesses{$processID} = defined($4) ? $4 : $1;
        }


        # 30243: 17: 12/Oct/2004:09:32:31 -0500: INFO: Finished handling client.
        elsif ( $message =~ /^Finished handling client\.$/ ) {
            
            $closeCount++;
            
            delete($currentProcesses{$processID});
        }


        # 30243: 20: 12/Oct/2004:09:32:37 -0500: INFO: Search, index: 'posts', search: 'Are Halland {SORT:DATE:DESC}{debug}', found: 139 documents and returned: 16 (+1 search report), in: 0.001451 seconds.
        # 30243: 20: 12/Oct/2004:09:32:37 -0500: INFO: Search, index: 'posts', search: 'Are Halland {SORT:DATE:DESC}{debug}', found: 139 documents and returned: 16, in: 0.001451 seconds.
        # 30243: 20: 12/Oct/2004:09:32:37 -0500: INFO: Search, index: 'posts', search: 'Are Halland {SORT:DATE:DESC}{debug}', error: -129.
        elsif ( $message =~ /^Search, (index|indices): '(.*?)', search: '(.*?)', found: (\d+) document(|s) and returned: (\d+).*?, in: ([\d\.]+) seconds\.$/ ) {

            my $indices = $2;
            my $search = &_cleanSearch($3);
            my $found = $4;
            my $returned = $6;
            my $searchTime = $7;

            # Skip this search if there is a filter and it does not match it
            if ( $searchFilter && (index($searchFilter, $search) == -1) ) {
                next;
            }

            # Increment counters 
            $searchCount++;
            $searchPerHourCount++;
            $searchPerDayCount++;
            
            if ( $totalsPerIndexFlag ) {
                foreach my $index ( split(/, /, $indices) ) {
                    $searchIndexCounts{$index} = defined($searchIndexCounts{$index}) ? $searchIndexCounts{$index} + 1 : 1;
                }
            }
                        
            if ( $searchFrequenciesFlag || $topSearchesFlag ) {
                $searches{$search} = defined($searches{$search}) ? $searches{$search} + 1 : 1;
            }

            if ( $totalsPerClientFlag ) {
                my $host = ($currentProcesses{$processID}) ? $currentProcesses{$processID} : 'localhost';
                $searchClientCounts{$host} = defined($searchClientCounts{$host}) ? $searchClientCounts{$host} + 1 : 1;
            }
            
            if ( $searchTimingsFlag ) {
                if ( $searchTime > $maxSearchTime ) {
                    $maxSearchTime = $searchTime
                }
                
                if ( $searchTime < $minSearchTime ) {
                    $minSearchTime = $searchTime
                }
                
                $totalSearchTime += $searchTime;
                $totalSearchSquaredTime += ($searchTime * $searchTime);
    
                push @searchTimes, $searchTime;
            }
            
            if ( $slowSearchesFlag ) {
                if ( ($slowSearchTime != -1) && ($searchTime >= $slowSearchTime) ) {
                    if ( defined($slowSearchTimes{$searchTime}) ) {
                        $slowSearchTimes{$searchTime} .= "\0";
                    }
                    else {
                        $slowSearchTimes{$searchTime} = '';
                    }
#                     $slowSearchTimes{$searchTime} .= $logLine;
#                     $slowSearchTimes{$searchTime} .= sprintf("%s (found: %d documents and returned: %d)", $search, $found, $returned);
#                     $slowSearchTimes{$searchTime} .= sprintf("%s (%d / %d)", $search, $found, $returned);
                    $slowSearchTimes{$searchTime} .= sprintf("%s (%d documents)", $search, $found);
                }
            }
        }
        elsif ( $message =~ /^Search, (index|indices): '(.*?)', search: '(.*?)', spi error: ([\-\d]+)\.$/ ) {

            my $indices = $2;
            my $search = &_cleanSearch($3);

            # Skip this search if there is a filter and it does not match it
            if ( $searchFilter && (index($searchFilter, $search) == -1) ) {
                next;
            }

            # Increment counters 
            $searchErrorCount++;
            $searchPerHourCount++;
            $searchPerDayCount++;

            if ( $totalsPerIndexFlag ) {
                foreach my $index ( split(/, /, $indices) ) {
                    $searchIndexErrorCounts{$index} = defined($searchIndexErrorCounts{$index}) ? $searchIndexErrorCounts{$index} + 1 : 1;
                }
            }

            if ( $searchFrequenciesFlag || $topSearchesFlag ) {
                $searches{$search} = defined($searches{$search}) ? $searches{$search} + 1 : 1;
            }

            if ( $totalsPerClientFlag ) {
                my $host = ($currentProcesses{$processID}) ? $currentProcesses{$processID} : 'localhost';
                $searchClientErrorCounts{$host} = defined($searchClientErrorCounts{$host}) ? $searchClientErrorCounts{$host} + 1 : 1;
            }

            if ( $searchErrorsFlag ) {
                push @searchErrors, $logLine;
            }
        }


        # 30243: 20: 12/Oct/2004:09:32:37 -0500: INFO: Relevance feedback terms, total: 100, unique: 100, used: 10.
        elsif ( $message =~ /^Relevance feedback terms, total: (\d+), unique: (\d+), used: (\d+)\.$/ ) {

            $feedbackCount++;
        }
    
    
        # 30243: 20: 12/Oct/2004:09:32:37 -0500: INFO: Retrieving document, index: 'posts', document key: '1', byte range: 1 100, item: 'document', mime type: 'text/plain'.
        # 30243: 20: 12/Oct/2004:09:32:37 -0500: INFO: Retrieving document, index: 'posts', document key: '1', line range: 1 100, item: 'document', mime type: 'text/plain'.
        # 30243: 20: 12/Oct/2004:09:32:37 -0500: INFO: Retrieving document, index: 'posts', document key: '1', item: 'document', mime type: 'document', mime type: 'text/plain'.
        elsif ( $message =~ /^Retrieving document, index: '.*?', document key: '.*?'.*$/  ) {
            
            my $indexList;
            my $error = 0;
            
            if ( $message =~ /^Retrieving document, index: '(.*?)', document key: '(.*?)', byte range: (\d+) (\d+), item: '(.*?)', mime type: '(.*?)'\.$/ ) {
    
                $retrievalCount++;
                $indexList = $1;
            }
            elsif ( $message =~ /^Retrieving document, index: '(.*?)', document key: '(.*?)', item: '(.*?)', mime type: '(.*?)'\.$/ ) {
    
                $retrievalCount++;
                $indexList = $1;
            }
            elsif ( $message =~ /^Retrieving document, index: '(.*?)', document key: '(.*?)', byte range: (\d+) (\d+), item: '(.*?)', mime type: '(.*?)', spi error: ([\-\d]+)\.$/ ) {
    
                $retrievalErrorCount++;
                $indexList = $1;
                $error = 1;
            }
            elsif ( $message =~ /^Retrieving document, index: '(.*?)', document key: '(.*?)', item: '(.*?)', mime type: '(.*?)', spi error: ([\-\d]+)\.$/ ) {
    
                $retrievalErrorCount++;
                $indexList = $1;
                $error = 1;
            }
            else {
                printf("Failed to parse line: '%s'.\n", $logLine);
                next;
            }
    
            
            if ( defined($indexList) ) {
    
                if ( $error == 0 ) {
    
                    if ( $totalsPerIndexFlag ) {
                        foreach my $index ( split(/, /, $indexList) ) {
                            $retrievalIndexCounts{$index} = defined($retrievalIndexCounts{$index}) ? $retrievalIndexCounts{$index} + 1 : 1;
                        }
                    }
        
                    if ( $totalsPerClientFlag ) {
                        my ($host) = ($currentProcesses{$processID}) ? $currentProcesses{$processID} : 'localhost';
                        $retrievalClientCounts{$host} = defined($retrievalClientCounts{$host}) ? $retrievalClientCounts{$host} + 1 : 1;
                    }
                }
                else {
    
                    if ( $totalsPerIndexFlag ) {
                        foreach my $index ( split(/, /, $indexList) ) {
                            $retrievalIndexErrorCounts{$index} = defined($retrievalIndexErrorCounts{$index}) ? $retrievalIndexErrorCounts{$index} + 1 : 1;
                        }
                    }
    
                    if ( $totalsPerClientFlag ) {
                        my ($host) = ($currentProcesses{$processID}) ? $currentProcesses{$processID} : 'localhost';
                        $retrievalClientErrorCounts{$host} = defined($retrievalClientErrorCounts{$host}) ? $retrievalClientErrorCounts{$host} + 1 : 1;
                    }

                    if ( $retrievalErrorsFlag ) {
                        push @retrievalErrors, $logLine;
                    }
                }
            }
        }
    

        # 30243: 20: 12/Oct/2004:09:32:37 -0500: INFO: Important information.
        else {
            $infoCount++;
        }
    }


    # 30243: 20: 12/Oct/2004:09:32:37 -0500: WARN: Warning.
    elsif ( $type eq 'WARN' ) {

        $warningCount++;

        if ( $warningsFlag ) {
            push @warnings, $logLine;
        }
    }    
    

    # 30243: 20: 12/Oct/2004:09:32:37 -0500: ERROR: Error.
    elsif ( $type eq 'ERROR' ) {

        $errorCount++;

        if ( $errorsFlag ) {
            push @errors, $logLine;
        }
    }    

    
    # 30243: 20: 12/Oct/2004:09:32:37 -0500: FATAL: Fatal.
    elsif ( $type eq 'FATAL' ) {

        $fatalCount++;

        if ( $fatalsFlag ) {
            push @fatals, $logLine;
        }
    }    

    
    # 30243: 20: 12/Oct/2004:09:32:37 -0500: DEBUG: Debug.
    elsif ( $type eq 'DEBUG' ) {

        $debugCount++;

        if ( $debugsFlag ) {
            push @debugs, $logLine;
        }
    }    

    
    else {
        printf(STDERR "Invalid type: '%s', in log line: '%s'.\n", $type, $logLine);
        next;
    }



    # Get the current process count
    my $currentProcessCount = scalar(keys(%currentProcesses));

    # Set the max process count 
    if ( $currentProcessCount > $maxProcessCount ) {
        
        $maxProcessCount = $currentProcessCount;

        if ( $maximumProcessesFlag ) {
            $maxProcessCounts{$maxProcessCount} = $date;
        }
    }

    # Set the max process per hour count 
    if ( $currentProcessCount > $maxProcessPerHourCount ) {
        $maxProcessPerHourCount = $currentProcessCount;
    }
    
    # Set the max process per day count 
    if ( $currentProcessCount > $maxProcessPerDayCount ) {
        $maxProcessPerDayCount = $currentProcessCount;
    }
    

    if ( $processesPerHourFlag || $searchesPerHourFlag ) {

        my $dateKey = sprintf("%d/%02d/%02d:%02d:00:00", $year, $month, $day, $hours);
        
        if ( $processesPerHourFlag ) {
            if ( !$maxProcessPerHourCounts{$dateKey} ) {
                $maxProcessPerHourCounts{$dateKey} = 0;
            }
            if ( $maxProcessPerHourCount > $maxProcessPerHourCounts{$dateKey} ) {
                $maxProcessPerHourCounts{$dateKey} = $maxProcessPerHourCount;
                $maxProcessPerHourCount = 0;
            }
        }
    
        if ( $searchesPerHourFlag ) {
            if ( !$searchPerHourCounts{$dateKey} ) {
                $searchPerHourCounts{$dateKey} = 0;
            }
            $searchPerHourCounts{$dateKey} += $searchPerHourCount;
            $searchPerHourCount = 0;
        }
    }


    if ( $processesPerDayFlag || $searchesPerDayFlag ) {
    
        my $dateKey = sprintf("%d/%02d/%02d", $year, $month, $day);
        
        if ( $processesPerDayFlag ) {
            if ( !$maxProcessPerDayCounts{$dateKey} ) {
                $maxProcessPerDayCounts{$dateKey} = 0;
            }
            if ( $maxProcessPerDayCount > $maxProcessPerDayCounts{$dateKey} ) {
                $maxProcessPerDayCounts{$dateKey} = $maxProcessPerDayCount;
                $maxProcessPerDayCount = 0;
            }
        }
    
        if ( $searchesPerDayFlag ) {
            if ( !$searchPerDayCounts{$dateKey} ) {
                $searchPerDayCounts{$dateKey} = 0;
            }
            $searchPerDayCounts{$dateKey} += $searchPerDayCount;
            $searchPerDayCount = 0;
        }
    }


}


# Close the log file
if ( defined($logFilePath) ) {
    close($LOG_FILE);
}







# Output variables
my $textFileName = undef;
my $textFilePath = undef;
my $TEXT_FILE = undef;

my $excelFileName = undef;
my $excelFilePath = undef;
my $excelWorkBook = undef;
my $formatTitleLeft = undef;
my $formatTitleRight = undef;
my $formatNumber = undef;
my $formatFloat = undef;
my $formatPercent = undef;


# Create the text file name and path
if ( $format eq $main::formatText ) {

    # Create the text file name
    $textFileName = sprintf("%s.txt", $main::fileNameSearchLogReport);

    # Create the text path
    $textFilePath = sprintf("%s/%s", $temporaryDirectoryPath, $textFileName);

    # Create the text file
    if ( ! open($TEXT_FILE, '>', $textFilePath) ) {
        printf(STDERR "Failed to create the text file: '%s', exiting.\n", $textFilePath);
        exit (-1);
    }
}

# Create the excel file name, path, workbook and formats
elsif ( $format eq $main::formatExcel ) {

    # Create the excel file name
    $excelFileName = sprintf("%s.xls", $main::fileNameSearchLogReport);

    # Create the excel file path
    $excelFilePath = sprintf("%s/%s", $temporaryDirectoryPath, $excelFileName);

    # Create the excel workbook
    $excelWorkBook = Spreadsheet::WriteExcel->new($excelFilePath);
    if ( !$excelWorkBook ) {
        printf(STDERR "Failed to create the excel workbook: '%s', exiting.\n", $excelFilePath);
        exit (-1);
    }

    # Create the excel formats
    $formatTitleLeft = $excelWorkBook->add_format(bold => '1');
    $formatTitleRight = $excelWorkBook->add_format(bold => '1', align => 'right');
    $formatNumber = $excelWorkBook->add_format(num_format => '#,##0');
    $formatFloat = $excelWorkBook->add_format(num_format => '#,##.####0');
    $formatPercent = $excelWorkBook->add_format(num_format => '0.00%');
}




# List the search stats
if ( $totalsFlag ) {

    if ( $format eq $main::formatText ) {
        printf($TEXT_FILE "Totals :\n\n");
        printf($TEXT_FILE "  Connections                : %6d\n", $connectCount);
        printf($TEXT_FILE "  Closes                     : %6d\n", $closeCount);
        printf($TEXT_FILE "  Searches                   : %6d\n", $searchCount);
        printf($TEXT_FILE "  Search errors              : %6d\n", $searchErrorCount);
        printf($TEXT_FILE "  Feedbacks                  : %6d\n", $feedbackCount);
        printf($TEXT_FILE "  Retrievals                 : %6d\n", $retrievalCount);
        printf($TEXT_FILE "  Retrieval errors           : %6d\n", $retrievalErrorCount);
        printf($TEXT_FILE "  Infos                      : %6d\n", $infoCount);
        printf($TEXT_FILE "  Warnings                   : %6d\n", $warningCount);
        printf($TEXT_FILE "  Errors                     : %6d\n", $errorCount);
        printf($TEXT_FILE "  Fatals                     : %6d\n", $fatalCount);
        printf($TEXT_FILE "  Debugs                     : %6d\n", $debugCount);
        printf($TEXT_FILE "\n\n");
    }
    elsif ( $format eq $main::formatExcel ) {

        # Add a worksheet
        my $workSheet = $excelWorkBook->add_worksheet('Totals');

        # Set the column widths
        $workSheet->set_column(0, 1, 20);
            
        $workSheet->write_string('A1', 'Totals :', $formatTitleLeft);

            
        $workSheet->write_string('A3', 'Connections', $formatTitleLeft);
        $workSheet->write_string('A4', 'Closes', $formatTitleLeft);
        $workSheet->write_string('A5', 'Searches', $formatTitleLeft);
        $workSheet->write_string('A6', 'Search errors', $formatTitleLeft);
        $workSheet->write_string('A7', 'Feedbacks', $formatTitleLeft);
        $workSheet->write_string('A8', 'Retrievals', $formatTitleLeft);
        $workSheet->write_string('A9', 'Retrieval errors', $formatTitleLeft);
        $workSheet->write_string('A10', 'Infos', $formatTitleLeft);
        $workSheet->write_string('A11', 'Warnings', $formatTitleLeft);
        $workSheet->write_string('A12', 'Errors', $formatTitleLeft);
        $workSheet->write_string('A13', 'Fatals', $formatTitleLeft);
        $workSheet->write_string('A14', 'Debugs', $formatTitleLeft);

        $workSheet->write('B3', $connectCount, $formatNumber);
        $workSheet->write('B4', $closeCount, $formatNumber);
        $workSheet->write('B5', $searchCount, $formatNumber);
        $workSheet->write('B6', $searchErrorCount, $formatNumber);
        $workSheet->write('B7', $feedbackCount, $formatNumber);
        $workSheet->write('B8', $retrievalCount, $formatNumber);
        $workSheet->write('B9', $retrievalErrorCount, $formatNumber);
        $workSheet->write('B10', $infoCount, $formatNumber);
        $workSheet->write('B11', $warningCount, $formatNumber);
        $workSheet->write('B12', $errorCount, $formatNumber);
        $workSheet->write('B13', $fatalCount, $formatNumber);
        $workSheet->write('B14', $debugCount, $formatNumber);

    }
}    



if ( $totalsPerIndexFlag ) {

    my $workSheet = undef;
    my $i = 1;

    if ( $format eq $main::formatExcel ) {

        # Add a worksheet
        $workSheet = $excelWorkBook->add_worksheet('Totals per index');

        # Set the column widths
        $workSheet->set_column(0, 1, 20);
    }            


    if ( %searchIndexCounts ) {

        if ( $format eq $main::formatText ) {
            printf($TEXT_FILE "Searches per index :\n\n");
            foreach my $searchIndexCount ( sort(keys(%searchIndexCounts)) ) {
                printf($TEXT_FILE "  %-20s       : %6d\n", $searchIndexCount, $searchIndexCounts{$searchIndexCount});
            }
            printf($TEXT_FILE "\n\n");
        }
        elsif ( $format eq $main::formatExcel ) {

            $workSheet->write_string(sprintf("A%d", $i), 'Searches per index :', $formatTitleLeft);
            $i += 2;
            
            foreach my $searchIndexCount ( sort(keys(%searchIndexCounts)) ) {
                $workSheet->write_string(sprintf("A%d", $i), $searchIndexCount);
                $workSheet->write(sprintf("B%d", $i), $searchIndexCounts{$searchIndexCount}, $formatNumber);
                $i++;
            }
            
            $i += 2;
        }
    }
    

    if ( %searchIndexErrorCounts ) {

        if ( $format eq $main::formatText ) {
            printf($TEXT_FILE "Search errors per index :\n\n");
            foreach my $searchIndexErrorCount ( sort(keys(%searchIndexErrorCounts)) ) {
                printf($TEXT_FILE "  %-20s       : %6d\n", $searchIndexErrorCount, $searchIndexErrorCounts{$searchIndexErrorCount});
            }
            printf($TEXT_FILE "\n\n");
        }
        elsif ( $format eq $main::formatExcel ) {

            $workSheet->write_string(sprintf("A%d", $i), 'Search errors per index :', $formatTitleLeft);
            $i += 2;

            foreach my $searchIndexErrorCount ( sort(keys(%searchIndexErrorCounts)) ) {
                $workSheet->write_string(sprintf("A%d", $i), $searchIndexErrorCount);
                $workSheet->write(sprintf("B%d", $i), $searchIndexErrorCounts{$searchIndexErrorCount}, $formatNumber);
                $i++;
            }
            
            $i += 2;
        }
    }

    
    if ( %retrievalIndexCounts ) {

        if ( $format eq $main::formatText ) {
            printf($TEXT_FILE "Retrievals per index :\n\n");
            foreach my $retrievalIndexCount ( sort(keys(%retrievalIndexCounts)) ) {
                printf($TEXT_FILE "  %-20s       : %6d\n", $retrievalIndexCount, $retrievalIndexCounts{$retrievalIndexCount});
            }
            printf($TEXT_FILE "\n\n");
        }
        elsif ( $format eq $main::formatExcel ) {

            $workSheet->write_string(sprintf("A%d", $i), 'Retrievals per index :', $formatTitleLeft);
            $i += 2;

            foreach my $retrievalIndexCount ( sort(keys(%retrievalIndexCounts)) ) {
                $workSheet->write_string(sprintf("A%d", $i), $retrievalIndexCount);
                $workSheet->write(sprintf("B%d", $i), $retrievalIndexCounts{$retrievalIndexCount}, $formatNumber);
                $i++;
            }
            
            $i += 2;
        }
    }


    if ( %retrievalIndexErrorCounts ) {

        if ( $format eq $main::formatText ) {
            printf($TEXT_FILE "Retrieval errors per index :\n\n");
            foreach my $retrievalIndexErrorCount ( sort(keys(%retrievalIndexErrorCounts)) ) {
                printf($TEXT_FILE "  %-20s       : %6d\n", $retrievalIndexErrorCount, $retrievalIndexErrorCounts{$retrievalIndexErrorCount});
            }
            printf($TEXT_FILE "\n\n");
        }
        elsif ( $format eq $main::formatExcel ) {

            $workSheet->write_string(sprintf("A%d", $i), 'Retrieval errors per index :', $formatTitleLeft);
            $i += 2;

            foreach my $retrievalIndexErrorCount ( sort(keys(%retrievalIndexErrorCounts)) ) {
                $workSheet->write_string(sprintf("A%d", $i), $retrievalIndexErrorCount);
                $workSheet->write(sprintf("B%d", $i), $retrievalIndexErrorCounts{$retrievalIndexErrorCount}, $formatNumber);
                $i++;
            }
            
            $i += 2;
        }
    }
}


    
if ( $totalsPerClientFlag ) {
    
    my $workSheet = undef;
    my $i = 1;

    if ( $format eq $main::formatExcel ) {

        # Add a worksheet
        $workSheet = $excelWorkBook->add_worksheet('Totals per client');

        # Set the column widths
        $workSheet->set_column(0, 1, 20);
    }            


    if ( %searchClientCounts ) {

        if ( $format eq $main::formatText ) {
            printf($TEXT_FILE "Searches per client :\n\n");
            foreach my $searchClientCount ( sort(keys(%searchClientCounts)) ) {
                printf($TEXT_FILE "  %-20s       : %6d\n", $searchClientCount, $searchClientCounts{$searchClientCount});
            }
            printf($TEXT_FILE "\n\n");
        }
        elsif ( $format eq $main::formatExcel ) {

            $workSheet->write_string(sprintf("A%d", $i), 'Searches per client :', $formatTitleLeft);
            $i += 2;
            
            foreach my $searchClientCount ( sort(keys(%searchClientCounts)) ) {
                $workSheet->write_string(sprintf("A%d", $i), $searchClientCount);
                $workSheet->write(sprintf("B%d", $i), $searchClientCounts{$searchClientCount}, $formatNumber);
                $i++;
            }
            
            $i += 2;
        }
    }


    if ( %searchClientErrorCounts ) {

        if ( $format eq $main::formatText ) {
            printf($TEXT_FILE "Search errors per client :\n\n");
            foreach my $searchClientErrorCount ( sort(keys(%searchClientErrorCounts)) ) {
                printf($TEXT_FILE "  %-20s       : %6d\n", $searchClientErrorCount, $searchClientErrorCounts{$searchClientErrorCount});
            }
            printf($TEXT_FILE "\n\n");
        }
        elsif ( $format eq $main::formatExcel ) {

            $workSheet->write_string(sprintf("A%d", $i), 'Search errors per client :', $formatTitleLeft);
            $i += 2;
            
            foreach my $searchClientErrorCount ( sort(keys(%searchClientErrorCounts)) ) {
                $workSheet->write_string(sprintf("A%d", $i), $searchClientErrorCount);
                $workSheet->write(sprintf("B%d", $i), $searchClientErrorCounts{$searchClientErrorCount}, $formatNumber);
                $i++;
            }
            
            $i += 2;
        }
    }


    if ( %retrievalClientCounts ) {

        if ( $format eq $main::formatText ) {
            printf($TEXT_FILE "Retrievals per client :\n\n");
            foreach my $retrievalClientCount ( sort(keys(%retrievalClientCounts)) ) {
                printf($TEXT_FILE "  %-20s       : %6d\n", $retrievalClientCount, $retrievalClientCounts{$retrievalClientCount});
            }
            printf($TEXT_FILE "\n\n");
        }
        elsif ( $format eq $main::formatExcel ) {

            $workSheet->write_string(sprintf("A%d", $i), 'Retrievals per client :', $formatTitleLeft);
            $i += 2;

            foreach my $retrievalClientCount ( sort(keys(%retrievalClientCounts)) ) {
                $workSheet->write_string(sprintf("A%d", $i), $retrievalClientCount);
                $workSheet->write(sprintf("B%d", $i), $retrievalClientCounts{$retrievalClientCount}, $formatNumber);
                $i++;
            }
            
            $i += 2;
        }
    }


    if ( %retrievalClientErrorCounts ) {

        if ( $format eq $main::formatText ) {
            printf($TEXT_FILE "Retrieval errors per client :\n\n");
            foreach my $retrievalClientErrorCount ( sort(keys(%retrievalClientErrorCounts)) ) {
                printf($TEXT_FILE "  %-20s       : %6d\n", $retrievalClientErrorCount, $retrievalClientErrorCounts{$retrievalClientErrorCount});
            }
            printf($TEXT_FILE "\n\n");
        }
        elsif ( $format eq $main::formatExcel ) {

            $workSheet->write_string(sprintf("A%d", $i), 'Retrieval errors per client :', $formatTitleLeft);
            $i += 2;

            foreach my $retrievalClientErrorCount ( sort(keys(%retrievalClientErrorCounts)) ) {
                $workSheet->write_string(sprintf("A%d", $i), $retrievalClientErrorCount);
                $workSheet->write(sprintf("B%d", $i), $retrievalClientErrorCounts{$retrievalClientErrorCount}, $formatNumber);
                $i++;
            }
            
            $i += 2;
        }
    }
}

    

if ( $maximumProcessesFlag || $processesPerHourFlag || $processesPerDayFlag ) {
    
    my $workSheet = undef;
    my $i = 1;

    if ( $format eq $main::formatExcel ) {

        # Add a worksheet
        $workSheet = $excelWorkBook->add_worksheet('Process Counts');

        # Set the column widths
        $workSheet->set_column(0, 1, 20);
    }            


    if ( $maximumProcessesFlag ) {
    
        if ( %maxProcessCounts ) {
    
            if ( $format eq $main::formatText ) {
                printf($TEXT_FILE "Maximum Processes :\n\n");
                foreach my $maxProcessCount ( sort { $a <=> $b } keys(%maxProcessCounts) ) {
                    printf($TEXT_FILE "  %-20s       : %6d\n", $maxProcessCounts{$maxProcessCount}, $maxProcessCount);
                }
                printf($TEXT_FILE "\n\n");
            }
            elsif ( $format eq $main::formatExcel ) {
    
                $workSheet->write_string(sprintf("A%d", $i), 'Maximum Processes :', $formatTitleLeft);
                $i += 2;

                foreach my $maxProcessCount ( sort { $a <=> $b } keys(%maxProcessCounts) ) {
                    $workSheet->write_string(sprintf("A%d", $i), $maxProcessCounts{$maxProcessCount});
                    $workSheet->write(sprintf("B%d", $i), $maxProcessCount, $formatNumber);
                    $i++;
                }
            
                $i += 2;
            }
        }
    }
    
        
    if ( $processesPerHourFlag ) {
    
        if ( %maxProcessPerHourCounts ) {
    
            if ( $format eq $main::formatText ) {
                printf($TEXT_FILE "Maximum Processes Per Hour :\n\n");
                foreach my $maxProcessPerHourCount ( sort(keys(%maxProcessPerHourCounts)) ) {
                    printf($TEXT_FILE "  %-20s       : %6d\n", $maxProcessPerHourCount, $maxProcessPerHourCounts{$maxProcessPerHourCount});
                }
                printf($TEXT_FILE "\n\n");
            }
            elsif ( $format eq $main::formatExcel ) {
    
                $workSheet->write_string(sprintf("A%d", $i), 'Maximum Processes Per Hour :', $formatTitleLeft);
                $i += 2;

                foreach my $maxProcessPerHourCount ( sort(keys(%maxProcessPerHourCounts)) ) {
                    $workSheet->write_string(sprintf("A%d", $i), $maxProcessPerHourCount);
                    $workSheet->write(sprintf("B%d", $i), $maxProcessPerHourCounts{$maxProcessPerHourCount}, $formatNumber);
                    $i++;
                }
            
                $i += 2;
            }
        }
    }
    
    
    if ( $processesPerDayFlag ) {
    
        if ( %maxProcessPerDayCounts ) {
    
            if ( $format eq $main::formatText ) {
                printf($TEXT_FILE "Maximum Processes Per Day :\n\n");
                foreach my $maxProcessPerDayCount ( sort(keys(%maxProcessPerDayCounts)) ) {
                    printf($TEXT_FILE "  %-20s       : %6d\n", $maxProcessPerDayCount, $maxProcessPerDayCounts{$maxProcessPerDayCount});
                }
                printf($TEXT_FILE "\n\n");
            }
            elsif ( $format eq $main::formatExcel ) {
    
                $workSheet->write_string(sprintf("A%d", $i), 'Maximum Processes Per Day :', $formatTitleLeft);
                $i += 2;

                foreach my $maxProcessPerDayCount ( sort(keys(%maxProcessPerDayCounts)) ) {
                    $workSheet->write_string(sprintf("A%d", $i), $maxProcessPerDayCount);
                    $workSheet->write(sprintf("B%d", $i), $maxProcessPerDayCounts{$maxProcessPerDayCount}, $formatNumber);
                    $i++;
                }
            
                $i += 2;
            }
        }
    }    
}



if ( $searchesPerHourFlag || $searchesPerDayFlag ) {
    
    my $workSheet = undef;
    my $i = 1;

    if ( $format eq $main::formatExcel ) {

        # Add a worksheet
        $workSheet = $excelWorkBook->add_worksheet('Search Counts');

        # Set the column widths
        $workSheet->set_column(0, 1, 20);
    }            


    if ( $searchesPerHourFlag ) {
    
        if ( %searchPerHourCounts ) {
    
            if ( $format eq $main::formatText ) {
                printf($TEXT_FILE "Searches Per Hour :\n\n");
                foreach my $searchPerHourCount ( sort(keys(%searchPerHourCounts)) ) {
                    printf($TEXT_FILE "  %-20s       : %6d\n", $searchPerHourCount, $searchPerHourCounts{$searchPerHourCount});
                }
                printf($TEXT_FILE "\n\n");
            }
            elsif ( $format eq $main::formatExcel ) {
    
                $workSheet->write_string(sprintf("A%d", $i), 'Searches Per Hour :', $formatTitleLeft);
                $i += 2;

                foreach my $searchPerHourCount ( sort(keys(%searchPerHourCounts)) ) {
                    $workSheet->write_string(sprintf("A%d", $i), $searchPerHourCount);
                    $workSheet->write(sprintf("B%d", $i), $searchPerHourCounts{$searchPerHourCount}, $formatNumber);
                    $i++;
                }
            
                $i += 2;
            }
        }
    }
    
        
    if ( $searchesPerDayFlag ) {
    
        if ( %searchPerDayCounts ) {
    
            if ( $format eq $main::formatText ) {
                printf($TEXT_FILE "Searches Per Day :\n\n");
                foreach my $searchPerDayCount ( sort(keys(%searchPerDayCounts)) ) {
                    printf($TEXT_FILE "  %-20s       : %6d\n", $searchPerDayCount, $searchPerDayCounts{$searchPerDayCount});
                }
                printf($TEXT_FILE "\n\n");
            }
            elsif ( $format eq $main::formatExcel ) {
    
                $workSheet->write_string(sprintf("A%d", $i), 'Searches Per Day :', $formatTitleLeft);
                $i += 2;

                foreach my $searchPerDayCount ( sort(keys(%searchPerDayCounts)) ) {
                    $workSheet->write_string(sprintf("A%d", $i), $searchPerDayCount);
                    $workSheet->write(sprintf("B%d", $i), $searchPerDayCounts{$searchPerDayCount}, $formatNumber);
                    $i++;
                }
            
                $i += 2;
            }
        }
    }
}


    
if ( $searchTimingsFlag || $slowSearchesFlag ) {

    my $workSheet = undef;
    my $i = 1;

    if ( $format eq $main::formatExcel ) {

        # Add a worksheet
        $workSheet = $excelWorkBook->add_worksheet('Search Timings');

        # Set the column widths
        $workSheet->set_column(0, 1, 20);
    }            


    if ( $searchTimingsFlag ) {

        @searchTimes = sort { $a <=> $b } @searchTimes;
        
        my $meanSearchTime = ($totalSearchTime / $searchCount);
        my $VarianceSearchTime = ($totalSearchSquaredTime - (($totalSearchTime * $totalSearchTime) / $searchCount)) / $searchCount;
        my $standardDeviationTime = sqrt($VarianceSearchTime);
        
        my $medianSearchTimeOffset = (($searchCount + 1) / 2) - 1;
        
        my ($medianSearchTime);
        if ( $medianSearchTimeOffset == abs($medianSearchTimeOffset) ) {
            $medianSearchTime = $searchTimes[$medianSearchTimeOffset];
        }
        else {
            $medianSearchTime = ($searchTimes[$medianSearchTimeOffset - 0.5] + $searchTimes[$medianSearchTimeOffset + 0.5]) / 2;
        }

        if ( $format eq $main::formatText ) {
            printf($TEXT_FILE "Search timings :\n\n");
            printf($TEXT_FILE "  Minimum                    : %12.6f\n", $minSearchTime);
            printf($TEXT_FILE "  Maximum                    : %12.6f\n", $maxSearchTime);
            printf($TEXT_FILE "  Mean                       : %12.6f\n", $meanSearchTime);
            printf($TEXT_FILE "  Median                     : %12.6f\n", $medianSearchTime);
            printf($TEXT_FILE "  Variance                   : %12.6f\n", $VarianceSearchTime);
            printf($TEXT_FILE "  Standard Deviation         : %12.6f\n", $standardDeviationTime);
            printf($TEXT_FILE "\n\n");
        }
        elsif ( $format eq $main::formatExcel ) {
    
            $workSheet->write_string(sprintf("A%d", $i), 'Search timings :', $formatTitleLeft);
            $i += 2;
    
            $workSheet->write_string(sprintf("A%d", $i), 'Minimum', $formatTitleLeft);
            $workSheet->write(sprintf("B%d", $i), $minSearchTime, $formatFloat);
            $i++;

            $workSheet->write_string(sprintf("A%d", $i), 'Maximum', $formatTitleLeft);
            $workSheet->write(sprintf("B%d", $i), $maxSearchTime, $formatFloat);
            $i++;

            $workSheet->write_string(sprintf("A%d", $i), 'Mean', $formatTitleLeft);
            $workSheet->write(sprintf("B%d", $i), $meanSearchTime, $formatFloat);
            $i++;

            $workSheet->write_string(sprintf("A%d", $i), 'Median', $formatTitleLeft);
            $workSheet->write(sprintf("B%d", $i), $medianSearchTime, $formatFloat);
            $i++;

            $workSheet->write_string(sprintf("A%d", $i), 'Variance', $formatTitleLeft);
            $workSheet->write(sprintf("B%d", $i), $VarianceSearchTime, $formatFloat);
            $i++;

            $workSheet->write_string(sprintf("A%d", $i), 'Standard Deviation', $formatTitleLeft);
            $workSheet->write(sprintf("B%d", $i), $standardDeviationTime, $formatFloat);
            $i += 2;
        }
    }
    
    
    if ( $slowSearchesFlag ) {
    
        if ( scalar(keys(%slowSearchTimes)) > 0 ) {
    
            if ( $format eq $main::formatText ) {
                printf($TEXT_FILE "Slow Searches (%d out of %d - %5.2f%%) :\n\n", scalar(keys(%slowSearchTimes)), $searchCount, ((scalar(keys(%slowSearchTimes)) / $searchCount) * 100));
        
                my @slowSearchTimes = sort { $b <=> $a } (keys(%slowSearchTimes));
                foreach my $slowSearchTime ( ($slowSearchCount > 0) ? splice(@slowSearchTimes, 0, $slowSearchCount) : @slowSearchTimes ) {
                    foreach my $search ( split(/\0/, $slowSearchTimes{$slowSearchTime}) ) {
                        printf($TEXT_FILE "  %11.6f  %s\n", $slowSearchTime, $search);
                    }
                }
                printf($TEXT_FILE "\n\n");
            }
            elsif ( $format eq $main::formatExcel ) {

                $workSheet->write_string(sprintf("A%d", $i), sprintf("Slow Searches (%d out of %d - %5.2f%%) :", scalar(keys(%slowSearchTimes)), $searchCount, ((scalar(keys(%slowSearchTimes)) / $searchCount) * 100)), $formatTitleLeft);
                $i += 2;
                
                my @slowSearchTimes = sort { $b <=> $a } (keys(%slowSearchTimes));
                foreach my $slowSearchTime ( ($slowSearchCount > 0) ? splice(@slowSearchTimes, 0, $slowSearchCount) : @slowSearchTimes ) {
                    foreach my $search ( split(/\0/, $slowSearchTimes{$slowSearchTime}) ) {
                        $workSheet->write(sprintf("A%d", $i), $slowSearchTime, $formatFloat);
                        my $unicodeSearch = $search;
                        Encode::from_to($unicodeSearch, "UTF8", "UTF-16BE");
                        $workSheet->write_unicode(sprintf("B%d", $i), $unicodeSearch);
                        $i++;
                    }
                }

                $i += 2;
            }
        }
    }
}

    

if ( $topSearchesFlag || $searchFrequenciesFlag ) {

    my $workSheet = undef;
    my $i = 1;

    if ( $format eq $main::formatExcel ) {

        # Add a worksheet
        $workSheet = $excelWorkBook->add_worksheet('Search Reports');

        # Set the column widths
        $workSheet->set_column(0, 1, 20);
    }            


    if ( $topSearchesFlag ) {
    
        if ( scalar( keys(%searches) ) > 0 ) {
    
            if ( $format eq $main::formatText ) {
                printf($TEXT_FILE "Top Searches :\n\n");
                my @searches = sort({$searches{$b} <=> $searches{$a}} keys(%searches));
                foreach my $search ( ($topSearchCount > 0) ? splice(@searches, 0, $topSearchCount) : @searches ) {
                    printf($TEXT_FILE "%5d  %s\n", $searches{$search}, $search);
                }
                printf($TEXT_FILE "\n\n");
            }
            elsif ( $format eq $main::formatExcel ) {

                $workSheet->write_string(sprintf("A%d", $i), 'Top Searches :', $formatTitleLeft);
                $i += 2;
                
                my @searches = sort({$searches{$b} <=> $searches{$a}} keys(%searches));
                foreach my $search ( ($topSearchCount > 0) ? splice(@searches, 0, $topSearchCount) : @searches ) {
                    $workSheet->write(sprintf("A%d", $i), $searches{$search}, $formatNumber);
                    my $unicodeSearch = $search;
                    Encode::from_to($unicodeSearch, "UTF8", "UTF-16BE");
                    $workSheet->write_unicode(sprintf("B%d", $i), $unicodeSearch);
                    $i++;
                }

                $i += 2;
            }
        }
    }
    
    
    if ( $searchFrequenciesFlag ) {
    
        if ( scalar( keys(%searches) ) > 0 ) {
            foreach my $searchFrequency ( values(%searches) ) {
                $searchFrequencies{$searchFrequency} = defined($searchFrequencies{$searchFrequency}) ? $searchFrequencies{$searchFrequency} + 1 : 1;
            }
        }

        if ( scalar(keys(%searchFrequencies)) > 0 ) {
    
            if ( $format eq $main::formatText ) {
    
                printf($TEXT_FILE "Search Frequencies :\n\n");
        
                my @searchFrequencies = sort({$searchFrequencies{$b} <=> $searchFrequencies{$a}} keys(%searchFrequencies));
                foreach my $searchFrequency ( ($topSearchFrequenciesCount > 0) ? splice(@searchFrequencies, 0, $topSearchFrequenciesCount) : @searchFrequencies ) {
                    printf($TEXT_FILE "%5d  %5d\n", $searchFrequency, $searchFrequencies{$searchFrequency});
                }
                printf($TEXT_FILE "\n\n");
            }
            elsif ( $format eq $main::formatExcel ) {
    
                $workSheet->write_string(sprintf("A%d", $i), 'Search Frequencies :', $formatTitleLeft);
                $i += 2;
                
                my @searchFrequencies = sort({$searchFrequencies{$b} <=> $searchFrequencies{$a}} keys(%searchFrequencies));
                foreach my $searchFrequency ( ($topSearchFrequenciesCount > 0) ? splice(@searchFrequencies, 0, $topSearchFrequenciesCount) : @searchFrequencies ) {
                    $workSheet->write(sprintf("A%d", $i), $searchFrequency, $formatNumber);
                    $workSheet->write(sprintf("B%d", $i),  $searchFrequencies{$searchFrequency}, $formatNumber);
                    $i++;
                }

                $i += 2;
            }
        }
    }
}



if ( $searchErrorsFlag || $retrievalErrorsFlag || $warningsFlag || $errorsFlag || $fatalsFlag || $debugsFlag ) {

    my $workSheet = undef;
    my $i = 1;

    if ( $format eq $main::formatExcel ) {

        # Add a worksheet
        $workSheet = $excelWorkBook->add_worksheet('Log Reports');

        # Set the column widths
        $workSheet->set_column(0, 1, 20);
    }            


    if ( $searchErrorsFlag ) {
    
        if ( $format eq $main::formatText ) {
            
            printf($TEXT_FILE "Search errors :\n\n");
    
            if ( scalar(@searchErrors) > 0 ) {
                foreach my $searchError ( @searchErrors ) {
                    printf($TEXT_FILE "  %s\n", $searchError);
                }
                printf($TEXT_FILE "\n\n");
            }
            else {
                printf($TEXT_FILE "  none\n\n");
            }
        }
        elsif ( $format eq $main::formatExcel ) {

            $workSheet->write_string(sprintf("A%d", $i), 'Search errors :', $formatTitleLeft);
            $i += 2;
            
            if ( scalar(@searchErrors) > 0 ) {
                foreach my $searchError ( @searchErrors ) {
                    my $unicodeSearchError = $searchError;
                    Encode::from_to($unicodeSearchError, "UTF8", "UTF-16BE");
                    $workSheet->write_unicode(sprintf("B%d", $i), $unicodeSearchError);
                    $i++;
                }
            }
            else {
                $workSheet->write_string(sprintf("B%d", $i), "none");
                $i++;
            }

            $i += 2;
        }
    }    
    
    
    if ( $retrievalErrorsFlag ) {
    
        if ( $format eq $main::formatText ) {
            
            printf($TEXT_FILE "Retrieval errors :\n\n");
        
            if ( scalar(@retrievalErrors) > 0 ) {
                foreach my $retrievalError ( @retrievalErrors ) {
                    printf($TEXT_FILE "  %s\n", $retrievalError);
                }
            }
            else {
                printf($TEXT_FILE "  none\n");
            }
            printf($TEXT_FILE "\n\n");
        }
        elsif ( $format eq $main::formatExcel ) {

            $workSheet->write_string(sprintf("A%d", $i), 'Retrieval errors :', $formatTitleLeft);
            $i += 2;
            
            if ( scalar(@retrievalErrors) > 0 ) {
                foreach my $retrievalError ( @retrievalErrors ) {
                    my $unicodeRetrievalError = $retrievalError;
                    Encode::from_to($unicodeRetrievalError, "UTF8", "UTF-16BE");
                    $workSheet->write_unicode(sprintf("B%d", $i), $unicodeRetrievalError);
                    $i++;
                }
            }
            else {
                $workSheet->write_string(sprintf("B%d", $i), "none");
                $i++;
            }

            $i += 2;
        }
    }
    
    
    if ( $warningsFlag ) {
    
        if ( $format eq $main::formatText ) {
            
            printf($TEXT_FILE "Warnings :\n\n");
        
            if ( scalar(@warnings) > 0 ) {
                foreach my $warning ( @warnings ) {
                    printf($TEXT_FILE "  %s\n", $warning);
                }
            }
            else {
                printf($TEXT_FILE "  none\n");
            }
            printf($TEXT_FILE "\n\n");
        }
        elsif ( $format eq $main::formatExcel ) {

            $workSheet->write_string(sprintf("A%d", $i), 'Warnings :', $formatTitleLeft);
            $i += 2;
            
            if ( scalar(@warnings) > 0 ) {
                foreach my $warning ( @warnings ) {
                    my $unicodeWarning = $warning;
                    Encode::from_to($unicodeWarning, "UTF8", "UTF-16BE");
                    $workSheet->write_unicode(sprintf("B%d", $i), $unicodeWarning);
                    $i++;
                }
            }
            else {
                $workSheet->write_string(sprintf("B%d", $i), "none");
                $i++;
            }

            $i += 2;
        }
    }
    
    
    if ( $errorsFlag ) {
    
        if ( $format eq $main::formatText ) {

            printf($TEXT_FILE "Errors :\n\n");
        
            if ( scalar(@errors) > 0 ) {
                foreach my $error ( @errors ) {
                    printf($TEXT_FILE "  %s\n", $error);
                }
            }
            else {
                printf($TEXT_FILE "  none\n");
            }
            printf($TEXT_FILE "\n\n");
        }
        elsif ( $format eq $main::formatExcel ) {

            $workSheet->write_string(sprintf("A%d", $i), 'Errors :', $formatTitleLeft);
            $i += 2;
            
            if ( scalar(@errors) > 0 ) {
                foreach my $error ( @errors ) {
                    my $unicodeError = $error;
                    Encode::from_to($unicodeError, "UTF8", "UTF-16BE");
                    $workSheet->write_unicode(sprintf("B%d", $i), $unicodeError);
                    $i++;
                }
            }
            else {
                $workSheet->write_string(sprintf("B%d", $i), "none");
                $i++;
            }

            $i += 2;
        }
    }
    
    
    if ( $fatalsFlag ) {
    
        if ( $format eq $main::formatText ) {

            printf($TEXT_FILE "Fatals :\n\n");
        
            if ( scalar(@fatals) > 0 ) {
                foreach my $fatal ( @fatals ) {
                    printf($TEXT_FILE "  %s\n", $fatal);
                }
            }
            else {
                printf($TEXT_FILE "  none\n");
            }
            printf($TEXT_FILE "\n\n");
        }
        elsif ( $format eq $main::formatExcel ) {

            $workSheet->write_string(sprintf("A%d", $i), 'Fatals :', $formatTitleLeft);
            $i += 2;
            
            if ( scalar(@fatals) > 0 ) {
                foreach my $fatal ( @fatals ) {
                    my $unicodeFatal = $fatal;
                    Encode::from_to($unicodeFatal, "UTF8", "UTF-16BE");
                    $workSheet->write_unicode(sprintf("B%d", $i), $unicodeFatal);
                    $i++;
                }
            }
            else {
                $workSheet->write_string(sprintf("B%d", $i), "none");
                $i++;
            }

            $i += 2;
        }
    }
    
    
    if ( $debugsFlag ) {
    
        if ( $format eq $main::formatText ) {

            printf($TEXT_FILE "Debugs :\n\n");
        
            if ( scalar(@debugs) > 0 ) {
                foreach my $debug ( @debugs ) {
                    printf($TEXT_FILE "  %s\n", $debug);
                }
            }
            else {
                printf($TEXT_FILE "  none\n");
            }
            printf($TEXT_FILE "\n\n");
        }
        elsif ( $format eq $main::formatExcel ) {

            $workSheet->write_string(sprintf("A%d", $i), 'Debugs :', $formatTitleLeft);
            $i += 2;
            
            if ( scalar(@debugs) > 0 ) {
                foreach my $debug ( @debugs ) {
                    my $unicodeDebug = $debug;
                    Encode::from_to($unicodeDebug, "UTF8", "UTF-16BE");
                    $workSheet->write_unicode(sprintf("B%d", $i), $unicodeDebug);
                    $i++;
                }
            }
            else {
                $workSheet->write_string(sprintf("B%d", $i), "none");
                $i++;
            }

            $i += 2;
        }
    }
}


# Close the text file
if ( $format eq $main::formatText ) {
    close($TEXT_FILE);
}

# Close the excel workbook
elsif ( $format eq $main::formatExcel ) {
    $excelWorkBook->close();
}




# Send email
if ( $destination eq $main::destinationEmail ) {

#     printf(STDERR "Sending email from: '%s', to: '%s'.\n", $emailFromAddress, $emailToAddresses);

    # Create the boundary
    my $boundary = "====" . time() . "====";

    
    # Create the mail hash
    my %mail = (
        'from' => $emailFromAddress,
        'to' => $emailToAddresses,
        'subject' => $emailSubject,
        'content-type' => sprintf("multipart/mixed; boundary=\"%s\"", $boundary),
        'body' => '',
        'smtp' => 'mail',
    );


    # Add the message
    my $message = sprintf("%s\n\n", $reportTitle);
    $mail{'body'} .= sprintf("--%s\nContent-Transfer-Encoding: 7bit\nContent-Type: text/plain; charset=\"iso-8859-1\"\n\n%s\n", $boundary, $message);


    # Add according to disposition
    if ( $emailDisposition eq $main::emailDispositionAttachment ) {

        if ( $format eq $main::formatText ) {

            # Add the log report - compressed attachement
            if ( $textFilePath && -f $textFilePath ) {
                $mail{'body'} .= &_createMailAttachment($boundary, $textFileName, $textFilePath, 1);
            }
        }
        elsif ( $format eq $main::formatExcel ) {

            # Add the excel file - uncompressed attachement
            if ( $excelFilePath && -f $excelFilePath ) {
                $mail{'body'} .= &_createMailAttachment($boundary, $excelFileName, $excelFilePath, 0);
            }
        }
    }
    else {

        # Add the log report
        if ( $textFilePath && -f $textFilePath ) {
            $mail{'body'} .= &_createMailInline($boundary, '', $textFileName, $textFilePath);
        }
    }


    # Final boundary
    $mail{'body'} .= sprintf("--%s--\n", $boundary);


    # Send the email
    if ( ! sendmail(%mail) ) {
        printf(STDERR "Failed to send the email, error: '%s', exiting.\n", $Mail::Sendmail::error);
        exit (-1);
    }

}

elsif ( $destination eq $main::destinationStdout ) {

    # Open the text file
    if ( ! open($TEXT_FILE, $textFilePath) ) {
        printf(STDERR "Failed to open the text file: '%s', exiting.\n", $textFilePath);
        exit (-1);
    }

    # Write it to stdout
    while (<$TEXT_FILE>) {
        print $_;
    }

    # Close the text file
    close ($TEXT_FILE);

}



# Clean up
if ( $cleanupFlag || ($destination eq $main::destinationStdout) ) {

    if ( $format eq $main::formatText ) {
        unlink($textFilePath);
    }
    elsif ( $format eq $main::formatExcel ) {
        unlink($excelFilePath);
    }
}



exit (0);



