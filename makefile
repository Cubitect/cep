all:
	gcc -O3 cep.c attack.c bitboard.c board.c eval.c hash.c init.c io.c makemove.c misc.c movegen.c pv.c search.c xboard.c -o cep

