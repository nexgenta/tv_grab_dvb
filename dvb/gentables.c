/* Generate a C header from an IDL-alike description of a set of MPEG/DVB
 * sections and descriptors.
 *
 * Usage:
 *   gentables [SOURCEFILE]
 *
 * If SOURCEFILE is not specified, gentables reads from standard input and
 * does not output the usual preprocessor prologue and epilogue.
 */

/*
 * Copyright 2010 Mo McRoberts.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define IDENT_SIZE 64

static int line = 1, col = 1, tabsize = 4;
static const char *filename = "*stdin*";
static FILE *fin, *fout;
static char header_macro[64];
static int nextchar = -1;

#define CHECKNL(c) if(c == '\n') { line++; col = 1; } else if(c == '\t') { col += tabsize; } else { col++; }

typedef struct fieldset_struct fieldset_t;
typedef struct field_struct field_t;

struct field_struct
{
	char type[IDENT_SIZE];
	char ident[IDENT_SIZE];
	size_t bits;
};

struct fieldset_struct
{
	field_t *fields;
	size_t nfields;
	size_t nbits;
};

static void parse_block(const char *base, int *seq);

static void
unexpected_eof(const char *where)
{
	if(where)
	{
		fprintf(stderr, "%s:%d:%d: unexpected end of file while %s\n", filename, line, col, where);
	}
	else
	{
		fprintf(stderr, "%s:%d:%d: unexpected end of file\n", filename, line, col);
	}
	exit(EXIT_FAILURE);
}

static int
get_char(const char *where, int fatal)
{
	int c;
	
	if(nextchar != -1)
	{
		c = nextchar;
		nextchar = -1;
		return c;
	}
	if((c = getc(fin)) == EOF)
	{
		if(fatal)
		{
			unexpected_eof(where);
		}
		return -1;
	}
	return c;
}

static void
push_char(int ch)
{
	nextchar = ch;
}

static void
skip_whitespace(int fatal)
{
	int c, d, com;

	com = 0;
	while(1)
	{
		c = get_char(NULL, fatal);
		if(!com && c == '/')
		{
			d = get_char(NULL, fatal);
			if(d == '*')
			{
				CHECKNL(c);
				CHECKNL(d);
				com = 1;
				continue;
			}
			ungetc(d, fin);
			push_char(c);
			return;
		}
		else if(com)
		{
			CHECKNL(c);
			if(c == '*')
			{
				d = get_char(NULL, fatal);
				CHECKNL(d);
				if(d == '/')
				{
					com = 0;
					continue;
				}
			}
			continue;
		}
		else if(!isspace(c))
		{
			ungetc(c, fin);
			return;
		}
		CHECKNL(c);
	}
}

static void
expect_char(int ch)
{
	int c;
	char sbuf[32];
	
	sprintf(sbuf, "expecting '%c'", ch);
	c = get_char(sbuf, 1);
	if(ch != c)
	{
		fprintf(stderr, "%s:%d:%d: expecting '%c', found '%c'\n", filename, line, col, ch, c);
		exit(EXIT_FAILURE);
	}
	CHECKNL(c);
	return;   
}

static char *
parse_ident(void)
{
	static char ident[IDENT_SIZE];
	int c;
	size_t i;

	for(i = 0; i < sizeof(ident) - 8; i++)
	{
		c = get_char("parsing identifier", 1);
		if(!i)
		{
			if(!isalpha(c))
			{
				break;
			}
		}
		else
		{
			if(!isalnum(c) && c != '_')
			{
				break;
			}
		}
		CHECKNL(c);
		ident[i] = c;
	}
	ident[i] = 0;
	if(!i)
	{
		fprintf(stderr, "%s:%d:%d: expected identifier, found '%c'\n", filename, line, col, c);
		exit(EXIT_FAILURE);
	}
	ungetc(c, fin);
	skip_whitespace(1);
	return ident;   
}

static char *
parse_operator(void)
{
	static const char *opers[] = {
		"=", "==", "|=", "&=", "!=", "^", "++", "--", "/", "+", "-", "*",
		"**", "!", "%=", "^=", "~", "<", ">", ">=", "<=", "<<", ">>",
		NULL
	};
	static char oper[8];
	size_t i;
	int c;

	for(i = 0; i < 2; i++)
	{
		oper[i] = get_char("parsing operator", 1);
	}
	oper[i] = 0;
	for(c = 0; opers[c]; c++)
	{
		if(!strcmp(opers[c], oper))
		{
			CHECKNL(oper[0]);
			CHECKNL(oper[1]);
			skip_whitespace(1);
			return oper;
		}
	}
	ungetc(oper[1], fin);
	oper[1] = 0;
	for(c = 0; opers[c]; c++)
	{
		if(!strcmp(opers[c], oper))
		{
			CHECKNL(oper[0]);
			skip_whitespace(1);
			return oper;
		}
	}
	fprintf(stderr, "%s:%d:%d: expected operator, found '%c'\n", filename, line, col, oper[0]);
	exit(EXIT_FAILURE);	
}

static int
parse_dec(void)
{
	static char buffer[64];
	int c;
	size_t i;

	for(i = 0; i < sizeof(buffer) - 1; i++)
	{
		c = get_char("parsing integer", 1);
		if(!isdigit(c))
		{
			break;
		}
		CHECKNL(c);
		buffer[i] = c;		
	}
	buffer[i] = 0;
	if(!i)
	{
		fprintf(stderr, "%s:%d:%d: expected decimal integer, found '%c'\n", filename, line, col, c);
		exit(EXIT_FAILURE);
	}
	ungetc(c, fin);
	skip_whitespace(1);
	return atoi(buffer);
}

static void
write_field(field_t *field)
{
	if(field->bits == 8)
	{
		fprintf(fout, "\tuint8_t %s;\n", field->ident);
	}
	else
	{
		fprintf(fout, "\tuint8_t %s:%d;\n", field->ident, field->bits);
	}
}

static void
write_table(const char *ident, fieldset_t *fields)
{
	size_t i, r, nbits, start;
	int insec;

	fprintf(fout, "typedef struct %s_s %s_t;\n\n", ident, ident);
	fprintf(fout, "struct %s_s\n"
			"{\n", ident);
	nbits = 0;
	start = 0;
	insec = 0;
	for(i = 0; i < fields->nfields; i++)
	{
		if(!insec && fields->fields[i].bits != 8)
		{
			insec = 1;
			start = i;
			fprintf(fout, "#if BYTE_ORDER == BIG_ENDIAN\n");
		}
		write_field(&(fields->fields[i]));
		nbits += fields->fields[i].bits;
		if(!(nbits % 8) && insec)
		{
			insec = 0;
			fprintf(fout, "#else\n");
			for(r = i; (int) r >= (int) start; r--)
			{
				write_field(&(fields->fields[r]));
			}
			fprintf(fout, "#endif\n");
		}
	}
	fprintf(fout, "} PACKED_STRUCT;\n\n");
}

static void
free_fields(fieldset_t *fields)
{
	free(fields->fields);
	fields->fields = NULL;
	fields->nfields = 0;
	fields->nbits = 0;
}

static void
add_field(fieldset_t *fields, const char *type, const char *ident, size_t bits)
{
	static const char *hilo_suffix[] = {"lo", "hi"};
	const char *suf;
	char id[IDENT_SIZE];
	int c, hilo;
	size_t i, rem, fcount;
	
	if(!bits)
	{
		bits = 8;
	}
	c = 0;
	while(1)
	{
		if(c)
		{
			sprintf(id, "%s__%d", ident, c);
		}
		else
		{
			strcpy(id, ident);
		}
/*		fprintf(stderr, "trying %s\n", id); */
		for(i = 0; i < fields->nfields; i++)
		{
			if(!strcmp(id, fields->fields[i].ident))
			{
				break;
			}
		}
		if(i == fields->nfields)
		{
			break;
		}
		c++;
	}
	rem = 8 - (fields->nbits % 8);
	if(bits > rem)
	{
		i = bits - rem;
		fcount = 1 + (i / 8) + ((i % 8) ? 1 : 0);
	}
	else
	{
		fcount = 1;
	}
	if(NULL == (fields->fields = realloc(fields->fields, sizeof(field_t) * (fields->nfields + fcount))))
	{
		fprintf(stderr, "%s:%d:%d: Failed to allocate an extra field entry for %s %s:%d\n", filename, line, col, type, ident, bits);
		exit(EXIT_FAILURE);
	}
/*	fprintf(stderr, "[type=%s, ident=%s, bits=%d, i=%d]\n", type, id, bits, (bits / 8) + 2); */
	c = 0;
	hilo = 0;
	if(fcount == 2)
	{
		hilo = 1;
	}
	fcount--;	
	while(bits)
	{
		strncpy(fields->fields[fields->nfields].type, type, IDENT_SIZE - 1);
		rem = 8 - (fields->nbits % 8);
/*		fprintf(stderr, "[bits=%d, fields->nfields=%d, fields->nbits=%d, rem=%d]\n", bits, fields->nfields, fields->nbits, rem); */
		if(!c && rem >= bits)
		{
			/* Doesn't span any octets */
			strncpy(fields->fields[fields->nfields].ident, id, IDENT_SIZE - 1);
			fields->fields[fields->nfields].bits = bits;
			fields->nfields++;
			fields->nbits += bits;
			break;
		}
		if(hilo)
		{
			suf = hilo_suffix[fcount];
			snprintf(fields->fields[fields->nfields].ident, IDENT_SIZE, "%s_%s", id, suf);
		}
		else
		{
			snprintf(fields->fields[fields->nfields].ident, IDENT_SIZE, "%s_%d", id, fcount);
		}
		if(rem >= bits)
		{
			fields->fields[fields->nfields].bits = bits;
			fields->nbits += bits;
			fields->nfields++;
			break;
		}
		fields->fields[fields->nfields].bits = rem;
		fields->nbits += rem;
		bits -= rem;
		rem = 0;
		c++;
		fcount--;
		fields->nfields++;
	}
}

static void
parse_for(const char *base, int *seq)
{
	expect_char('(');
	parse_ident();
	parse_operator();
	parse_dec();
	expect_char(';');
	skip_whitespace(1);
	parse_ident();
	parse_operator();
	parse_ident();
	expect_char(';');
	skip_whitespace(1);
	parse_ident();
	parse_operator();
	expect_char(')');
	skip_whitespace(1);
	expect_char('{');
	parse_block(base, seq);
}


static void
parse_block(const char *base, int *seq)
{
	char ident[IDENT_SIZE], type[IDENT_SIZE], field[IDENT_SIZE];
	int c;
	fieldset_t fields;
	size_t width;

	memset(&fields, 0, sizeof(fields));
	while(1)
	{
		skip_whitespace(1);
		c = get_char("parsing section", 1);
		if(c == '}')
		{
			break;
		}
		ungetc(c, fin);
		strcpy(type, parse_ident());
/*		fprintf(stderr, "type = %s\n", type); */
		if(!strcmp(type, "for"))
		{
			if(fields.nfields)
			{
				write_table(ident, &fields);
				(*seq)++;
				free_fields(&fields);
			}
			parse_for(base, seq);
		}
		else if(!strcmp(type, "if"))
		{
			if(fields.nfields)
			{
				write_table(ident, &fields);
				(*seq)++;
				free_fields(&fields);
			}
			/* parse_if() */
		}
		else if(!strcmp(type, "while"))
		{
			if(fields.nfields)
			{
				write_table(ident, &fields);
				(*seq)++;
				free_fields(&fields);
			}
			/* parse_while() */
		}
		else
		{
			c = get_char("parsing field", 1);
			if(c == '(')
			{
				/* variable descriptor() */
				skip_whitespace(1);
				expect_char(')');
				skip_whitespace(1);
				expect_char(';');
				skip_whitespace(1);
				if(fields.nfields)
				{
					write_table(ident, &fields);
					(*seq)++;
					free_fields(&fields);
				}
			}
			else
			{
				ungetc(c, fin);
				if(!fields.nfields)
				{
					if(*seq)
					{
						snprintf(ident, IDENT_SIZE, "%s_%d", base, *seq);
					}
					else
					{
						strcpy(ident, base);
					}
				}
				strcpy(field, parse_ident());
/*			fprintf(stderr, "field = %s\n", field); */
				c = get_char("parsing field", 1);
				if(c == ':')
				{
					width = parse_dec();
/*				fprintf(stderr, "found field length = %d\n", width); */
				}
				else
				{
					width = 0;
/*				fprintf(stderr, "field with no length\n"); */
				}
				add_field(&fields, type, field, width);
				expect_char(';');
			}
		}
		skip_whitespace(1);
		do
		{
			c = get_char("parsing section", 1);
		}
		while(c == ';');
		ungetc(c, fin);			
	}
/*	fprintf(stderr, "[found end of table]\n"); */
	if(fields.nfields)
	{
		write_table(ident, &fields);
		(*seq)++;
		free_fields(&fields);
	}
}

static void
parse_table(void)
{
	char base[IDENT_SIZE];
	int seq;

	strcpy(base, parse_ident());
	expect_char('(');
	skip_whitespace(1);
	expect_char(')');
	skip_whitespace(1);
	expect_char('{');
/*	fprintf(stderr, "[starting table: '%s']\n", ident); */
	seq = 0;
	parse_block(base, &seq);
}
			
int
main(int argc, char **argv)
{
	int c;
	char *p;
	   
	(void) argc;
	(void) argv;

	fin = stdin;
	fout = stdout;

	if(argc > 1)
	{
		if(argc == 2)
		{
			if(NULL == (fin = fopen(argv[1], "r")))
			{
				perror(argv[1]);
				exit(EXIT_FAILURE);
			}
			if((p = strrchr(argv[1], '.')))
			{
				if(!strcasecmp(p, ".idl"))
				{
					*p = 0;
				}
			}
			p = strrchr(argv[1], '/');
			if(p)
			{
				p++;
			}
			else
			{
				p = argv[1];
			}
			for(c = 0; c < 32 && *p; p++)
			{
				if(isalnum(*p))
				{
					header_macro[c] = toupper(*p);
				}
				else
				{
					header_macro[c] = '_';
				}
				c++;
			}
			header_macro[c] = '_';
			c++;
			header_macro[c] = 'H';
			c++;
			header_macro[c] = '_';
			c++;
			header_macro[c] = 0;
		}
		else
		{
			fprintf(stderr, "Usage: %s [INFILE]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	if(header_macro[0])
	{
		fprintf(fout, "/* Generated automatically by gentables */\n\n"
				"#ifndef %s\n"
				"# define %s\n\n", header_macro, header_macro);
		fprintf(fout, "# undef PACKED_STRUCT\n"
				"# ifdef __GNUC__\n"
				"#  define PACKED_STRUCT __attribute__((__packed__))\n"
				"# else\n"
				"#  define PACKED_STRUCT /* */\n"
				"# endif\n\n");
	}
	while(1)
	{
		skip_whitespace(0);
		if((c = getc(fin)) == EOF)
		{
			break;
		}
		if(isalpha(c))
		{
			ungetc(c, fin);
			parse_table();
			continue;
		}
		fprintf(stderr, "%s:%d:%d: expecting section identifier, found '%c'\n", filename, line, col, c);
		exit(EXIT_FAILURE);
	}
	if(header_macro[0])
	{
		fprintf(fout, "\n#endif /*!%s*/\n", header_macro);
	}
	return 0;
}
