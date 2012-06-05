function assertTaint(obj, bool, label, desc) {
  if (isTainted(obj) === bool) {
    print("PASS " + label + ": " + desc);
  } else {
    print("FAIL " + label + ": " + desc);
  }
}

function assertTaint2(obj, bool, label, desc) {
  if (isTainted2(obj) === bool) {
    print("PASS (2) " + label + ": " + desc);
  } else {
    print("FAIL (2) " + label + ": " + desc);
  }
}

var taint, taint2, isTainted, isTainted2;

(function () {
  var tag = new SecurityTag();
  var tag2 = new SecurityTag();

  taint = function(obj) {
    return tag.addTo(obj);
  };
  taint2 = function(obj) {
    return tag2.addTo(obj);
  };

  isTainted = function(obj) {
    return tag.isOn(obj);
  };
  isTainted2 = function(obj) {
    return tag2.isOn(obj);
  };
})();

function runTest() {

  var tainted = taint("foo");
  var array_with_taint = ["bar", tainted, "baz"];
  var array = ["bar", "baz"];

  assertTaint(array_with_taint.toString(), true, "Array.toString", "tainted component should make output tainted");
  assertTaint(array.toString(), false, "Array.toString", "should be untainted");
  array = taint(array);
  assertTaint(array.toString(), true, "Array.toString", "should be tainted if array is tainted");

  var tainted_bool = taint(true);
  var bool = true;

  assertTaint(tainted_bool.toString(), true, "Boolean.toString", "should be tainted");
  assertTaint(bool.toString(), false, "Boolean.toString", "should be untainted");
  assertTaint(tainted_bool.valueOf(), true, "Boolean.valueOf", "should be tainted");
  assertTaint(bool.valueOf(), false, "Boolean.valueOf", "should be untainted");
  assertTaint(new Boolean(tainted_bool), true, "new Boolean()", "tainted boolean");
  assertTaint(new Boolean(bool), false, "new Boolean()", "untainted boolean");

  var tainted_year = taint("2000");
  var year = "2000";
  var tainted_month = taint(10);
  var month = 10;

  assertTaint(Date.UTC(tainted_year, tainted_month), true, "Date.UTC", "year and month tainted");
  assertTaint(Date.UTC(tainted_year, month), true, "Date.UTC", "year tainted");
  assertTaint(Date.UTC(year, tainted_month), true, "Date.UTC", "month tainted");
  assertTaint(Date.UTC(year, month), false, "Date.UTC", "no parts tainted");
  assertTaint(Date.parse(tainted_year), true, "Date.parse", "tainted string");
  assertTaint(Date.parse(year), false, "Date.parse", "untainted string");
  assertTaint(new Date(tainted_year), true, "new Date()", "tainted string");
  assertTaint(new Date(year), false, "new Date()", "untainted string");

  var date_getters = ["toString", "toISOString", "toDateString", "toTimeString", "toLocaleString", "toLocaleDateString", "toLocaleTimeString",
                 "valueOf", "getTime", "getFullYear", "getUTCFullYear", "toGMTString", "getMonth", "getUTCMonth", "getDate", "getUTCDate",
                 "getDay", "getUTCDay", "getHours", "getUTCHours", "getMinutes", "getUTCMinutes", "getSeconds", "getUTCSeconds", "getMilliseconds",
                 "getUTCMilliseconds", "getTimezoneOffset", "getYear", "toJSON"];
  var date = new Date(year);
  var tainted_date = new Date(tainted_year);
  var i;
  for (i = 0; i < date_getters.length; i++)
    assertTaint(date[date_getters[i]](), false, "Date#" + date_getters[i], "untainted date");
  for (i = 0; i < date_getters.length; i++)
    assertTaint(tainted_date[date_getters[i]](), true, "Date#" + date_getters[i], "tainted date");

  var date_setters = ["setTime", "setMilliseconds", "setUTCMilliseconds", "setSeconds", "setUTCSeconds", "setMinutes", "setUTCMinutes",
                      "setHours", "setUTCHours", "setDate", "setUTCDate", "setMonth", "setUTCMonth", "setFullYear", "setUTCFullYear",
                      "setYear"];
  for (i = 0; i < date_setters.length; i++) {
    date = new Date(year);
    date[date_setters[i]](year);
    assertTaint(date, false, "Date#" + date_setters[i], "untainted parameter");
  }
  for (i = 0; i < date_setters.length; i++) {
    date = new Date(year);
    date[date_setters[i]](year);
    assertTaint(date, false, "Date#" + date_setters[i], "tainted parameter");
    date = new Date(tainted_year);
    date[date_setters[i]](taint2(year));
    assertTaint(date, true, "Date#" + date_setters[i], "tainted parameter");
    assertTaint2(date, true, "Date#" + date_setters[i], "tainted parameter");
  }

  var tainted_message = taint("foo");
  var message = "foo";
  var tainted_name = taint("FooError");
  var name = "FooError";

  assertTaint(new Error(tainted_message).message, true, "Error.message", "constructed with a tainted string");
  assertTaint(new Error(message).message, false, "Error.message", "constructed with an untainted string");

  var error = new Error();

  error.message = tainted_message;
  assertTaint(error.toString(), true, "Error.toString", "has a tainted message");

  error.message = message;
  assertTaint(error.toString(), false, "Error.toString", "has an untainted message");

  error = new Error();
  error.name = tainted_name;
  assertTaint(error.toString(), true, "Error.toString", "has a tainted name");

  error.name = name;
  assertTaint(error.toString(), false, "Error.toString", "has an untainted name");
  error.message = tainted_message;
  assertTaint(error.toString(), true, "Error.toString", "has a tainted name");
  error.name = name;
  error.message = message;
  assertTaint(error.toString(), false, "Error.toString", "has an untainted name and message");
}

runTest.call({});
