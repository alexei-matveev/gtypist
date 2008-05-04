#!/usr/bin/perl -w

# converts from ktouch's .ktouch to gtypist's .typ-file
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

use strict qw(subs vars refs);
use Cwd; # Cwd::getcwd
use gtypist;

# configurable variables
my $lines_per_drill = 4; # 1-11 for [Dd]: and D:, 1-22 for [Ss]:
my $drill_type = "D:"; # [DdSs]


my $ktouchfilename = undef;
my $typfilename = undef;
my $KTOUCHFILE = undef;
my $TYPFILE = undef;

# some sanity checks
if ($lines_per_drill < 1) {
    die "Invalid lines_per_drill: $lines_per_drill\n";
}
if ($drill_type eq "D:" || $drill_type eq "d:") {
    if ($lines_per_drill > 11) {
	die "Invalid lines_per_drill for [Dd]:: $lines_per_drill\n";
    }
} else {
    if ($drill_type ne "S:" && $drill_type ne "s:") {
	die "Invalid drill_type: $drill_type\n";
    }
    if ($lines_per_drill > 22) {
	die "Invalid lines_per_drill for [Ss]:: $lines_per_drill\n";
    }
}

while(defined($ktouchfilename = shift(@ARGV)))
{
    if (substr($ktouchfilename, rindex($ktouchfilename, ".")) ne ".ktouch" ||
	! (-f $ktouchfilename)) { 
	print "Skipping $ktouchfilename.\n";
	next;
    }
    $typfilename = $ktouchfilename;
    substr($typfilename, rindex($typfilename, ".")) = ".typ";
    print "Converting $ktouchfilename to $typfilename...\n";

    open(KTOUCHFILE, "$ktouchfilename") ||
	die "Couldn't open $ktouchfilename for reading: $!";
    open(TYPFILE, ">$typfilename") ||
	die "Couldn't open $typfilename for writing: $!";

    print TYPFILE "# created by ktouch2typ.pl from " . 
	getAbsoluteFilename($ktouchfilename) . "\n# on " . `date`;
    print TYPFILE "# ktouch2typ.pl is part of gtypist (http://www.gnu.org/software/gtypist/)\n";
    print TYPFILE "# ktouch can be found at http://ktouch.sourceforge.net\n";
    print TYPFILE "# If you have suggestions about these lessons,\n";
    print TYPFILE "# please send mail to haavard\@users.sourceforge.net\n";
    print TYPFILE "# (or whoever is the current ktouch maintainer), with\n";
    print TYPFILE "# cc to bug-gtypist\@gnu.org\n\n";
    print TYPFILE "G:MENU\n\n";

    my $line = undef;
    my $done = 0;
    my $lesson_counter = 1;
    my @lesson_names = (); # this is needed for the menu
    while (!$done)
    {
	while (defined($line = <KTOUCHFILE>) && isBlankorComment($line)) { 
	    if (isComment($line)) {
		# make sure that '#' is at the beginning of the line
		$line =~ s/^\s*//;
		print TYPFILE $line;
	    }
	}
	if (!defined($line)) { $done = 1; next;	}
	
	print TYPFILE "*:S_LESSON$lesson_counter\n";
	print TYPFILE "K:12:MENU\n";
	# $line contains the first non-blank, non-comment line (which is the
	# name of the lesson)
	chomp($line);
	$lesson_names[$lesson_counter] = $line;
	print TYPFILE getBanner("Lesson $lesson_counter: " . $line);

	convert_lesson($lesson_counter, \*KTOUCHFILE, \*TYPFILE);

	print TYPFILE "G:E_LESSON$lesson_counter\n\n";
	
	if (!defined($line)) {
	    $done = 1;
	}
	++$lesson_counter;
    }

    --$lesson_counter;

    generate_jump_table($lesson_counter, \*TYPFILE);
    generate_menu("ktouch lesson ($ktouchfilename)",
		  $lesson_counter, \*TYPFILE, @lesson_names);

    close(TYPFILE) || die "Couldn't close $typfilename: $!";
    close(KTOUCHFILE) || die "Couldn't close $ktouchfilename: $!";
}


# this reads from KTOUCHFILE until it finds a blank line or a comment
sub convert_lesson($lesson_counter *KTOUCHFILE *TYPFILE)
{
    my $lesson_counter = shift;
    my $ktouchfile = shift;
    my $typfile = shift;
    my $line = undef;
    my $line_counter = 0;
    my $drill_counter = 1;

    while (defined($line = <$ktouchfile>) && !isBlankorComment($line))
    {
	chomp($line);
	if ($line_counter == 0) {
	    print $typfile "*:LESSON${lesson_counter}_D$drill_counter\n";
	    print $typfile "I:($drill_counter)\n";
	    print $typfile "${drill_type}$line\n";
	} else {
	    print $typfile " :$line\n";
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
# this is not necessary any more: it's implied in D:
#   print $typfile "Q: Press Y to continue, N to repeat, or Fkey 12 to exit\n";
#    print $typfile "N:LESSON${lesson_counter}_D$drill_counter\n";
}

# Local Variables:
# compile-command: "./ktouch2typ.pl german.ktouch norwegian.ktouch g.ktouch"
# End:
