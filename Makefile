DEF=-DSEND_INTERVAL=1000000
send:send.c
	$(CC) -g send.c -o send $(DEF)
