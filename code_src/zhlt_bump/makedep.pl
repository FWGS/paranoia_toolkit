#!/usr/bin/perl

# Copyright (C) 2000  Sean Cavanaugh
# This file is licensed under the terms of the GNU Public License
# (see GPL.txt, or http://www.gnu.org/copyleft/gpl.txt)

@filespecs =
(
    '*.cpp',
    '*.c',
    '*.h',  
);

@pathspecs =
(
    '',
    'template',
    'common',
    'hlcsg',
    'hlbsp',
    'hlvis',
    'hlrad',
    'netvis',
);

$debug = 0;
$bUseIncludeEnv = 0;
$MODE = "\$(OUTDIR)";
#$EXT = ".o";
#$EXT = ".obj";


# rip include references out of source files
sub ScanSource
{
    my ($file) = @_;
    
    my @rval = ();
    open(SOURCEFILE, "<$file") || die "Could not open file $file\n";
    while (<SOURCEFILE>)
    {
        if (/^[\s]*#include[\s]*/)
        {
            s/^[\s]*#include[\s]*//;    # remove #include and trailing characters
            s/[\<\>\"\n]//g;            # remove all quotes and <> around filename
            s/\.h.*/\.h/i;              # truncate everything after the .h
            push(@rval, $_);            # add the include reference to the list
        }
    }
    close SOURCEFILE;   
    return @rval;
}

my $table;
my $flagged;

# attempt to locate header in include path/pathspecs, remove from list if not found
sub TranslateHeaders
{
    my ($file, @headers) = @_;
    
    my @translated = ();
    
    my @searchpaths = @pathspecs;
    push @searchpaths, '';
                
    foreach my $header (@headers)
    {
        my $found = 0;
        foreach my $path (@searchpaths)
        {
            if (!$found)
            {
                my $prefix;
                if ($path eq "")
                {
                    $prefix = '';
                }
                else
                {
                    $prefix = "$path/";
                }
                my $find = $prefix.$header;
                print STDERR "searching for [$find]\n" if $debug;
                if (-e $find)
                {
                    $_ = $find;
                    my $translatedfile = $_;
                    push @translated, $translatedfile;
                    print STDERR "found $translatedfile\n" if $debug;
                    $found = 1;
                }
            }
        }
        if (!$found)
        {
            print STDERR "removing unfindable header $header\n";
        }
    }

    print STDERR "file [$file] depends on @translated\n" if $debug;
    $table{$file}= [ @translated ];
    $flagged{$file}=0;

# TODO: handle environemnt variable INCLUDE
    
    return @translated;
}

sub RecurseGenerateList
{
    my ($filename) = @_;
    
    my $rval = $filename;
    $flagged{$filename} = 1; # prevent dependency recursion

    print STDERR "begin [$filename]:[@{$table{$filename}}]\n" if $debug;
    
    foreach $item ( @{$table{$filename}} )
    {
        if ($flagged{$item} == 1)
        {
            print STDERR "$filename flagged already\n" if $debug;
            goto skip;
        }
        if ($item eq $filename)
        {
            print STDERR "item == filename ($filename)\n" if $debug;
            goto skip;
        }
        if ($item eq "")
        {
            print STDERR "item == null\n" if $debug;
            goto skip;
        }
        $rval .= ' '.RecurseGenerateList($item);
        $flagged{$item} = 1; # prevent include recursion
skip:
    }
    
    return $rval;
}

sub ResetFlags
{
    foreach $item ( keys %flagged )
    {
        $flagged{$item} = 0;
    }
}

        

sub main
{
    foreach my $path (@pathspecs)
    {
        foreach my $filespec (@filespecs)
        {
            my $search;
            if ($path eq "")
            {
                $search = $filespec;
            }
            else
            {
                $search = "$path/$filespec";
            }
            my @files = glob($search);

            foreach my $file ( @files )
            {
                TranslateHeaders($file, ScanSource($file));
            }
        }
    }            
    
    my $x = 0;
    foreach $item ( sort keys %flagged )
    {
        print STDERR "working on file [$item]\n" if $debug;
        ResetFlags();
        my $biglist = RecurseGenerateList($item);
        $_ = $item;
        if (s/([\w]*)\.(cpp|c)/$MODE\/$1\$(OBJEXT)/gi)    # We only care about object files and their dependencies
        {
            print STDOUT "$_: $biglist\n";
        }
    }
    $x++;
    return 0;
}

&main;
