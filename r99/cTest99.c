
#include "stdio.h"
#include "float.h"
int i,j,k;
char l,m,n;
int ftoa2();
main (argc,argv) int argc; char *argv[]; {

	int i;
	int fd;
	double dd;
	char ch;
	char str[64];
	double z;
	double y;
	char *ui;
	char *uj;
	double st;
	double pi;


    // Run test cases

	testArith();
	testControl();
	add();
	testFunc();
	testArrays();
	testPointer();
	testMemAll();
	testLogical();
	return;


	st = 1.0;
	pi = 3.1461234;

	fd = 2;

	for (i=0; i <= 20; i += 1){
		dd = i * pi;
		printf("\n%i%20.10f - ",i, dd);
	}




	printf("\nStarting Test..\n");
	dd = 1.2345;
	dd = dd+2.0;
	ftoa2(dd,5,str);
	puts(str);



	for (i=0; i <= 360; i += 10){
		dd = sin(float(i)*3.141592654/180.0);
		printf("\n%20.10f - ",dd);
		//fp2string(dd);
	}


	printf("\n Testing disc i/o");
    printf("\nOutput argc for main = %d\n",argc);
	for(i=1;i< argc;i++)
	{
		printf("%s%c",argv[i],(i<argc-1) ? ' ' : '\n');
	}

	fd = fopen("test1","w");
	printf ("Create new file test1 for writing fd = %d\n",fd);
	ch = putc('T',fd);
	printf("One character written...\n");
	fclose(fd);

	fd = fopen("test1","r");
	printf ("Open file test1 for reading fd = %d\n",fd);
	ch = getc(fd);
	printf ("Output char  = %c\n",ch);
	fclose(fd);
	printf("\nEnd of Test\n");

}
/* convert double number to string (f format) */

int ftoa2(x,f,str)
double x;	/* the number to be converted */
int f;		/* number of digits to follow decimal point */
char *str;	/* output string */
{
	double scale;		/* scale factor */
	int i,				/* copy of f, and # digits before decimal point */
		d;				/* a digit */

	if( x < 0.0 ) {
		*str++ = '-' ;
		x = -x ;
	}
	i = f ;
	scale = 2.0 ;
	while ( i-- )
		scale *= 10.0 ;
	x += 1.0 / scale ;
	/* count places before decimal & scale the number */
	i = 0 ;
	scale = 1.0 ;
	while ( x >= scale ) {
		scale *= 10.0 ;
		i++ ;
	}
	while ( i-- ) {
		/* output digits before decimal */
		scale = floor(0.5 + scale * 0.1 ) ;
		d = ifix( x / scale ) ;
		*str++ = d + '0' ;
		x -= float(d) * scale ;
	}
	if ( f <= 0 ) {
		*str = NULL ;
		return 0;
	}
	*str++ = '.' ;
	while ( f-- ) {
		/* output digits after decimal */
		x *= 10.0 ;
		d = ifix(x) ;
		*str++ = d + '0' ;
		x -= float(d) ;
	}
	*str = NULL ;
}

int fp2string(fp) double fp; {
	int *ptr;
	ptr = &fp;
	printf("%04x", *ptr++); // gives 12AB
	printf("%04x", *ptr++); // gives 12AB
	printf("%04x\n", *ptr++); // gives 12AB

}

int upper(c, ch) int c; char ch; {
	c = ch;
	return c;

}
// Arithmetic Operations
testArith() {
	 int a, b, result;
     a = 5;
     b = 3;
     result = a + b;
    printf("Arithmetic Operations Result: %d\n", result);
}

// Control Flow
 testControl() {
	 int x;
     x = 10;
    if (x > 5) {
        printf("Control Flow: x is greater than 5\n");
    } else {
        printf("Control Flow: x is less than or equal to 5\n");
    }
}

 // Arrays
 testArrays() {
 	    int arr[5];
 	    int i;
 	   arr[0] = 1;
 	   arr[1] = 2;
 	   arr[2] = 3;
 	   arr[3] = 4;
 	   arr[4] = 5;

 	 //   arr= {1, 2, 3, 4, 5};
 	    printf("Arrays: ");
 	    for (i = 0; i < 5; i++) {
 	        printf("%d ", arr[i]);
 	    }
 	     printf("\n");
 }


 // Function Calls
 int add(a, b) int a; int b; {
     return (a + b);
 }

 testFunc() {
     int result;
     result= add(3, 4);
     printf("Function Calls Result: %d\n", result);
 }

 // Pointer Operations
  testPointer() {
     int x;
     int *ptr;
     x = 10;
     *ptr = &x;
     printf("Pointer Operations: Value of x: %x\n", *ptr);
 }

 // Memory Allocation
  testMemAll() {
     int *ptr;
     //*ptr = (int *)malloc(sizeof(int));
     *ptr = 5;
     printf("Memory Allocation: Value of ptr: %d\n", *ptr);
     free(ptr);
 }

 // Additional Logical Tests
  testLogical() {
     int a;
     int b;
     a= 5; b= 3;
     printf("Logical Operations: a >= b: %d\n", a >= b);
     printf("Logical Operations: a <= b: %d\n", a <= b);
     printf("Logical Operations: a == b: %d\n", a == b);
     printf("Logical Operations: a != b: %d\n", a != b);
 }



