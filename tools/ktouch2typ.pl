#!/usr/bin/perl -w

# converts from ktouch's .ktouch to gtypist's .typ-file
# send comments and suggestions to bug-gtypist@gnu.org

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
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


# configurable variables
my $lines_per_drill = 4; # 1-11 for O: and D:, 1-22 for P:
my $drill_type = "O:";


my $ktouchfilename = undef;
my $typfilename = undef;
my $KTOUCHFILE = undef;
my $TYPFILE = undef;

my $i;
my $j;

# some sanity checks
if ($lines_per_drill < 1) {
    die "Invalid lines_per_drill: $lines_per_drill\n";
}
if ($drill_type eq "O:" || $drill_type eq "D:") {
    if ($lines_per_drill > 11) {
	die "Invalid lines_per_drill for [OD]:: $lines_per_drill\n";
    }
} else {
    if ($drill_type ne "P:") {
	die "Invalid drill_type: $drill_type\n";
    }
    if ($lines_per_drill > 22) {
	die "Invalid lines_per_drill for P:: $lines_per_drill\n";
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
    print TYPFILE "# ktouch can be found at http://ktouch.sourceforge.net\n\n";
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
	# $line contains the first non-blank, non-comment line (which is the
	# name of the lesson)
	chomp($line);
	$lesson_names[$lesson_counter] = $line;
	print TYPFILE getBanner("Lesson $lesson_counter: " . $line);

	convert_lesson($lesson_counter);

	print TYPFILE "G:E_LESSON$lesson_counter\n\n";
	
	if (!defined($line)) {
	    $done = 1;
	}
	++$lesson_counter;
    }

    --$lesson_counter;

    # generate jump-table
    print TYPFILE "\n# jump-table\n";
    $i = 1;
    while ($i <= $lesson_counter)
    {
	print TYPFILE "*:E_LESSON$i\n";
	if ($i < $lesson_counter) {
	    print TYPFILE "Q: Do you want to continue to lesson " . ($i + 1) .
		" [Y/N] ?\n";
	    print TYPFILE "N:MENU\nG:S_LESSON" . ($i + 1) . "\n";
	} else {
	    print TYPFILE "G:MENU\n";
	}
	++$i;
    }

    # calculate the total # of pages
    my $total_n_pages = 0;
    $i = $lesson_counter;
    while ($i > 0) {
	# how many lessons fit on this page ($j) ?
	$j = 11; # one key is always reserved for "exit"
	# do we need to reserve a key for "previous page" ?
	if ($total_n_pages > 0) { --$j; }
	# do we need to reserve a key for "next page" ?
	if ($i > $j) { --$j; }
	
	$i -= ($j > $i) ? $i : $j;
	++$total_n_pages;
    }

    my $page_counter = 1;
    my $cur_lesson = 1;
    # how many lessons fit on this menu-page
    my $n_lessons_this_page = undef;
    # these are the indices of the "Next page" and "Previous page" menu-items
    my $prev_page_idx = undef;
    my $next_page_idx = undef;

    # generate menu (possibly many pages)
    print TYPFILE "\n\n*:MENU";
    while ($cur_lesson <= $lesson_counter)
    {
	print TYPFILE "\n*:MENU_P$page_counter\n";

	# find out how many lessons fit on this menu-page
	$n_lessons_this_page = 11; # Fkey 12 is reserved for "exit"
	if ($page_counter > 1) { --$n_lessons_this_page; } # "prev"
	if ($page_counter < $total_n_pages) { --$n_lessons_this_page; } #"next"

	# bind function keys
	$i = $cur_lesson;
	while ($i <= $lesson_counter && $i<$cur_lesson + $n_lessons_this_page)
	{
	    print TYPFILE "K:" . ($i - $cur_lesson + 1) . ":S_LESSON$i\n";
	    ++$i;
	}
	if ($i == $lesson_counter + 1) {
	    # this is the last page, so bind unused keys to "NULL"
	    # i=lesson_counter+1 => fkey=lesson_counter-cur_lesson+2
	    $i = $lesson_counter - $cur_lesson + 2; 
	    while ($i <= $n_lessons_this_page) {
		print TYPFILE "K:$i:NULL\n"; ++$i;
	    }
	}
	
	# find the indices of the "Next page" and "Previous page" menu-items
	if ($page_counter > 1) { 
	    $prev_page_idx = $n_lessons_this_page + 1;
	} else {
	    $prev_page_idx = undef;
	}
	if ($page_counter < $total_n_pages) {
	    if ($page_counter > 1) {
		$next_page_idx = $n_lessons_this_page + 2;
	    } else {
		$next_page_idx = $n_lessons_this_page + 1;
	    }
	} else {
	    $next_page_idx = undef;
	}

	# add "previous page" binding
	if (defined($prev_page_idx)) {
	    print TYPFILE "K:$prev_page_idx:MENU_P" . ($page_counter - 1)."\n";
	}
	# add "next page" binding
	if (defined($next_page_idx)) {
	    print TYPFILE "K:$next_page_idx:MENU_P" . ($page_counter + 1)."\n";
	}
	# add "exit" binding
	print TYPFILE "K:12:EXIT\n\n";

	# generate T: command:
	print TYPFILE getBanner("Lesson selection menu - " . 
				"[page $page_counter of $total_n_pages]");
	print TYPFILE "T:this file contains the following $lesson_counter " .
	    "lessons:\n :\n";
	$i = $cur_lesson;
	while ($i <= $lesson_counter && $i < $cur_lesson + $n_lessons_this_page)
	{
	    print TYPFILE " :        Fkey " . ($i - $cur_lesson + 1) .
		" - $lesson_names[$i]\n";
	    ++$i;
	}
	if ($i == $lesson_counter + 1) {
	    # this is the last page, so insert blank continuation-lines
	    # i=lesson_counter+1 => fkey=lesson_counter-cur_lesson+2
	    $i = $lesson_counter - $cur_lesson + 2; 
	    while ($i <= $n_lessons_this_page) {
		print TYPFILE " :\n"; ++$i;
	    }
	}
	# print help for "prev"
	if (defined($prev_page_idx)) {
	    print TYPFILE " :        Fkey $prev_page_idx - " .
		"Previous menu page...\n";
	}
	# print help for "next"
	if (defined($next_page_idx)) {
	    print TYPFILE " :        Fkey $next_page_idx - " .
		"Next menu page...\n";
	}
	# print help for "exit"
	print TYPFILE " :        Fkey 12 - Quit program\n";

	# TODO: should there be a Q: to avoid the "Press Enter to cont."-msg ?
	#print TYPFILE "Q: Please select a lesson";
	$cur_lesson += $n_lessons_this_page;
	++$page_counter;
    }

    print TYPFILE "\n*:EXIT\nX:\n";

    close(TYPFILE) || die "Couldn't close $typfilename: $!";
    close(KTOUCHFILE) || die "Couldn't close $ktouchfilename: $!";
}

sub getAbsoluteFilename
{
    if ($_[0] =~ /^\/.*/) {
	return $_[0];
    } else {
	return Cwd::getcwd() . "/" .  $_[0];
    }
}

sub isBlank { return $_[0] =~ /^[ \t]*$/; }
sub isComment { return $_[0] =~ /^[ \t]*\#.*$/; }
sub isBlankorComment { return isBlank($_[0]) || isComment($_[0]); }

# this creates a B:-command (centered on 66 columns because
# "gtypist <version>" is in the right corner)
sub getBanner
{
    my $banner = $_[0];
    # remove whitespace at the beginning ...
    $banner =~ s/^\s*//;
    # ... and at the end
    $banner =~ s/\s*$//;
    return "B:" . " " x (33 - int((length($banner) / 2))) . $banner . "\n";
}

# this reads from KTOUCHFILE until it finds a blank line or a comment
sub convert_lesson
{
    my $line = undef;
    my $line_counter = 0;
    my $drill_counter = 1;
    my $lesson_counter = $_[0];

    while (defined($line = <KTOUCHFILE>) && !isBlankorComment($line))
    {
	chomp($line);
	if ($line_counter == 0) {
	    print TYPFILE "*:LESSON${lesson_counter}_D$drill_counter\n";
	    print TYPFILE "I:($drill_counter)\n";
	    print TYPFILE "${drill_type}$line\n";
	} else {
	    print TYPFILE " :$line\n";
	}

	++$line_counter;
	if ($line_counter == $lines_per_drill) {
	    print TYPFILE
		"Q: Press Y to continue, N to repeat, or Fkey 12 to exit\n";
	    print TYPFILE "N:LESSON${lesson_counter}_D$drill_counter\n";
	    $line_counter = 0; ++$drill_counter;
	}
    }
}

# Local Variables:
# compile-command: "./ktouch2typ.pl german.ktouch norwegian.ktouch g.ktouch"
# End:




