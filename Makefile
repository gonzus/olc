CPPFLAGS += -DMEM_CHECK=1

CFLAGS += -std=c99
CFLAGS += -Wall
#CFLAGS += -Wextra
CFLAGS += -Wno-comment

ALL_FLAGS += -g

%.o : %.c
	$(CC) -c $(ALL_FLAGS) $(CFLAGS) $(CPPFLAGS) -o $@ $<

gonzo: codearea.o olc.o example.o
	$(CC) $(ALL_FLAGS) $^ -o $@

clean:
	rm -f *.o
	rm -f gonzo
