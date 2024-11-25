CC = mpic++
CCFLAGS = 

ifeq ($(DEBUG),1)
	CCFLAGS += -DDEBUG
endif

main: main.cpp
	$(CC) $(CCFLAGS) main.cpp -o main
