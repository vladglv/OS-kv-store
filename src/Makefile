# Custom makefile

CC=clang -g -O0 -fPIC -D_XOPEN_SOURCE=700 -DSTANDALONE=1 -std=c11 -Wmain -W -Wall -Wthread-safety -pedantic -pedantic-errors 

CCS0=-fsanitize=address,undefined,leak
CCS1=-fsanitize=thread,undefined
CCS2=-fsanitize=memory,undefined

# For CCS2:
# Use to get meaningful stack traces in error messages add '-fno-omit-frame-pointer'.
# To get perfect stack traces you may need to disable in-lining (just use -O1) and tail call elimination '-fno-optimize-sibling-calls'.

LIBRARY=-lrt -pthread
EXECUTABLE=kv
CLEANSCRIPT=clean.sh
SOURCES=kv.c

all: valgrind address thread memory remove_dups

remove_dups:
	echo "`cat $(CLEANSCRIPT) | sort | uniq`" > $(CLEANSCRIPT)

valgrind:
	$(CC) -o $(EXECUTABLE)_$@ $(SOURCES) $(LIBRARY)
	echo rm $(EXECUTABLE)_$@ >> $(CLEANSCRIPT)

address:
	$(CC) $(CCS0) -o $(EXECUTABLE)_$@ $(SOURCES) $(LIBRARY)
	echo rm $(EXECUTABLE)_$@ >> $(CLEANSCRIPT)

thread:
	$(CC) $(CCS1) -o $(EXECUTABLE)_$@ $(SOURCES) $(LIBRARY)
	echo rm $(EXECUTABLE)_$@ >> $(CLEANSCRIPT)

memory:
	$(CC) $(CCS2) -o $(EXECUTABLE)_$@ $(SOURCES) $(LIBRARY)
	echo rm $(EXECUTABLE)_$@ >> $(CLEANSCRIPT)

clean:
	sh $(CLEANSCRIPT)
	rm $(CLEANSCRIPT)
