.SUFFIXES: .html .in.xml .xml .js .min.js

# The default installation is for a default-install OpenBSD box that's
# running HTTPS (only) for the blogger.

# File-system location (directory) of static media.
# See HTURI for the web-visible component.
HTDOCS = /var/www/htdocs
#
# URL location (path) of static media.
# See HTDOCS.
HTURI = 
#
# File-system location (directory) of CGI script.
# See CGIURI.
CGIBIN = /var/www/cgi-bin
#
# File-system location of database.
# See RDDIR.
DATADIR = /var/www/data
#
# Web-server relative location of system log file.
LOGFILE = /logs/dblg-system.log
#
# Compilation and link options.
# If on a static architecture, STATIC is -static; otherwise empty.
# I use /usr/local for kcgi and ksql, hence using them here.
STATIC = -static
CFLAGS += -I/usr/local/include
LDFLAGS += -L/usr/local/lib
#
# Web-server relative location of DATADIR.
# See DATADIR.
RDDIR = /data
#
# Name of installed CGI script, since some servers like to have ".cgi"
# appended to everything.
CGINAME = dblg
#
# URL location (filename) of CGI script.
# See CGIBIN and CGINAME.
CGIURI = /cgi-bin/$(CGINAME)
#
# Default email and password hash for administrator on installation.
# The password is the password hash, which will depend upon the
# operating system where you actually run the system.
# In this case, it's for OpenBSD using "blowfish,a" hashing.
AEMAIL = $(shell whoami)@$(shell hostname)
AHASH = $$2b$$10$$rQrWpJndeJAcIumy3kxugu5Dwrbtl9OOfVc7gN/ITBwrATYFGsL3y
#
# If on an HTTPS-only installation, should be "-DSECURE".
SECURE = -DSECURE
#
# URI (relative, if desired to your web server) of server reports.
# I use GoAccess, https://goaccess.io, for mine.
# This is used for a "server" button in the blog editor.
# If empty, the button is removed.
REPURI =
#
# URI of blog viewer (installed by "installclient").
# This is used by some "cancel" buttons that redirect back to viewing
# the blog entry.
# If empty, those buttons are removed.
BLOGURI = /blog.html
#
# Main site that blog is sitting on.
# This is by the login page's "cancel" button.
# If empty, the button is removed.
SITEURI = /index.html
#
# File-system location (directory) of Swagger API.
APIDOCS = /var/www/htdocs/api-docs

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
VERSION	 = 0.0.3

all: dblg dblg.db $(HTMLS) $(JSMINS) $(CSSS)

api: dblg.json schema.png schema.html

installserver: all
	mkdir -p $(HTDOCS)
	install -m 0444 dblg.html dblg.min.js dblg.css $(HTDOCS)

installclient: all
	mkdir -p $(HTDOCS)
	install -m 0444 blog.html blog.min.js blog.css $(HTDOCS)

installwww: all
	mkdir -p $(HTDOCS)
	install -m 0444 $(HTMLS) $(JSMINS) $(CSSS) $(HTDOCS)

installapi: api
	mkdir -p $(APIDOCS)
	install -m 0444 schema.html schema.png dblg.json $(APIDOCS)

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

schema.html: dblg.sql
	sqliteconvert dblg.sql >$@

schema.png: dblg.sql
	sqliteconvert -i dblg.sql >$@

clean:
	rm -f dblg $(HTMLS) $(JSMINS) $(OBJS) dblg.db myproject.tgz dblg.json schema.html schema.png
	rm -rf dblg.dSYM cov-int

dblg.db: dblg.sql
	rm -f $@
	sed -e "s!@AEMAIL@!$(AEMAIL)!" \
	    -e 's!@AHASH@!$(AHASH)!' dblg.sql | sqlite3 $@

dblg: $(OBJS)
	$(CC) $(STATIC) -o $@ $(OBJS) $(LDFLAGS) -lkcgi -lkcgijson -lkcgixml -lz -lksql -lsqlite3

dblg.json: dblg.in.json
	@rm -f $@
	sed -e "s!@VERSION@!$(VERSION)!g" dblg.in.json >$@
	@chmod 400 $@

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
