/* Generated by re2c */
#line 1 "config/config5.re"
// ignored
#line 6 "config/config5.re"

// code

#line 9 "config/config5.c"
{
	YYCTYPE yych;
	if ((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	switch (yych) {
	case 'a':	goto yy4;
	default:	goto yy2;
	}
yy2:
	++YYCURSOR;
yy3:
#line 11 "config/config5.re"
	{ return 0; }
#line 23 "config/config5.c"
yy4:
	yych = *(YYMARKER = ++YYCURSOR);
	switch (yych) {
	case 'b':	goto yy5;
	default:	goto yy3;
	}
yy5:
	++YYCURSOR;
	if ((YYLIMIT - YYCURSOR) < 3) YYFILL(3);
	yych = *YYCURSOR;
	switch (yych) {
	case 'b':	goto yy5;
	case 'c':	goto yy8;
	default:	goto yy7;
	}
yy7:
	YYCURSOR = YYMARKER;
	goto yy3;
yy8:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'c':	goto yy9;
	default:	goto yy7;
	}
yy9:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'c':	goto yy10;
	default:	goto yy7;
	}
yy10:
	++YYCURSOR;
#line 10 "config/config5.re"
	{ return 1; }
#line 58 "config/config5.c"
}
#line 13 "config/config5.re"

// ignored
#line 19 "config/config5.re"

// max
#define YYMAXFILL 3
#line 25 "config/config5.re"

// eof
