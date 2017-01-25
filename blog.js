/*	$Id$ */
/*
 * Copyright (c) 2016 Kristaps Dzonsons <kristaps@bsd.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
(function(root) {
	'use strict';

	var options = null;
	var cgiuri = null;

	/*
	 * Asynchronously download content from url.
	 * Invokes the setup, error, and success functions depending
	 * upon the state.
	 */
	function sendQuery(url, setup, error, success) 
	{
		var xmh = new XMLHttpRequest();

		if (null !== setup)
			setup();

		xmh.onreadystatechange=function() {
			if (xmh.readyState === 4 && 
		 	    xmh.status === 200) {
				console.log(url + ': success!');
				if (null !== success)
					success(xmh.responseText);
			} else if (xmh.readyState === 4) {
				console.log(url + 
					': failure: ' + xmh.status);
				if (null !== error)
					error(xmh.status);
			}
		};

		console.log(url + ': sending request');
		xmh.open('GET', url, true);
		xmh.send(null);
	}

	function getQueryVariable(variable)
	{
		var query, vars, i, pair;

		query = window.location.search.substring(1);
		vars = query.split("&");

		for (i = 0; i < vars.length; i++) {
			pair = vars[i].split("=");
			if(pair[0] == variable)
				return(pair[1]);
		}
		return(null);
	}

	function find(name)
	{
		var e;

		if (null === (e = document.getElementById(name)))
			console.log('cannot find node: ' + name);

		return(e);
	}

	function hide(key)
	{
		var e;

		e = (typeof key === 'string' || 
		     key instanceof String) ? find(key) : key;

		if (null !== e && ! e.classList.contains('hide'))
			e.classList.add('hide');

		return(e);
	}

	function show(key)
	{
		var e;

		e = (typeof key === 'string' || 
		     key instanceof String) ? find(key) : key;

		if (null !== e)
			e.classList.remove('hide');

		return(e);
	}

	function showc(node, cls)
	{
		var l, i;

		l = node.getElementsByClassName(cls);
		for (i = 0; i < l.length; i++)
			show(l[i]);
	}

	function hidec(node, cls)
	{
		var l, i;

		l = node.getElementsByClassName(cls);
		for (i = 0; i < l.length; i++)
			hide(l[i]);
	}

	function rattr(node, cls, attr)
	{
		var l, i;

		l = node.getElementsByClassName(cls);
		for (i = 0; i < l.length; i++)
			l[i].removeAttribute(attr);
	}

	function attr(node, cls, attr, txt)
	{
		var l, i;

		l = node.getElementsByClassName(cls);
		for (i = 0; i < l.length; i++)
			l[i].setAttribute(attr, txt);
	}

	function replhtml(node, cls, txt)
	{
		var l, i;

		l = node.getElementsByClassName(cls);
		for (i = 0; i < l.length; i++)
			l[i].innerHTML = txt;
	}

	function repl(node, cls, txt)
	{
		var l, i;

		l = node.getElementsByClassName(cls);
		for (i = 0; i < l.length; i++) {
			while (l[i].firstChild)
				l[i].removeChild(l[i].firstChild);
			l[i].appendChild(document.createTextNode(txt));
		}
	}

	/*
	 * Either populate a formatted calendar date (moment.js) or the
	 * raw date from the native functions.
	 */
	function abscal(node, cls, epoch)
	{
		var str, d;

		if (typeof moment === 'undefined') {
			d = new Date();
			d.setTime(epoch * 1000);
			str = d.toDateString();
		} else
			str = moment.unix(epoch).calendar();

		repl(node, cls, str);
	}

	/*
	 * Either populate HTML-ised markdown (if a showdown.js
	 * converter exists) or the raw markdown content otherwise.
	 */
	function markdown(node, cls, conv, md)
	{
		if (null !== conv)
			replhtml(node, cls, conv.makeHtml(md));
		else
			repl(node, cls, md);
	}

	function parser(resp)
	{
		var res;

		try  { 
			res = JSON.parse(resp);
		} catch (error) {
			console.log('JSON parse fail: ' + resp);
			return(null);
		}

		return(res);
	}

	/*
	 * The following functions (removebtnSetup, removebtnError,
	 * removebtnuncheck, removebtncheck, removebtn) all relate to
	 * invoking the "delete" button on a blog entry.
	 * Obviously this is only available to the owner, but that's
	 * managed server side.
	 * The purpose of the complexity is to have an "are you sure"
	 * prompt printed before the removal.
	 */

	function removebtnSetup(e)
	{
		showc(e, 'blog-delete-pending');
		hidec(e, 'blog-delete-submit');
	}

	function removebtnError(e)
	{
		hidec(e, 'blog-delete-pending');
		showc(e, 'blog-delete-submit');
		showc(e, 'blog-delete-check');
		hidec(e, 'blog-delete-commit');
	}

	function removebtnuncheck(e, entry)
	{
		showc(e, 'blog-delete-check');
		hidec(e, 'blog-delete-commit');
	}

	function removebtncheck(e, entry)
	{
		hidec(e, 'blog-delete-check');
		showc(e, 'blog-delete-commit');
	}

	function removebtn(e, entry)
	{
		sendQuery(cgiuri + '/remove.json?entryid=' + entry.id, 
			function() { removebtnSetup(e); }, 
			function() { removebtnError(e); },
			function() { location.href='index.html'; });
	}

	/*
	 * Blog entries were downloaded properly.
	 * Parse and process.
	 */
	function loadSuccess(resp)
	{
		var res = parser(resp);
		var e, sub, cln, list, i, j, sz, conv;

		/* Something went wrong in parsing our JSON. */

		if (null === res) {
			console.log('bailing: no parsed JSON');
			return;
		}

		/* If defined, get our Markdown converter. */

		if (typeof showdown !== 'undefined')
			conv = new showdown.Converter();
		else
			conv = null;

		/* Get the node we'll clone and repeat. */

		if (null === (e = find('blog'))) {
			console.log('bailing: no root blog element');
			return;
		} else if (null === (sub = e.children[0])) {
			console.log('bailing: no root blog element child');
			return;
		}

		e.removeChild(sub);

		/*
		 * Configure the number of elements we'll show.
		 * We just take the number that the server gave us and
		 * cut it by the limit.
		 * Note: we shouldn't have more than the number of
		 * requested elements!
		 */

		sz = res.entries.length;
		if (null !== options.limit && sz > options.limit) {
			console.log('warning: more blog entries than requested');
			sz = options.limit;
		}

		for (i = 0; i < sz; i++) {
			cln = sub.cloneNode(true);
			e.appendChild(cln);

			if (res.entries[i].aside.length)
				cln.classList.add('blog-has-aside');

			if (res.entries[i].image.length)
				cln.classList.add('blog-has-image');

			if (res.entries[i].aside.length &&
			    res.entries[i].image.length)
				cln.classList.add('blog-has-image-aside');
			if (res.entries[i].aside.length &&
			    0 === res.entries[i].image.length)
				cln.classList.add('blog-has-only-aside');
			if (0 === res.entries[i].aside.length &&
			    res.entries[i].image.length)
				cln.classList.add('blog-has-only-image');

			abscal(cln, 'blog-ctime', 
				res.entries[i].ctime);
			abscal(cln, 'blog-mtime', 
				res.entries[i].mtime);

			if (res.entries[i].mtime !== 
			    res.entries[i].ctime) {
				hidec(cln, 'blog-ctime-box');
				showc(cln, 'blog-mtime-box');
			} else {
				showc(cln, 'blog-ctime-box');
				hidec(cln, 'blog-mtime-box');
			}

			if (null !== res.entries[i].image &&
			    res.entries[i].image.length) {
				showc(cln, 'blog-image-box');
				attr(cln, 'blog-image', 'src', 
					res.entries[i].image);
				attr(cln, 'blog-image-link', 'href', 
					res.entries[i].image);
			} else {
				hidec(cln, 'blog-image-box');
				rattr(cln, 'blog-image', 'src');
				rattr(cln, 'blog-image-link', 'href');
			}

			repl(cln, 'blog-author', 
				res.entries[i].user.name);

			if (null !== res.entries[i].user.link &&
			    res.entries[i].user.link.length) 
				attr(cln, 'blog-author-link', 'href', 
					res.entries[i].user.link);
			else
				rattr(cln, 'blog-author-link', 'href');

			repl(cln, 'blog-title', 
				res.entries[i].title);

			if (null !== options.blog) {
				showc(cln, 'blog-canon-box');
				attr(cln, 'blog-canon-link', 'href',
					options.blog + '?entryid=' + 
					res.entries[i].id);
			} else {
				hidec(cln, 'blog-canon-box');
				rattr(cln, 'blog-canon-link', 'href');
			}

			if (null !== options.blog) {
				showc(cln, 'blog-facebooklink');
				attr(cln, 'blog-facebooklink', 
					'data-href',
					options.blog + '?entryid=' + 
					res.entries[i].id);
				attr(cln, 'blog-facebooklink',
					'data-numposts', 
					(sz > 1 ? '2' : '20'));
			} else {
				hidec(cln, 'blog-facebooklink');
			}

			markdown(cln, 'blog-content',
				conv, res.entries[i].content);
			markdown(cln, 'blog-aside',
				conv, res.entries[i].aside);

			if (null !== res.entries[i].coords) {
				showc(cln, 'blog-coords');
				attr(cln, 'blog-coords', 'href',
					'https://maps.google.com/?t=k&q=' + 
					res.entries[i].coords.lat + ',' + 
					res.entries[i].coords.lng + 
					'&zoom=13&sensor=false');
			} else
				hidec(cln, 'blog-coords');

			if (null !== res.user &&
			    res.user.id === res.entries[i].user.id) {
				showc(cln, 'blog-control');
				hidec(cln, 'blog-delete-pending');
				showc(cln, 'blog-delete-submit');
				hidec(cln, 'blog-delete-commit');
				showc(cln, 'blog-delete-check');
				list = cln.getElementsByClassName('blog-nodelete');
				for (j = 0; j < list.length; j++) {
					list[j].onclick = function(root, entry) {
						return function() {
							removebtnuncheck(root, entry);
						}
					}(cln, res.entries[i]);
				}
				list = cln.getElementsByClassName('blog-delete-check');
				for (j = 0; j < list.length; j++) {
					list[j].onclick = function(root, entry) {
						return function() {
							removebtncheck(root, entry);
						}
					}(cln, res.entries[i]);
				}
				list = cln.getElementsByClassName('blog-delete');
				for (j = 0; j < list.length; j++)
					list[j].onclick = function(root, entry) {
						return function() {
							removebtn(root, entry);
						}
					}(cln, res.entries[i]);

				list = cln.getElementsByClassName('blog-edit');
				for (j = 0; j < list.length; j++)
					if (null != options.editor) {
						show(list[j]);
						list[j].href = options.editor + 
							'?entryid=' + res.entries[i].id;
					} else
						hide(list[j]);
			
			} else
				hidec(cln, 'blog-control');
		}

		show(e);

		/*if (document.location.hash &&
		    '' !== document.location.hash) {
			setTimeout(function() {
				if (location.hash) {
					window.scrollTo(0, 0);
					window.location.href = hash;
				}
			}, 1);
		}*/
	}

	function loadSetup()
	{
		hide('blog');
	}

	/*
	 * Function asynchronously invokes the dblg blog at "uri" with
	 * the given options dictionary "opts".
	 * The options consist of all optional values: "editor", a
	 * string pointing to the editor URI (no editor, if null);
	 * "limit", an integer limiting the number of pulled articles
	 * (else all); "blog", the URI string for the blog page; "lang",
	 * a string limiting the languages of pulled articles; and
	 * "order", a string of either "ctime" or "mtime" being the
	 * default sort order of pulled articles.
	 *
	 * Lastly, the query string is scanned for "entryid" integer,
	 * which is also appended to the request variable to limit the
	 * blog access.  This can be overridden by the "entryid" value
	 * passed into the "opts" dictionary.
	 */
	function blogclient(uri, opts)
	{
		var entryid, query;

		options = {};
		cgiuri = uri;

		if (null !== opts && typeof opts !== 'undefined') {
			options.editor = 
				(typeof opts.editor === 'string') ? 
				opts.editor : null;
			options.entryid = 
				(typeof opts.entryid === 'number') ? 
				opts.entryid : null;
			options.limit = 
				(typeof opts.limit === 'number') ? 
				opts.limit : null;
			options.blog = 
				(typeof opts.blog === 'string') ? 
				opts.blog : null;
			options.lang = 
				(typeof opts.lang === 'string') ? 
				opts.lang : null;
			options.order = 
				(typeof opts.order === 'string') ? 
				opts.order : null;
		} else {
			options.entryid = null;
			options.editor = null;
			options.limit = null;
			options.blog = null;
			options.lang = null;
			options.order = null;
		}

		query = '';

		/* 
		 * First pull the query string IFF we didn't pass into
		 * the options dictionary.
		 */

		if (null === options.entryid &&
		    null !== (entryid = getQueryVariable('entryid')))
			query += (0 === query.length ? '?' : '&') +
				'entryid=' + entryid;

		if (null !== options.entryid)
			query += (0 === query.length ? '?' : '&') +
				'entryid=' + options.entryid;
		if (null !== options.order)
			query += (0 === query.length ? '?' : '&') +
				'order=' + options.order;
		if (null !== options.limit)
			query += (0 === query.length ? '?' : '&') +
				'limit=' + options.limit;
		if (null !== options.lang)
			query += (0 === query.length ? '?' : '&') +
				'lang=' + options.lang;

		return(sendQuery(cgiuri + '/public.json' + query,
			loadSetup, null, loadSuccess));
	}

	root.blogclient = blogclient;
})(this);
