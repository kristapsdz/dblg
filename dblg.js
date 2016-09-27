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

	var mde = null;
	var media = null;

	function sendPost(url, fd, setup, error, success, progress, arg) 
	{
		var xmh = new XMLHttpRequest();

		if (null !== setup)
			setup(arg);

		xmh.upload.addEventListener("progress", function(e) {
			if (e.lengthComputable) {
				var percentage = Math.round((e.loaded * 100) / e.total);
				if (null !== progress)
					progress(percentage, arg);
			}
		}, false);

		xmh.onreadystatechange=function() {
			if (xmh.readyState === 4 && xmh.status === 200) {
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

	function rattr(key, attr)
	{
		var e;

		e = (typeof key === 'string' || 
		     key instanceof String) ? find(key) : key;
		if (null !== e)
			e.removeAttribute(attr);
	}


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
		rattr('submitform-geolocate', 'disabled');
		hide('submitform-geolocating');
	}

	function nogeolocate()
	{
		show('submitform-ungeolocated');
		rattr('submitform-geolocate', 'disabled');
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

	function loadSuccess(resp)
	{
		var res = parser(resp);
		var url;

		if (null === res)
			return;

		if (res.user.attrs.admin)
			show('label-admin');
		else
			hide('label-admin');
		
		show('submitform-geolocating');
		hide('submitform-geolocated');
		hide('submitform-ungeolocated');
		attr('submitform-geolocate', 'disabled', 'disabled');

		if (navigator.geolocation)
			navigator.geolocation.getCurrentPosition
				(geolocate, nogeolocate);
		else
			nogeolocate();

		attr('user-input-link', 'value', 
			null !== res.user.link ? res.user.link : '');
		attr('user-input-email', 'value', res.user.email);
		attr('user-input-name', 'value', res.user.name);
		if (null !== res.user.cloud) {
			attr('user-input-cloudkey', 
				'value', res.user.cloud.key);
			attr('user-input-cloudsecret', 
				'value', res.user.cloud.secret);
			attr('user-input-cloudpath', 
				'value', res.user.cloud.path);
			attr('user-input-cloudname', 
				'value', res.user.cloud.name);
		} else {
			attr('user-input-cloudkey', 'value', '');
			attr('user-input-cloudsecret', 'value', '');
			attr('user-input-cloudpath', 'value', '');
			attr('user-input-cloudname', 'value', '');
		}
		show('loaded');
		hide('loading');
		show('loggedin');
		hide('login');

		new Clipboard(find('submitform-clipboard'));

		mde = new SimpleMDE({
			insertTexts: { link: ["[", "]()"], image: ["![](", ")"] }
		});

		if (null !== res.entry && 
	   	    res.entry.user.id === res.user.id) {
			url = '@BLOGURI@';
			if (url.length)
				url += '?entryid=' + res.entry.id;
			mde.value(res.entry.content);
			attr('submitform-title', 'value', res.entry.title);
			attr('submitform-entryid', 'value', res.entry.id);
			show('submitform-existing');
			if (url.length) {
				show('submitform-cancel');
				find('submitform-cancel').onclick = function() {
					location.href = url;
				};
			} else
				hide('submitform-cancel');
		} else {
			url = '@BLOGURI@';
			attr('submitform-entryid', 'value', '-1');
			hide('submitform-existing');
			if (url.length) {
				show('submitform-cancel');
				find('submitform-cancel').onclick = function() {
					location.href = url;
				};
			} else
				hide('submitform-cancel');
		}

		attr('submitform-clipboard', 'disabled', 'disabled');
		rattr('submitform-media-btn', 'disabled');
		find('submitform-media-btn').onclick = function(user) {
			return function() {
				uploadCloud(res.user);
			};
		}(res.user);

		if (res.user.attrs.admin)
			admin(res);
	}

	function dblg()
	{
		var i, list, env, qs, url;

		url = '@REPURI@';
		qs = ''
		if (null !== (env = getQueryVariable('entryid')))
			qs = '?entryid=' + env;

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
		find('logout').onclick = logout;
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

		attr('submitform-media-btn', 'disabled', 'disabled');

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
		dblg();
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
			window.location.reload();
	}

	function submit()
	{
		find('submitform-markdowninput').value = mde.value();
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

	function usersite()
	{
		location.href = '@SITEURI@';
	}

	function logout()
	{
		sendQuery('@CGIURI@/logout.json', 
			null, logoutSuccess, logoutSuccess);
	}

	root.dblg = dblg;
})(this);

window.addEventListener('load', function(){
	dblg();
});
