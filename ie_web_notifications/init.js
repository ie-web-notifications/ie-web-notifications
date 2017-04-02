//function(native) {
'use strict';

// it's implemented that way to privately keep listeners.
// A very simple impl from https://developer.mozilla.org/en/docs/Web/API/EventTarget
var mixinEventTarget = function () {
  var listeners = {};

  this.addEventListener = function (type, callback) {
    if (!(type in listeners)) {
      listeners[type] = [];
    }
    listeners[type].push(callback);
  };
  this.removeEventListener = function (type, callback) {
    if (!(type in listeners)) {
      return;
    }
    var stack = listeners[type];
    for (var i = 0, l = stack.length; i < l; i++) {
      if (stack[i] === callback) {
        stack.splice(i, 1);
        return;
      }
    }
  };
  this.dispatchEvent = function (event) {
    if (!(event.type in listeners)) {
      return true;
    }
    var stack = listeners[event.type];
    //event.target = this; // it's not supported, however, this of the handler will be the notification object.
    for (var i = 0, l = stack.length; i < l; i++) {
      stack[i].call(this, event);
    }
    return !event.defaultPrevented;
  };
};

var warnToUseEventTarget = function (deprecatedFunc) {
  console.log("Please consider using of 'notification.addEventListener('" + deprecatedFunc + "', your-handler)' instead of 'notification.on" + deprecatedFunc + "'.");
};

// helper to simplify native code
native.setFireEvent(function (target, /*string*/nativeEvent) {
  if (nativeEvent === "click") {
    if (target.onclick) {
      warnToUseEventTarget(nativeEvent);
      target.onclick();
    }
  } else if (nativeEvent === "show") {
    if (target.onshow) {
      warnToUseEventTarget(nativeEvent);
      target.onshow();
    }
    // Leave it for compatibility reasons for a while.
    // https://github.com/ie-web-notifications/ie-web-notifications.github.io/issues/18
    if (target.onshown) {
      console.log("Using of a wrong property 'onshown' for show event, it should be 'onshow' (without n) which is by the way deprecated.");
      warnToUseEventTarget(nativeEvent);
      target.onshown();
    }
  } else if (nativeEvent === "error") {
    if (target.onerror) {
      warnToUseEventTarget(nativeEvent);
      target.onerror();
    }
  } else if (nativeEvent === "close") {
    if (target.onclose) {
      warnToUseEventTarget(nativeEvent);
      target.onclose();
    }
  } else if (nativeEvent === "replace") {
    if (target.onreplaced) {
      console.log("Using of non-standard feature 'onreplaced'");
      warnToUseEventTarget(nativeEvent);
      target.onreplaced();
    }
  }
  // "new Event(...)" does not work
  var ev = document.createEvent('Event');
  ev.initEvent(nativeEvent, false, true);
  target.dispatchEvent(ev);
});

window.Notification = function Notification(title, options) {
  Object.defineProperty(this, "title", {
    __proto__: null,
    value: title,
    enumerable: true
  });
  if (options) {
    if (typeof options.body === "string") {
      Object.defineProperty(this, "body", {
        __proto__: null,
        value: options.body,
        enumerable: true
      });
    }
    if (typeof options.tag === "string") {
      Object.defineProperty(this, "tag", {
        __proto__: null,
        value: options.tag,
        enumerable: true
      });
    }
  }
  mixinEventTarget.call(this);
  native(this, title, options);
};
window.Notification.prototype = Object.create(native.prototype);

Object.defineProperties(window.Notification, {
  'permission': {
    __proto__: null,
    get: function () { return native.permission; },
    enumerable: true
  },
  'requestPermission': {
    __proto__: null,
    value: function (callback) {
      return native.requestPermission(callback);
    },
    enumerable: true
  }
});
//}