
emc: emc_lexer.l emc.y emc.cc engma.cc
	bison -v -d emc.y
	flex --header-file=lexer.h emc_lexer.l
	g++ -g lex.yy.c emc.tab.c emc.cc engma.cc -lfl
