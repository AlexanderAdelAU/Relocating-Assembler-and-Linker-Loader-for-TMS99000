/*
 ** atoi -- convert ascii string to integer
 **
 ** Small-C has no initialisers in declarations and no // comments,
 ** so both are spelled out here.
 */

atoi(str)
char *str;
{
	int res;
	int i;

	res = 0;
	i = 0;

	/* iterate through str and build the result */
	while (str[i] != '\0') {
		res = res * 10 + (str[i] - '0');
		i++;
	}
	return (res);
}
