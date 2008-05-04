#!/usr/bin/perl -w

# functions to create a menu + jump-table for a set of gtypist-lessons
# plus some miscellaneous functions to convert to gtypist-lessons

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
# "gtypist <version>" is in the top right corner)
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

sub generate_menu($$*@)
{
    my $title = shift @_;
    my $n_lessons = shift @_;
    my $fh = shift @_;
    my @lesson_names = @_;
    my $i;

    print $fh "\n*:MENU\n";
    print $fh "M: \"$title\"\n";
    for ($i = 1; $i <= $n_lessons; $i++)
    {
	print $fh " :S_LESSON$i \"" . $lesson_names[$i]. "\"\n";
    }
}

END { }

1;

# Local Variables:
# compile-command: "./tt2typ.pl ~/src/tipptrainer-0.4/data/german/"
# End:
