default: gitdiff.c
	gcc -o gitdiff gitdiff.c -lcurses

clean:
	rm -f gitdiff 

