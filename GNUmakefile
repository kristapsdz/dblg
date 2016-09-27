.SUFFIXES:	.html .in.xml .xml .js .min.js

ifeq ($(shell uname), Darwin)
PREFIX		?= /Users/kristaps/Sites
HTDOCS		 = $(PREFIX)
HTURI		 = /~kristaps
CGIBIN		 = $(PREFIX)
DATADIR		 = $(PREFIX)
LOGFILE		 = $(PREFIX)/dblg_log
STATIC		 =
LDFLAGS		 =
RDDIR		 = $(PREFIX)
CGINAME		 = dblg.cgi
CGIURI		 = /~kristaps/$(CGINAME)
AHASH		 = foobar
SECURE		 =
REPURI	 	 =
else
PREFIX	 	 = /var/www/vhosts/divelog.blue
HTDOCS		 = $(PREFIX)/htdocs
HTURI		 = 
CGIBIN		 = $(PREFIX)/cgi-bin
DATADIR		 = $(PREFIX)/data
LOGFILE		 = /logs/dblg-system.log
STATIC		 = -static
CFLAGS		+= -I/usr/local/include
LDFLAGS		+= -L/usr/local/lib
RDDIR		 = /vhosts/divelog.blue/data
CGINAME		 = dblg
CGIURI		 = /cgi-bin/$(CGINAME)
AHASH		 = $$2b$$10$$rQrWpJndeJAcIumy3kxugu5Dwrbtl9OOfVc7gN/ITBwrATYFGsL3y
SECURE		 = -DSECURE
REPURI	 	 = $(HTURI)/report/report.html
endif

OBJS	 = dblg.o
HTMLS	 = dblg.html
JSMINS	 = dblg.min.js
CSSS	 = dblg.css
AEMAIL	 = $(shell whoami)@$(shell hostname)
SITEURI	 = $(HTURI)/index.html
BLOGURI	 = $(HTURI)/blog.html
CFLAGS	+= -g -W -Wall -O2 $(SECURE)
CFLAGS	+= -DLOGFILE=\"$(LOGFILE)\"
CFLAGS	+= -DDATADIR=\"$(RDDIR)\"

all: dblg dblg.db $(HTMLS) $(JSMINS) $(CSSS)

updatecgi: all
	mkdir -p $(CGIBIN)
	mkdir -p $(HTDOCS)
	install -m 0555 dblg $(CGIBIN)/$(CGINAME)
	install -m 0444 $(HTMLS) $(JSMINS) $(CSSS) $(HTDOCS)

# Only allow this on dev.
ifeq ($(shell uname), Darwin)
installcgi: updatecgi
	mkdir -p $(DATADIR)
	rm -f $(DATADIR)/dblg.db
	rm -f $(DATADIR)/dblg.db-wal
	rm -f $(DATADIR)/dblg.db-shm
	install -m 0666 dblg.db $(DATADIR)
	chmod 0777 $(DATADIR)
endif

clean:
	rm -f dblg $(HTMLS) $(JSMINS) $(OBJS) dblg.db
	rm -rf dblg.dSYM

dblg.db: dblg.sql
	rm -f $@
	sed -e "s!@AEMAIL@!$(AEMAIL)!" \
	    -e "s!@AHASH@!$(AHASH)!" dblg.sql | sqlite3 $@

dblg: $(OBJS)
	$(CC) $(STATIC) -o $@ $(OBJS) $(LDFLAGS) -lkcgi -lkcgijson -lz -lksql -lsqlite3

.js.min.js:
	sed -e "s!@HTURI@!$(HTURI)!g" \
	    -e "s!@SITEURI@!$(SITEURI)!g" \
	    -e "s!@BLOGURI@!$(BLOGURI)!g" \
	    -e "s!@REPURI@!$(REPURI)!g" \
	    -e "s!@CGIURI@!$(CGIURI)!g" $< >$@

.xml.html:
	sed -e "s!@HTURI@!$(HTURI)!g" \
	    -e "s!@SITEURI@!$(SITEURI)!g" \
	    -e "s!@BLOGURI@!$(BLOGURI)!g" \
	    -e "s!@REPURI@!$(REPURI)!g" \
	    -e "s!@CGIURI@!$(CGIURI)!g" $< >$@
