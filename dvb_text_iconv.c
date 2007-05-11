#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <iconv.h>

#define MAX 1024
static char buf[MAX * 2], result[MAX * 10];
static const char HEX[16] = "0123456789ABCDEF";

enum CS {
	CS_UNKNOWN,
	ISO6937,
	ISO8859_5,
	ISO8859_6,
	ISO8859_7,
	ISO8859_8,
	ISO8859_9,
	ISO10646,
	CS_OTHER,
};
static enum CS cs_old = CS_UNKNOWN;
static iconv_t cd;

/* The spec says ISO-6937, but many stations get it wrong and use ISO-8859-1. */
char *iso6937_encoding = "ISO6937";

/* Quote the xml entities in the string passed in.
 */
char *xmlify(const char *s) {
	enum CS cs_new = CS_UNKNOWN;

	switch ((unsigned char)*s) {
		case 0x20 ... 0xFF: // if the first byte of the text field has a value in the range of "0x20" to "0xFF" then this and all subsequent bytes in the text item are coded using the default coding table (table 00 - Latin alphabet) of figure A.1
			cs_new = ISO6937;
			break;
		case 0x01: // if the first byte of the text field is in the range "0x01" to "0x05" then the remaining bytes in the text item are coded in accordance with character coding table 01 to 05 respectively, which are given in figures A.2 to A.6 respectively
			cs_new = ISO8859_5;
			s += 1;
			break;
		case 0x02:
			cs_new = ISO8859_6;
			s += 1;
			break;
		case 0x03:
			cs_new = ISO8859_7;
			s += 1;
			break;
		case 0x04:
			cs_new = ISO8859_8;
			s += 1;
			break;
		case 0x05:
			cs_new = ISO8859_9;
			s += 1;
			break;
		case 0x10:
			// if the first byte of the text field has a value "0x10" then the following two bytes carry a 16-bit value (uimsbf) N to indicate that the remaining data of the text field is coded using the character code table specified by ISO Standard 8859, Parts 1 to 9.
			cs_new = CS_OTHER
				+ ((unsigned char)s[1] << 8)
				+  (unsigned char)s[2];
			s += 3;
			break;
		case 0x11: // if the first byte of the text field has a value "0x11" then the ramaining bytes in the text item are coded in pairs in accordance with the Basic Multilingual Plane of ISO/IEC 10646-1
			cs_new = ISO10646;
			s += 1;
			break;
		case 0x06 ... 0x0F: case 0x12 ... 0x1F: // Values for the first byte of "0x00", "0x06" to "0x0F", and "0x12" to "0x1F" are reserved for future use.
			fprintf(stderr, "Reserved encoding: %02x\n", *s);
			return NULL;
		case 0x00: // empty string
			return "";
	} // switch

	if (cs_old != cs_new || cs_old == CS_UNKNOWN) {
		if (cd) {
			iconv_close(cd);
			cd = NULL;
		} // if
		switch (cs_new) {
			case ISO6937:
				cd = iconv_open("UCS2", iso6937_encoding);
				break;
			case ISO8859_5:
				cd = iconv_open("UCS2", "ISO8859-5");
				break;
			case ISO8859_6:
				cd = iconv_open("UCS2", "ISO8859-6");
				break;
			case ISO8859_7:
				cd = iconv_open("UCS2", "ISO8859-7");
				break;
			case ISO8859_8:
				cd = iconv_open("UCS2", "ISO8859-8");
				break;
			case ISO8859_9:
				cd = iconv_open("UCS2", "ISO8859-9");
				break;
			case ISO10646:
				cd = iconv_open("UCS2", "ISO-10646/UTF8");
				break;
			default: {
				char from[14];
				int i = cs_new - CS_OTHER;
				snprintf(from, sizeof(from), "ISO8859-%d", i);
				cd = iconv_open("UCS2", from);
				 }
		} // switch
		cs_old = cs_new;
	} // if

	char *inbuf = (char *)s;
	size_t inbytesleft = strlen(s);
	char *outbuf = (char *)buf;
	size_t outbytesleft = MAX * 2;
	size_t ret = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
	// FIXME: handle errors

	char *b = buf, *r = result;
	for ( ; b < outbuf; b += 2) {
		int i = (unsigned char)b[0] + (unsigned char)b[1] * 256;
		switch (i) {
			case '\t':
			case '\n':
			case ' ' ... '%': // &
			case '\'' ... ';': // <
			case '=': // >
			case '?' ... 0x7E:
				*r++ = (char)i;
				break;
			case '&':
				*r++ = '&';
				*r++ = 'a';
				*r++ = 'm';
				*r++ = 'p';
				*r++ = ';';
				break;
			case '<':
				*r++ = '&';
				*r++ = 'l';
				*r++ = 't';
				*r++ = ';';
				break;
			case '>':
				*r++ = '&';
				*r++ = 'g';
				*r++ = 't';
				*r++ = ';';
				break;
			case 0x0000 ... 0x0008:
			case 0x000B ... 0x001F:
			case 0x007F:
				fprintf(stderr, "Illegal char %04x\n", i);
			default:
				*r++ = '&';
				*r++ = '#';
				*r++ = 'x';
				if (i & 0xF000) *r++ = HEX[(i >> 12) & 0xF];
				if (i & 0xFF00) *r++ = HEX[(i >>  8) & 0xF];
				if (i & 0xFFF0) *r++ = HEX[(i >>  4) & 0xF];
				*r++ = HEX[(i >>  0) & 0xF];
				*r++ = ';';
				break;
		} // switch
	} // for

	*r = '\0';
	return result;
} // xmlify

#ifdef MAIN
int main(int argc, char **argv) {
	if (argc > 1)
		printf("%s\n%s\n", argv[1], xmlify(argv[1]));
	return 0;
} // main
#endif
