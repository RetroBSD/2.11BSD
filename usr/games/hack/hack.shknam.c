/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.shknam.c - version 1.0.2 */

#include "hack.h"

char *shkliquors[] = {
#ifdef	VERSION7
	"Nj", "Ts", "Go", "Os", "Gr", "Ko", "We", "Sy", "Sa", "Na", "Ky",
	"Wa", "Sw", "Kl", "Ra", "Gl", "Bz", "Kr", "Hr", "Le", "Br", "Bi",
	"Th", "Sr", "Bu", "El", "Fl", "Va", "Sc", "Zu",
#else	VERSION7
	/* Ukraine */
	"Njezjin", "Tsjernigof", "Gomel", "Ossipewsk", "Gorlowka",
	/* N. Russia */
	"Konosja", "Weliki Oestjoeg", "Syktywkar", "Sablja",
	"Narodnaja", "Kyzyl",
	/* Silezie */
	"Walbrzych", "Swidnica", "Klodzko", "Raciborz", "Gliwice",
	"Brzeg", "Krnov", "Hradec Kralove",
	/* Schweiz */
	"Leuk", "Brig", "Brienz", "Thun", "Sarnen", "Burglen", "Elm",
	"Flims", "Vals", "Schuls", "Zum Loch",
#endif	VERSION7
	0
};

char *shkbooks[] = {
#ifdef	VERSION7
	"Sk", "Ka", "Rh", "En", "La", "Lo", "Cr", "Ma", "Ba", "Ki", "Lu",
	"Ei", "Gw", "Kt", "Ne", "Sn", "By", "Kv", "Ca", "Gb", "Km", "Kg",
	"Dr", "In", "Cl", "Li", "Cu", "Du", "Ib", "Ks",
#else	VERSION7
	/* Eire */
	"Skibbereen", "Kanturk", "Rath Luirc", "Ennistymon", "Lahinch",
	"Loughrea", "Croagh", "Maumakeogh", "Ballyjamesduff",
	"Kinnegad", "Lugnaquillia", "Enniscorthy", "Gweebarra",
	"Kittamagh", "Nenagh", "Sneem", "Ballingeary", "Kilgarvan",
	"Cahersiveen", "Glenbeigh", "Kilmihil", "Kiltamagh",
	"Droichead Atha", "Inniscrone", "Clonegal", "Lisnaskea",
	"Culdaff", "Dunfanaghy", "Inishbofin", "Kesh",
#endif	VERSION7
	0
};

char *shkarmors[] = {
#ifdef	VERSION7
	"De", "Kc", "Bo", "Yi", "Gz", "Sr", "Ak", "Ti", "Ar", "Er", "Ik",
	"Kd", "Sv", "Pe", "Ml", "Bt", "Ay", "Zo", "Bb", "Tf", "Av", "Ks",
	"Mk", "Mg", "Mi", "Bc", "Kk", "Al", "Po", "Nh",
#else	VERSION7
	/* Turquie */
	"Demirci", "Kalecik", "Boyabai", "Yildizeli", "Gaziantep",
	"Siirt", "Akhalataki", "Tirebolu", "Aksaray", "Ermenak",
	"Iskenderun", "Kadirli", "Siverek", "Pervari", "Malasgirt",
	"Bayburt", "Ayancik", "Zonguldak", "Balya", "Tefenni",
	"Artvin", "Kars", "Makharadze", "Malazgirt", "Midyat",
	"Birecik", "Kirikkale", "Alaca", "Polatli", "Nallihan",
#endif	VERSION7
	0
};

char *shkwands[] = {
#ifdef	VERSION7
	"Yr", "Tr", "Mw", "Pn", "Rd", "Ll", "Lf", "YF", "Me", "Ry", "Bd",
	"Cg", "Lw", "Ln", "Cb", "Nn", "Tu", "Iv", "Bm", "Lc", "Kh", "Bn",
	"Dn", "Mv", "Ui", "St", "Sg", "Ch", "Gh", "Kn", "Dv",
#else	VERSION7
	/* Wales */
	"Yr Wyddgrug", "Trallwng", "Mallwyd", "Pontarfynach",
	"Rhaeader", "Llandrindod", "Llanfair-ym-muallt",
	"Y-Fenni", "Measteg", "Rhydaman", "Beddgelert",
	"Curig", "Llanrwst", "Llanerchymedd", "Caergybi",
	/* Scotland */
	"Nairn", "Turriff", "Inverurie", "Braemar", "Lochnagar",
	"Kerloch", "Beinn a Ghlo", "Drumnadrochit", "Morven",
	"Uist", "Storr", "Sgurr na Ciche", "Cannich", "Gairloch",
	"Kyleakin", "Dunvegan",
#endif	VERSION7
	0
};

char *shkrings[] = {
#ifdef	VERSION7
	"Fe", "Fl", "Gl", "Ha", "Hy", "Hb", "Im", "Ju", "Kj", "Ms", "Mj",
	"Mc", "Ol", "Sd", "Ss", "Sn", "Ta", "Tw", "Wi", "Yp", "Rj", "Va",
	"Kx", "Ab", "Ek", "Rv", "Av", "Hp", "Ly", "Ge", "Oe", "Kb", "Fa",
#else	VERSION7
	/* Hollandse familienamen */
	"Feyfer", "Flugi", "Gheel", "Havic", "Haynin", "Hoboken",
	"Imbyze", "Juyn", "Kinsky", "Massis", "Matray", "Moy",
	"Olycan", "Sadelin", "Svaving", "Tapper", "Terwen", "Wirix",
	"Ypey",
	/* Skandinaviske navne */
	"Rastegaisa", "Varjag Njarga", "Kautekeino", "Abisko",
	"Enontekis", "Rovaniemi", "Avasaksa", "Haparanda",
	"Lulea", "Gellivare", "Oeloe", "Kajaani", "Fauske",
#endif	VERSION7
	0
};

char *shkfoods[] = {
#ifdef	VERSION7
	"Dj", "Tb", "Td", "Pn", "Bd", "Pr", "Bo", "Sq", "Nb", "Dm", "Au",
	"Bx", "Pp", "Bf", "Tl", "Se", "Bp", "Tz", "Kq", "Nz", "Pc", "Pm",
	"Pj", "Ku", "Pb", "Tc", "Mn", "Tp", "Sm", "Bs", "Tg", "Su",
#else	VERSION7
	/* Indonesia */
	"Djasinga", "Tjibarusa", "Tjiwidej", "Pengalengan",
	"Bandjar", "Parbalingga", "Bojolali", "Sarangan",
	"Ngebel", "Djombang", "Ardjawinangun", "Berbek",
	"Papar", "Baliga", "Tjisolok", "Siboga", "Banjoewangi",
	"Trenggalek", "Karangkobar", "Njalindoeng", "Pasawahan",
	"Pameunpeuk", "Patjitan", "Kediri", "Pemboeang", "Tringanoe",
	"Makin", "Tipor", "Semai", "Berhala", "Tegal", "Samoe",
#endif	VERSION7
	0
};

char *shkweapons[] = {
#ifdef	VERSION7
	"Vo", "Ro", "Lq", "Tv", "Gu", "Mq", "Nv", "Vz", "Pq", "Ur", "Cn",
	"Fc", "Lz", "Vr", "Qu", "Lr", "Ec", "Cz", "Ey", "Cc", "Mo", "Jo",
	"Ps", "Jm", "Fu", "Lo", "Sm", "Em", "Eg", "Ez", "Lh",
#else	VERSION7
	/* Perigord */
	"Voulgezac", "Rouffiac", "Lerignac", "Touverac", "Guizengeard",
	"Melac", "Neuvicq", "Vanzac", "Picq", "Urignac", "Corignac",
	"Fleac", "Lonzac", "Vergt", "Queyssac", "Liorac", "Echourgnac",
	"Cazelon", "Eypau", "Carignan", "Monbazillac", "Jonzac",
	"Pons", "Jumilhac", "Fenouilledes", "Laguiolet", "Saujon",
	"Eymoutiers", "Eygurande", "Eauze", "Labouheyre",
#endif	VERSION7
	0
};

char *shkgeneral[] = {
#ifdef	VERSION7
	"He", "Pf", "As", "Mb", "Aa", "Pk", "Kb", "Wt", "Ap", "Sc", "At",
	"Uk", "Ai", "Ab", "Uv", "Gs", "Lk", "Vn", "Yk", "Uh", "Ot", "Ug",
	"Tm", "Wp", "Ji", "Qc", "Xi", "Yr", "Xy", "Yb", "Hv",
#else	VERSION7
	/* Suriname */
	"Hebiwerie", "Possogroenoe", "Asidonhopo", "Manlobbi",
	"Adjama", "Pakka Pakka", "Kabalebo", "Wonotobo",
	"Akalapi", "Sipaliwini",
	/* Greenland */
	"Annootok", "Upernavik", "Angmagssalik",
	/* N. Canada */
	"Aklavik", "Inuvik", "Tuktoyaktuk",
	"Chicoutimi", "Ouiatchouane", "Chibougamau",
	"Matagami", "Kipawa", "Kinojevis",
	"Abitibi", "Maganasipi",
	/* Iceland */
	"Akureyri", "Kopasker", "Budereyri", "Akranes", "Bordeyri",
	"Holmavik",
#endif	VERSION7
	0
};

struct shk_nx {
	char x;
	char **xn;
} shk_nx[] = {
	{ POTION_SYM,	shkliquors },
	{ SCROLL_SYM,	shkbooks },
	{ ARMOR_SYM,	shkarmors },
	{ WAND_SYM,	shkwands },
	{ RING_SYM,	shkrings },
	{ FOOD_SYM,	shkfoods },
	{ WEAPON_SYM,	shkweapons },
	{ 0,		shkgeneral }
};

findname(nampt, let) char *nampt; char let; {
register struct shk_nx *p = shk_nx;
register char **q;
register int i;
	while(p->x && p->x != let) p++;
	q = p->xn;
	for(i=0; i<dlevel; i++) if(!q[i]){
		/* Not enough names, try general name */
		if(let) findname(nampt, 0);
		else (void) strcpy(nampt, "Dirk");
		return;
	}
	(void) strncpy(nampt, q[i], PL_NSIZ);
	nampt[PL_NSIZ-1] = 0;
}
