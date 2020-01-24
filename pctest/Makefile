CFLAGS := -g -O0 -Wall
LDLIBS := -lwebsockets
INCLUDES := -I../libaistotrp/lib -I../libaistotrp/libwebsockets/include -I../../optee_client/public
CPPFLAGS := $(INCLUDES)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS := -L/usr/local/opt/libwebsockets/lib
endif

TARGET = pctest

# Makefile for TEEP Agent of UT
$(TARGET): $(TARGET).c ../aist-otrp-testapp/main.c ../libaistotrp/lib/main.c tee_client_api_stub.c

clean:
	$(RM) $(TARGET)
