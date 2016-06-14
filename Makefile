CC = g++
CFLAGS = -g -Wall

readSmt: lex.yy.o readSmt.tab.o utils.o readSmt.cpp
	$(CC) $(CFLAGS) lex.yy.o readSmt.tab.o readSmt.cpp -o readSmt

utils.o: utils.cpp
	$(CC) $(CFLAGS) -c utils.cpp

lex.yy.o: lex.yy.c
	$(CC) $(CFLAGS) -c lex.yy.c

lex.yy.c: readSmt.l readSmt.tab.o
	flex readSmt.l

readSmt.tab.o: readSmt.tab.c readSmt.tab.h
	$(CC) $(CFLAGS) -c readSmt.tab.c

readSmt.tab.c: readSmt.y
	bison -d readSmt.y

.PHONY : clean
clean:
	-rm -f *.o readSmt.tab.* readSmt lex.yy.* *~
