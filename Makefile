all:
	gcc midi-asdf.c -lasound -o midi-asdf
clean:
	rm midi-asdf
