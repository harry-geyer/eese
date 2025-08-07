
build/libs/nanocobs/cobs.o: libs/nanocobs/cobs.c
	mkdir -p $(@D)
	$(CC) -c $(CFLAGS) -Ilibs/nanocobs $< -o $@

$(NANOCOBS): build/libs/nanocobs/cobs.o
	mkdir -p $(@D)
	$(AR) rcs $@ $^
