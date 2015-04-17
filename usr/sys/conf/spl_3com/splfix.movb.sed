# splfix.movb.sed - replace "jsr[ \t]pc,_spl*" calls with 'movb'
#       instructions.  The sed 't' command is used liberally to short circuit
#       the script - if a match is made there's no sense continuing on because
#       there can never be more than a single 'spl' call per line.
#
# NOTE: there are embedded tabs (\t) characters in many of the patterns below
#	be *very* careful when editing or cutting&pasting that the tabs do not
#	get converted to spaces!
#
# Do the easy ones - these do not save the previous spl.

s/jsr[	 ]*pc,__spl0/clrb 177776/g
t
s/jsr[	 ]*pc,__spl\([1-7]\)/movb $40*\1,177776/g
t
s/jsr[	 ]*pc,__splsoftclock/movb $40,177776/g
t
s/jsr[	 ]*pc,__splnet/movb $100,177776/g
t
s/jsr[	 ]*pc,__splimp/movb $300,177776/g
t
s/jsr[	 ]*pc,__splbio/movb $240,177776/g
t
s/jsr[	 ]*pc,__spltty/movb $240,177776/g
t
s/jsr[	 ]*pc,__splclock/movb $300,177776/g
t
s/jsr[	 ]*pc,__splhigh/movb $340,177776/g
t

# Now the harder ones.  It is *very* tempting to read ahead a line and optimize
# the "movb PS,r0; movb $N,PS; movb r0,foo" into "movb PS,foo; movb $N,PS".  
# Alas, this would break code which relied on "splN()" being a function which 
# returned a value in r0.

s/jsr[	 ]*pc,_spl0/movb 177776,r0; clrb 177776/g
t
s/jsr[	 ]*pc,_spl\([1-7]\)/movb 177776,r0; movb $40*\1,177776/g
t
s/jsr[	 ]*pc,_splsoftclock/movb 177776,r0; movb $40,177776/g
t
s/jsr[	 ]*pc,_splnet/movb 177776,r0; movb $100,177776/g
t
s/jsr[	 ]*pc,_splbio/movb 177776,r0; movb $240,177776/g
t
s/jsr[	 ]*pc,_splimp/movb 177776,r0; movb $300,177776/g
t
s/jsr[	 ]*pc,_spltty/movb 177776,r0; movb $240,177776/g
t
s/jsr[	 ]*pc,_splclock/movb 177776,r0; movb $300,177776/g
t
s/jsr[	 ]*pc,_splhigh/movb 177776,r0; movb $340,177776/g
t
