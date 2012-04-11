function assertTaint(obj, bool, label, desc) {
  if (isTainted(obj) == bool) {
    print("PASS " + label + ": " + desc);
  } else {
    print("FAIL " + label + ": " + desc);
  }
}

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
