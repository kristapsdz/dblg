.SUFFIXES: .html .in.xml .xml .js .min.js

# The default installation is for a default-install OpenBSD box that's
# running HTTPS (only) for the blogger.

# File-system location (directory) of static media.
HTDOCS = /var/www/htdocs
#
# URL location (path) of static media.
HTURI = 
#
# File-system location (directory) of CGI script.
CGIBIN = /var/www/cgi-bin
#
# File-system location of database.
DATADIR = /var/www/data
#
# Web-server relative location of system log file.
LOGFILE = /logs/dblg-system.log
#
# Compilation and link options.
# If on a static architecture, STATIC is -static; otherwise empty.
STATIC = -static
CFLAGS += -I/usr/local/include
LDFLAGS += -L/usr/local/lib
#
# Web-server relative location of DATADIR.
RDDIR = /data
#
# Name of installed CGI script.
CGINAME = dblg
#
# URL location (filename) of CGI script.
CGIURI = /cgi-bin/$(CGINAME)
#
# Default email and password hash for administrator on installation.
AEMAIL = $(shell whoami)@$(shell hostname)
AHASH = $$2b$$10$$rQrWpJndeJAcIumy3kxugu5Dwrbtl9OOfVc7gN/ITBwrATYFGsL3y
#
# If on an HTTPS-only installation, should be "-DSECURE".
SECURE = -DSECURE
#
# URI of server reports.
REPURI =
#
# URI of blog display.
BLOGURI = /dblg.html
#
# Main site that blog is sitting on.
SITEURI = /index.html

# Override these with an optional local file.
sinclude GNUmakefile.local

# Don't edit anything below here.

OBJS	 = dblg.o
HTMLS	 = dblg.html blog.html
JSMINS	 = dblg.min.js blog.min.js
CSSS	 = dblg.css blog.css
CFLAGS	+= -g -W -Wall -O2 $(SECURE)
CFLAGS	+= -DLOGFILE=\"$(LOGFILE)\"
CFLAGS	+= -DDATADIR=\"$(RDDIR)\"
VERSION	 = 0.0.1

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
	rm -f dblg $(HTMLS) $(JSMINS) $(OBJS) dblg.db myproject.tgz
	rm -rf dblg.dSYM cov-int

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
