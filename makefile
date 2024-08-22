all:
	gcc mailclient.c -o mailclient
	gcc popserver.c -o popserver
	gcc smtpmail.c -o smtpmail

clean:
	rm mailclient popserver smtpmail