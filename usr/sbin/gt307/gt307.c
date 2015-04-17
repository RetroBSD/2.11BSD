/*
 *  GT307
 *
 * Read and set a Computer Products inc GT307 clock calendar card
 *
 * Author  R Birch
 * Date    July 1999
 *
 * You can use this in two ways.  If you use Daylight Savings Time when
 * the card is intialised then it can be used to maintain local time.
 * If DST is not used then this is probably best used to maintain the card
 * on UTC.
 *
 * Usage:
 *
 *    gt307 [-d n][-l] [yymmddhhmm.ss]
 *
 *    -d sets DST on or off n = 1 (on) n = 0 (off)
 *    -l switches output between long or short.  Short is suitable
 *      as an input into date(8)
 *    The time string is the same as date(8) for input.
*/

#if !defined(lint) && defined(DOSCCS)
static char sccsid[] = "@(#)GT307.c 1.00 (2.11BSD) 99/7/4";
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <sys/errno.h>
#include <sys/types.h>

#define BASE_ADDR 0177704
#define IO_VEC 0450
#define SEG_REG 6
#define IO_SIZE 1

#define REG_A   012
#define REG_B   013
#define REG_C   014
#define REG_D   015

#define UIP    0200
#define DV2    0100
#define DV1     040
#define DV0     020
#define RS3     010
#define RS2      04
#define RS1      02
#define RS0      01

#define SET    0200
#define PIE    0100
#define AIE     040
#define UIE     020
#define SQWE    010
#define DM       04
#define D24      02
#define DSE      01

#define IRQF    0200
#define PF      0100
#define AF       040
#define UF       020

#define VRT     0200

#define CONST   1

char *days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
char *months[] = {"Jan","Feb","Mar","Apr","May","Jun",
		  "Jul","Aug","Sep","Oct","Nov","Dec"};

main(argc, argv)
int argc;
char *argv[];
{

        register unsigned char   *io_ptr;
        char    time_str[80];
        unsigned int    seg_reg = SEG_REG;
        unsigned int    io_size = IO_SIZE;
        unsigned int    physaddr = BASE_ADDR;
        unsigned char   memval;
        int     ch;
        int     lflag;
        int     dst_flag;
        int     year;
        int     bigyear;
        int     month;
        int     date;
        int     day;
        int     hours;
        int     minutes;
        int     seconds;

/*
 * Set up the page address pointer.  This is written to allow simple
 * changing of the #defines at the start of the program to ripple through
 * to the rest of the code.  Do not allow 0 because that would collide
 * with the first 8kb of code or data.  Do not allow 7 because that would
 * collide with our stack.
*/
	assert(seg_reg > 0 && seg_reg < 7);
	io_ptr = (unsigned char *)((u_int)020000 * seg_reg);
/*
 * Map the new page to the physical address of the card.
*/
        if	(phys(seg_reg, io_size, physaddr) == -1)
		err(1, "phys(%d, %d, 0%o)", seg_reg, io_size, physaddr);
/*
 * process command line
*/
        dst_flag = 0;           /* Set defaults */
        lflag = 0;

        while	((ch = getopt(argc, argv,"d:l")) != EOF)
                switch((char)ch){
                        case 'd':
                                dst_flag = atoi(optarg) ? 1 : 0;
                                break;
                        case 'l':
                                lflag = 1;
                                break;
                        default:
                                errx(1,"invalid parameters.\n");
                        }
        argc -= optind;
        argv += optind;

        if	(argc > 1)
                errx(1,"invalid parameters.");

/*
 * If we still have an argument left then we are setting the clock,
 * otherwise we are displaying what is in the card.
*/
        if	(!argc)
                goto display;
        goto setup;

display:
/*
 * Look at the card to check whether the time on the card is
 * valid or not.
*/
        memval = (unsigned char)*(io_ptr + REG_D);
        if	((memval & VRT) == 0)
                errx(1," board clock invalid, reinitialise.");
/*
        Get the time from the card
*/
        memval = (unsigned char)*(io_ptr + REG_A);
        while((memval & UIP) == 1)
                memval = (unsigned char)*(io_ptr + REG_A);

        seconds = (int)*(io_ptr);
        minutes = (int)*(io_ptr + 2);
        hours = (int)*(io_ptr + 4);
        day = (int)*(io_ptr + 6);
        date = (int)*(io_ptr + 7);
        month = (int)*(io_ptr + 8);
        year = (int)*(io_ptr + 9);

        if(year <= 70)
                bigyear = year + 2000;
        else
                bigyear = year + 1900;

        if	(lflag == 0)
                printf("%02d%02d%02d%02d%02d.%02d\n",year,month,date,
                        hours,minutes,seconds);
        else
                printf("%s %s %d %02d:%02d:%02d %04d\n",
                        days[weekday(date,month, bigyear)],months[month - 1],
                        date,hours,minutes,seconds,bigyear);
        exit(0);

/*
 * set the card up given the input time contained in command line.
*/
setup:

/*
 *      Reset card for interrupts
*/
        memval = (unsigned char)(SET | D24 | DM);
        if	(dst_flag == 1)
                memval = memval | DSE;
        *(io_ptr + REG_B) = (unsigned char)memval;

        memval = (unsigned char)DV1;
        *(io_ptr + REG_A) = (unsigned char)memval;

        strcpy(time_str,*argv);

        seconds = 0;
        minutes = 0;
        hours = 0;
        day = 0;
        date = 0;
        month = 0;
        year = 0;

        (void)sscanf(time_str,"%2d%2d%2d%2d%2d.%2d",&year,&month,&date,&hours,
                &minutes,&seconds);

        if	(year <= 70)
                bigyear = year + 2000;
        else
                bigyear = year + 1900;

        day = weekday(date,month,bigyear) + 1;

        *(io_ptr) = (unsigned char)seconds;
        *(io_ptr + 2) = (unsigned char)minutes;
        *(io_ptr + 4) = (unsigned char)hours;
        *(io_ptr + 6) = (unsigned char)day;
        *(io_ptr + 7) = (unsigned char)date;
        *(io_ptr + 8) = (unsigned char)month;
        *(io_ptr + 9) = (unsigned char)year;

/*
 *      Start the card
*/
        memval = (unsigned char)*(io_ptr + 013);
        memval = memval & 0177;
        *(io_ptr + 013) = (unsigned char)memval;

        exit(0);
	}

int weekday(day, month, year)
int day, month, year;
{
        int index, yrndx, mondx;
        
        if(month <= 2)
        {
                month += 12;
                year--;
        }

        yrndx = year + (year/4) - (year/100) + (year/400);
        mondx = (2 * month) + (3 * (month + 1)) / 5;
        index = day + mondx + yrndx + CONST;

        return(index % 7);
}       
