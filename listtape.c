
#include <stdio.h>
#include <fcntl.h>
#define TAPE_BUFFER_SIZE 100000L


#define TAPE_IRG 0200
#define BCD_TM 017
                      
char		buffer[TAPE_BUFFER_SIZE];
int		eor = 0;	/* Report eor */
int		bin = 0;	/* Doing binary */
int		p7b = 0;	/* Doing BCD tape */
int		mark = 0;	/* Show marks */
int		reclen = 130;

char bcd_ascii[64] = {
	'b',	/* 0           - space */
	'1',	/* 1        1  - 1 */
	'2',	/* 2       2   - 2 */
	'3',	/* 3       21  - 3 */
	'4',	/* 4      4    - 4 */
	'5',	/* 5      4 1  - 5 */
	'6',    /* 6      42   - 6 */
	'7',	/* 7	  421  - 7 */
	'8',	/* 8     8     - 8 */
	'9',	/* 9     8  1  - 9 */
	'0',	/* 10    8 2   - 0 */
	'=',    /* 11    8 21  - equal */
	'\'',	/* 12    84    - apostrophe */
	':',    /* 13    84 1  - colon */
	'>',	/* 14    842   - greater than */
	'&',	/* 15    8421  - radical 017 {? */
	' ',    /* 16   A      - substitute blank */
	'/',	/* 17   A   1  - slash */
	'S',	/* 18   A  2   - S */
	'T',	/* 19   A  21  - T */
	'U',	/* 20   A 4    - U */
	'V',	/* 21   A 4 1  - V */
	'W',	/* 22   A 42   - W */
	'X',	/* 23   A 421  - X */
	'Y',	/* 24   A8     - Y */
	'Z',	/* 25   A8  1  - Z */
	'|',	/* 26   A8 2   - record mark */
	',',	/* 27   A8 21  - comma */
	'(',	/* 28   A84    - paren */
	'~',	/* 29   A84 1  - word separator */
	'\\',	/* 30   A842   - left oblique */
	'"',    /* 31   A8421  - segment mark */
	'-',	/* 32  B       - hyphen */
	'J',	/* 33  B    1  - J */
	'K',	/* 34  B   2   - K */
	'L',	/* 35  B   21  - L */
	'M',	/* 36  B  4    - M */
	'N',	/* 37  B  4 1  - N */
	'O',	/* 38  B  42   - O */
	'P',	/* 39  B  421  - P */
	'Q',	/* 40  B 8     - Q */
	'R',	/* 41  B 8  1  - R */
	'!',	/* 42  B 8 2   - exclamation */
	'$',	/* 43  B 8 21  - dollar sign */
	'*',	/* 44  B 84    - asterisk */
	']',	/* 45  B 84 1  - right bracket */
	';',    /* 46  B 842   - semicolon */
	'_',    /* 47  B 8421  - delta */
	'+',    /* 48  BA      - ampersand or plus */
	'A',	/* 49  BA   1  - A */
	'B',    /* 50  BA  2   - B */
	'C',	/* 51  BA  21  - C */
	'D',	/* 52  BA 4    - D */
	'E',	/* 53  BA 4 1  - E */
	'F',	/* 54  BA 42   - F */
	'G',	/* 55  BA 421  - G */
	'H',	/* 56  BA8     - H */
	'I',	/* 57  BA8  1  - I */
	'?',	/* 58  BA8 2   - question mark 032 */
	'.',	/* 59  BA8 21  - period */
	')',	/* 60  BA84    - paren */
	'[',	/* 61  BA84 1  - left bracket 035 */
	'<',	/* 62  BA842   - less than 036 */
	'}'	/* 63  BA8421  - group mark 037 */
};

void usage() {
   fprintf(stderr,"Usage: listtape [-b] [-e] [-p] [-r#] <tapefile>\n");
   fprintf(stderr,"     -r#: Characters per record #\n");
   fprintf(stderr,"     -b:  Use IBSYS binary translation\n");
   fprintf(stderr,"     -m:  Show record marks |\n");
   fprintf(stderr,"     -e:  Show end of records as {\n");
   fprintf(stderr,"     -p:  Read BCD tape instead of TAP format\n");
   exit(1);
}

/* Read one record from tape */
int read_tape(FILE *f, int *len) {
   unsigned long int sz;
   *len = 0;
   if (p7b) {
	static unsigned char lastchar = 0xff;
	unsigned char	    ch;
        sz = 0;
	if (lastchar != 0xff) 
	    buffer[sz++] = lastchar;
	/* Check if last char was Tape Mark */
	else if (lastchar == (BCD_TM|TAPE_IRG)) {
	    lastchar = 0xff;
	    *len = -1;
	    return 1;
	}
	lastchar = 0xff;
	while(fread(&ch, sizeof(unsigned char), 1, f) == 1) {
	    if (ch & TAPE_IRG) {
	        lastchar = ch;
		*len = sz;
	        return 1;
	    }
	    buffer[sz++] = ch;
	}
	if (sz != 0) {
	   *len = sz;
	   return 1;
	}
	return 0;
   } else {
	unsigned char	xlen[4];
	int		i;
        if (fread(&xlen, sizeof(unsigned char), 4, f) != 4) 
	    return 0;
	/* Convert to number */
	sz = xlen[0];
	sz |= (xlen[1]) << 8;
	sz |= (xlen[2]) << 16;
	sz |= (xlen[3]) << 24;

        /* Check for EOM */
        if (sz == 0xffffffff)  {
	    *len = -2;
	    return 0;
	}
	/* Check for EOF */
        if (sz == 0) {
	   *len = -1;
	   return 1;
        }
        if (sz > TAPE_BUFFER_SIZE) {
	   fprintf(stderr, "Block to big for buffer\n");
	   return 0;
        }
	*len = sz;
	sz = 0x7fffffff & ((sz + 1) & ~1);
        if (fread(buffer, 1, sz, f) != sz)  {
           fprintf(stderr, "read error\n");
           return 0;
        }
        /* Read backward length */
        fread(&sz, sizeof(unsigned long int), 1, f);
   }
   return 1;
}


main(int argc, char *argv[]) {
   int		sz;
   int		i;
   int		col;
   char		*p;
   FILE		*tape;

   while(--argc && **(++argv) == '-') {
   	switch(tolower((*argv)[1])) {
	case 'r':
		reclen = atoi(&(*argv)[2]);
      	fprintf(stderr,"Recordlen set: %d\n",reclen);
		break;
	case 'e':
		eor = 1;
		break;
	case 'b':
		bin = 1;
		break;
	case 'p':
		p7b = 1;
		break;
	case 'm':
		mark = 1;
		break;
      default:
      	fprintf(stderr,"Unknown option: %s\n",*argv);
      }
   }

   if(argc != 1) {
   	usage();
   }

   /* Open input */
   if((tape = fopen(*argv,"rb")) == NULL) {
	fprintf(stderr,"Can't open tape file %s: ",*argv);
        perror("");
        exit(1);
   }

   /* Process records of the file */
   while(read_tape(tape, &sz)) {
	if (sz == -2) 
	    break;
	if (sz == -1) 
	    puts("*EOF*");
	else {
	    p = buffer;
	    col = 0;
	    for(i = 0; i < sz; i++)  {
		char	ch = *p++ & 077;
		if (bin) {
		    ch ^= (ch & 020) << 1;
		    if (ch == 012)
			    ch = 0;   
		}
		if (ch == 032) {
		   if (mark) {
			putchar(bcd_ascii[ch]);
			col++;
		   } else {
			putchar('\n');
			col = 0;
		   }
		} else {
		   putchar(bcd_ascii[ch]);
		   if((++col == reclen && sz != (i+1))){
		       putchar('\n');
		       col = 0;
		   }
		}
	    }
	    if (eor)
		putchar('{');
	    putchar('\n');
	}
    }
    fclose(tape);
}
