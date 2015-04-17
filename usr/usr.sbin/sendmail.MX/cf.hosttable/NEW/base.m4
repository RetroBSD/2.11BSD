############################################################
#
#	General configuration information
#
#	This information is basically just "boiler-plate"; it must be
#	there, but is essentially constant.
#
#	Information in this file should be independent of location --
#	i.e., although there are some policy decisions made, they are
#	not specific to Berkeley per se.
#
#		%W%	%Y%	%G%
#
############################################################

include(version.m4)

##########################
###   Special macros   ###
##########################

# my name
DnMAILER-DAEMON
# UNIX header format
DlFrom $g  $d
# delimiter (operator) characters
Do.:%@!^=/[]
# format of a total name
Dq$g$?x ($x)$.
# SMTP login message
De$j Sendmail $v/$V ready at $b

# forwarding host -- redefine this if you can't talk to the relay directly
DF$R

###################
###   Options   ###
###################

# location of alias file
OA/etc/aliases
# default delivery mode (deliver in background)
Odbackground
# (don't) connect to "expensive" mailers
#Oc
# temporary file mode
OF0644
# default GID
Og1
# location of help file
OH/usr/share/misc/sendmail.hf
# log level
OL9
# default messages to old style
Oo
# queue directory
OQ/usr/spool/mqueue
# read timeout -- violates protocols
Or2h
# status file
OS/var/log/sendmail.st
# queue up everything before starting transmission
Os
# default timeout interval
OT3d
# time zone names (V6 only)
OtPST,PDT
# default UID
Ou1
# wizard's password
OWa/FjIfuGKXyc2

###############################
###   Message precedences   ###
###############################

Pfirst-class=0
Pspecial-delivery=100
Pjunk=-100

#########################
###   Trusted users   ###
#########################

Troot
Tdaemon
Tuucp
Teric
Tnetwork

#############################
###   Format of headers   ###
#############################

H?P?Return-Path: <$g>
HReceived: $?sfrom $s $.by $j ($v/$V)
	id $i; $b
H?D?Resent-Date: $a
H?D?Date: $a
H?F?Resent-From: $q
H?F?From: $q
H?x?Full-Name: $x
HSubject:
# HPosted-Date: $a
# H?l?Received-Date: $b
H?M?Resent-Message-Id: <$t.$i@$j>
H?M?Message-Id: <$t.$i@$j>

###########################
###   Rewriting rules   ###
###########################


################################
#  Sender Field Pre-rewriting  #
################################
S1
#R$*<$*>$*		$1$2$3				defocus

###################################
#  Recipient Field Pre-rewriting  #
###################################
S2
#R$*<$*>$*		$1$2$3				defocus

#################################
#  Final Output Post-rewriting  #
#################################
S4

R@			$@				handle <> error addr

# externalize local domain info
R$*<$*LOCAL>$*		$1<$2$D>$3			change local info
R$*<$*LOCAL.ARPA>$*	$1<$2$D>$3			change local info
R$*<$+>$*		$1$2$3				defocus
R@$+:@$+:$+		@$1,@$2:$3			<route-addr> canonical

# UUCP must always be presented in old form
R$+@$-.UUCP		$2!$1				u@h.UUCP => h!u

# delete duplicate local names -- mostly for arpaproto.mc
R$+%$=w@$=w		$1@$3				u%UCB@UCB => u@UCB
R$+%$=w@$=w.ARPA	$1@$3.ARPA			u%UCB@UCB => u@UCB

###########################
#  Name Canonicalization  #
###########################
S3

# handle "from:<>" special case
R<>			$@@				turn into magic token

# basic textual canonicalization -- note RFC733 heuristic here
R$*<$*<$*<$+>$*>$*>$*	$4				3-level <> nesting
R$*<$*<$+>$*>$*		$3				2-level <> nesting
R$*<$+>$*		$2				basic RFC821/822 parsing
R$+ at $+		$1@$2				"at" -> "@" for RFC 822
R$*<$*>$*		$1$2$3				in case recursive

# make sure <@a,@b,@c:user@d> syntax is easy to parse -- undone later
R@$+,$+			@$1:$2				change all "," to ":"

# localize and dispose of domain-based addresses
R@$-:$+			$@$>6<@$1.$D>:$2		local <route-addr>
R@$+:$+			$@$>6<@$1>:$2			domain <route-addr>

# more miscellaneous cleanup
R$+			$:$>8$1				host dependent cleanup
R$+:$*;@$+		$@$1:$2;@$3			list syntax
R$+@$+			$:$1<@$2>			focus on domain
R$+<$+@$+>		$1$2<@$3>			move gaze right
R$+<@$->		$1<@$2.$D>			tack on local domain
R$+<@$+>		$@$>6$1<@$2>			already canonical

# convert old-style addresses to a domain-based address
R$+%$-			$@$>6$1<@$2.$D>			user%host
R$+%$+			$@$>6$1<@$2>			user%host.domain
R$-:$+			$@$>6$2<@$1.$D>			host:user
R$-.$+			$@$>6$2<@$1.$D>			host.user
R$+^$+			$1!$2				convert ^ to !
R$-!$+			$@$>6$2<@$1.UUCP>		resolve uucp names
R$-=$+			$@$>6$2<@$1.BITNET>		resolve bitnet names
