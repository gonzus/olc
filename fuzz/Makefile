all: first

C_TESTS =\
	OLC_Decode.c \
	OLC_CodeLength.c \
	OLC_IsValid.c \
	OLC_IsShort.c \
	OLC_IsFull.c \
	OLC_Shorten.c \
	OLC_RecoverNearest.c \

EXE_TESTS = $(C_TESTS:.c=)

CFLAGS += -std=c99
CFLAGS += -Wall
CFLAGS += -I..
#CFLAGS += -Wextra
CFLAGS += -Wno-comment

ALL_FLAGS += -g

%.o : %.c
	$(CC) -c $(ALL_FLAGS) $(CFLAGS) $(CPPFLAGS) -o $@ $<

first: $(EXE_TESTS)

OLC_Decode: OLC_Decode.o ../olc.c
	clang -g -fsanitize=fuzzer,address $^ -o $@

OLC_CodeLength: OLC_CodeLength.o ../olc.c
	clang -g -fsanitize=fuzzer,address $^ -o $@

OLC_IsValid: OLC_IsValid.o ../olc.c
	clang -g -fsanitize=fuzzer,address $^ -o $@

OLC_IsShort: OLC_IsShort.o ../olc.c
	clang -g -fsanitize=fuzzer,address $^ -o $@

OLC_IsFull: OLC_IsFull.o ../olc.c
	clang -g -fsanitize=fuzzer,address $^ -o $@

OLC_Shorten: OLC_Shorten.o ../olc.c
	clang -g -fsanitize=fuzzer,address $^ -o $@

OLC_RecoverNearest: OLC_RecoverNearest.o ../olc.c
	clang -g -fsanitize=fuzzer,address $^ -o $@

clean:
	rm -f *.o crash-* slow-unit-*
	rm -fr *.dSYM
	rm -f $(EXE_TESTS)
