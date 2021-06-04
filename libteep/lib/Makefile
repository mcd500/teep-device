build = .

all: $(build)/libteep.a

$(build)/libteep.a: $(build)/libteep.o $(build)/qcbor-ext.o
	$(CROSS_COMPILE)ar cr $@ $^

$(build)/%.o: %.c
	$(CROSS_COMPILE)gcc -c -o $@ $< $(CFLAGS)

$(build)/libteep.o: libteep.c libteep.h qcbor-ext.h
$(build)/qcbor-ext.o: qcbor-ext.c qcbor-ext.h
