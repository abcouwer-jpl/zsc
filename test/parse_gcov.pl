#!/usr/bin/perl

use strict;
use warnings;

### Max length of filenames reported by gcov, needed to space out
### table of results
my($maxlen) = 0;
my($verbose) = 0;

my($FILE) = "file";
my($PCT) = "pct";
my($TESTED) = "tested";
my($UNTESTED) = "untested";
my($UNC_ASSERT) = "unc_assert";
my($LINES) = "lines";

my($GCOV) = "gcov --relative-only";

### Location of input .gcda files, AND location of $GOV output file.
my($OBJDIR) = ".";

### Directory to be scanned for .c, .cc files
my(@dirs);

### Filename containing cached $GCOV report output
my($GCOVOUTPUT) = "";

### Either Line-by-line contents of $GCOVOUTPUT or the current output of $GCOV
my(@gcov_output);

while ($#ARGV >= 0) {
  $_ = shift(@ARGV);

  if ($_ eq "-g") {
    die "No argument provided for -gcov option" if (!@ARGV);
    $GCOV = shift(@ARGV);
  } elsif ($_ eq "-o") {
    die "No argument provided for -o option" if (!@ARGV);
    $OBJDIR = shift(@ARGV);
  } elsif ($_ eq "-s") {
    die "No argument provided for -s option" if (!@ARGV);
    push @dirs, shift(@ARGV);
  } elsif ($_ eq "-v") {
    if ($#ARGV >= 0 && $ARGV[0] =~ /^\d+$/) {
      $verbose = shift(@ARGV);
    } else {
      $verbose = 1;
    }
  } elsif ($_ eq "-l") {
    die "No argument provided for -l option" if (!@ARGV);
    $GCOVOUTPUT = shift(@ARGV);
  } elsif ($_ eq "-h" || $_ eq "--help") {
    usage(0);
  } elsif (-d $_) {
    push @dirs, $_;
  } else {
    print STDERR "Unrecognized option \"$_\"\n";
    usage(-1);
  }
}

if (scalar (@dirs) == 0) {
    push @dirs, ".";
}

if (!$GCOVOUTPUT) {
  # Read all the *.gcno files in the directory.
  # / Read all the *.c and *.cc files in the directory.
  #
  # We use "opendir" instead of `ls` to avoid warnings about
  # *.cc not matching any files.


  map {
    my($dir) = $_;
    my(@files);

    print "== Scanning \"$dir\" for source files\n";
    if (opendir (DIR, $dir)) {
      map {
        my($file) = $_;
    
        if ($file =~ /\.(c|cc)$/) {
    
          # Sigh... gcov44 gets confused if we include a file that does not have
          # corresponding gcda/gcno files.  It doesn't just ignore the file
          # with missing coverage, but also hoses the coverage metrics on other
          # files that do have valid gcda/gcno files.  Here we'll check to see
          # if the corresponding gcda exists before adding the source file to
          # the list of files that we'll process.
          my($cov_file) = "$dir/$OBJDIR/$file";
          $cov_file =~ s/\.(c|cc)$/\.gcda/;
          if (-f $cov_file){
              # Simple "grep" on readdir wouldn't include the $dir path, 
              # so add it here.
              push @files, "$dir/$file";
              print "  == Found $files[$#files]\n"
              if $verbose > 3;
          }
        }
      } sort readdir DIR;
      closedir DIR;
    }
    if (!@files) {
    printf "No input files found in directory %s\n",
    $dir;
    usage(-1);
    }
    my($gcov_command) = "$GCOV -o $dir/$OBJDIR " . join (" ", sort @files );
    print "== running \"$gcov_command\"\n" if $verbose;
    push @gcov_output, (`$gcov_command`);
    for (@gcov_output) {
        print "  == $_" if $verbose;
    }
    
  } @dirs;
} else {
  @gcov_output = `/bin/cat $GCOVOUTPUT`;
} 

my(@required_results);
my(@ac_results);
my(@results);

### Current filename
my($file);


for (@gcov_output) {
  chomp;
  if (/File/) {
    # eat everything up to the name of the file
    ($file = $_) =~ s|^.*/||g;
    $file =~ s/File //g;
    $file =~ s/\'//g;
    my $len = length($file);
    print " == new maxlen of $len\n" if $verbose and ($len > $maxlen);
    $maxlen = $len if ($len > $maxlen);
    print " == $file\n" if $verbose;
    
  } elsif (/Lines executed/) {

    # eat everything up to the colon, leaving "xx.xx% of yyy"
    s/^.*://g;
    my @array = split(/\% of /);
    my %rec;

    $rec{$FILE} = $file;
    $rec{$PCT} = $array[0];
    $rec{$LINES} = $array[1];
    $rec{$TESTED} = int($array[0]*$array[1]/100.0);
    $rec{$UNTESTED} = $rec{$LINES} - $rec{$TESTED};

    filter_assertion_lines(\%rec);

    push (@results, \%rec);
  }
}

summarize_results("All", @results);


sub count_lines_matching_regexp {
  my ($regexp, $filename) = @_;
  my ($lines) = 0;

  if (open (FILE, "<$filename")) {
    my(@contents) = <FILE>;

    close FILE;
    map {
      my($line) = $_;

      $lines++ if $line =~ /$regexp/;
    } @contents;
  } else {
    print STDERR "*** Failed to open \"$filename\"!\n";
    exit 1;
  }

  return $lines;
}


# If a file does not have 100% coverage, count how many lines are
# FSW_ASSERTs that always fail.
sub filter_assertion_lines {
  my $rec = $_[0];
  
  if ($rec->{$PCT} != 100.0) {
    my $unconditional_assertions =
    count_lines_matching_regexp
    ('^\s*\#+\s*:\s*\d+\s*:\s*Assert(_\d)?\s*\(\s*(0|FALSE)\s*(,|\))',
     $rec->{$FILE} . ".gcov");
    
    #Now count *every* line that was not tested.  If we compute this
    #number based upon the percentages reported, there are rounding
    #errors that contaminate the results.
    my $untested_lines =
    count_lines_matching_regexp
    ('^\s*#+\s*:\s*\d+\s*:',
     $rec->{$FILE} . ".gcov");


    $rec->{$UNC_ASSERT} = $unconditional_assertions;
    $rec->{$UNTESTED} = $untested_lines - $rec->{$UNC_ASSERT};
    $rec->{$TESTED} = $rec->{$LINES} - $rec->{$UNTESTED} - $rec->{$UNC_ASSERT};
    $rec->{$PCT} = 100.0*($rec->{$LINES} - $rec->{$UNTESTED}) / $rec->{$LINES};
  } else {
    $rec->{$UNC_ASSERT} = 0;
  }
}

sub summarize_results {
  my $group = shift(@_);
  my @res = @_;
  my $rec;
  my $total_lines = 0;
  my $tested_lines = 0;
  
  return if (!@res);
  
  print "\n#################### $group files #######################\n";

  for $rec (@res) {
    $total_lines += $rec->{$LINES};
    $tested_lines += $rec->{$PCT}*$rec->{$LINES}/100.0;
  }
  my @sorted_results = sort { -($a->{$PCT} <=> $b->{$PCT}) || 
                                $a->{$FILE} cmp $b->{$FILE}} @res;  
  
  printf "%30s \t%6s  \t%6s  \t%s \t%s    %s\n", "Filename", "Percent", "Lines", "Lines",  "Lines",   "Unconditional";
  printf "%30s \t%6s  \t%6s  \t%s \t%s %s\n", "",         "tested",  "total", "tested", "untested", "assertion lines";
  
  for (@sorted_results) {
    printf "%30s \t%6.2f%%   \t%6s  \t%6d \t%6d\t %6d\n", 
      $_->{$FILE}, $_->{$PCT}, $_->{$LINES}, $_->{$TESTED}, $_->{$UNTESTED}, $_->{$UNC_ASSERT};
  }
  $tested_lines = int($tested_lines);
  my $tested_pct = int(100*$tested_lines/$total_lines);
  print "\n$group coverage:  $tested_lines / $total_lines  lines  ($tested_pct\%)\n";
}


sub usage {
print <<END;
$0: 

  Generate code coverage statistics, e.g. from FSW Unit Tests, by
  running gcov over profiler output files (.gcda suffix).

  This script runs gcov on C/C++ source files for any executable that
  has been run with code coverage enabled.

  Options:

  -g <gcov-path>        Use alternate gcov binary instead of $GCOV
  -h                    Print this informative message and exit.
  -l <log-file>     Read gcov output from file INSTEAD OF scanning dirs.
  -o <obj-dir>          Pass -o option to gcov [ $OBJDIR ]
  -s <source-file-dir>  Run gcov on .c files found in source-file-dir.
  -v [verbose]      Set verbosity level (between 0-10)

Usage: $0 [ -g gcov | -h | -l file | -o dir | -s dir | -v [\#] | dir ] ...

END

  exit ($_[0]);
}


