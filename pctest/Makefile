CFLAGS := -g -O0 -Wall
LDLIBS := -lwebsockets
INCLUDES := -I../libteep/lib -I./libwebsockets/build/include -I../../optee_client/public -I../teep-agent-ta
CPPFLAGS := $(INCLUDES) -DPCTEST=1
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS := -L/usr/local/opt/libwebsockets/lib
endif
LDFLAGS+=-Llibwebsockets/build/lib

TARGET = pctest

# Makefile for TEEP Agent of UT
$(TARGET): $(TARGET).c ../teep-broker-app/main.c ../libteep/lib/main.c ../teep-agent-ta/teep_message.c ../teep-agent-ta/ta-store.c
#	$(CC) $(LDFLAGS) $(CFLAGS) $^ -o $@

clean:
	$(RM) $(TARGET)

run: $(TARGET)
	./$(TARGET)

run-jose: $(TARGET)
	./$(TARGET) --jose --tamurl http://127.0.0.1:3000/api/tam_jose
