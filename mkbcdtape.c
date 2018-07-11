#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

char verbose = 1;		// 1 for verbose mode (default), 0 for "quiet" mode
int  reclen = 84;		// Default record len.
int  block = 10;		// Default records per block.
typedef unsigned long int	uint32;	// Header unit
//
//	Parity table for the 64 BCD characters (without parity bit,
//	wordmark bits, etc).
//


char parity_table[64] = {
        /* 0    1    2    3    4    5    6    7 */
        0000,0100,0100,0000,0100,0000,0000,0100,
        0100,0000,0000,0100,0000,0100,0100,0000,
        0100,0000,0000,0100,0000,0100,0100,0000,
        0000,0100,0100,0000,0100,0000,0000,0100,
        0100,0000,0000,0100,0000,0100,0100,0000,
        0000,0100,0100,0000,0100,0000,0000,0100,
        0000,0100,0100,0000,0100,0000,0000,0100,
        0100,0000,0000,0100,0000,0100,0100,0000
};

const char ascii_to_six[128] = {
/*   000 nul 001 soh 002 stx 003 etx 004 eot 005 enq 006 ack 007 bel */
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,
/*   010 bs  011 ht  012 nl  013 vt  014 np  015 cr  016 so  017 si */
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,
/*   020 dle 021 dc1 022 dc2 023 dc3 024 dc4 025 nak 026 syn 027 etb */
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,
/*   030 can 031 em  032 sub 033 esc 034 fs  035 gs  036 rs  037 us */
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,
/*   040 sp  041  !  042  "  043  #  044  $  045  %  046  &  047  ' */
	020,	052,	037,	-1,	053,    077,    017,	014,
/*   050  (  051  )  052  *  053  +  054  ,  055  -  056  .  057  / */
	034,	074,	054,	060,	033,	040,	073,	021,
/*   060  0  061  1  062  2  063  3  064  4  065  5  066  6  067  7 */
	012,	001,	002,	003,	004,	005,	006,	007,
/*   070  8  071  9  072  :  073  ;  074  <  075  =  076  >  077  ? */
	010,	011,	015,	056,	076,	013,	016,	072,
/*   100  @  101  A  102  B  103  C  104  D  105  E  106  F  107  G */
	-1,	061,	062,	063,	064,	065,	066,	067,
/*   110  H  111  I  112  J  113  K  114  L  115  M  116  N  117  O */
	070,	071,	041,	042,	043,	044,	045,	046,
/*   120  P  121  Q  122  R  123  S  124  T  125  U  126  V  127  W */
	047,	050,	051,	022,	023,	024,	025,	026,
/*   130  X  131  Y  132  Z  133  [  134  \  135  ]  136  ^  137  _ */
	027,	030,	031,	075,	036,	055,	035,	057,
/*   140  `  141  a  142  b  143  c  144  d  145  e  146  f  147  g */
	014,	061,	062,	063,	064,	065,	066,	067,
/*   150  h  151  i  152  j  153  k  154  l  155  m  156  n  157  o */
	070,	071,	041,	042,	043,	044,	045,	046,
/*   160  p  161  q  162  r  163  s  164  t  165  u  166  v  167  w */
	047,	050,	051,	022,	023,	024,	025,	026,
/*   170  x  171  y  172  z  173  {  174  |  175  }  176  ~  177 del */
	027,	030,	031,     -1,	032,	 -1,	035,	-1,
};

void write_block(FILE *f, uint32 len, unsigned char *buffer) {
     uint32 wlen = 0x7fffffff & ((len + 1) & ~1);
     fwrite(&len, sizeof(uint32), 1, f);
     fwrite(buffer, sizeof(unsigned char), wlen, f);
     fwrite(&len, sizeof(uint32), 1, f);
}

int main(int argc, char **argv)
{
   FILE *tape;
   FILE *in;
   char	*tname = NULL;
   unsigned char  *tape_buffer;
   unsigned char  *line_buffer;
   unsigned char  *tape_char;
   uint32	sz;
   uint32       blen;
   uint32	tape_mark = 0;
   char	 	ch;
   int		len;
   int	   	record = 0;
   int		last = 0;
   int 		i;

   void usage();


   //	Do options processing
   while(--argc && **(++argv) == '-') {
   	switch(tolower((*argv)[1])) {
      case 'o':
	 tname = *++argv;
	 break;
      case 'q':
      	 verbose = 0;
         break;
      case 'r':
      	 reclen = atoi(&(*argv)[2]);
         break;
      case 'b':
      	 block = atoi(&(*argv)[2]);
         break;
      default:
      	fprintf(stderr,"Unknown option: %s\n",*argv);
         usage();
      }
   }

   if(argc == 0 || tname == NULL) {
   	usage();
   }

   if(verbose) {
	   printf("Tape file name is %s\n",tname);
   }

   if((tape = fopen(tname,"wb")) == NULL) {
   	fprintf(stderr,"Can't open tape file %s: ",tname);
      perror("");
      exit(1);
   }

   if((tape_buffer = (unsigned char  *)calloc(reclen * block,1)) == NULL) {
   	fprintf(stderr,"calloc of tape buffer failed...\n");
      exit(1);
   }

   if((line_buffer = (unsigned char  *)calloc(reclen,1)) == NULL) {
   	fprintf(stderr,"calloc of tape buffer failed...\n");
      exit(1);
   }

   while(--argc > 0) {
	tname = *argv++;
	if (strcmp(tname, "-") == 0) {
	   /* Flush out buffer if anything in it. */
	   if (blen != 0) {
 	      write_block(tape, blen, tape_buffer);
	      blen = 0;
           }
	   /* Put out a tape mark */
	   fwrite(&tape_mark, sizeof(uint32), 1, tape);
	   printf("EOF: %d records\n", record - last);
	   last = record;
	} else {
	    /* Process a file */
	    if ((in = fopen(tname, "r")) == NULL) {
	        fprintf(stderr, "Can't open input file %s: ", tname);
	        perror("");
	        exit(1);
	    }
            fprintf(stderr, "Reading file %s\n", tname);
	    blen = len = 0;
	    tape_char = line_buffer;
	    while((ch = fgetc(in)) != EOF) {
		int eol = 0;
		int eof = 0;
		switch(ch) {
		case '\r': break;
		case '\n': eol = 1;/* Do eol */ break;
		case '\t': 
			while((len & 7) != 0 && len < reclen) {
			     *tape_char++ = 0120;
			     len++;
			}
			break;
		case '~': 
			if (len > 0)
			    eol = 1; 
			/* Read rest of line */
			while((ch = fgetc(in)) != EOF && ch != '\n');
			eof = 1;
			break; /* term block and write EOF */
		default:
			if (ascii_to_six[ch] == -1) 
			    fprintf(stderr, "Invalid char %c\n\r", ch);
			ch = 077 & ascii_to_six[ch];
			
			if (ch == 0)
			    ch = 012;
			ch |= parity_table[ch];
			*tape_char++ = ch;
			len++;
			break;
		}

		/* If at record length, grab input until end of line */
		if (len == reclen) {
		     while((ch = fgetc(in)) != EOF && ch != '\n');
		     eol = 1;
		}

		/* Put record into buffer if at end of line */
		if (eol) {
		   record++;
		   /* Fill in record with blanks */
		   while (len < reclen) {
		     *tape_char++ = 0120;
		     len++;
		   }

		   /* If first char is $ output it in it's own record */
		   if (*line_buffer == 053/*$*/) {
		       /* Flush out buffer */
		      if (blen != 0) {
 	      		  write_block(tape, blen, tape_buffer);
		          blen = 0;
		      }
 	      	      write_block(tape, reclen, line_buffer);
		   } else {
		      /* Copy to buffer */
		      for(i = 0; i < reclen; i++) {
			 tape_buffer[blen++] = line_buffer[i];
		      }
		   }
		   len = 0;
		   tape_char = line_buffer;
		   /* If buffer full, dump it */
		   if (blen == (reclen * block)) {
 	      	      write_block(tape, blen, tape_buffer);
		      blen = 0;
		   }
		}
		   
 		if (eof) {
		    /* Flush out buffer */
		    if (blen != 0) {
 	      	      write_block(tape, blen, tape_buffer);
		      blen = 0;
		    }
	            /* Write tape mark */
		    fwrite(&tape_mark, sizeof(uint32), 1, tape);
		    printf("File: %d records\n", record - last);
		    last = record;
		}
	    }

	    /* If buffer not empty, flush it */
	    if (len != 0) {
	       int i;
	       /* Fill in record with blanks */
	       while (len < reclen) {
                   *tape_char++ = 0120;
		   len++;
	       }

	       /* Put $ lines in record by themselfs */
	       if (*line_buffer == 053/*$*/) {
		  if (blen != 0) {
 	      	      write_block(tape, blen, tape_buffer);
	              blen = 0;
		   }
 	      	   write_block(tape, reclen, line_buffer);
	       } else {
	           for(i = 0; i < len; i++) {
	              tape_buffer[blen++] = line_buffer[i];
	           }
	       }
	       len = 0;
	       tape_char = line_buffer;
	    }
	    fclose(in);
        }   
   }
   /* Flush out buffer if anything in it. */
   if (blen != 0) 
        write_block(tape, blen, tape_buffer);

   /* Write tape mark */
   fwrite(&tape_mark, sizeof(uint32), 1, tape);
   /* Put EOM */
   sz = 0xffffffff;
   fwrite(&sz, sizeof(uint32), 1, tape);

   if (record != last)	
       printf("EOF: %d records\n", record - last);
   printf("Output after %d records\n",record);

   free(tape_buffer);
   free(line_buffer);
   fclose(tape);
   printf("Done.\n");
   return(0);
}


void usage()
{
   fprintf(stderr,"Usage: mkbcdtape [-o <tapefile>] [-b#] [-r#] <inputfiles>\n");
   fprintf(stderr,"	-o: name Output file name\n");
   fprintf(stderr,"	-b#: # lines per block\n");
   fprintf(stderr,"	-r#: Characters per record #\n");
	exit(1);
}

