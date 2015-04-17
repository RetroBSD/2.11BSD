#!/bin/sh
nm -pog $* | fgrep -v ' U ' | sort +2 \
	| awk '{ \
		if ($3 == last3) { \
			if (first) \
				print last0; \
			print $0; \
			first = 0; \
		} else { \
			first = 1; \
			last0 = $0; \
		} \
		last3 = $3; \
	}'
