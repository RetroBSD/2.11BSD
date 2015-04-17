#!/usr/bin/perl
#
# $Source: /usr/users/louie/ntp/RCS/extract.pl,v $ $Revision: 3.4.1.1 $ $Date: 89/03/22 18:32:36 $
#
$HOST = '10.2.0.96';
if ($#ARGV != 0) {
	die "Must specify internet address of host.";
}
$HOST = $ARGV[1];
while(<stdin>) {
	if(/^host: $HOST/) {
		s/host: //;
		s/\(/ /g;
		s/\)/ /g;
		s/:/ /g;
		@A = split(' ');
		print $A[3],"\n";
	 }
}
