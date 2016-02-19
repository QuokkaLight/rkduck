all: crumbs

crumbs: crumbs.o
	gcc crumbs.o -o crumbs

crumbs.o: crumbs.c
	gcc -c crumbs.c

clean:
	rm crumbs crumbs.o
