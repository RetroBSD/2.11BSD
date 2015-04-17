# include	<stdio.h>
/* fake portable I/O routines, for those
    sites so backward as to not have the
     port. library */

FILE	*cin, *cout;
FILE	*Fin, *Fout;

copen( s, c ) char *s; {
  FILE	*f;

  if( c == 'r' ){
    Fin = f = fopen( s, "r" );
    }

  else if( c == 'a' ){
    f = fopen( s, "a" );
    }

  else {  /* c == w */
    f = fopen( s, "w" );
    }

  return( f );
  }

cflush(x){ /* fake! sets file to x */
  fflush(Fout);
  Fout = x;
  }

system(){
  error( "The function \"system\" is called" );
  }

cclose(i){
  fclose(i);
  }

cexit(i){
  fflush(Fout);
  exit(i);
  }
