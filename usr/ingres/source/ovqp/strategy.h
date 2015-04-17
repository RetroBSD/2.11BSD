# define	opGTGE	opGT
# define	opLTLE	opLT
struct simp
{
	int		relop;	/* value of relop in simp clause*/
	int		att;	/* attno of attribute */
	struct symbol *const;	/* pointer to constant value symbol */
}	Simp[NSIMP];

int	Nsimp;		/* Current no. entries in Simp vector */
int	Fmode;		/* find-mode determined by strategy */

struct key
{
	struct symbol	*keysym;
	int		dnumber;
};

char	Keyl[MAXTUP];
char	Keyh[MAXTUP];

struct key	Lkey_struct[MAXDOM+1];
struct key	Hkey_struct[MAXDOM+1];
