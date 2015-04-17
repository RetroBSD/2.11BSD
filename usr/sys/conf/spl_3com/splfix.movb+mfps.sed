# splfix.movb+mfps.sed - replace "jsr[ \t]pc,_spl*" calls with 'movb' and 'mfps'
#	instructions.  The sed 't' command is used liberally to short circuit
#	the script - if a match is made there's no sense continuing on because
#	there can never be more than a single 'spl' call per line.
#
# NOTE: there are embedded tabs (\t) characters in many of the patterns below
#	be *very* careful when editing or cutting&pasting that the tabs do not
#	get converted to spaces!
#
# Do the easy ones - these do not save the previous spl.

s/jsr[	 ]*pc,__spl0/clrb 177776/
t
s/jsr[	 ]*pc,__spl\([1-7]\)/movb $40*\1,177776/
t
s/jsr[	 ]*pc,__splsoftclock/movb $40,177776/
t
s/jsr[	 ]*pc,__splnet/movb $100,177776/
t
s/jsr[	 ]*pc,__splimp/movb $300,177776/
t
s/jsr[	 ]*pc,__splbio/movb $240,177776/
t
s/jsr[	 ]*pc,__spltty/movb $240,177776/
t
s/jsr[	 ]*pc,__splclock/movb $300,177776/
t
s/jsr[	 ]*pc,__splhigh/movb $340,177776/
t

# Now the harder ones.  It is *very* tempting to read ahead a line and optimize
# the "mfps r0; movb $N,PS; movb r0,foo" into "mfps foo; movb $N,PS".  Alas, 
# this would break code which relied on "splN()" being a function which 
# returned a value in r0.

s/jsr[	 ]*pc,_spl0/mfps r0; clrb 177776/g
t
s/jsr[	 ]*pc,_spl\([1-7]\)/mfps r0; movb $40*\1,177776/g
t
s/jsr[	 ]*pc,_splsoftclock/mfps r0; movb $40,177776/g
t
s/jsr[	 ]*pc,_splnet/mfps r0; movb $100,177776/g
t
s/jsr[	 ]*pc,_splbio/mfps r0; movb $240,177776/g
t
s/jsr[	 ]*pc,_splimp/mfps r0; movb $300,177776/g
t
s/jsr[	 ]*pc,_spltty/mfps r0; movb $240,177776/g
t
s/jsr[	 ]*pc,_splclock/mfps r0; movb $300,177776/g
t
s/jsr[	 ]*pc,_splhigh/mfps r0; movb $340,177776/g
t
