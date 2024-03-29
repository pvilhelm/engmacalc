
GPP = g++
GCC = gcc
CPPFLAGS = -g3 -ggdb3 -std=gnu++20
CFLAGS = -g
OBJ = emc.tab.o lex.yy.o compile.o emc.o Io.o util_string.o

engmac: engma.cc $(OBJ) lexer.h  libjitruntime.so
	$(GPP) $(CPPFLAGS) -L/mnt/c/repos/engmacalc/ engma.cc -o engmac $(OBJ) -ljitruntime -lgccjit -lstdc++fs
	sudo cp libjitruntime.so /usr/lib/libjitruntime.so  
	
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

util_string.o: util_string.cc util_string.hh
	$(GPP) $(CPPFLAGS) util_string.cc -c -o util_string.o

# Std lib
Io.o: Std/Io/Io.c
	$(GCC) $(CFLAGS) -fPIC -c Std/Io/Io.c
	
libjitruntime.so: jit_runtime.cc Io.o
	$(GPP) $(CPPFLAGS) -fPIC -c jit_runtime.cc
	$(GPP) $(CPPFLAGS) -shared -o libjitruntime.so jit_runtime.o Io.o

.PHONY : runtest
runtest :
	mkdir -p testrun
	cd testrun && runtest --srcdir ../testsuite --objdir ../ $(TEST_ARGS)

.PHONY : clean
clean :
	rm -f $(OBJ) engmac
	rm -Rf testrun