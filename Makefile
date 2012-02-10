default: gitdiff.c commitlist.c keys.c
	gcc -g -o gitdiff gitdiff.c commitlist.c keys.c -lcurses

clean:
	rm -f gitdiff 

