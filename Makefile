all:
	gcc -g -Wall odin.c -o odin

brick:
	gcc -g -Wall -DVOID_MY_WARRANTY odin.c -o odin

