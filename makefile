
GPP = g++
CPPFLAGS = -g

engmac: engma.cc emc.o lex.yy.o emc.tab.o lexer.h
	$(GPP) $(CPPFLAGS) engma.cc  emc.o -o engmac emc.tab.o lex.yy.o
	
lex.yy.c: emc_lexer.l
	flex --header-file=lexer.h emc_lexer.l
	
lex.yy.o: lex.yy.c
	$(GPP) $(CPPFLAGS) lex.yy.c -c -o lex.yy.o

emc.tab.c: emc.y
	bison -v -d emc.y
		
emc.tab.o: emc.tab.c
	$(GPP) $(CPPFLAGS) emc.tab.c -c -o emc.tab.o
	
emc.o: emc.cc
	$(GPP) $(CPPFLAGS) emc.cc -c -o emc.o


.PHONY : clean
clean :
	rm emc.o emc.tab.o lex.yy.o engmac