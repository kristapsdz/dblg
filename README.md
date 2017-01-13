## Synopsis

This is a read-only repository mirror for dblg, a [simple] dynamic
blogging utility.
dblg is a small, no-nonsense web application supporting the bare minimum
required for publishing blog (or "micro-blog") content.

I run it on Mac OS X and OpenBSD (with
[pledge(2)](http://man.openbsd.org/pledge) support).

It has three components:

1. A back-end server.  This is a tiny CGI script written in C,
[dblg.c](dblg.c).  It links to [kcgi](https://kristaps.bsd.lv/kcgi) and
[ksql](https://kristaps.bsd.lv/ksql), and uses
[SQLite](https://sqlite.org) for its backing store.  It produces JSON
objects or an Atom feed (experimental!), so it can be driven by any front-end conforming
to its expectations.

2. An editing front-end, [dblg.xml](dblg.xml), [dblg.css](dblg.css), and
[dblg.js](dblg.js).  This part drives the back-end by providing an
interface for saving entries, publishing them, and doing user
management.  The editor front-end is usually installed on the same
machine as the back-end, but that's really just a matter of convenience
of keeping both in sync.  You can build your own editor front-end that
drives the back-end server---it's just JSON and HTML forms---but that's
kind of a PITA.

3. A blog viewer front-end, [blog.js](blog.js) and, for reference,
[blog.xml](blog.xml), [blog.css](blog.css).  Your public-facing pages
will want to be filled in with blog content, which they can do by
including the JavaScript file [blog.js](blog.js) and having an HTML
element with identifier `blog` filled-in as in the reference.  Your page
will then invoke the script when desired.

This tool is still under development, as is its documentation.  For a
view of it working, see my [diving blog](https://divelog.blue).

The project is being tracked by
[Coverity](https://scan.coverity.com/projects/dblg) for static analysis
of the C source code.

Some features:

- Simple, easy-to-audit code.
- Straightforward user administration (admins, regular users).
- Publish directly or delay publication and edit privately.
- Support for cloud-based images, for now limited to
[Cloudinary](http://cloudinary.com/) (which I use for the
[divelog](https://divelog.blue)).
- Geolocation and coordinates attached to each entry (or suppressed).
- Supports language tags.
- Responsive layout for small displays.
- Strong HTTP caching with etags, full compression support.
- Security: cookies with security extensions and full support for CSP
  (no in-line JavaScript)
- Atom feed support (experimental!).

The default editor front-end uses [moment.js](http://momentjs.com/) for
formatting dates, [clipboard.js](https://clipboardjs.com/) for copying
to the clipboard (apparently this is hard?), and
[js-sha1](https://github.com/emn178/js-sha1) for SHA1 hashing the cloud
authorisation.

The default viewer uses [moment.js](http://momentjs.com/) and
[showdown](https://github.com/showdownjs/showdown) for formatting
MarkDown.

## Documentation

Beyond this document, you can also view the
[database schema](https://kristaps.bsd.lv/dblg/schema.html) or the
[RESTful API](https://kristaps.bsd.lv/dblg).

## Installation

As mentioned, there are three components to this blogger: the back-end,
the editor, and the viewer.  The back-end and editor usually go in the
same place, so I'll start with them.

### Editor front-end and server back-end

You'll first need to download and configure the software.  This is
fairly straightforward.  First, download.  Then, open the
[GNUmakefile](GNUmakefile).  Override the variables you'd like in a
GNUmakefile.local file (which won't be touched).  The
[GNUmakefile](GNUmakefile) documents each variable and has an example
deployment on a default OpenBSD machine.

You'll need [kcgi](https://kristaps.bsd.lv/kcgi) and
[ksql](https://kristaps.bsd.lv/ksql) for the back-end server.

Once configured, run `make installcgi` for first-time to install the
database and the CGI script; else, `make updatecgi` only to freshen the
CGI script and not touch the database.

If you're upgrading from an old version of dblg (i.e., with `make
updatecgi` and differnt versions), the database may have changed.  Each
version with a changing upgrade has a database upgrade script as
dblg-OLD-NEW.sql.  Run those in order.

To install the editor tools, use `make installserver`.  This will only
install the HTML, JavaScript, and CSS for the editor.

### Viewer front-end

To install the viewer, you don't need to download this software.  Simply
reference the existing [blog.xml](blog.xml) and [blog.css](blog.css),
then include [blog.js](blog.js) files in your application. 

```html
<script src="//cdn.rawgit.com/kristapsdz/dblg/master/blog.js"></script>
<script>
  window.addEventListener('load', function(){
    blogclient('/cgi-bin/dblg', {
      blog: '/blog.html',
      editor: '/dblg.html',
      limit: 5
    });
  });
</script>
```

In this invocation, the script is pulled from GitHub's CDN and invoked
in an embedded HTML script.  The embedded script calls the `blogclient`
function, passing it some example values.  The `blogclient` function
consists of two arguments:

```javascript
blogclient(url, options);
```

The `url` argument is the URL of the blog CGI script.  The `options`
dictionary is optional and may consist of any or all of the following
optional values:

- `editor`: string pointing to the editor URL (no editor, if null)
- `limit`: an integer limiting the number of pulled articles (else all)
- `blog`: the URL for the blog page
- `lang`: a string limiting the languages of pulled articles
- `order`: a string of either `ctime` or `mtime` being the default sort
  order of pulled articles
- `entryid`: an integer that's the unique identifier of a specific entry
  to pull down (if unspecified, this is checked in the query string)

## License

All sources use the ISC (like OpenBSD) license.
See the [LICENSE.md](LICENSE.md) file for details.
