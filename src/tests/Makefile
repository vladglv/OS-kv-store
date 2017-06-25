#Enter Make test1 for test 1
#Enter Make test2 for test 2

CC=clang
LIBS=-lrt -lpthread
CFLAGS=-g -O0 -fPIC -D_XOPEN_SOURCE=700 -std=c11 -Wmain -W -Wall -Wthread-safety -pedantic -pedantic-errors
CCS=-fsanitize=address,undefined,leak
SOURCE1=../kv.c comp310_a2_test1.c
SOURCE2=../kv.c comp310_a2_test2.c
SOURCE3=../kv.c read_eval.c

EXEC1=os_test1
EXEC11=os_test1_vg
EXEC2=os_test2
EXEC21=os_test2_vg
EXEC3=os_read_eval
EXEC31=os_read_eval_vg

all: test1 test2 read_eval

test1: $(SOURCE1)
	$(CC) $(CCS) -o $(EXEC1) $(CFLAGS) $(SOURCE1) $(LIBS)
	$(CC) -o $(EXEC11) $(CFLAGS) $(SOURCE1) $(LIBS)

test2: $(SOURCE2)
	$(CC) $(CCS) -o $(EXEC2) $(CFLAGS) $(SOURCE2) $(LIBS)
	$(CC) -o $(EXEC21) $(CFLAGS) $(SOURCE2) $(LIBS)

read_eval: $(SOURCE3)
	$(CC) $(CCS) -o $(EXEC3) $(CFLAGS) $(SOURCE3) $(LIBS)
	$(CC) -o $(EXEC31) $(CFLAGS) $(SOURCE3) $(LIBS)

clean:
	rm $(EXEC1) $(EXEC11) $(EXEC2) $(EXEC21) $(EXEC3) $(EXEC31)
