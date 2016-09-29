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

	/* Markdown editor object. */
	var mde = null;

	/*
	 * Post a FormData object ("fd") to the given "url" with
	 * functions for "setup" (accepts "arg"), "error" (accepts error
	 * code then "arg"), "success" (accepts response then "arg"), or
	 * progress (accepts percentage then "arg").
	 */
	function sendPost(url, fd, setup, error, success, prog, arg) 
	{
		var xmh = new XMLHttpRequest();

		if (null !== setup)
			setup(arg);

		xmh.upload.addEventListener("progress", function(e) {
			if (e.lengthComputable) {
				var percentage = Math.round
					((e.loaded * 100) / e.total);
				if (null !== prog)
					prog(percentage, arg);
			}
		}, false);

		xmh.onreadystatechange = function() {
			if (xmh.readyState === 4 && 
			    xmh.status === 200) {
				console.log(url + ': success!');
				if (null !== success)
					success(xmh.responseText, arg);
			} else if (xmh.readyState === 4) {
				console.log(url + ': failure: ' + xmh.status);
				if (null !== error)
					error(xmh.status, arg);
			}
		};

		console.log(url + ': posting request');
		xmh.open('POST', url, true);
		xmh.send(fd);
	}

	/*
	 * Post a form ("e") with functions for "setup" (accepts "e"),
	 * "error" (accepts error code then "e"), "success" (accepts
	 * response then "e").
	 */
	function sendForm(e, setup, error, success) 
	{
		var xmh = new XMLHttpRequest();
		var url = e.action;

		if (null !== setup)
			setup(e);

		xmh.onreadystatechange=function() {
			if (xmh.readyState === 4 && 
		  	    xmh.status === 200) {
				console.log(url + ': success!');
				if (null !== success)
					success(e, xmh.responseText);
			} else if (xmh.readyState === 4) {
				console.log(url + ': failure: ' + 
					xmh.status);
				if (null !== error)
					error(e, xmh.status);
			}
		};

		console.log(e.action + ': sending form');
		xmh.open(e.method, e.action, true);
		xmh.send(new FormData(e));
		return(false);
	}

	/*
	 * GET a request to "url" with functions for "setup", "error"
	 * (accepts error code), "success" (accepts response).
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

	/*
	 * Get (first) query variable for "variable" or null.
	 */
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

	/*
	 * Find element by identifier name.
	 */
	function find(v)
	{
		var e = null;

		if (typeof v !== 'string')
			console.log('cannot find non-string');
		else if (null === (e = document.getElementById(v)))
			console.log('cannot find node: ' + v);

		return(e);
	}

	/*
	 * Hide element (or by element name).
	 */
	function hide(key)
	{
		var e;

		e = (typeof key === 'string' || 
		     key instanceof String) ? find(key) : key;

		if (null !== e && ! e.classList.contains('hide'))
			e.classList.add('hide');

		return(e);
	}

	/*
	 * Show element (or by element name).
	 */
	function show(key)
	{
		var e;

		e = (typeof key === 'string' || 
		     key instanceof String) ? find(key) : key;

		if (null !== e)
			e.classList.remove('hide');

		return(e);
	}

	/*
	 * Replace text of element (or by element name).
	 */
	function repl(key, txt)
	{
		var e;

		e = (typeof key === 'string' || 
		     key instanceof String) ? find(key) : key;
		if (null === e)
			return;
		while (e.firstChild)
			e.removeChild(e.firstChild);
		e.appendChild(document.createTextNode(txt));
	}

	/*
	 * Remove attribute of element (or by element name).
	 */
	function rattr(key, attr)
	{
		var e;

		e = (typeof key === 'string' || 
		     key instanceof String) ? find(key) : key;
		if (null !== e)
			e.removeAttribute(attr);
	}

	/*
	 * Set attribute of element (or by element name).
	 */
	function attr(key, attr, txt)
	{
		var e;
		
		e = (typeof key === 'string' || 
		     key instanceof String) ? find(key) : key;
		if (null !== e)
			e.setAttribute(attr, txt);
	}

	function loadSetup()
	{
		hide('loaded');
		show('loading');
	}

	function loadError(code)
	{
		hide('loading');
		show('loaded');
		hide('loggedin');
		show('login');
	}

	function reload()
	{
		window.location.reload();
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

	function geolocate(pos)
	{
		rattr('submitform-latitude', 'disabled');
		rattr('submitform-longitude', 'disabled');
		attr('submitform-latitude', 
			'value', pos.coords.latitude);
		attr('submitform-longitude', 
			'value', pos.coords.longitude);
		show('submitform-geolocated');
		hide('submitform-geolocating');
	}

	function nogeolocate()
	{
		show('submitform-ungeolocated');
	}
	
	function restart()
	{
		location.href = window.location.pathname;
	}

	function remove(id)
	{
		return(sendQuery('@CGIURI@/remove.json?entryid=' + id, 
			null, restart, restart));
	}

	function uploadSuccess(resp, e)
	{
		var list, i, upres;

		if (null === (upres = parser(resp)))
			return;

		list = e.getElementsByClassName('submit');
		for (i = 0; i < list.length; i++)
			show(list[i]);
		list = e.getElementsByClassName('pending');
		for (i = 0; i < list.length; i++)
			hide(list[i]);

		attr('submitform-media-result', 'value', upres.secure_url);
		rattr('submitform-clipboard', 'disabled');
	}

	function uploadProgress(pct, e)
	{
		var list, i;

		list = e.getElementsByClassName('percentage');
		for (i = 0; i < list.length; i++) {
			while (list[i].firstChild)
				list[i].removeChild(list[i].firstChild);
			list[i].appendChild(document.createTextNode(pct));
		}
	}

	function uploadSetup(e)
	{

		genericSetup(e);
		uploadProgress(0, e);
		attr('submitform-media-result', 'value', '');
		attr('submitform-clipboard', 'disabled', 'disabled');
	}

	function uploadCloud(u)
	{
		var e, root, fd, url, d, sig, args;

		root = find('mediaform');
		e = find('submitform-media');

		if (0 === e.files.length || ! u.cloud.set)
			return;

		d = Math.floor(new Date().getTime() / 1000.0);
		args = 'folder=' + u.cloud.path + '&timestamp=' + d;

		sig = sha1(args + u.cloud.secret);

		fd = new FormData();
		fd.append('file', e.files[0]);
		fd.append('api_key', u.cloud.key);
		fd.append('folder', u.cloud.path);
		fd.append('timestamp', d);
		fd.append('signature', sig);
		url = 'https://api.cloudinary.com/v1_1/' + 
			u.cloud.name + '/upload';

		return(sendPost(url, fd, genericSetup, genericError, 
			uploadSuccess, uploadProgress, root));
	}

	function admin(res)
	{
		var sub, e, list, i, j, cln;

		e = find('userlist');
		sub = e.children[0];
		e.removeChild(sub);
		for (i = 0; i < res.users.length; i++) {
			cln = sub.cloneNode(true);
			list = cln.getElementsByClassName('userlist-name');
			for (j = 0; j < list.length; j++) 
				repl(list[j], res.users[i].name);
			list = cln.getElementsByClassName('userlist-email-link');
			for (j = 0; j < list.length; j++) 
				attr(list[j], 'href', 'mailto:' + res.users[i].email);
			list = cln.getElementsByClassName('userlist-admin');
			for (j = 0; j < list.length; j++) 
				if (res.users[i].attrs.admin)
					show(list[j]);
				else
					hide(list[j]);
			e.appendChild(cln);
		}
	}

	function userInit(u)
	{
		attr('user-input-email', 'value', u.email);
		attr('user-input-link', 'value', 
			null !== u.link ? u.link : '');
		attr('user-input-name', 'value', u.name);
		cloudInit(u.cloud);
	}

	function cloudInit(cloud)
	{
		if (null === cloud) {
			attr('user-input-cloudkey', 'value', '');
			attr('user-input-cloudsecret', 'value', '');
			attr('user-input-cloudpath', 'value', '');
			attr('user-input-cloudname', 'value', '');
			return;
		}
		attr('user-input-cloudkey', 'value', cloud.key);
		attr('user-input-cloudsecret', 'value', cloud.secret);
		attr('user-input-cloudpath', 'value', cloud.path);
		attr('user-input-cloudname', 'value', cloud.name);
	}

	/*
	 * Fill in the entry itself and things about it (whether it's
	 * been published, etc., etc.).
	 */
	function entryInit(entry, user)
	{
		var url;

		if (null === entry || entry.user.id !== user.id) {
			attr('submitform-entryid', 'value', '-1');
			hide('submitform-existing');
			hide('submitform-cancel');
			url = '@BLOGURI@';
			if (url.length) {
				show('submitform-cancel');
				find('submitform-cancel').onclick = 
					function() {
						location.href = url;
					};
			}
			return;
		}

		mde.value(entry.content);
		attr('submitform-title', 'value', entry.title);
		attr('submitform-entryid', 'value', entry.id);
		show('submitform-existing');
		hide('submitform-cancel');
		repl('submitform-existing-ctime', 
			moment.unix(entry.ctime).fromNow());

		if (entry.attrs.pending) {
			show('submitform-existing-unpublished');
			hide('submitform-existing-published');
		} else {
			hide('submitform-existing-unpublished');
			show('submitform-existing-published');
		}

		find('submitform-remove').onclick = 
			function(id) {
				return function() { remove(id); };
			}(entry.id);

		url = '@BLOGURI@';
		if (url.length) {
			url += '?entryid=' + entry.id;
			show('submitform-cancel');
			find('submitform-cancel').onclick = 
				function() {
					location.href = url;
				};
		}
	}

	/*
	 * Fill in the list of pending articles.
	 */
	function pendingInit(pending)
	{
		var e, sub, i, j, list, cln;

		if (0 === pending.length) {
			hide('editlist');
			show('noeditlist');
			return;
		} else {
			show('editlist');
			hide('noeditlist');
		}

		e = find('editlist');
		sub = e.children[0];
		e.removeChild(sub);

		for (i = 0; i < pending.length; i++) {
			cln = sub.cloneNode(true);
			e.appendChild(cln);
			list = cln.getElementsByClassName
				('editlist-ctime');
			for (j = 0; j < list.length; j++)
				repl(list[j], moment.unix
					(pending[i].ctime).fromNow());
			list = cln.getElementsByClassName
				('editlist-mtime');
			for (j = 0; j < list.length; j++)
				repl(list[j], moment.unix
					(pending[i].mtime).fromNow());
			list = cln.getElementsByClassName
				('editlist-mtimebox');
			for (j = 0; j < list.length; j++)
				if (pending[i].mtime !== 
				    pending[i].ctime)
					show(list[j]);
				else
					hide(list[j]);
			list = cln.getElementsByClassName
				('editlist-link');
			for (j = 0; j < list.length; j++)
				list[j].href = window.location.pathname + 
					'?entryid=' + pending[i].id;
			list = cln.getElementsByClassName
				('editlist-title');
			for (j = 0; j < list.length; j++)
				repl(list[j], pending[i].title);
			list = cln.getElementsByClassName
				('editlist-titlebox');
			for (j = 0; j < list.length; j++)
				if (pending[i].title.length) 
					show(list[j]);
				else
					hide(list[j]);
			list = cln.getElementsByClassName
				('editlist-notitlebox');
			for (j = 0; j < list.length; j++)
				if (pending[i].title.length) 
					hide(list[j]);
				else
					show(list[j]);
		}
	}

	function clipboardInit()
	{
		new Clipboard(find('submitform-clipboard'));
		attr('submitform-clipboard', 'disabled', 'disabled');
	}

	function loadSuccess(resp)
	{
		var res = parser(resp);

		if (null === res)
			return;

		if (res.user.attrs.admin)
			show('label-admin');
		else
			hide('label-admin');

		mde = new SimpleMDE({
			insertTexts: { 
				link: ["[", "]()"], 
				image: ["![](", ")"] 
			}
		});
		
		show('submitform-geolocating');
		hide('submitform-geolocated');
		hide('submitform-ungeolocated');

		if (navigator.geolocation)
			navigator.geolocation.getCurrentPosition
				(geolocate, nogeolocate);
		else
			nogeolocate();

		userInit(res.user);
		clipboardInit();

		entryInit(res.entry, res.user);

		show('loaded');
		hide('loading');
		show('loggedin');
		hide('login');

		rattr('submitform-media-btn', 'disabled');
		find('submitform-media-btn').onclick = 
			function(user) {
				return function() {
					uploadCloud(res.user);
				};
			}(res.user);
		if (res.user.attrs.admin)
			admin(res);

		pendingInit(res.pending);
	}

	function blogeditor()
	{
		var i, list, env, qs, url;

		list = document.getElementsByTagName('form');
		for (i = 0; i < list.length; i++)
			list[i].reset();

		find('modpassform').onsubmit = modpass;
		find('modlinkform').onsubmit = modlink;
		find('modemailform').onsubmit = modemail;
		find('modnameform').onsubmit = modname;
		find('modcloudform').onsubmit = modcloud;
		find('adduserform').onsubmit = adduser;
		find('loginform').onsubmit = login;
		find('submitform').onsubmit = submit;
		find('submitform-save').onclick = save;
		find('logout').onclick = logout;

		attr('submitform-media-btn', 'disabled', 'disabled');

		url = '@REPURI@';
		if (url.length) {
			show('server');
			find('server').onclick = function() {
				location.href = '@REPURI@';
			};
		} else
			hide('server');

		url = '@SITEURI@';
		if (url.length) {
			show('usersite');
			find('usersite').onclick = function() {
				location.href = url;
			};
		} else
			hide('usersite');

		qs = '';
		if (null !== (env = getQueryVariable('entryid')))
			qs = '?entryid=' + env;
		return(sendQuery('@CGIURI@/index.json' + qs, 
			loadSetup, loadError, loadSuccess));
	}

	function genericSetup(e)
	{
		var list, i;

		list = e.getElementsByClassName('error');
		for (i = 0; i < list.length; i++)
			hide(list[i]);
		list = e.getElementsByClassName('submit');
		for (i = 0; i < list.length; i++)
			hide(list[i]);
		list = e.getElementsByClassName('pending');
		for (i = 0; i < list.length; i++)
			show(list[i]);
	}

	function genericError(e, code)
	{
		var list, i;

		list = e.getElementsByClassName('submit');
		for (i = 0; i < list.length; i++)
			show(list[i]);
		list = e.getElementsByClassName('pending');
		for (i = 0; i < list.length; i++)
			hide(list[i]);
		list = e.getElementsByClassName('error' + code);
		for (i = 0; i < list.length; i++)
			show(list[i]);
	}

	function reloadSuccess(e, resp)
	{ 
		if (null !== mde) {
			mde.value('');
			mde.toTextArea();
			mde = null;
		}
		genericSuccess(e, resp);
		blogeditor();
	}

	function genericSuccess(e, resp)
	{
		var list, i;

		list = e.getElementsByClassName('error');
		for (i = 0; i < list.length; i++)
			hide(list[i]);
		list = e.getElementsByClassName('submit');
		for (i = 0; i < list.length; i++)
			show(list[i]);
		list = e.getElementsByClassName('pending');
		for (i = 0; i < list.length; i++)
			hide(list[i]);
	}

	function login()
	{
		var e = find('loginform');
		return(sendForm(e, genericSetup, 
			genericError, reloadSuccess));
	}

	function modlink()
	{
		return(sendForm(find('modlinkform'), 
			genericSetup, genericError, reloadSuccess));
	}

	function modpass()
	{
		return(sendForm(find('modpassform'), 
			genericSetup, genericError, reloadSuccess));
	}

	function modemail()
	{
		return(sendForm(find('modemailform'), 
			genericSetup, genericError, reloadSuccess));
	}

	function adduser()
	{
		return(sendForm(find('adduserform'), 
			genericSetup, genericError, reloadSuccess));
	}

	function modcloud()
	{
		return(sendForm(find('modcloudform'), 
			genericSetup, genericError, reloadSuccess));
	}

	function modname()
	{
		return(sendForm(find('modnameform'), 
			genericSetup, genericError, reloadSuccess));
	}

	function submitSuccess(e, resp)
	{
		var res = parser(resp);
		var url;

		if (null !== mde) {
			mde.value('');
			mde.toTextArea();
			mde = null;
		}
		genericSuccess(e, resp);

		if (null === res)
			return;

		url = '@BLOGURI@';
		if (url.length) {
			url += '?entryid=' + res.id;
			location.href = '@BLOGURI@?entryid=' + res.id;
		} else
			reload();
	}

	function saveSetup(e)
	{
		var list, i;

		list = e.getElementsByClassName('error');
		for (i = 0; i < list.length; i++)
			hide(list[i]);
		list = e.getElementsByClassName('save');
		for (i = 0; i < list.length; i++)
			hide(list[i]);
		list = e.getElementsByClassName('saving');
		for (i = 0; i < list.length; i++)
			show(list[i]);
	}

	function saveSuccess(resp, e)
	{
		var res = parser(resp);
		var list, i;

		list = e.getElementsByClassName('save');
		for (i = 0; i < list.length; i++)
			show(list[i]);
		list = e.getElementsByClassName('saving');
		for (i = 0; i < list.length; i++)
			hide(list[i]);
		if (null === res)
			return;
		location.href = 
			window.location.pathname + 
			'?entryid=' + res.id;
	}

	function saveError(code, e)
	{
		var list, i;

		list = e.getElementsByClassName('save');
		for (i = 0; i < list.length; i++)
			show(list[i]);
		list = e.getElementsByClassName('saving');
		for (i = 0; i < list.length; i++)
			hide(list[i]);
	}

	/*
	 * Invoked to publish the current article later.
	 * This effectively delists the article (if already listed) and
	 * saves its contents.
	 */
	function save()
	{
		var fd;

		find('submitform-markdowninput').value = mde.value();
		fd = new FormData(find('submitform'));
		fd.append('save', '1');
		return(sendPost(find('submitform').action, fd, saveSetup, 
			saveError, saveSuccess, null, find('submitform')));
	}

	/*
	 * Publish the current article.
	 */
	function submit()
	{
		find('submitform-markdowninput').value = mde.value();
		/*
		 * XXX: this can be out of sync since
		 * 'submitform-nocoords' is set by an asynchronous
		 * callback for the geolocator.
		 */
		if (find('submitform-nocoords').checked) {
			attr('submitform-latitude', 
				'disabled', 'disabled');
			attr('submitform-longitude', 
				'disabled', 'disabled');
		}
		return(sendForm(find('submitform'), 
			genericSetup, genericError, submitSuccess));
	}

	function logoutSuccess()
	{
		hide('loading');
		show('loaded');
		hide('loggedin');
		show('login');
	}

	function logout()
	{
		sendQuery('@CGIURI@/logout.json', 
			null, logoutSuccess, logoutSuccess);
	}

	root.blogeditor = blogeditor;
})(this);

window.addEventListener('load', function(){
	blogeditor();
});
