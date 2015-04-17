# splfix.mfps.sed - replace "jsr[ \t]pc,_splN" calls with mfps and spl 
#	instructions.  The sed 't' command is used liberally to short circuit
#	the script - if a match is made there's no sense continuing on because
#	there can never be more than a single 'spl' call per line.
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
s/jsr[	 ]*pc,__splimp/spl 5/g
t
s/jsr[	 ]*pc,__spltty/spl 5/g
t
s/jsr[	 ]*pc,__splclock/spl 6/g
t
s/jsr[	 ]*pc,__splhigh/spl 7/g
t

# Now the harder ones.  It is *very* tempting to read ahead a line and optimize
# the "mfps r0; spl N; movb r0,foo" into "mfps foo; splN".  Alas, this would
# break code which relied on "splN()" being a function which returned a value
# in r0.

s/jsr[	 ]*pc,_spl\([0-7]\)/mfps r0; spl \1/g
t
s/jsr[	 ]*pc,_splsoftclock/mfps r0; spl 1/g
t
s/jsr[	 ]*pc,_splnet/mfps r0; spl 2/g
t
s/jsr[	 ]*pc,_splbio/mfps r0; spl 5/g
t
s/jsr[	 ]*pc,_splimp/mfps r0; spl 5/g
t
s/jsr[	 ]*pc,_spltty/mfps r0; spl 5/g
t
s/jsr[	 ]*pc,_splclock/mfps r0; spl 6/g
t
s/jsr[	 ]*pc,_splhigh/mfps r0; spl 7/g
t

#
# A couple special cases.  If the PS is being loaded from a variable then use
# the 'mtps' instruction.
#

s/movb[	 ]*\(.*[^,]\),\*\$-2/mtps \1/g
t
s/movb[	 ]*\(.*[^,]\),\*\$177776/mtps \1/g
t
