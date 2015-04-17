# splfix.profil.sed - replace "jsr[ \t]pc,_splN" calls with 'movb' and 'spl'
#       instructions.  The sed 't' command is used liberally to short circuit
#       the script - if a match is made there's no sense continuing on because
#       there can never be more than a single 'spl' call per line.
#
# Do the easy ones - these do not save the previous spl.

s/jsr[	 ]*pc,__spl\([0-6]\)/spl \1/g
t
s/jsr[	 ]*pc,__spl7/spl 6/g
t
s/jsr[	 ]*pc,__splsoftclock/spl 1/g
t
s/jsr[	 ]*pc,__splnet/spl 2/g
t
s/jsr[	 ]*pc,__splbio/spl 5/g
t
s/jsr[	 ]*pc,__splimp/spl 5/g
t
s/jsr[	 ]*pc,__spltty/spl 5/g
t
s/jsr[	 ]*pc,__splclock/spl 6/g
t
s/jsr[	 ]*pc,__splhigh/spl 6/g
t

# Now the harder ones.  It is *very* tempting to read ahead a line and optimize
# the "movb PS,r0; spl $N; movb r0,foo" into "mfps foo; spl $N".  Alas, 
# this would break code which relied on "splN()" being a function which 
# returned a value in r0.

s/jsr[	 ]*pc,_spl\([0-6]\)/movb 177776,r0; spl \1/g
t
s/jsr[	 ]*pc,_spl7/movb 177776,r0; spl 6/g
t
s/jsr[	 ]*pc,_splsoftclock/movb 177776,r0; spl 1/g
t
s/jsr[	 ]*pc,_splnet/movb 177776,r0; spl 2/g
t
s/jsr[	 ]*pc,_splbio/movb 177776,r0; spl 5/g
t
s/jsr[	 ]*pc,_splimp/movb 177776,r0; spl 5/g
t
s/jsr[	 ]*pc,_spltty/movb 177776,r0; spl 5/g
t
s/jsr[	 ]*pc,_splclock/movb 177776,r0; spl 6/g
t
s/jsr[	 ]*pc,_splhigh/movb 177776,r0; spl 6/g
t
