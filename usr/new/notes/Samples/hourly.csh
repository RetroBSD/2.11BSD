#! /bin/csh -f
# this is only here because we need to do an su on the cron command
# line and that makes it tough to pass an argument
#
exec /usr/spool/notes/.utilities/cron.csh hourly
exit 0
