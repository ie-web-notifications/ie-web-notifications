//function(native) {
'use strict';

window.Notification = function Notification(title, options) {
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