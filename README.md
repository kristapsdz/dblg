## Synopsis

This is a read-only repository mirror for dblg, a [simple] dynamic
blogging utility.
dblg is a small, no-nonsense web application supporting the bare minimum
required for publishing blog (or "micro-blog") content.

It has three components:

1. A back-end server.  This is a tiny CGI script written in C,
[dblg.c](dblg.c).  It links to [kcgi](https://kristaps.bsd.lv/kcgi) and
[ksql](https://kristaps.bsd.lv/ksql), and uses
[SQLite](https://sqlite.org) for its backing store.  It produces JSON
objects, so it can be driven by any front-end conforming to its
expectations.

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
function, passing it some example values.

## License

All sources use the ISC (like OpenBSD) license.
See the [LICENSE.md](LICENSE.md) file for details.
