
/*
** mess.c -- message functions
*/

/* Print a literal string without carriage return to console */
putls(char *str){
	while(*str)
		putchar(*str++);
}

puts2(char *str1, char *str2) {
  putls(str1);
  putls(str2);
  }

puts3(char *str1, char *str2, char *str3) {
  putls(str1);
  putls(str2);
  putls(str3);
  }
cant(char *str) {
  error2(str, " - Can't Open");
  }
error2(char *str1, char *str2) {
  putls(str1);
  error(str2);
  }
error(char *reason) {
  putls(reason);
  exit(0);
  }
