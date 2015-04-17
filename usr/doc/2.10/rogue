.RP
.ds RH A Guide to the Dungeons of Doom
.ds CH
.ds CF - % -
.TL
A Guide to the Dungeons of Doom
.AU
Michael C. Toy
.AI
Computer Systems Research Group
Department of Electrical Engineering and Computer Science
University of California
Berkeley, California  94720
.AB
Rogue is a visual CRT based fantasy game which runs
under the UNIX timesharing system.  This paper describes
how to play rogue and gives a few hints for those who might
otherwise get lost in the Dungeons of Doom.
.AE
.NH
Introduction
.PP
You have just finished your years as a student at the local fighter's guild.
After much practice and sweat you have finally completed your training and
are ready to embark upon a perilous adventure.  As a test of your skills,
the local guildmasters have sent you into the Dungeons of Doom.  Your
task is to return with the Amulet of Yendor.  Your reward for the completion
of this task will be a full membership in the local guild. In addition, you are
allowed to keep all the loot you bring back from the dungeons.
.PP
In preparation for your journey, you are given an enchanted sword, taken
from a dragon's hoard in the far off Dark Mountains.  You are also outfitted
with elf-crafted armor and given enough food to reach the dungeons.  You
say goodbye to family and friends for what may be the last time and head
up the road.
.PP
You set out on your way to the dungeons and after several days of uneventful
travel, you see the ancient ruins that mark the entrance to the Dungeons
of Doom.  It is late at night so you make camp at the entrance and spend
the night sleeping under the open skies.  In the morning you gather
your sword, put on your armor, eat what is almost your last food and enter
the dungeons.
.NH
What is going on here?
.PP
You have just begun a game of rogue.  Your goal is to grab as much treasure
as you can, find the Amulet of Yendor, and get out of the Dungeons of Doom
alive.  On the screen, a map of where you have been and what you have seen on
the current dungeon level is kept.  As you explore more of the level,
it appears on the screen in front of you.
.PP
Rogue differs from most computer fantasy games in that it is screen
oriented.  Commands are all one or two keystrokes\(dg
.FS
\(dgAs opposed to pseudo English sentences.
.FE
and the results of your commands are displayed
graphically on the screen rather than being explained in words.
.PP
Another major difference between rogue and other computer fantasy games
is that once you have solved all the puzzles in a standard fantasy game,
it has lost most of its excitement and it ceases to be fun.  Rogue on the
other hand generates a new dungeon every time you play it and
even the author finds it an entertaining and exciting game.
.NH
What do all those things on the screen mean?
.PP
In order to understand what is going on in rogue you have to first get
some grasp of what rogue is doing with the screen.
The rogue screen is intended to replace the "You can see ..." descriptions
of standard fantasy games.  Here is a sample of what a rogue screen might
look like.
.in +1i
.nf
.cs R 15
                  ---------------------
                  |...................+
                  |...@...........[...|
                  |........B..........|
                  |...................|
                  --------+------------


.cs R
Level: 1  Gold: 0      Hp: 12(12)  Str: 16  Ac: 6  Exp: 1/0
.fi
.in 0
.NH 2
The bottom line
.PP
At the bottom line of the screen is a few pieces of cryptic information,
describing your current status.  Here is an explanation of what these
things mean:
.IP Level 8
This number indicates how deep you have gone in the dungeon.  It starts
at one and goes up forever\(dg.
.FS
\(dgOr until you get killed or decide to quit.
.FE
.IP Gold
The number of gold pieces you have managed to find and keep with
you so far.
.IP Hp
Your current and maximum hit points.  Hit points indicate how much
damage you can take before you die.  The more you get hit in a
fight, the lower they
get.  You can regain hit points by resting. The number in parentheses is
the maximum number your hit points can reach.
.IP Str
Your current strength.  This can be any integer less than or
equal to eighteen.  The higher the number, the stronger you are.
.IP Ac
Your current armor class.  This number indicates how effective
your armor is in stopping blows from unfriendly creatures.  The lower
this number is, the more effective the armor.
.IP Exp
These two numbers give your current experience level and experience points.
As you do things, you gain experience points.  At certain experience point
totals, you gain an experience level.  The more experienced you are, the
better you are able to fight and to withstand magical attacks.
.NH 2
The top line
.PP
The top line of the screen is reserved for printing messages that describe
things that are impossible to represent visually.  If you see a
"--More--" on the top line, this means that rogue wants to print another
message on the screen, but it wants to make certain that you have read
the one that is there first.  To read the next message, just press a
space.
.NH 2
The rest of the screen
.PP
The rest of the screen is the map of the level as you have explored it so far.
Each symbol on the screen represents something.  Here is a list of what
the various symbols mean:
.IP @
This symbol represents you, the adventurer.
.IP "-|" 6
These symbols represent the walls of rooms.
.IP +
A door to/from a room.
.IP .
The floor of a room.
.IP #
The floor of a passage between rooms.
.IP *
A pile or pot of gold.
.IP )
A weapon of some sort.
.IP ]
A piece of armor.
.IP !
A flask containing a magic potion.
.IP ?
A piece of paper, usually a magic scroll.
.IP ^
A trap, watch out for these.
.IP %
The passage leading down to the next level.
.IP :
A piece of food.
.IP A-Z
The uppercase letters represent the various inhabitants of the
Dungeons of Doom.  Watch out, they can be mean.
.NH
Commands
.PP
Commands are given to rogue by pressing single letters.  Most commands can
be preceded by a count to repeat them (e.g. typing "10s" will do ten searches)
The list of commands is rather long, but it can be read at any time during
the game with the ? command.  Here it is for reference, with a short
explanation of each command.
.IP ? 6
The help command.  Asks for a character to give help on.  If you type
a "*", it will list all the commands, otherwise it will explain what the
character you typed does.
.IP /
This is the "What is that on the screen?" command. A "/" followed by any
character that you see on the level, will tell you what that character
is.  For instance, typing "/@" will tell you that the @ symbol represents
you, the player.
.IP "h , H"
Move left.  You move one space to the left.  If you use upper case
h, you will continue to move left until you run into something.  This
works for all movement commands (e.g. "L" means run in direction "l")
.IP j
Move down.
.IP k
Move up.
.IP l
Move right.
.IP y
Move diagonally up and left.
.IP u
Move diagonally up and right.
.IP b
Move diagonally down and left.
.IP n
Move diagonally down and right.
.IP f
Find prefix.  When followed by a direction it means to continue moving
in the specified direction until you pass something interesting or
run into a wall.
.IP t
Throw an object.  This is a prefix command.  Follow it with a direction and
you throw an object in the specified direction.  (e.g. type "th" to throw
something left.)
.IP >
If you are standing over the passage down to the next level, this command
means to climb down.
.IP s
Search for traps and secret doors.  Examine each space immediately adjacent
to you for the existence of a trap or secret door.  There is a large chance
that even if there is something there, you won't find it so you might
have to search a while before you find something.
.IP " "
(space) Rest.  This is the "do nothing" command.
This is good for waiting and healing.
.IP i
Inventory.  List what you are carrying in your pack.
.IP I
Selective inventory.  Tells you what a single item in your pack is.
.IP q
Quaff.  Drink one of the potions you are carrying.
.IP r
Read.  Read one of the scrolls in your pack.
.IP e
Eat food.  Take some food out of your pack and eat it.
.IP w
Wield a weapon.  Take a weapon out of your pack and carry it.  You must be
wielding  weapon to use it (except to throw things).  To fire an arrow,
you must wield the bow.  You can only wield one weapon at a time.
.IP W
Wear armor.  Take a piece of armor out of your pack and put it on.  You can
only wear one suit of armor at a time.
.IP T
Take armor off.  You can't remove armor that is cursed.
This takes extra time.
.IP d
Drop an object.  Take something out of your pack and leave it lying
on the floor.  Only one object can occupy each space.
.IP o
Examine and set options.  This command is further explained in the section
on options.
.IP ^L
REdraws the screen. Useful if spurious messages or transmission errors
have messed up the display.
.IP v
Prints the program version number.
.IP Q
Quit.  Leave the game.
.IP R
Repeat last message.  Useful when a message disappears before you can
read it.
.IP S
Save the current game in a file.  Caveat: Rogue won't let you start
up a copy of a saved game, and it removes the save file as soon as
you start up a restored game.  This is to prevent people from saving
a game just before a dangerous position and then restarting it
if they die.  To restore a saved game, give the file name as an argument
to rogue. As in
.ti +1i
.nf
% rogue save_file
.NH
Dealing with objects
.PP
When you find something in the dungeon, it is common to want to pick the
object up.  This is accomplished in rogue by walking over the object.  If
you are carrying too many things, the program will tell you and it won't pick
up the object, otherwise it will add it to your pack and if the notify
option is set, tell you what you just picked up.
.PP
Many of the commands that operate on objects must prompt you to find
out which object you want to use.  If you change your mind and don't want to
do that command after all, just press an escape and the command will be
aborted.
.NH
Light
.PP
Rooms in the dungeons are either lit or dark.  If you walk into a lit room,
the entire room will be drawn on the screen as soon as you enter.  If you
walk into a dark room, it will only be displayed as you explore it.  Upon
leaving a dark room, all objects inside the room which might move are
removed from the screen.  In the darkness you can only see one space
in all directions around you.
.NH
Fighting
.PP
If you see a monster and you wish to fight it, just attempt to run into it.
Many times a monster you find will mind its own business unless you attack
it.  It is often the case that discretion is the better part of valor.
.NH
Armor
.PP
There are various sorts of armor lying around in the dungeon.  Some of it
is enchanted, some is cursed and some is just normal.  Different armor
types have different armor classes.  The lower the armor class, the
more protection the armor affords against the blows of monsters.
If a piece of armor is enchanted or
cursed, its armor class will be higher or lower than normal.  Here is
a list of the various armor types and their normal armor class.
.TS
center box;
c c
l | c.
Type	Class
=
Leather armor	8
Studded leather / Ring mail	7
Scale mail	6
Chain mail	5
Banded mail / Splint mail	4
Plate mail	3
.TE
.NH
Options
.PP
Due to variations in personal tastes and conceptions of the way rogue
should do things, there are a set of options you can set that cause
rogue to behave in various different ways.
.NH 2
Setting the options
.PP
There are basically two ways to set the options.  The first is with the
"o" command of rogue, the second is with the ROGUEOPTS environment
variable.  On Version 6 systems, there is no equivalent of
the ROGUEOPTS feature.
.NH 3
Using the "o" command
.PP
When you press "o" in rogue, it clears the screen and displays the current
settings for all the options.  It then places the cursor by the value of the
first option and waits for you to type.  You can type a RETURN which means to
go to the next option, a "\-" which means to go to the previous option, an
escape which means to return to the game, or you can give the option a
value.  For boolean options this merely involves pressing "t" for true or
"f" for false.  For string options, type the new value followed by a
return.
.NH 3
Using the ROGUEOPTS variable
.PP
The ROGUEOPTS variable is a string containing a comma separated list of
initial values for the various options.  Boolean variables can be turned
on by listing their name and turned off by putting a "no" in front of the
name.  Thus to set up an environment variable so that jump is on, terse is
off, the name is set to "Conan the Barbarian" and the fruit is "mango",
use the command
.nf
.nf
.ti +3
% setenv ROGUEOPTS "jump,noterse,name=Conan the Barbarian,fruit=mango" \(dg
.fi
.ti +3
% setenv ROGUEOPTS "jump,noterse,name=Conan the Barbarian,fruit=mango" \(dg
.fi
.FS
\(dgFor those of you who use the bourne shell, the commands would be
.in +3
.nf
$ ROGUEOPTS="jump,noterse,name=Conan the Barbarian,fruit=mango"
$ export ROGUEOPTS
.fi
.in +0
.FE
.NH 2
Option list
.PP
Here is a list of the options and an explanation of what each one is for.
The default value for each is enclosed in square brackets.
.IP "terse [noterse]" 25
Useful for those who are tired of the sometimes lengthy messages of rogue.
This is a useful option for those on slow terminals.  This option defaults to
on if your are on a slow (under 1200 baud) terminal.
.IP "jump [nojump]"
If this option is set, running moves will not be displayed until you
reach the end of the move.  This saves considerable cpu time and
display time.  This option defaults to on if you are using a slow terminal.
.IP "step [nostep]
When step is set, lists of things, like inventories or "*" responses to
"Which item do you wish to xxxx? " questions, are displayed one item
at a time on the top of the screen, rather than clearing the screen,
displaying the list, then re-displaying the dungeon level.
.IP "flush [noflush]"
If flush is set, all typeahead is thrown away after each round of battle.
This is useful for those who type way ahead and watch to their dismay as
a Kobold kills them.
.IP "askme [noaskme]"
Upon reading a scroll or quaffing a potion which does not automatically
identify it upon use, rogue will ask you what to name it so you can
recognize it in the future.
.IP "name [account name]"
This is the name of your character.  It is used if you get on the top ten
scorer's list.  It should be less than eighty characters long.
.IP "fruit [slime-mold]"
This should hold the name of a fruit that you enjoy eating.  It is basically
a whimsy that the program uses in a couple of places.
.IP "file [rogue.save]"
The default file name for saving the game.  If your phone is hung up by
accident, rogue will automatically save the game in this file.  The
file name may contain the special character "~" which expands to be
your home directory.
.NH
Acknowledgements
.PP
Rogue was originally conceived of by Glenn Wichman and Michael Toy.  The
help of Ken Arnold in making the program easier to use and putting the
finishing touches on is greatly appreciated.  I would also like to thank
Marty McNary, Scott Nelson, Daniel Jensen, Kipp Hickman, Joe Kalash,
Steve Maurer, Bill Joy, Mark Horton and Jan Miller for their ideas
and assistance.
