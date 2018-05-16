<?php


////////////////////////////////////////////////////////////////////////////////
// ALL SUPPORTED GETVAR FLAGS
////////////////////////////////////////////////////////////////////////////////
define('_GETVAR_BASIC',		0 <<  0);
define('_GETVAR_NOGET',		1 <<  0);
define('_GETVAR_NOPOST',	1 <<  1);
define('_GETVAR_HTMLSAFE',	1 <<  3);
define('_GETVAR_URLSAFE',	1 <<  4);
define('_GETVAR_NOTRIM',	1 <<  5);
define('_GETVAR_NODOUBLE',	1 <<  6);
define('_GETVAR_UNICODE',	1 <<  7);
define('_GETVAR_NULL',		1 <<  8);
define('_GETVAR_CURRENCY',	1 <<  9);




////////////////////////////////////////////////////////////////////////////////
// LIST OF ALL UTF-8 UNICODE SPACE CHARACTER TO CONVERT INTO NORMAL [SPACE]
////////////////////////////////////////////////////////////////////////////////
define('_GETVAR_SPACE', [
	"\xC2\xA0",		//	NON-BREAKING SPACE
	"\xE2\x80\x80",	//	EN QUAD
	"\xE2\x80\x81",	//	EM QUAD
	"\xE2\x80\x82",	//	EN SPACE
	"\xE2\x80\x83",	//	EM SPACE
	"\xE2\x80\x84",	//	THREE-PER-EM SPACE
	"\xE2\x80\x85",	//	FOUR-PER-EM SPACE
	"\xE2\x80\x86",	//	SIX-PER-EM SPACE
	"\xE2\x80\x87",	//	FIGURE SPACE
	"\xE2\x80\x88",	//	PUNCTUATION SPACE
	"\xE2\x80\x89",	//	THIN SPACE
	"\xE2\x80\x8A",	//	HAIR SPACE
	"\xE2\x80\x8B",	//	ZERO WIDTH SPACE
	"\xE2\x80\x8C",	//	ZERO WIDTH NON-JOINER
	"\xE2\x80\x8D",	//	ZERO WIDTH JOINER
	"\xE2\x80\xAF",	//	NARROW NO-BREAK SPACE
	"\xE2\x81\x9F",	//	MEDIUM MATHEMATICAL SPACE
	"\xE2\x81\xA0",	//	WORD JOINER
	"\xE3\x80\x80",	//	IDEOGRAPHIC SPACE
	"\xEF\xBB\xBF",	//	ZERO WIDTH NO-BREAK SPACE
]);
