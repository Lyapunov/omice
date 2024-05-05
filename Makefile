CC = g++
CFLAGS=-Wall -std=c++11
APP=omice
MERGE=omice.cpp
SRC=src

CPP_FILES := $(wildcard *.cpp *.h)

all: $(APP)

$(APP): $(CPP_FILES)
	rm -rf $(MERGE)
	cd $(SRC); ../merge_main.pl main.cpp ../$(MERGE)
	$(CC) $(CFLAGS) $(MERGE) -o $(APP)
	chmod 755 $(APP)

clean:
	rm -rf $(APP) $(MERGE)
