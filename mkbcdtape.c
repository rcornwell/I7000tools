#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "ubcd.h"


char verbose = 1;		// 1 for verbose mode (default), 0 for "quiet" mode
int  reclen = 84;		// Default record len.
int  block = 10;		// Default records per block.
//
//	Parity table for the 64 BCD characters (without parity bit,
//	wordmark bits, etc).
//

int parity_table[64] = {
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0
};

int main(int argc, char **argv)
{
   FILE *tape;
   FILE *in;
   char	*tname = NULL;
   unsigned char  *tape_buffer;
   unsigned char  *line_buffer;
   unsigned char  *tape_char;
   char	 ch;
   int   len;
   int	 blen;
   int   record = 0;

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
	if ((in = fopen(tname, "r")) == NULL) {
	    fprintf(stderr, "Can't open input file %s: ", tname);
	    perror("");
	    exit(1);
	}
        blen = len = 0;
	tape_char = line_buffer;
	while((ch = fgetc(in)) != EOF) {
	    int eol = 0;
	switch(ch) {
	case '\r': break;
	case '\n': eol = 1;/* Do eol */ break;
	case '\t': 
	   while((len & 7) != 0) {
	   *tape_char++ = 060;
	   len++;
	   }
	   break;
	case '~': eol = 1; break; /* term block and write EOF */
	default:
		*tape_char++ = ch /*ascii_bcd[ch]*/;
		len++;
		break;
	}
	if (eol || len == reclen) {
	   /* Fill in record with blanks */
	   record++;
	   while (len < reclen) {
	     *tape_char++ = 060;
	     len++;
	   }
	   if ((*line_buffer == 043/*$*/ && blen != 0) ||
	        blen == (reclen * block)) {
	      unsigned long sz = blen;
	      fwrite(&sz, sizeof(unsigned long), 1, tape);
	      fwrite(tape_buffer, blen, 1, tape);
	      fwrite(&sz, sizeof(unsigned long), 1, tape);
	      blen = 0;
	   }
	   if (*line_buffer == 043/*$*/) {
	      unsigned long sz = len;
	      fwrite(&sz, sizeof(unsigned long), 1, tape);
	      fwrite(line_buffer, len, 1, tape);
	      fwrite(&sz, sizeof(unsigned long), 1, tape);
	   } else {
	      int i;
	      for(i = 0; i < len; i++) {
		 tape_buffer[blen++] = line_buffer[i];
	      }
 	   }
	   len = 0;
	   tape_char = line_buffer;
	}
	if (ch == '~') {
	   unsigned long sz = 0;
	   fwrite(&sz, sizeof(unsigned long), 1, tape);
	}
	   
	}
        fclose(in);
	if (len != 0) {
	   unsigned long sz;
	   int i;
	   /* Fill in record with blanks */
	   while (len < reclen) {
	     *tape_char++ = 060;
	     len++;
	   }
	   if ((*line_buffer == 043/*$*/ && blen != 0) ||
	        blen == (reclen * block)) {
	      unsigned long sz = blen;
	      fwrite(&sz, sizeof(unsigned long), 1, tape);
	      fwrite(tape_buffer, blen, 1, tape);
	      fwrite(&sz, sizeof(unsigned long), 1, tape);
	      blen = 0;
	   }
	   for(i = 0; i < len; i++) {
	      tape_buffer[blen++] = line_buffer[i];
	   }
	}
	if (blen != 0) {
	      unsigned long sz = blen;
	      fwrite(&sz, sizeof(unsigned long), 1, tape);
	      fwrite(tape_buffer, blen, 1, tape);
	      fwrite(&sz, sizeof(unsigned long), 1, tape);
	      blen = 0;
	   }
   }	   
   {
	/* Put EOM */
	   unsigned long sz = 0xffffffff;
	   fwrite(&sz, sizeof(unsigned long), 1, tape);
   }
	
   printf("EOF after %d records\n",record);

   free(tape_buffer);
   free(line_buffer);
   fclose(tape);
   printf("Done.\n");
   return(0);
}


void usage()
{
	fprintf(stderr,"Usage: bcdtape [-o <tapefile>] [-b#] [-r#] <inputfiles>\n");
   fprintf(stderr,"	-o: name Output file name\n");
   fprintf(stderr,"	-b#: # lines per block\n");
   fprintf(stderr,"	-r#: Characters per record #\n");
	exit(1);
}

