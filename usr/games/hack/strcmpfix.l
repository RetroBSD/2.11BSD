WS	[\t\n ]*
STR	\"([^"]|\\\")*\"
EXP	[^,"]+
INT	[0-9]+
%%
strncmp{WS}\({WS}{EXP}{WS}\,{WS}{STR}{WS}\,{WS}{INT}{WS}\) {

		/*
			convert `strncmp(<exp>, <str>, <int_const>)'
			to      `strncmp(<exp>, <str>, <new_const>)'
			where
				<new_const> = strlen(<str>) < <int_const>
					? strlen(<str>)
					: <int_const>
		*/

		char  *sstart, *send, *istart, *iend, index ();
		int   slen, ival;

		sstart = index (yytext,   '"');
		send   = index (sstart+1, '"');
		slen   = send-sstart-1;
		for  (istart = index (send, ',') + 1;
		      *istart < '0'  ||  '9' < *istart;
		      istart++)
		  continue;
		for  (iend = istart;
		      '0' <= *iend  &&  *iend <= '9';
		      iend++)
		  continue;
		ival = atoi (istart);

		printf ("%.*s%d%s", istart-yytext, yytext,
		                      slen < ival ? slen : ival,
		                      iend);
	}
strcmp{WS}\(p-{INT}{WS}\,{WS}{STR}{WS}\) {

		/*
			convert `strcmp(p-<int_const>, <str>)'
			to      `strcmp(p-<new_const>, <str>)'
			where
				<new_const> = strlen(<str>)
		*/

		char  *sstart, *send, *istart, *iend, index ();
		int   slen, ival;

		sstart = index (yytext,   '"');
		send   = index (sstart+1, '"');
		slen   = send-sstart-1;
		istart = index (yytext, '-') + 1;
		for  (iend = istart;
		      '0' <= *iend  &&  *iend <= '9';
		      iend++)
		  continue;
		ival = atoi (istart);

		printf ("%.*s%d%s", istart-yytext, yytext, slen, iend);
	}
