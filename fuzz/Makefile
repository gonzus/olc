all: OLC_Decode

fuzz_tests =\
	OLC_Decode.c \

CFLAGS += -std=c99
CFLAGS += -Wall
CFLAGS += -I..
#CFLAGS += -Wextra
CFLAGS += -Wno-comment

ALL_FLAGS += -g

%.o : %.c
	$(CC) -c $(ALL_FLAGS) $(CFLAGS) $(CPPFLAGS) -o $@ $<

OLC_Decode: OLC_Decode.o ../olc.c
	clang -g -fsanitize=fuzzer,address $^ -o $@

clean:
	rm -f *.o crash-* slow-unit-*
	rm -fr *.dSYM
	rm -f OLC_Decode
