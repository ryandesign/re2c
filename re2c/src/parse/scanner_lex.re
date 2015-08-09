#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>

#include "src/ir/dfa/dfa.h"
#include "src/globals.h"
#include "src/parse/extop.h"
#include "src/parse/parser.h"
#include "src/parse/scanner.h"
#include "y.tab.h"

extern YYSTYPE yylval;

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#define	YYCTYPE		unsigned char
#define	YYCURSOR	cursor
#define	YYLIMIT		lim
#define	YYMARKER	ptr
#define	YYCTXMARKER ctx
#define	YYFILL(n)	{cursor = fill(cursor, n);}

#define	RETURN(i)	{cur = cursor; return i;}

namespace re2c
{

/*!re2c
zero    = "\000";
dot     = .;
esc     = dot \ [\\];
istring = "[" "^" ((esc \ [\]]) | "\\" dot)* "]" ;
cstring = "["     ((esc \ [\]]) | "\\" dot)* "]" ;
dstring = "\""    ((esc \ ["] ) | "\\" dot)* "\"";
sstring = "'"     ((esc \ ['] ) | "\\" dot)* "'" ;
letter  = [a-zA-Z];
digit   = [0-9];
lineno  = [1-9] digit*;
number  = "0" | ("-"? [1-9] digit*);
name    = (letter|digit|"_")+;
cname   = ":" name;
space   = [ \t];
ws      = (space | [\r\n]);
eol     = ("\r\n" | "\n");
config  = "re2c" cname+ ("@" name)?;
value   = [^\r\n; \t]* | dstring | sstring;
lineinf = lineno (space+ dstring)? eol;
*/

Scanner::ParseMode Scanner::echo()
{
	char *cursor = cur;
	bool ignore_eoc = false;
	int  ignore_cnt = 0;

	if (eof && cursor == eof) // Catch EOF
	{
		return Stop;
	}

	tok = cursor;
echo:
/*!re2c
   beginRE     =  "%{" | "/*!re2c";
   beginRE     {
					if (rFlag)
					{
						fatal("found standard 're2c' block while using -r flag");
					}
					if (!(DFlag || flag_skeleton))
					{
						out.write((const char*)(tok), (const char*)(&cursor[-7]) - (const char*)(tok));
					}
					tok = cursor;
					RETURN(Parse);
				}
	"/*!rules:re2c"	{
					if (rFlag)
					{
						mapCodeName.clear();
					}
					else
					{
						fatal("found 'rules:re2c' block without -r flag");
					}
					tok = cursor;
					RETURN(Rules);
				}
	"/*!use:re2c"	{
					if (!rFlag)
					{
						fatal("found 'use:re2c' block without -r flag");
					}
					reuse();
					if (!(DFlag || flag_skeleton))
					{
						out.write((const char*)(tok), (const char*)(&cursor[-11]) - (const char*)(tok));
					}
					tok = cursor;
					RETURN(Reuse);
				}
	"/*!max:re2c" {
					if (!DFlag)
					{
						out.insert_yymaxfill ();
					}
					tok = pos = cursor;
					ignore_eoc = true;
					goto echo;
				}
	"/*!getstate:re2c" {
					tok = pos = cursor;
					out.insert_state_goto (topIndent);
					ignore_eoc = true;
					goto echo;
				}
	"/*!ignore:re2c" {
					tok = pos = cursor;
					ignore_eoc = true;
					goto echo;
				}
	"/*!types:re2c" {
					tok = pos = cursor;
					ignore_eoc = true;
					if (!DFlag)
					{
						out.insert_line_info ();
						out << "\n";
						out.insert_types ();
						out << "\n";
						out.write_line_info (cline, get_fname ().c_str ());
					}
					goto echo;
				}
	"*" "/"	"\r"? "\n"	{
					cline++;
					if (ignore_eoc)
					{
						if (ignore_cnt)
						{
							out.write_line_info (cline, get_fname ().c_str ());
						}
						ignore_eoc = false;
						ignore_cnt = 0;
					}
					else if (!(DFlag || flag_skeleton))
					{
						out.write((const char*)(tok), (const char*)(cursor) - (const char*)(tok));
					}
					tok = pos = cursor;
					goto echo;
				}
	"*" "/"		{
					if (ignore_eoc)
					{
						if (ignore_cnt)
						{
							out << "\n";
							out.write_line_info (cline, get_fname ().c_str ());
						}
						ignore_eoc = false;
						ignore_cnt = 0;
					}
					else if (!(DFlag || flag_skeleton))
					{
						out.write((const char*)(tok), (const char*)(cursor) - (const char*)(tok));
					}
					tok = pos = cursor;
					goto echo;
				}
	"\n" space* "#" space* "line" space+ / lineinf {
					set_sourceline(cursor);
					goto echo;
				}
	"\n"		{
					if (ignore_eoc)
					{
						ignore_cnt++;
					}
					else if (!(DFlag || flag_skeleton))
					{
						out.write((const char*)(tok), (const char*)(cursor) - (const char*)(tok));
					}
					tok = pos = cursor;
					cline++;
					goto echo;
				}
	zero		{
					if (!(ignore_eoc || DFlag || flag_skeleton))
					{
						out.write((const char*)(tok), (const char*)(cursor) - (const char*)(tok) - 1);
						// -1 so we don't write out the \0
					}
					if(cursor == eof)
					{
						RETURN(Stop);
					}
				}
	*			{
					goto echo;
				}
*/
}

int Scanner::scan()
{
	char *cursor = cur;
	uint32_t depth;

scan:
	tchar = cursor - pos;
	tline = cline;
	tok = cursor;
	switch (lexer_state)
	{
		case LEX_NORMAL:       goto start;
		case LEX_CONFIG:       goto config;
		case LEX_CONFIG_VALUE: goto value;
		case LEX_FLEX_NAME:    goto flex_name;
	}

start:
/*!re2c
	"{"			{
					depth = 1;
					goto code;
				}

	":" / "=>"	{
					RETURN(*tok);
				}

	":="		{
					cur = cursor;
					tok += 2; /* skip ":=" */
					depth = 0;
					goto code;
				}

        "//"            {
				goto nextLine;
			}
	"/*"		{
					depth = 1;
					goto comment;
				}

   endRE    =  "%}" | "*/";
   endRE    {
					tok = cursor;
					RETURN(0);
				}

	dstring		{
					cur = cursor;
					SubStr s (tok + 1, cur - tok - 2);
					if (bCaseInsensitive || bCaseInverted)
					{
						yylval.regexp = strToCaseInsensitiveRE (s);
					}
					else
					{
						yylval.regexp = strToRE (s);
					}
					return STRING;
				}

	sstring		{
					cur = cursor;
					SubStr s (tok + 1, cur - tok - 2);
					if (bCaseInverted)
					{
						yylval.regexp = strToRE (s);
					}
					else
					{
						yylval.regexp = strToCaseInsensitiveRE (s);
					}
					return STRING;
				}

	"\""		{
					fatal("unterminated string constant (missing \")");
				}
	"'"			{
					fatal("unterminated string constant (missing ')");
				}

	istring		{
					cur = cursor;
					SubStr s (tok, cur - tok);
					yylval.regexp = invToRE (s);
					return RANGE;
				}

	cstring		{
					cur = cursor;
					SubStr s (tok, cur - tok);
					yylval.regexp = ranToRE (s);
					return RANGE;
				}

	"["			{
					fatal("unterminated range (missing ])");
				}

	"<>" / (space* ("{" | "=>" | ":=")) {
					RETURN(NOCOND);
				}
	"<!"		{
					RETURN(SETUP);
				}
	[<>,()|=;/\\]	{
					RETURN(*tok);
				}

	"*"			{
					yylval.op = *tok;
					RETURN(STAR);
				}
	[+?]		{
					yylval.op = *tok;
					RETURN(CLOSE);
				}

	"{0,}"		{
					yylval.op = '*';
					RETURN(CLOSE);
				}

	"{" [0-9]+ "}"	{
					yylval.extop.minsize = atoi((char *)tok+1);
					yylval.extop.maxsize = atoi((char *)tok+1);
					RETURN(CLOSESIZE);
				}

	"{" [0-9]+ "," [0-9]+ "}"	{
					yylval.extop.minsize = atoi((char *)tok+1);
					yylval.extop.maxsize = MAX(yylval.extop.minsize,atoi(strchr((char *)tok, ',')+1));
					RETURN(CLOSESIZE);
				}

	"{" [0-9]+ ",}"		{
					yylval.extop.minsize = atoi((char *)tok+1);
					yylval.extop.maxsize = -1;
					RETURN(CLOSESIZE);
				}

	"{" [0-9]* ","		{
					fatal("illegal closure form, use '{n}', '{n,}', '{n,m}' where n and m are numbers");
				}

	"{" name "}"	{
					if (!FFlag) {
						fatal("curly braces for names only allowed with -F switch");
					}
					cur = cursor;
					yylval.str = new std::string (tok + 1, cur - tok - 2);
					return ID;
				}

	config		{
					cur = cursor;
					tok += 5; /* skip "re2c:" */
					lexer_state = LEX_CONFIG;
					yylval.str = new std::string (tok, cur - tok);
					return CONFIG;
				}

	name / (space+ [^=>,])	{
					cur = ptr > tok ? ptr - 1 : cursor;
					yylval.str = new std::string (tok, cur - tok);
					if (FFlag)
					{
						lexer_state = LEX_FLEX_NAME;
						return FID;
					}
					else
					{
						return ID;
					}
				}

	name / (space* [=>,])	{
					cur = ptr > tok ? ptr - 1 : cursor;
					yylval.str = new std::string (tok, cur - tok);
					return ID;
				}

	name / [^]	{
					if (!FFlag) {
						cur = cursor;
						yylval.str = new std::string (tok, cur - tok);
						return ID;
					} else {
						/* Add one char in front and one behind instead of 's or "s */
						cur = cursor;
						SubStr s (tok, cur - tok);
						if (bCaseInsensitive || bCaseInverted)
						{
							yylval.regexp = strToCaseInsensitiveRE (s);
						}
						else
						{
							yylval.regexp = strToRE (s);
						}
						return STRING;
					}
				}

	"."			{
					cur = cursor;
					yylval.regexp = mkDot();
					return RANGE;
				}

	space+		{
					goto scan;
				}

	eol space* "#" space* "line" space+ / lineinf {
					set_sourceline(cursor);
					goto scan;
				}

	eol			{
					if (cursor == eof) RETURN(0);
					pos = cursor;
					cline++;
					goto scan;
				}

	*			{
					fatalf("unexpected character: '%c'", *tok);
					goto scan;
				}
*/

flex_name:
/*!re2c
	eol
	{
		YYCURSOR = tok;
		lexer_state = LEX_NORMAL;
		return FID_END;
	}
	*
	{
		YYCURSOR = tok;
		goto start;
	}
*/

code:
/*!re2c
	"}"			{
					if (depth == 0)
					{
						fatal("Curly braces are not allowed after ':='");
					}
					else if (--depth == 0)
					{
						cur = cursor;
						yylval.code = new Code (tok, cur - tok, get_fname (), tline);
						return CODE;
					}
					goto code;
				}
	"{"			{
					if (depth == 0)
					{
						fatal("Curly braces are not allowed after ':='");
					}
					else
					{
						++depth;
					}
					goto code;
				}
	"\n" space* "#" space* "line" space+ / lineinf {
					set_sourceline(cursor);
					goto code;
				}
	"\n" /  ws	{
					if (depth == 0)
					{
						goto code;
					}
					else if (cursor == eof)
					{
						fatal("missing '}'");
					}
					pos = cursor;
					cline++;
					goto code;
				}
	"\n"		{
					if (depth == 0)
					{
						cur = cursor;
						tok += strspn(tok, " \t\r\n");
						while (cur > tok && strchr(" \t\r\n", cur[-1]))
						{
							--cur;
						}
						yylval.code = new Code (tok, cur - tok, get_fname (), tline);
						return CODE;
					}
					else if (cursor == eof)
					{
						fatal("missing '}'");
					}
					pos = cursor;
					cline++;
					goto code;
				}
	zero		{
					if (cursor == eof)
					{
						if (depth)
						{
							fatal("missing '}'");
						}
						RETURN(0);
					}
					goto code;
				}
	dstring | sstring	{
					goto code;
				}
	*			{
					goto code;
				}
*/

comment:
/*!re2c
	"*/"		{
					if (--depth == 0)
					{
						goto scan;
					}
					else
					{
						goto comment;
					}
				}
	"/*"		{
					++depth;
					fatal("ambiguous /* found");
					goto comment;
				}
	"\n" space* "#" space* "line" space+ / lineinf {
					set_sourceline(cursor);
					goto comment;
				}
	"\n"		{
					if (cursor == eof)
					{
						RETURN(0);
					}
					tok = pos = cursor;
					cline++;
					goto comment;
				}
	*			{
					if (cursor == eof)
					{
						RETURN(0);
					}
					goto comment;
				}
*/

nextLine:
/*!re2c                                  /* resync emacs */
   "\n"     { if(cursor == eof) {
                  RETURN(0);
               }
               tok = pos = cursor;
               cline++;
               goto scan;
            }
   *        {  if(cursor == eof) {
                  RETURN(0);
               }
               goto nextLine;
            }
*/

config:
/*!re2c
	space+		{
					goto config;
				}
	"=" space*	{
					lexer_state = LEX_CONFIG_VALUE;
					cur = cursor;
					RETURN('=');
				}
	*			{
					fatal("missing '='");
				}
*/

value:
/*!re2c
	number		{
					cur = cursor;
					yylval.number = atoi(std::string (tok, cur - tok).c_str());
					lexer_state = LEX_NORMAL;
					return NUMBER;
				}
	value		{
					cur = cursor;
					yylval.str = new std::string (tok, cur - tok);
					lexer_state = LEX_NORMAL;
					return VALUE;
				}
*/
}

static void escape (std::string & dest, const std::string & src)
{
	dest = src;
	size_t l = dest.length();
	for (size_t p = 0; p < l; ++p)
	{
		if (dest[p] == '\\')
		{
			dest.insert(++p, "\\");
			++l;
		}
	}
}

void Scanner::set_sourceline(char *& cursor) 
{
sourceline:
	tok = cursor;
/*!re2c	
	lineno		{
					cur = cursor;
					cline = atoi(std::string (tok, cur - tok).c_str());
					goto sourceline; 
				}
	dstring		{
					cur = cursor;
					escape (in.file_name, std::string (tok + 1, cur - tok - 2));
			  		goto sourceline; 
				}
	"\n"			{
  					if (cursor == eof)
  					{
						--cursor; 
					}
			  		else
			  		{
			  			pos = cursor; 
			  		}
			  		tok = cursor;
			  		return; 
				}
	*			{
  					goto sourceline;
  				}
*/
}

} // end namespace re2c