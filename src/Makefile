TARGET := ../jsonchart
CC     := cc
CFLAGS := -Wall -Werror -O2 -D JSON_CHART_CLI
PREFIX?=/usr/local

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

all: $(TARGET)

install:
	mkdir -p $(PREFIX)/bin
	install -c -m 555 $(TARGET) $(PREFIX)/bin

OBJS = cstr.o cJSON.o chart.o jpath.o

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

clean:
	rm *.o
	rm $(TARGET)

chart.o: chart.c chart.h cJSON.h jpath.h
jpath.o: jpath.c jpath.h cJSON.h cstr.h
cstr.o: cstr.c cstr.h
cJSON.o: cJSON.c cJSON.h
