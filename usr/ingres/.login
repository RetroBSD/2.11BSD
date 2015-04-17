#tset -Q -e^\? -s -r -n -d \?vt100 > /tmp/tset.$$
#source /tmp/tset.$$
#rm /tmp/tset.$$
if ( "$TERM" == dialup || "$TERM" == network) then
  setenv TERM vt220
  echo -n "Terminal ($TERM)? "
  set argv = "$<"
  if ( "$argv" != '' ) setenv TERM "$argv"
endif
stty dec new erase ^\? kill ^U eof ^D
set path = (~/bin /bin /usr/ucb /usr/bin /usr/local /usr/new)
alias ls '/bin/ls -FC'
alias l '/bin/ls -logF'
alias pwd 'echo ${cwd}'
echo "Not even you can prevent head crashes\!"
