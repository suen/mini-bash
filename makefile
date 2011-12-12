INSTALL = /usr/bin


minish: shell.c
	gcc -o minish shell.c

install:
	@echo Only root can install the file to $(INSTALL)
	cp minish $(INSTALL)
