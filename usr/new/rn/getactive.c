#include <stdio.h>
#include "config.h"
#include "EXTERN.h"
#include "server.h"

main(argc, argv)
	int		argc;
	char	 	*argv[];
{
	char		ser_line[256];
	int		response;
	register char	*server;
	register FILE	*actfp;

	if (argc != 2) {
		fprintf(stderr, "Usage: getactive filename\n");
		exit(1);
	}

	server = getserverbyfile(SERVER_FILE);
	if (server == NULL) {
		fprintf(stderr, "Couldn't get name of news server from %s\n",
			SERVER_FILE);
		fprintf(stderr,
	  "Either fix this file, or put NNTPSERVER in your environment.\n");
		exit(1);
	}

	response = server_init(server);
	if (response < 0) {
		fprintf(stderr,
			"getactive: Can't get active file from server %s.\n",
				server);
		exit(1);
	}

	if (handle_server_response(response, server) < 0)
		exit(1);

	put_server("LIST");	/* tell server we want the active file */
	(void) get_server(ser_line, sizeof(ser_line));
	if (*ser_line != CHAR_OK) {		/* and then see if that's ok */
		fprintf(stderr,
			"getactive: Can't get active file from server.\n");
		fprintf(stderr, "Server said: %s\n", ser_line);
		exit(1);
	}

	actfp = fopen(argv[1], "w");		/* and get ready */
	if (actfp == NULL) {
		close_server();
		perror(argv[1]);
		exit(1);
	}

	while (get_server(ser_line, sizeof(ser_line)) >= 0) {  /* while */
		if (ser_line[0] == '.')		/* there's another line */
			break;			/* get it and write it to */
		if (actfp != NULL) {		/* the temporary active file */
			fputs(ser_line, actfp);
			putc('\n', actfp);
		}
	}

	(void) fclose(actfp);
	close_server();
}
