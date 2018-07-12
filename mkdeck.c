/***********************************************************************
*
* Build up a deck. Remove blank and invalid cards.
*
***********************************************************************/

#include <stdio.h>
#include <string.h>

unsigned char        parity_table[64] = {
    /* 0    1    2    3    4    5    6    7 */
    0000, 0100, 0100, 0000, 0100, 0000, 0000, 0100,
    0100, 0000, 0000, 0100, 0000, 0100, 0100, 0000,
    0100, 0000, 0000, 0100, 0000, 0100, 0100, 0000,
    0000, 0100, 0100, 0000, 0100, 0000, 0000, 0100,
    0100, 0000, 0000, 0100, 0000, 0100, 0100, 0000,
    0000, 0100, 0100, 0000, 0100, 0000, 0000, 0100,
    0000, 0100, 0100, 0000, 0100, 0000, 0000, 0100,
    0100, 0000, 0000, 0100, 0000, 0100, 0100, 0000
};
          
void dmp_card(unsigned short image[80]) {
    int 	   t;
    int		   i, j;
    unsigned char  out[160]; 

    /* Fill buffer */
    for (i = 0; i < 80; i++) {
        unsigned short int  col = image[i];
        out[i*2] = (col >> 6) & 077;
        out[i*2+1] = col & 077;
    }

    /* Now set parity */
    for (i = 0; i < 160; i++)
        out[i] |= 0100 ^ parity_table[(int)out[i]];
    out[0] |= 0x80;     /* Set record mark */
    i = 160;
    fwrite(out, 1, i, stdout);
}

void print_card(unsigned short image[80]) {
    int		   t;
    int		   i, j;

    for (t = i = 0; i < 80; i++) t |= image[i];
    if (t == 0) {
        printf("blank\n");
    } else {
        t = 0;
        for (j = 0, i = 0; i < 72; i++) {
     	    t = (t << 1) | image[i] & 1;
    	    if (j == 2) {
    	        putchar('0' + t);
    	        t = 0;
    	        j = 0;
    	    } else 
    	        j++;
    	    if (i == 35)
    	        putchar(' ');
        }
        putchar(' ');
        t = 0;
        for (j = 0, i = 0; i < 72; i++) {
    	    t = (t << 1) | ((image[i] & 2)>>1);
    	    if (j == 2) {
    	        putchar('0' + t);
    	        t = 0;
    	        j = 0;
    	    } else 
    	        j++;
    	    if (i == 35)
    	        putchar(' ');
        }
        putchar('\n');
    }
}

int
main(int argc, char *argv[]) {
    unsigned short image[80];
    int		   col;
    int		   t;
    int		   i, j;
    int		   ch;
    int		   card;
    char	   *n;
    char	   *of;
    FILE	   *f;

    col = 0;
    t = 0;
    card = 0;
    if ((f = fopen(n, "r")) == NULL) {
         fprintf(stderr, "Unable to open: %s\n", n);
         exit(1);
      }
    memset(image, 0, sizeof(image));
    while((n = *++argv) != NULL) {
	if ((f = fopen(n, "r")) == NULL) {
         fprintf(stderr, "Unable to open: %s\n", n);
         exit(1);
      }

      while(!feof(f)) {
          ch = fgetc(f);
	  image[col] |= (ch & 077) << t;
	  if (t) {
	      t = 0;
	      col++;
	  } else
	      t = 6;
	  if (col == 80 || ch & 0x80) {
	     if (col != 0) {
	          card++;
		  printf("%d ", card);
		  print_card(image);
              }
	      memset(image, 0, sizeof(image));
	      col = 0;
	      t = 0;
	  }
     } 
     fclose(f);
     if (col != 0) {
	card++;
	printf("%d ", card);
	print_card(image);
     }
   }
   return (0);
   
}

