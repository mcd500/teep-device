CFLAGS := -g -O0 -Wall
LDLIBS := -lwebsockets
INCLUDES := -I../libteep/lib -I../libteep/libwebsockets/include -I../../optee_client/public -I../teep-agent-ta
CPPFLAGS := $(INCLUDES) -DPCTEST
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS := -L/usr/local/opt/libwebsockets/lib
endif

TARGET = pctest

# Makefile for TEEP Agent of UT
$(TARGET): $(TARGET).c ../teep-broker-app/main.c ../libteep/lib/main.c ../teep-agent-ta/teep_message.c

clean:
	$(RM) $(TARGET)

run: $(TARGET)
	./$(TARGET)
