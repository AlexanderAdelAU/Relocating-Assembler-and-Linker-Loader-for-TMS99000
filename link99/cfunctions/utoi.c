/*
 ** atoi -- convert ascii string to integer
 */

int atoi(str)
	char *str; {
	// Initialise res to 0
	int res = 0;
	int i = 0;

	// Iterate through the string strg and compute res
	while (str[i] != '\0') {
		res = res * 10 + (str[i] - '0');
		i++;
	}
	return res;
}
