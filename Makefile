default: gitdiff.c commitlist.c
	gcc -g -o gitdiff gitdiff.c commitlist.c -lcurses

clean:
	rm -f gitdiff 

