CFLAGS = \
	-Wall \
	-I$(OPTEE_DIR)/optee_client/public \
	-I$(TAREF_DIR)/build/include
LDFLAGS = \
	-L$(OPTEE_DIR)/out-br/target/usr/lib

LIBS = -lteec

all: App-optee

App-optee: App-optee.o
	$(CROSS_COMPILE)gcc -o $@ $^ $(LDFLAGS) $(LIBS)

App-optee.o: ../App-optee.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -c -o $@ $<

