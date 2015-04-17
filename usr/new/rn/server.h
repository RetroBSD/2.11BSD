#ifdef SERVER

EXT	char	*getserverbyfile();
EXT	int	server_init();
EXT	void	put_server();
EXT	int	get_server();
EXT	void	close_server();

#include "../nntp/common/response_codes.h"
#endif
