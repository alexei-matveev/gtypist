#!/usr/bin/perl -w

# functions to create a menu + jump-table for a set of gtypist-lessons
# plus some miscellaneous functions to convert to gtypist-lessons

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
package gtypist;


BEGIN {
    use Exporter   ();
    use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
    # set the version for version checking
    $VERSION     = 1.00;
    @ISA         = qw(Exporter);
    @EXPORT      = qw(&getAbsoluteFilename &isBlank &isComment &isBlankorComment &getBanner &generate_jump_table &getTotalNrMenuPages &generate_menu);
    %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],
    

    # your exported package globals go here,
    # as well as any optionally exported functions
    @EXPORT_OK   = (); #qw($Var1 %Hashit &func3);
}
use vars      @EXPORT_OK;

# non-exported package globals go here
#use vars      qw(@more $stuff);

# initialize package globals, first exported ones
#$Var1   = '';
#%Hashit = ();

# then the others (which are still accessible as $Some::Module::stuff)
#$stuff  = '';
#@more   = ();

sub getAbsoluteFilename($)
{
    if ($_[0] =~ /^\/.*/) {
	return $_[0];
    } else {
	return Cwd::getcwd() . "/" .  $_[0];
    }
}

sub isBlank($) { return $_[0] =~ /^[ \t]*$/; }
sub isComment($) { return $_[0] =~ /^[ \t]*\#.*$/; }
sub isBlankorComment($) { return isBlank($_[0]) || isComment($_[0]); }

# this creates a B:-command (centered on 66 columns because
# "gtypist <version>" is in the right corner)
sub getBanner($)
{
    my $banner = $_[0];
    # remove whitespace at the beginning ...
    $banner =~ s/^\s*//;
    # ... and at the end
    $banner =~ s/\s*$//;
    return "B:" . " " x (33 - int((length($banner) / 2))) . $banner . "\n";
}

sub generate_jump_table($*)
{
    my $n_lessons = shift;
    my $fh = shift;
    print $fh "\n# jump-table\n";
    my $i = 1;
    while ($i <= $n_lessons)
    {
	print $fh "*:E_LESSON$i\n";
	if ($i < $n_lessons) {
	    print $fh "Q: Do you want to continue to lesson " . ($i + 1) .
		" [Y/N] ?\n";
	    print $fh "N:MENU\nG:S_LESSON" . ($i + 1) . "\n";
	} else {
	    print $fh "G:MENU\n";
	}
	++$i;
    }
}

sub getTotalNrMenuPages($)
{
    # calculate the total # of pages
    my $total_n_pages = 0;
    my $i = $_[0];
    my $j;
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
    return $total_n_pages;
}

sub generate_menu($*@)
{
    my $n_lessons = shift @_;
    my $total_n_pages = getTotalNrMenuPages($n_lessons);
    my $fh = shift @_;
    my @lesson_names = @_;
    my $page_counter = 1;
    my $cur_lesson = 1;
    # how many lessons fit on this menu-page
    my $n_lessons_this_page = undef;
    # these are the indices of the "Next page" and "Previous page" menu-items
    my $prev_page_idx = undef;
    my $next_page_idx = undef;
    my $i;
    my $j;

    # generate menu (possibly many pages)
    print $fh "\n\n*:MENU";
    while ($cur_lesson <= $n_lessons)
    {
	print $fh "\n*:MENU_P$page_counter\n";

	# find out how many lessons fit on this menu-page
	$n_lessons_this_page = 11; # Fkey 12 is reserved for "exit"
	if ($page_counter > 1) { --$n_lessons_this_page; } # "prev"
	if ($page_counter < $total_n_pages) { --$n_lessons_this_page; } #"next"

	# bind function keys
	$i = $cur_lesson;
	while ($i <= $n_lessons && $i<$cur_lesson + $n_lessons_this_page)
	{
	    print $fh "K:" . ($i - $cur_lesson + 1) . ":S_LESSON$i\n";
	    ++$i;
	}
	if ($i == $n_lessons + 1) {
	    # this is the last page, so bind unused keys to "NULL"
	    # i=$n_lessons+1 => fkey=$n_lessons-cur_lesson+2
	    $i = $n_lessons - $cur_lesson + 2; 
	    while ($i <= $n_lessons_this_page) {
		print $fh "K:$i:NULL\n"; ++$i;
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
	    print $fh "K:$prev_page_idx:MENU_P" . ($page_counter - 1)."\n";
	}
	# add "next page" binding
	if (defined($next_page_idx)) {
	    print $fh "K:$next_page_idx:MENU_P" . ($page_counter + 1)."\n";
	}
	# add "exit" binding
	print $fh "K:12:EXIT\n\n";

	# generate T: command:
	print $fh getBanner("Lesson selection menu - " . 
				"[page $page_counter of $total_n_pages]");
	print $fh "T:this file contains the following $n_lessons lessons:\n :\n";
	$i = $cur_lesson;
	while ($i <= $n_lessons && $i < $cur_lesson + $n_lessons_this_page)
	{
	    print $fh " :        Fkey " . ($i - $cur_lesson + 1) .
		" - $lesson_names[$i]\n";
	    ++$i;
	}
	if ($i == $n_lessons + 1) {
	    # this is the last page, so insert blank continuation-lines
	    # i=lesson_counter+1 => fkey=lesson_counter-cur_lesson+2
	    $i = $n_lessons - $cur_lesson + 2; 
	    while ($i <= $n_lessons_this_page) {
		print $fh " :\n"; ++$i;
	    }
	}
	# print blank (T:-) line so that the user can easily distinguish
	# between keys which start a lesson and those which do prev_page,
	# next_page and exit
	print $fh " :\n";
	# print help for "prev"
	if (defined($prev_page_idx)) {
	    print $fh " :        Fkey $prev_page_idx - " .
		"Previous menu page...\n";
	}
	# print help for "next"
	if (defined($next_page_idx)) {
	    print $fh " :        Fkey $next_page_idx - " .
		"Next menu page...\n";
	}
	# print help for "exit"
	print $fh " :        Fkey 12 - Quit program\n\n";
	print $fh "Q: Please select a lesson or press Fkey12 to exit\n\n";

	$cur_lesson += $n_lessons_this_page;
	++$page_counter;
    }

    print $fh "\n*:EXIT\nX:\n";
}

END { }

1;

# Local Variables:
# compile-command: "./tt2typ.pl /mnt/src/tipptrainer-0.3.3/data/german/"
# End:
