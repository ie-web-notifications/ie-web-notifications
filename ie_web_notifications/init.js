//function(native) {
'use strict';

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