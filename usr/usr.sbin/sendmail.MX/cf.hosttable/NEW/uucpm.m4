############################################################
############################################################
#####
#####		UUCP Mailer specification
#####
#####		%W%	%Y%	%G%
#####
############################################################
############################################################

ifdef(`m4COMPAT',, `include(compat.m4)')

Muucp,	P=/usr/bin/uux, F=sDFMhuU, S=13, R=23, M=100000,
	A=uux - -r $h!rmail ($u)

S13
R$+@$+.UUCP		$2!$1				convert to old style
R$=w!$+			$2				strip local name
R$+			$:$U!$1				stick on our host name

S23
R$+@$+.UUCP		$2!$1				convert to old style
