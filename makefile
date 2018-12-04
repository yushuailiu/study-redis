vpath %.h include src
vpath %.c add
vpath %.c unit_tests
vpath %.c src

objects = sds.o simple.o
test: main.c $(objects)
	gcc -I include -I src $^ -o test -lcheck

all-test: $(objects)

$(objects): %.o: %.c
	gcc -c -I include -I src $< -o $@

.PHONY: clean
clean-test:
	rm *.o test