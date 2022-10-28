int strcmp(const char *p, const char *q)
{
	register const unsigned char *s1 = (const unsigned char *) p;
	register const unsigned char *s2 = (const unsigned char *) q;
	unsigned char c1, c2;

	do {
		c1 = (unsigned char) *s1++;
		c2 = (unsigned char) *s2++;
		if (c1 == '\0')
			goto str_term;
	} while (c1 == c2);

str_term:
	return c1 - c2;	
}
