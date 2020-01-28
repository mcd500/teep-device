CFLAGS := -g -O0 -Wall
LDLIBS := -lwebsockets
INCLUDES := -I../libteep/lib -I../libteep/libwebsockets/include -I../../optee_client/public -I../ta-aist-otrp
CPPFLAGS := $(INCLUDES)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS := -L/usr/local/opt/libwebsockets/lib
endif

TARGET = pctest

# Makefile for TEEP Agent of UT
$(TARGET): $(TARGET).c ../aist-otrp-testapp/main.c ../libteep/lib/main.c tee_client_api_stub.c ../ta-aist-otrp/teep_message.c

clean:
	$(RM) $(TARGET)

run: $(TARGET)
	./$(TARGET)
