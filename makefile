target:	player

player: player.c
		gcc player.c -o player -lpthread -lrt -Wall
clean:
		rm player