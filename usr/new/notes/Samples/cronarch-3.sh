echo -n "	NOTESFILE ARCHIVAL STARTED AT: " ; date
cd /usr/spool/notes
set nfutil	= /usr/spool/notes/.utilities
set AGE 	= 7
echo "Notesfile Locks at start:" ; /bin/ls -l /usr/spool/notes/.locks
du -s /usr/spool/notes/. /usr/spool/oldnotes/.
${nfutil}/shorten
echo "Logfiles after shorten: "
ls -l /usr/spool/notes/.utilities/net.log*
echo " ======"
echo " ====== Beginning Expiration of Notesfiles"
echo " ======"
foreach i (*)
	${nfutil}/nfarchive -d -${AGE} ${i}
end
echo " ===="
echo " ==== Cleanup and summary"
echo " ===="
du -s /usr/spool/notes/. /usr/spool/oldnotes/.
echo "Notesfile Locks at finish:" ; /bin/ls -l /usr/spool/notes/.locks
echo -n "	NOTESFILE ARCHIVAL COMPLETED AT: " ; date
