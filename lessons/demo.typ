# Typist v2.2 - improved typing tutor program for UNIX systems
# Copyright (C) 1998  Simon Baldwin (simonb@sco.com)
#
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#

#
# Demonstration of commands and features
#

*:DEMO_0
B:Demonstration of commands and features - B
T:This file demonstrates the commands that the program can do.
 :
 :The B command clears the screen, and if there is text following the
 :command that text is placed in the top 'banner' line of the screen.
 :No matter what else occurs, it stays there until replaced by text
 :from another B command.
 :
 :This demonstration used
 :
 :	B:Demonstration of commands and features - B
 :
 :to clear the screen.  The remainder of this file uses B commands to
 :indicate what it is demonstrating to you.


*:DEMO_1
B:Demonstration of commands and features - T
T:The simplest command is the T command.  This just outputs the text on
 :the line onto the screen.  As many lines as required may be displayed,
 :up to the limit of screen length.  After the display is done, the program
 :waits before proceeding:
 :
 :For example, the next screen shows the effect of
 :
 :	T:This is line one of a T command...
 :	 :...and this is line 2
T:This is line one of a T command...
 :...and this is line 2


*:DEMO_2
B:Demonstration of commands and features - D/O
T:The D command displays its text on alternate screen lines, and prompts
 :you to type the same text in on the intermediate lines.  Typing errors
 :are indicated with an inverse '^', or '>' if the character is a new
 :line.  The drill completes when it is typed correctly, or after a
 :number of attempts.  Delete and backspace are not recognised.  Escape
 :may be used to exit from the drill before completion.  The O command
 :does the same thing, but does not repeat until typed correctly.
 :
 :Here is an example drill, run on the next screen:
 :
 :	D:type these characters
 :	 :then type these
 :	 :press Escape to bypass this drill!
D:type these characters
 :then type these
 :press Escape to bypass this drill!


*:DEMO_3
B:Demonstration of commands and features - P
T:The P command displays its text on the screen, and prompts you to type
 :the text over the top of it.  Typing errors are highlighted in inverse
 :colours.  You get only one chance at this test, but delete and back-
 :space are recognised (errors still accumulate, however).  At the end
 :of the test, the typing speed and accuracy are displayed.
 :
 :Here is an example of a speed test.  Type this exactly
 :
 :	P:type this line in and dont escape
P:type this line in and dont escape
T:Here is another example.  Experiment with delete and backspace:
 :
 :	P:Overtype this paragraph with the same text.
 :	 :Note that capitals and punctuation are important.
 :	 :Experiment with delete and backspace keys.
 :	 :Use Escape to bypass the test!
P:Overtype this paragraph with the same text.
 :Note that capitals and punctuation are important.
 :Experiment with delete and backspace keys.
 :Use Escape to bypass the test!


*:DEMO_4
B:Demonstration of commands and features - I
T:The I command can display some brief instructions above a drill or
 :speed test.  Only two lines or less are available.  Unlike the T
 :command, I does not wait for any further keypresses before proceeding.
 :So it should really always be followed by either a D or a P.
 :I does clear all of the screen drill area, so in that way it's just
 :like a two-line T, though.
 :
 :Here's an example:
 :
 :	I:Here is a very short speed test.  You can either type in the
 :	 :whole thing, or just escape out of it:
 :	P:Very, very short test...
I:Here is a very short speed test.  You can either type in the
 :whole thing, or just escape out of it:
P:Very, very short test...


*:DEMO_5
B:Demonstration of commands and features - */G
T:The * places a label into the file.  The G can then be used to go to
 :that label.  The program really isn't fussy about label strings.  They
 :can be pretty much anything you like, and include spaces if that's what
 :you want.  Labels must be unique within files.
 :
 :For example:
 :
 :	G:MY_LABEL
 :	T:*** You won't see this, ever
 :	*:MY_LABEL
 :	T:We reached this message with a G command
G:MY_LABEL
T:*** You won't see this, ever
*:MY_LABEL
T:We reached this message with a G command


*:DEMO_6
B:Demonstration of commands and features - Q/Y/N/K
T:The Q command the prompt with its text on the message line, and waits for
 :a 'Y' or an 'N' before proceeding.  Other characters are ignored.
 :
 :The Y command will go to the label on its line if the result of the most
 :recent Q was 'Y'.  And similarly for the N command. K binds a function
 :key to a label.
 :
 :Here's an example.  As you can see, it can be clumsy, but mostly we
 :don't need anything as intricate:
 :
 :	Q: Press Y or N, and nothing else, to continue...
 :	Y:HIT_Y
 :	N:HIT_N
 :	T:*** You won't see this, ever
 :	*:HIT_Y
 :	T:You pressed Y
 :	G:JUMP_OVER
 :	*:HIT_N
 :	T:You pressed N
 :	*:JUMP_OVER
Q: Press Y or N, and nothing else, to continue...
Y:HIT_Y
N:HIT_N
T:*** You won't see this, ever
*:HIT_Y
T:You pressed Y
G:JUMP_OVER
*:HIT_N
T:You pressed N
*:JUMP_OVER


*:DEMO_7
B:Demonstration of commands and features - X
T:The last command to show is the X command.  This causes the program to
 :exit.  The program also exits if the end of the file is found (so if
 :you wanted to, you could always place a label there and just G to it).
 :
 :Here's a demonstration of the X command.  Since this is the end of
 :the demonstration, here is a good place to use it; the demonstration
 :will halt here.
 :
 :	X:
X:
