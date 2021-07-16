
GPP = g++
CPPFLAGS = -g
OBJ = emc.tab.o lex.yy.o compile.o emc.o

engmac: engma.cc $(OBJ) lexer.h  libjitruntime.so
	$(GPP) $(CPPFLAGS) -L/mnt/c/repos/engmacalc/ engma.cc -o engmac $(OBJ) -ljitruntime -lgccjit
	sudo cp /mnt/c/repos/engmacalc/libjitruntime.so /usr/lib/libjitruntime.so  
	
lex.yy.c: emc_lexer.l emc.hh
	flex --header-file=lexer.h emc_lexer.l
	
lex.yy.o: lex.yy.c
	$(GPP) $(CPPFLAGS) lex.yy.c -c -o lex.yy.o

emc.tab.c: emc.y emc.hh
	bison -v -d emc.y
		
emc.tab.o: emc.tab.c
	$(GPP) $(CPPFLAGS) emc.tab.c -c -o emc.tab.o
	
emc.o: emc.cc emc.hh
	$(GPP) $(CPPFLAGS) emc.cc -c -o emc.o
	
compile.o: compile.cc compile.hh emc.hh
	$(GPP) $(CPPFLAGS) compile.cc -c -o compile.o
	
libjitruntime.so: jit_runtime.cc
	$(GPP) $(CPPFLAGS) -fPIC -c jit_runtime.cc
	$(GPP) $(CPPFLAGS) -shared -o libjitruntime.so jit_runtime.o

.PHONY : clean
clean :
	rm $(OBJ) engmac