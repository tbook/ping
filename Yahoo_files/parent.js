__twttrlr(function(using, provide, loadrunner, define) {provide("xd/flash",function(e){function t(e,t){var n=t||Math.floor(Math.random()*100),r=['<object id="xdflashshim'+n+'" name="xdflashshim'+n+'"','type="application/x-shockwave-flash" classid="clsid:d27cdb6e-ae6d-11cf-96b8-444553540000"','width="1" height="1" style="position:absolute;left:-9999px;top:-9999px;">','<param name="movie" value="'+e+"&debug="+window.__XDDEBUG__+'"/>','<param name="wmode" value="window"/>','<param name="allowscriptaccess" value="always"/>',"</object>"].join(" ");return r}e({object:t})});
provide("xd/detection",function(e){function t(){try{return!!navigator.plugins["Shockwave Flash"]||!!(new ActiveXObject("ShockwaveFlash.ShockwaveFlash"))}catch(e){return!1}}e({getFlashEnabled:t,hasPostMessage:!!window.postMessage,isIE:!!navigator.userAgent.match("MSIE")})});
provide("util/util",function(e){function t(e){var t=1,n,r;for(;n=arguments[t];t++)for(r in n)if(!n.hasOwnProperty||n.hasOwnProperty(r))e[r]=n[r];return e}function n(e){for(var t in e)e.hasOwnProperty(t)&&(l(e[t])&&(n(e[t]),c(e[t])&&delete e[t]),(e[t]===undefined||e[t]===null||e[t]==="")&&delete e[t]);return e}function r(e,t){var n=0,r;for(;r=e[n];n++)if(t==r)return n;return-1}function i(e,t){if(!e)return null;if(e.filter)return e.filter.apply(e,[t]);if(!t)return e;var n=[],r=0,i;for(;i=e[r];r++)t(i)&&n.push(i);return n}function s(e,t){if(!e)return null;if(e.map)return e.map.apply(e,[t]);if(!t)return e;var n=[],r=0,i;for(;i=e[r];r++)n.push(t(i));return n}function o(e){return e&&e.replace(/(^\s+|\s+$)/g,"")}function u(e){return{}.toString.call(e).match(/\s([a-zA-Z]+)/)[1].toLowerCase()}function a(e){return e&&String(e).toLowerCase().indexOf("[native code]")>-1}function f(e,t){if(e.contains)return e.contains(t);var n=t.parentNode;while(n){if(n===e)return!0;n=n.parentNode}return!1}function l(e){return e===Object(e)}function c(e){if(!l(e))return!1;if(Object.keys)return!Object.keys(e).length;for(var t in e)if(e.hasOwnProperty(t))return!1;return!0}e({aug:t,compact:n,containsElement:f,filter:i,map:s,trim:o,indexOf:r,isNative:a,isObject:l,isEmptyObject:c,toType:u})});
provide("util/events",function(e){using("util/util",function(t){function r(){this.completed=!1,this.callbacks=[]}var n={bind:function(e,t){return this._handlers=this._handlers||{},this._handlers[e]=this._handlers[e]||[],this._handlers[e].push(t)},unbind:function(e,n){if(!this._handlers[e])return;if(n){var r=t.indexOf(this._handlers[e],n);r>=0&&this._handlers[e].splice(r,1)}else this._handlers[e]=[]},trigger:function(e,t){var n=this._handlers&&this._handlers[e];t.type=e;if(n)for(var r=0,i;i=n[r];r++)i.call(this,t)}};r.prototype.addCallback=function(e){this.completed?e.apply(this,this.results):this.callbacks.push(e)},r.prototype.complete=function(){this.results=makeArray(arguments),this.completed=!0;for(var e=0,t;t=this.callbacks[e];e++)t.apply(this,this.results)},e({Emitter:n,Promise:r})})});
provide("xd/base",function(e){using("util/util","util/events",function(t,n){function r(){}t.aug(r.prototype,n.Emitter,{transportMethod:"",init:function(){},send:function(e){var t;this._ready?this._performSend(e):t=this.bind("ready",function(){this.unbind("ready",t),this._performSend(e)})},ready:function(){this.trigger("ready",this),this._ready=!0},isReady:function(){return!!this._ready},receive:function(e){this.trigger("message",e)}}),e({Connection:r})})});
provide("xd/parent",function(e){using("xd/base","util/util","xd/detection",function(t,n,r){function u(e){var t=[];for(var n in e)t.push(n+"="+e[n]);return t.join(",")}function a(){}var i="__ready__",s=0,o;a.prototype=new t.Connection,n.aug(a.prototype,{_createChild:function(){this.options.window?this._createWindow():this._createIframe()},_createIframe:function(){var e={allowTransparency:!0,frameBorder:"0",scrolling:"no",tabIndex:"0",name:this._name()},t,i,s,u=n.aug(n.aug({},e),this.options.iframe);window.postMessage?(o||(o=document.createElement("iframe")),t=o.cloneNode(!1)):t=document.createElement('<iframe name="'+u.name+'">'),t.id=u.name;for(var a in u)a!="style"&&t.setAttribute(a,u[a]);var f=t.getAttribute("style");f&&typeof f.cssText!="undefined"?f.cssText=u.style:t.style.cssText=u.style;var l=this,c=function(){l.child=t.contentWindow,l._ready||l.init()};if(!t.addEventListener){var h=!1;t.attachEvent("onload",function(){if(h)return;h=!0,c()})}else t.addEventListener("load",c,!1);t.src=this._source(),(i=this.options.appendTo)?i.appendChild(t):(s=this.options.replace)?(i=s.parentNode,i&&i.replaceChild(t,s)):document.body.insertBefore(t,document.body.firstChild),r.isIE&&this.transportMethod&&this.transportMethod==="Flash"&&(t.src=t.src)},_createWindow:function(){var e={width:550,height:450,personalbar:"0",toolbar:"0",scrollbars:"1",resizable:"1"},t,r,i,s=n.aug(n.aug({},e),this.options.window),o=screen.width,a=screen.height;s.left=s.left||Math.round(o/2-s.width/2),s.top=s.top||Math.round(a/2-s.height/2),a<s.height&&(s.top=0,s.height=a);var f=this._name();t=window.open(this._source(),f,u(s)),t&&t.focus(),this.child=t,this.init()},_source:function(){return this.options.src},_name:function(){var e="_xd_"+s++;return window.parent&&window.parent!=window&&window.name&&(e=window.name+e),e}});var f=function(e){this.transportMethod="PostMessage",this.options=e,this._createChild()};f.prototype=new a,n.aug(f.prototype,{init:function(){function t(t){t.source===e.child&&(!e._ready&&t.data===i?e.ready():e.receive(t.data))}var e=this;window.addEventListener?window.addEventListener("message",t,!1):window.attachEvent("onmessage",t)},_performSend:function(e){this.child.postMessage(e,this.options.src)}});var l=function(e){this.transportMethod="Flash",this.options=e,this.token=Math.random().toString(16).substring(2),this._setup()};l.prototype=new a,n.aug(l.prototype,{_setup:function(){var e=this;using("xd/flash",function(t){window["__xdcb"+e.token]={receive:function(t){!e._ready&&t===i?e.ready():e.receive(t)},loaded:function(){}};var n=document.createElement("div");n.innerHTML=t.object("https://tfw-current.s3.amazonaws.com/xd/ft.swf?&token="+e.token+"&parent=true&callback=__xdcb"+e.token+"&xdomain="+e._host(),e.token),document.body.insertBefore(n,document.body.firstChild),e.proxy=n.firstChild,e._createChild()})},init:function(){},_performSend:function(e){this.proxy.send(e)},_host:function(){return this.options.src.replace(/https?:\/\//,"").split(/(:|\/)/)[0]},_source:function(){return this.options.src+(this.options.src.match(/\?/)?"&":"?")+"xd_token="+escape(this.token)}});var c=function(e){this.transportMethod="Fallback",this.options=e,this._createChild()};c.prototype=new a,n.aug(c.prototype,{init:function(){},_performSend:function(e){}}),e({connect:function(e){var t;return r.hasPostMessage?r.isIE&&e.window?r.getFlashEnabled()&&(t=new l(e)):t=new f(e):r.isIE&&r.getFlashEnabled()&&(t=new l(e)),t||(t=new c(e)),t}})})})});