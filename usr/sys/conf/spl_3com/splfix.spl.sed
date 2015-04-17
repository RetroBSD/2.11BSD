# splfix.spl.sed - replace "jsr[ \t]pc,_spl*" calls with 'movb' and 'spl'
#       instructions.  The sed 't' command is used liberally to short circuit
#       the script - if a match is made there's no sense continuing on because
#       there can never be more than a single 'spl' call per line.
#
# NOTE: there are embedded tabs (\t) characters in many of the patterns below
#	be *very* careful when editing or cutting&pasting that the tabs do not
#	get converted to spaces!
#
# Do the easy ones - these do not save the previous spl.

s/jsr[	 ]*pc,__spl\([0-7]\)/spl \1/g
t
s/jsr[	 ]*pc,__splsoftclock/spl 1/g
t
s/jsr[	 ]*pc,__splnet/spl 2/g
t
s/jsr[	 ]*pc,__splbio/spl 5/g
t
s/jsr[	 ]*pc,__splimp/spl 6/g
t
s/jsr[	 ]*pc,__spltty/spl 5/g
t
s/jsr[	 ]*pc,__splclock/spl 6/g
t
s/jsr[	 ]*pc,__splhigh/spl 7/g
t

# Now the harder ones.  It is *very* tempting to read ahead a line and optimize
# the "movb PS,r0; spl $N; movb r0,foo" into "movb PS,foo; spl $N".  Alas, 
# this would break code which relied on "splN()" being a function which 
# returned a value in r0.

s/jsr[	 ]*pc,_spl\([0-7]\)/movb 177776,r0; spl \1/g
t
s/jsr[	 ]*pc,_splsoftclock/movb 177776,r0; spl 1/g
t
s/jsr[	 ]*pc,_splnet/movb 177776,r0; spl 2/g
t
s/jsr[	 ]*pc,_splbio/movb 177776,r0; spl 5/g
t
s/jsr[	 ]*pc,_splimp/movb 177776,r0; spl 6/g
t
s/jsr[	 ]*pc,_spltty/movb 177776,r0; spl 5/g
t
s/jsr[	 ]*pc,_splclock/movb 177776,r0; spl 6/g
t
s/jsr[	 ]*pc,_splhigh/movb 177776,r0; spl 7/g
t
