#!/bin/csh -f
#
# What we have here is a csh script for sending netnews to NNTP sites.
#
set batchdir=/usr/spool/news/batch libdir=/usr/spool/news/lib
set path=( $libdir /usr/ucb /usr/bin /bin $path )
set pname=$0
set pname=$pname:t
echo ${pname}: "[$$]" begin `date`
#
# Go to where the action is
#
cd $batchdir
umask 022
#
#	For NNTP
#
#	Here "foo", "bar", and "zot" are the Internet names of
#	the machines to which to send.  We make the supposition
#	that the batch files will be a host's internet name.
#	So, for example "nike"'s internet name is "ames-titan.arpa".
#	Because of this, your sys file must have "ames-titan.arpa"
#	as the batch file output for the machine "nike".
#
foreach host ( {cad,zen,jade,cartan}.berkeley.edu decvax.dec.com cgl.ucsf.edu ucdavis.edu 128.54.0.1 )
	set lock=NNTP_LOCK.${host} tmp=${host}.tmp send=${host}.nntp
	shlock -p $$ -f ${lock}
	if ($status == 0) then
		if ( -e ${tmp} ) then
			cat ${tmp} >> ${send}
			rm ${tmp}
		endif
		if ( -e ${host} ) then
# if there's already other work to do, let the tmp file cool off.
# we'll pick it up again during the next iteration to make sure that
# we don't miss anything that inews is adding to it now.
			if ( -e ${send} ) then
				mv ${host} ${tmp}
			else
				mv ${host} ${send}
			endif
		endif
		if ( -e ${send} ) then
			echo ${pname}: "[$$]" begin ${host}
			time nntpxmit ${host}:${send}
			echo ${pname}: "[$$]" end ${host}
		endif
		rm -f ${lock}
	else
		echo ${pname}: "[$$]" ${host} locked by "[`cat ${lock}`]"
	endif
end
echo ${pname}: "[$$]" end `date`
