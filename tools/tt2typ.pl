#!/usr/bin/perl -w

# converts tipptrainer-0.3.3-lessons to gtypist's .typ-file
# send comments and suggestions to bug-gtypist@gnu.org

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, either version 3
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

use strict;
use Cwd; # Cwd::getcwd
use gtypist;

sub read_lesson_index($);
sub convert_lesson($$*);

# configurable variables
my $lines_per_drill = 4; # 1-11 for [Dd]:, 1-22 for [Ss]:
my $drill_type = "D:"; # [DdSs]

# TODO:
# - use kurs.* files
# - create drills so that they are arranged similar to the paragraphs
# in the lektion.* files

# some sanity checks
if ($lines_per_drill < 1) {
    die "Invalid lines_per_drill: $lines_per_drill\n";
}
if ($drill_type eq "D:" || $drill_type eq "d:") {
    if ($lines_per_drill > 11) {
	die "Invalid lines_per_drill for [OD]:: $lines_per_drill\n";
    }
} else {
    if ($drill_type ne "S:" && $drill_type ne "s:") {
	die "Invalid drill_type: $drill_type\n";
    }
    if ($lines_per_drill > 22) {
	die "Invalid lines_per_drill for [Ss]:: $lines_per_drill\n";
    }
}

if (!defined($ARGV[0]) || !(-d $ARGV[0])) {
    die "You must specify a data-subdirectory of tipptrainer " .
	"as the 1st argument.\n" . 
	"For example: 'tt2typ.pl ~/tipptrainer-0.3.3/data/german/'.\n" .
        "(you probably want data/german because the english lessons are incomplete)\n";
}
# remove trailing '/'
if ($ARGV[0] =~ /\/$/) {
    chop($ARGV[0]);
}
my $datadir = $ARGV[0];
if (!(-f "$datadir/lektion.index")) {
    die "$datadir/lektion.index not found!\n";
}

my $typfilename = substr($ARGV[0], rindex($ARGV[0], "/") + 1) . ".typ";
print "creating $typfilename...\n";

my @lesson_names = read_lesson_index("$datadir/lektion.index");

my $TYPFILE = undef;
open(TYPFILE, ">$typfilename") || 
    die "Couldn't open $typfilename for writing: $!";
print TYPFILE "# created by tt2typ.pl from $datadir\n";
print TYPFILE "# on " . `date`;
print TYPFILE
    "# tt2typ.pl is part of gtypist (http://www.gnu.org/software/gtypist/)\n";
print TYPFILE "# tipptrainer can be found at http://www.pingos.schulnetz.org/tipptrainer\n";
print TYPFILE "# If you have suggestions about these lessons, write to\n" .
    "# tipptrainer\@reith.8m.com, with cc to bug-gtypist\@gnu.org.\n\n";
print TYPFILE "G:MENU\n\n";
my $lesson_counter = 1;
my $TTFILE = undef;
my $line = undef;
while (-f "$datadir/lektion.$lesson_counter")
{
    open(TTFILE, "$datadir/lektion.a$lesson_counter") ||
	die "Couldn't open $datadir/lektion.a$lesson_counter for reading: $!";
    print TYPFILE "*:S_LESSON$lesson_counter\n";
    print TYPFILE "K:12:MENU\n";
    print TYPFILE getBanner($lesson_names[$lesson_counter]);
    print TYPFILE "T:\n";
    while (defined($line = <TTFILE>))
    {
	chomp($line);
	print TYPFILE " : $line\n";
    }
    close(TTFILE) || 
	die "Couldn't close $datadir/lektion.a$lesson_counter: $!";

    convert_lesson($lesson_counter, "$datadir/lektion.$lesson_counter",
		   \*TYPFILE);
    print TYPFILE "G:E_LESSON$lesson_counter\n\n";

    ++$lesson_counter;
}

--$lesson_counter;
generate_jump_table($lesson_counter, \*TYPFILE);
generate_menu("tipptrainer 0.6.0 lessons",
	      $lesson_counter, \*TYPFILE, @lesson_names);

close(TYPFILE) || die "Couldn't close $typfilename: $!";



sub read_lesson_index($)
{
    my $TTFILE = undef;
    my @lesson_names = ();
    open(TTFILE, $_[0]) ||
	die "Couldn't open $_[0] for reading: $!";
    while (defined($line = <TTFILE>))
    {
	$line =~ /^([0-9]+) (.*)$/;
	$lesson_names[$1] = "Lesson $1: $2";
    }
    close(TTFILE) || die "Couldn't close $_[0]: $!";
    return @lesson_names;
}

sub convert_lesson($$*)
{
    my $lesson_counter = shift;
    my $lesson_file = shift;
    my $typfile = shift;
    my $TTFILE = undef;
    my $drill_counter = 1;
    my $line_counter = 0;
    my $line = undef;
    open(TTFILE, $lesson_file) || 
	die "Couldn't open $lesson_file for reading: $!";
    while (defined($line = <TTFILE>))
    {
	# skip underline of (which is used as underline for headline)
	# and blank lines
	if ($line =~ /^[ \t]*-+[ \t]*/ || isBlank($line)) { next; }

	if (!isBlank($line)) {
	    chomp($line);
	    if ($line_counter == 0) {
		print $typfile "*:LESSON${lesson_counter}_D$drill_counter\n";
		print $typfile "I:($drill_counter)\n";
		print $typfile "${drill_type}$line\n";
	    } else {
		print $typfile " :$line\n";
	    }
	}
	++$line_counter;
	if ($line_counter == $lines_per_drill) {
# this is not necessary any more: it's implied in D:
#	    print $typfile
#		"Q: Press Y to continue, N to repeat, or Fkey 12 to exit\n";
#	    print $typfile "N:LESSON${lesson_counter}_D$drill_counter\n";
	    $line_counter = 0; ++$drill_counter;
	}
    }
    close(TTFILE) || die "Couldn't close $lesson_file: $!";
# this is not necessary any more: it's implied in D:
#   print $typfile "Q: Press Y to continue, N to repeat, or Fkey 12 to exit\n";
#   print $typfile "N:LESSON${lesson_counter}_D$drill_counter\n";
}

# Local Variables:
# compile-command: "./tt2typ.pl ~/src/tipptrainer-0.6.0/data/german/"
# End:
