
emc: emc_lexer.l emc.y emc.cc
	bison -d emc.y
	flex emc_lexer.l
	g++ -g lex.yy.c emc.tab.c emc.cc -lfl
