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
objects, an Atom feed, or static HTML5 so it can be driven by any
front-end conforming to its expectations.

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
will then invoke the script when desired.  A static version of blog
front-end, [blog-static.xml](blog-static.xml), is provided (though it
must be configured by the administrator) to have static blog content.
The latter is required if you want your blog spidered.  I use both: one
for "blog" front matter and the other for permanent-link articles.

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
- Atom feed support.
- Static HTML support.

The default editor front-end uses [moment.js](http://momentjs.com/) for
formatting dates, [clipboard.js](https://clipboardjs.com/) for copying
to the clipboard (apparently this is hard?), and
[js-sha1](https://github.com/emn178/js-sha1) for SHA1 hashing the cloud
authorisation.

The default viewer optionally (but *strongly* recommended) uses
[moment.js](http://momentjs.com/) and
[showdown](https://github.com/showdownjs/showdown) for formatting
MarkDown.  The default static page also uses showdown.

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

There are two ways of "viewing" blog content.  The first is to
dynamically load using JSON; the second, static HTML.

#### Dynamic content

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

- `blog`: the URL for the blog page
- `editor`: string pointing to the editor URL (no editor, if null)
- `entryid`: an integer that's the unique identifier of a specific entry
  to pull down (if unspecified, this is checked in the query string)
- `lang`: a string limiting the languages of pulled articles
- `limit`: an integer limiting the number of pulled articles (else all)
- `order`: a string of either `ctime` or `mtime` being the default sort
  order of pulled articles
- `postload`: a function that is invoked after all processing has been
  done after successfully downloading and parsing the results
- `rescroll`: a Boolean indicating that if there's a document hash,
  re-scroll to that hash after loading the page (and before `postload`)

The `blogclient` function asynchronously manipulates the DOM tree within
and including the `blog` identifier, the root element that (obviously)
should be included somewhere in the calling page.  When `blogclient` is
invoked, it synchronously adds the `hide` attribute to the root element
before starting asynchronously downloading blog content.

Upon success, the child of this element is removed, cloned, and
replicated for each blog element shown on the page.

```html
<div id="blog">
  <div>
    <!-- This is duplicated and filled in for each entry. -->
    <!-- Node manipulation happens over "class" attributes. -->
  </div>
</div>
```

Within the root element, the following classes have their contents
replaced by content.  Note that *calendar form* refers to using 
[moment.js](http://momentjs.com/)'s `calendar()` function, if found, or
simply a generic date string otherwise.  Note also that *markdown*
refers to HTML-ised markdown if
[showdown](https://github.com/showdownjs/showdown) is loaded, or the raw
markdown otherwise.

- `blog-ctime`: calendar form of entry creation time
- `blog-mtime`: calendar form of entry update time, which is initialised
  to entry creation time
- `blog-author`: name of blog entry author
- `blog-title`: title of blog entry
- `blog-aside`: article synopsis markdown content
- `blog-content`: article content markdown content

Furthermore, the following element attributes are set or unset.

- `blog-author-link`: if provided, `href` is set to the author's link;
  otherwise, the `href` attribute is removed
- `blog-canon-link`: if a `blog` argument is provided to the
  `blogclient` configuration object, `href` is set to that and the
  `?entryid=nnn` parameter; otherwise, the `href` attribute is removed
- `blog-coords`: if coordinates are provided, sets `href` to a Google
  maps link (in satellite mode) of the coordinates; otherwise, the
  `href` is removed
- `blog-fb-link`: if the `blog` argument is provided to the
  `blogclient` configuration object, `data-href` is set to the canonical
  blog link and `data-numposts` also set; otherwise, they are removed
- `blog-image`: if provided, `src` is set to the image link; otherwise,
  the `src` attribute is removed
- `blog-image-link`: if provided, `href` is set to the image link;
  otherwise, the `href` attribute is removed

The following have the `hide` attribute ("hidden") set on the containing
element given the noted conditions.

- `blog-canon-box`: hidden if a `blog` argument is not provided to the
  `blogclient` configuration object
- `blog-coords-box`: hidden if there are no coordinates
- `blog-ctime-box`: hidden if there is a modification time
- `blog-image-box`: hidden if there is no image
- `blog-mtime-box`: hidden if there is *not* a modification time

After filling in these fields for each blog entry, the `hide` class is
removed from the root element.

#### Static content

Static content is full-formed HTML5 produced by the back-end.  To use
this mode, which is not enabled by default, you'll need to enter the
administration page and provide a template that will be filled in by the
back-end and passed to the front-end.  A sample is provided in
[blog-static.xml](blog-static.xml).

The following template keys are populated:

- `dblg-aside`: aside content (Markdown)
- `dblg-author-link`: link to author, if given (otherwise blank)
- `dblg-author-name`: author name
- `dblg-canon`: canonical link to blog entry
- `dblg-canon-query`: blog entry query string
- `dblg-classes`: a string consisting of zero or more `author-has-link`,
  `blog-has-aside`, `blog-has-image`, `blog-has-image-aside`,
  `blog-has-only-aside`, `blog-has-only-image`, `blog-has-only-ctime`,
  `blog-has-mtime`, `blog-has-coords` (explanation fairly self
  explanatory)
- `dblg-content`: content (Markdown)
- `dblg-coord-lat-decimal`: latitude decimal (if coordinates given,
  else empty)
- `dblg-coord-lng-decimal`: longitude decimal (if coordinates given,
  else empty)
- `dblg-ctime`: epoch date of creation time
- `dblg-ctime-iso8601`: ISO 8601 formatted creation time
- `dblg-image`: blog image (if given)
- `dblg-mtime`: epoch date modification time (starting with creation
  time)
- `dblg-mtime-iso8601`: ISO 8601 formatted modification time
- `dblg-title`: entry title (*not* Markdown)

## License

All sources use the ISC (like OpenBSD) license.
See the [LICENSE.md](LICENSE.md) file for details.
