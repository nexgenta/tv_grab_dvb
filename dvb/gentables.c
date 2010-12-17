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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

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
	char ident[IDENT_SIZE];
	char next_tag[IDENT_SIZE];
	size_t seq;
	field_t *fields;
	size_t nfields;
	size_t nbits;
};

static void parse_block(fieldset_t *fields);

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
	if(nextchar != -1)
	{
		ungetc(nextchar, fin);
	}
	nextchar = ch;
}

static void
skip_whitespace(int fatal)
{
	int c, d, com;

	com = 0;
	for(;;)
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
			push_char(d);
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
			push_char(c);
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
	push_char(c);
	skip_whitespace(1);
	return ident;   
}

static char *
parse_operator(int fatal)
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
	push_char(oper[1]);
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
	if(fatal)
	{
		fprintf(stderr, "%s:%d:%d: expected operator, found '%c'\n", filename, line, col, oper[0]);
		exit(EXIT_FAILURE);	
	}
	push_char(oper[0]);
	return NULL;
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
	push_char(c);
	skip_whitespace(1);
	return atoi(buffer);
}

static void
parse_expression(void)
{
	int c;

	for(;;)
	{
		c = get_char("parsing expression", 1);
		push_char(c);
		if(isdigit(c))
		{
			parse_dec();
		}
		else if(isalpha(c) || c == '_')
		{
			parse_ident();
		}
		else
		{
			break;
		}
		if(NULL == parse_operator(0))
		{
			break;
		}
	}
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
free_fields(fieldset_t *fields)
{
	free(fields->fields);
	fields->fields = NULL;
	fields->nfields = 0;
	fields->nbits = 0;
}

static void
write_table(fieldset_t *fields)
{
	char ident[IDENT_SIZE];
	size_t i, r, nbits, start;
	int insec;
	
	if(!fields->nfields)
	{
		return;
	}
	if(fields->seq)
	{
		sprintf(ident, "%s_%d", fields->ident, fields->seq);
	}
	else
	{
		strcpy(ident, fields->ident);
	}
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
	fields->seq++;
	free_fields(fields);
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
	for(;;)
	{
		if(c)
		{
			sprintf(id, "%s__%d", ident, c);
		}
		else
		{
			strcpy(id, ident);
		}
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
parse_for(fieldset_t *fields)
{
	expect_char('(');
	parse_expression();
	expect_char(';');
	skip_whitespace(1);
	parse_expression();
	expect_char(';');
	skip_whitespace(1);
	parse_expression();
	expect_char(')');
	skip_whitespace(1);
	expect_char('{');
	parse_block(fields);
}

static void
parse_ifelse(fieldset_t *fields, const char *what)
{
	int c;
	const char *s;

	skip_whitespace(1);
	if(!strcmp(what, "else"))
	{
		c = get_char("parsing else clause", 1);
		if(isalpha(c))
		{
			push_char(c);
			s = parse_ident();
			if(strcmp(s, "if"))
			{
				fprintf(stderr, "%s:%d:%d: expected 'if' or block, found '%s'\n", filename, line, col, s);
				exit(EXIT_FAILURE);
			}
			parse_ifelse(fields, "if");
		}
		else if(c == '{')
		{
			CHECKNL(c);
			parse_block(fields);
			return;
		}
		fprintf(stderr, "%s:%d:%d: expected 'if' or block, found '%c'\n", filename, line, col, c);
		exit(EXIT_FAILURE);
	}
	skip_whitespace(1);
	expect_char('(');
	parse_expression();
	expect_char(')');
	skip_whitespace(1);
	expect_char('{');
	parse_block(fields);
}

static void
parse_attributes(fieldset_t *fields)
{
	char attr[IDENT_SIZE], value[IDENT_SIZE];
	int c;

	for(;;)
	{
		skip_whitespace(1);
		strcpy(attr, parse_ident());
		expect_char('=');
		skip_whitespace(1);
		strcpy(value, parse_ident());
		if(!strcmp(attr, "tag"))
		{
			strcpy(fields->next_tag, value);
		}
		else
		{
			fprintf(stderr, "%s:%d:%d: unknown attribute '%s'\n", filename, line, col, attr);
			exit(EXIT_FAILURE);
		}
		skip_whitespace(1);
		c = get_char("parsing attributes", 1);
		CHECKNL(c);
		if(c == ']')
		{
			return;
		}
		if(c == ',')
		{
			continue;
		}
		fprintf(stderr, "%s:%d:%d: expected ']' or ','\n", filename, line, col);
		exit(EXIT_FAILURE);
	}
}

/* If src has transient attributes, save the old values to dest and
 * apply; otherwise zero dest appropriately such that a matching
 * pop_attrs() will do nothing.
 *
 * If dest is NULL, push_attrs() merely applies transient attributes
 * in src to its current status without saving anything for later
 * restoration.
 */
static void
push_attrs(fieldset_t *dest, fieldset_t *src)
{
	if(src && src->next_tag[0])
	{
		if(dest)
		{
			dest->seq = src->seq;
			strcpy(dest->ident, src->ident);
		}
		strcpy(src->ident, src->next_tag);	   
		src->next_tag[0] = 0;
		src->seq = 0;
		return;
	}
	if(dest)
	{
		dest->seq = 0;
		dest->ident[0] = 0;
	}
}

static void
pop_attrs(fieldset_t *dest, fieldset_t *src)
{
	if(dest->ident[0])
	{
		src->seq = dest->seq;
		strcpy(src->ident, dest->ident);
	}
}

static int
has_attrs(fieldset_t *src)
{
	if(src && src->next_tag[0])
	{
		return 1;
	}
	return 0;
}

static void
parse_block(fieldset_t *fields)
{
	char type[IDENT_SIZE], field[IDENT_SIZE];
	int c;
	size_t width;
	fieldset_t attrs;

	/* attrs stores attributes when they're temporarily overridden
	 * while we recurse into a block.
	 */
	memset(&attrs, 0, sizeof(attrs));
	for(;;)
	{
		skip_whitespace(1);
		c = get_char("parsing section", 1);
		if(c == '}')
		{
			break;
		}
		if(c == '[')
		{
			CHECKNL(c);
			parse_attributes(fields);
			continue;
		}
		if(c == '{')
		{
			CHECKNL(c);
			write_table(fields);
			push_attrs(&attrs, fields);
			parse_block(fields);
			pop_attrs(&attrs, fields);	  
			continue;
		}
		push_char(c);
		strcpy(type, parse_ident());
		if(!strcmp(type, "for"))
		{
			write_table(fields);
			push_attrs(&attrs, fields);
			parse_for(fields);
			pop_attrs(&attrs, fields);
		}
		else if(!strcmp(type, "if") || !strcmp(type, "else"))
		{
			write_table(fields);
			push_attrs(&attrs, fields);
			parse_ifelse(fields, type);
			pop_attrs(&attrs, fields);
		}
		else if(!strcmp(type, "while"))
		{
			write_table(fields);
			push_attrs(&attrs, fields);
			/* parse_while() */
			pop_attrs(&attrs, fields);
		}
		else
		{
			if(has_attrs(fields))
			{
				fprintf(stderr, "%s:%d:%d: expected block following attributes\n", filename, line, col);
				exit(EXIT_FAILURE);
			}
			c = get_char("parsing field", 1);
			if(c == '(')
			{
				CHECKNL(c);
				/* variable descriptor() */
				skip_whitespace(1);
				expect_char(')');
				skip_whitespace(1);
				expect_char(';');
				skip_whitespace(1);
				write_table(fields);
			}
			else
			{
				ungetc(c, fin);
				strcpy(field, parse_ident());
				c = get_char("parsing field", 1);
				if(c == ':')
				{
					width = parse_dec();
				}
				else
				{
					width = 0;
				}
				if(fields->next_tag[0])
				{
					strcpy(field, fields->next_tag);
					fields->next_tag[0] = 0;
				}
				add_field(fields, type, field, width);
				expect_char(';');
			}
		}
		skip_whitespace(1);
		do
		{
			c = get_char("parsing section", 1);
		}
		while(c == ';');
		push_char(c);
	}
	write_table(fields);
}

static void
parse_table(fieldset_t *attrs)
{
	fieldset_t fields;

	if(attrs)
	{
		memcpy(&fields, attrs, sizeof(fields));
	}
	else
	{
		memset(&fields, 0, sizeof(fields));
	}
	strcpy(fields.ident, parse_ident());
	push_attrs(NULL, &fields);
	expect_char('(');
	skip_whitespace(1);
	expect_char(')');
	skip_whitespace(1);
	expect_char('{');
	parse_block(&fields);
}
			
int
main(int argc, char **argv)
{
	int c;
	const char *p;
	fieldset_t fields;

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
			p = strrchr(argv[1], '/');
			if(p)
			{
				filename = strdup(p + 1);
			}
			else
			{
				filename = strdup(argv[1]);
			}
			p = filename;
			for(c = 0; c < 32 && *p; p++)
			{
				if(*p == '.' && !strcmp(p, ".idl"))
				{
					break;
				}
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
		fprintf(fout, "/* Generated automatically from %s by gentables */\n\n"
				"#ifndef %s\n"
				"# define %s\n\n", filename, header_macro, header_macro);
		fprintf(fout, "# undef PACKED_STRUCT\n"
				"# ifdef __GNUC__\n"
				"#  define PACKED_STRUCT __attribute__((__packed__))\n"
				"# else\n"
				"#  define PACKED_STRUCT /* */\n"
				"# endif\n\n");
	}
	memset(&fields, 0, sizeof(fields));
	for(;;)
	{
		skip_whitespace(0);
		if((c = get_char("at top level", 0)) == EOF)
		{
			break;
		}
		if(isalpha(c))
		{
			push_char(c);
			parse_table(&fields);
			memset(&fields, 0, sizeof(fields));
			continue;
		}
		else if(c == '[')
		{
			CHECKNL(c);
			parse_attributes(&fields);
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
