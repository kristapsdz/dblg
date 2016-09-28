.SUFFIXES: .html .in.xml .xml .js .min.js

# File-system location (directory) of static media.
# HTDOCS = $(PREFIX)
#
# URL location (path) of static media.
# HTURI = /~kristaps
#
# File-system location (directory) of CGI script.
# CGIBIN = $(PREFIX)
#
# File-system location of database.
# DATADIR = $(PREFIX)
#
# Web-server relative location of system log file.
# LOGFILE = $(PREFIX)/dblg_log
#
# If on a static architecture, -static; otherwise empty.
# STATIC =
#
# Extract library -L flags.
# LDFLAGS =
#
# Web-server relative location of DATADIR.
# RDDIR = $(PREFIX)
#
# Name of installed CGI script.
# CGINAME = dblg.cgi
#
# URL location (filename) of CGI script.
# CGIURI = /~kristaps/$(CGINAME)
#
# Default password hash for administrator on installation.
# AHASH = foobar
#
# If on an HTTPS-only installation, should be "-DSECURE".
# SECURE =
#
# URI of server reports.
# REPURI =
#
# URI of blog display.
# BLOGURI =
#
# Main site that blog is sitting on.
# SITEURI =

# Here are the examples that I use.

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
BLOGURI	 	 =
SITEURI	 	 =
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
BLOGURI	 	 = $(HTURI)/blog.html
SITEURI	 	 = $(HTURI)/index.html
endif

OBJS	 = dblg.o
HTMLS	 = dblg.html blog.html
JSMINS	 = dblg.min.js blog.min.js
CSSS	 = dblg.css blog.css
AEMAIL	 = $(shell whoami)@$(shell hostname)
CFLAGS	+= -g -W -Wall -O2 $(SECURE)
CFLAGS	+= -DLOGFILE=\"$(LOGFILE)\"
CFLAGS	+= -DDATADIR=\"$(RDDIR)\"
VERSION	 = 0.1.0

all: dblg dblg.db $(HTMLS) $(JSMINS) $(CSSS)

installserver: all
	mkdir -p $(HTDOCS)
	install -m 0444 dblg.html dblg.min.js dblg.css $(HTDOCS)

installclient: all
	mkdir -p $(HTDOCS)
	install -m 0444 blog.html blog.min.js blog.css $(HTDOCS)

installwww: all
	mkdir -p $(HTDOCS)
	install -m 0444 $(HTMLS) $(JSMINS) $(CSSS) $(HTDOCS)

updatecgi: all
	mkdir -p $(CGIBIN)
	install -m 0555 dblg $(CGIBIN)/$(CGINAME)

installcgi: updatecgi
	mkdir -p $(DATADIR)
	rm -f $(DATADIR)/dblg.db
	rm -f $(DATADIR)/dblg.db-wal
	rm -f $(DATADIR)/dblg.db-shm
	install -m 0666 dblg.db $(DATADIR)
	chmod 0777 $(DATADIR)

clean:
	rm -f dblg $(HTMLS) $(JSMINS) $(OBJS) dblg.db
	rm -rf dblg.dSYM

dblg.db: dblg.sql
	rm -f $@
	sed -e "s!@AEMAIL@!$(AEMAIL)!" \
	    -e 's!@AHASH@!$(AHASH)!' dblg.sql | sqlite3 $@

dblg: $(OBJS)
	$(CC) $(STATIC) -o $@ $(OBJS) $(LDFLAGS) -lkcgi -lkcgijson -lz -lksql -lsqlite3

.js.min.js:
	sed -e "s!@HTURI@!$(HTURI)!g" \
	    -e "s!@SITEURI@!$(SITEURI)!g" \
	    -e "s!@BLOGURI@!$(BLOGURI)!g" \
	    -e "s!@VERSION@!$(VERSION)!g" \
	    -e "s!@REPURI@!$(REPURI)!g" \
	    -e "s!@CGIURI@!$(CGIURI)!g" $< >$@

.xml.html:
	sed -e "s!@HTURI@!$(HTURI)!g" \
	    -e "s!@SITEURI@!$(SITEURI)!g" \
	    -e "s!@VERSION@!$(VERSION)!g" \
	    -e "s!@BLOGURI@!$(BLOGURI)!g" \
	    -e "s!@REPURI@!$(REPURI)!g" \
	    -e "s!@CGIURI@!$(CGIURI)!g" $< >$@
