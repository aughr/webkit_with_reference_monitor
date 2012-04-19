function assertFunc(a, b, label, desc) {
  if (a == b) {
    //print("PASS " + label + ": " + desc);
  } else {
    print("FAIL " + label + ": " + desc);
  }
}

var taint, isTainted;

(function () {
  var tag = new SecurityTag();

  taint = function(obj) {
    return tag.addTo(obj);
  };

  isTainted = function(obj) {
    return tag.isOn(obj);
  };
})();

// Basic taint functionality
var global_a = taint(1);
assertFunc(isTainted(global_a), true, "Taint", "Simple variable tainted");
assertFunc(isTainted(this.global_a), true, "Global", "Property of global object tainted");
assertFunc(isTainted(this), false, "Global", "Global object untainted");

function runTests() {
  var b = global_a;
  assertFunc(isTainted(b), true, "Assignment", "Simple assignment statement");

  // no untaint in my implementation

  // Reassigning

  b = "untainted";
  assertFunc(isTainted(b), false, "Reassign", "Untaint due to reassigment");

  // Tainting Literals
  var a1 = taint(true);
  var a2 = taint("Hello World");
  var a3 = taint(1);
  var a4 = taint(-1.0);

  assertFunc(isTainted(a1), true, "taint literals", "boolean assigned variable");
  assertFunc(isTainted(true), false, "taint literals", "boolean literal");

  assertFunc(isTainted(a2), true, "taint literals", "string assigned variable");
  assertFunc(isTainted("Hello World"), false, "taint literals", "string literal");

  assertFunc(isTainted(a3), true, "taint literals", "integers/floats assigned variable");
  assertFunc(isTainted(1), false, "taint literals", "integers/floats literal");

  assertFunc(isTainted(a4), true, "taint literals", "floats assigned variable");
  assertFunc(isTainted(-1.0), false, "taint literals", "floats assigned variable");

  // Arithmetic

  var a = 10;
  a = taint(a);

  b = a + 4;
  var b1 = 4 + a;
  assertFunc(isTainted(b), true, "Binary Add", "b = a + 4");
  assertFunc(isTainted(b1), true, "Binary Add", "b = 4 + a");

  var c = a - 4;
  var c1 = 4 - a;
  assertFunc(isTainted(c), true, "Binary Subtraction", "b = a - 4");
  assertFunc(isTainted(c1), true, "Binary Subtraction", "b = 4 - a");

  var d = a * 2;
  var d1 = 2 * a;
  assertFunc(isTainted(d), true, "Binary Multiply", "d = a * 2");
  assertFunc(isTainted(d1), true, "Binary Multiply" , "d1 = 2 * a");

  var e = a / 2;
  var e1 = 2 / a;
  assertFunc(isTainted(e), true, "Binary Division", "e = a / 2");
  assertFunc(isTainted(e1), true, "Binary Division", "e1 = 2 / a");

  var f = a % 4;
  var f1 = 4 % a;
  assertFunc(isTainted(f), true, "Binary Modulus", "e = a % 4");
  assertFunc(isTainted(f1), true, "Binary Modulus", "e1 = 4 % a");

  var g = +a;
  assertFunc(isTainted(g), true, "Unary Plus", "g = +a");

  var h = -a;
  assertFunc(isTainted(h), true, "Unary Minus", "g = -a");

  a++;
  assertFunc(isTainted(a), true, "Postfix increment", "a++");
  ++a;
  assertFunc(isTainted(a), true, "Prefix increment", "++a");

  a--;
  assertFunc(isTainted(a), true, "Postfix decrement", "a--");
  --a;
  assertFunc(isTainted(a), true, "Prefix decrement", "++a");

  // Bitwise

  var aa = a & 4;
  var aa1 = 4 & a;
  assertFunc(isTainted(aa), true, "Bitwise And", " aa = a & 4");
  assertFunc(isTainted(aa1), true, "Bitwise And", " aa1 = 4 & a");

  var bb = a | 5;
  var bb1 = 5 | a;
  assertFunc(isTainted(bb), true, "Bitwise Or", "bb = a | 5");
  assertFunc(isTainted(bb1), true, "Bitwise Or", " bb1 = 5 | a");

  var cc = ~a;
  assertFunc(isTainted(cc), true, "Bitwise Not", "cc = ~a");

  var dd = a << 3;
  var dd1 = 3 << a;
  assertFunc(isTainted(dd), true, "Bitshift Left", "dd = a << 3");
  assertFunc(isTainted(dd1), true, "Bitshift Left", "dd1 = 3 << a");

  var ee = a >> 1;
  var ee1 = 204050 >> a;
  assertFunc(isTainted(ee), true, "Bitshift Right", "ee = a >> 1");
  assertFunc(isTainted(ee1), true, "Bitshift Right" , "ee1 = 204050 >> a");

  var ff = a >>> 1;
  var ff1 = 204050 >>> a;
  assertFunc(isTainted(ff), true, "Shift Right With Sign", "ff = a >>> 1");
  assertFunc(isTainted(ff1), true, "Shift Right With Sign", "ff1 = 204050 >>> a");

  var gg = a ^ 4;
  var gg1 = 4 ^ a;
  assertFunc(isTainted(gg), true, "XOR", "gg = a ^ 4");
  assertFunc(isTainted(gg1), true, "XOR", "gg1 = 4 ^ a");

  // Logical

  a1 = true;
  //a1 = taint(a1);
  a2 = true;
  a2 = taint(a2);

  var aaa = a1 && a2;
  var aaa1 = a2 && a1;
  var aaa2 = true && a2;
  var aaa3 = a2 && false;
  assertFunc(isTainted(aaa), true, "Logical And", "aaa = a1 && a2'");
  assertFunc(isTainted(aaa1), false, "Logical And", "aaa1 = a2' && a1");
  assertFunc(isTainted(aaa2), true, "Logical And", "aaa2 = true && a2'");
  assertFunc(isTainted(aaa3), false, "Logical And", "aaa3 = a2' && false");

  var bbb = a2 || a1;
  var bbb1 = a1 || a2;
  var bbb2 = a2 || false;
  var bbb3 = true || a2;
  assertFunc(isTainted(bbb), true, "Logical Or", "bbb = a2' || a1");
  assertFunc(isTainted(bbb1), false, "Logical Or", "bbb1 = a1 || a2'");
  assertFunc(isTainted(bbb2), true, "Logical Or", "bbb2 = a2' || false");
  assertFunc(isTainted(bbb3), false, "Logical Or", "bbb3 = true || a2'");

  var ccc = !a2;
  assertFunc(isTainted(ccc), true, "Logical Not", "ccc = !a2'");

  // Comparison
  var aaaa = a < 2;
  var aaaa1 = 2 < a;
  assertFunc(isTainted(aaaa), true, "Less Than", "aaaa = a < 2");
  assertFunc(isTainted(aaaa1), true, "Less Than", "aaaa1 = 2 < a");

  var bbbb = a > 4;
  var bbbb1 = 4 > a;
  assertFunc(isTainted(bbbb), true, "Greater Than", "bbbb = a > 4");
  assertFunc(isTainted(bbbb1), true, "Greater Than" , "bbbb1 = 4 > a");

  var cccc = a <= 2;
  var cccc1 = 2 <= a;
  assertFunc(isTainted(cccc), true, "Less Than Or Equal", "cccc = a <= 2");
  assertFunc(isTainted(cccc1), true, "Less Than Or Equal", "cccc1 = 2 <= a");

  var dddd = a >= 4;
  var dddd1 = 4 >= a;
  assertFunc(isTainted(dddd), true, "Greater Than Or Equal", "dddd = a >= 4");
  assertFunc(isTainted(dddd1), true, "Greater Than Or Equal", "dddd1 = 4 >= a");

  var eeee = a == 2;
  var eeee1 = 2 == a;
  assertFunc(isTainted(eeee), true, "Equals Equals" , "eeee = a == 2");
  assertFunc(isTainted(eeee1), true, "Equals Equals ", "eeee1 = 2 == a");

  var eeee_a = taint("foo") === "foo";
  var eeee1_a = "bar" === taint("bar");
  assertFunc(isTainted(eeee_a), true, "Equals Equals Equals" , "eeee = a === 2");
  assertFunc(isTainted(eeee1_a), true, "Equals Equals Equals", "eeee1 = 2 === a");

  var ffff = a != 2;
  var ffff1 = 2 != a;
  assertFunc(isTainted(ffff), true, "Not Equals", "ffff = a != 2");
  assertFunc(isTainted(ffff1), true, "Not Equals", "ffff1 = 2 != a");

  // String

  var a3 = "asdf";
  a3 = taint(a3);

  var aaaaa = a3 + 2;
  var aaaaa1 = 2 + a3;
  assertFunc(isTainted(aaaaa), true, "String Concat .", "aaaaa = a3' + 2");
  assertFunc(isTainted(aaaaa1), true, "String Concat.", "aaaaa1 = 2 + a3'");

  var bbbbb = a3 < "foo";
  var bbbbb1 = "foo" < a3;
  assertFunc(isTainted(bbbbb), true, "String Less Than", "bbbbb = a3' < \"foo\"");
  assertFunc(isTainted(bbbbb1), true, "String Less Than", "bbbbb1 = \"foo\" < a3'");

  var ccccc = a3 > "foo";
  var ccccc1 = "foo" > a3;
  assertFunc(isTainted(ccccc), true, "String Greater Than", "ccccc = a3' > \"foo\"");
  assertFunc(isTainted(ccccc1), true, "String Greater Than", "ccccc1 = \"foo\" > a3'");

  var ddddd = a3 <= "foo";
  var ddddd1 = "foo" <= a3;
  assertFunc(isTainted(ddddd), true, "String Less Than Or Equal", "ddddd = a3' <= \"foo\"");
  assertFunc(isTainted(ddddd1), true, "String Less Than Or Equal", "ddddd1 = \"foo\" <= a3'");

  var eeeee = a3 >= "foo";
  var eeeee1 = "foo" >= a3;
  assertFunc(isTainted(eeeee), true, "String Greater Than Or Equal", "eeeee = a3' >= \"foo\"");
  assertFunc(isTainted(eeeee1), true, "String Greater Than Or Equal", "eeeee1 = \"foo\" >= a3'");

  // Complex Assignment
  var aaaaaa = 4;
  aaaaaa += a;
  assertFunc(isTainted(aaaaaa), true, "Plus Equals", "aaaaaa += a");

  var bbbbbb = 4;
  bbbbbb -= a;
  assertFunc(isTainted(bbbbbb), true, "Minus Equals", "bbbbbb -= a");

  var cccccc = 4;
  cccccc *= a;
  assertFunc(isTainted(cccccc), true, "Times Equals", "cccccc *= a");

  var dddddd = 0;
  dddddd /= a;
  assertFunc(isTainted(dddddd), true, "Divide Equals", "dddddd /= a");

  var eeeeee = 3;
  eeeeee %= a;
  assertFunc(isTainted(eeeeee), true, "Modulo Equals", "eeeeee %= a");

  var ffffff = 2;
  ffffff <<= a;
  assertFunc(isTainted(ffffff), true, "Shift Left Equals", "ffffff <<= a");

  var gggggg = 2048;
  gggggg >>= a;
  assertFunc(isTainted(gggggg), true, "Shift Right Equals", "gggggg >>= a");

  var hhhhhh = 2048;
  hhhhhh >>>= a;
  assertFunc(isTainted(hhhhhh), true, "Shift Right With Sign Equals", "hhhhhh >>>= a");

  var iiiiii = 5;
  iiiiii &= a;
  assertFunc(isTainted(iiiiii), true, "And Equals", "iiiiii &= a");

  var jjjjjj = 7;
  jjjjjj |= a;
  assertFunc(isTainted(jjjjjj), true, "Or Equals", "jjjjjj |= a");

  var kkkkkk = 7;
  kkkkkk ^= a;
  assertFunc(isTainted(kkkkkk), true, "XOR Equals", "kkkkkk ^= a");

  // Simple Object

  var Pet = function(name, gender) {
    //if (!this instanceof Pet) {
      //return new Pet(name, gender);
    //}
    this.name = name;
    this.gender = gender;
    this.hello = function () { return "Hello, I am " + name + "."; };
  }

  var myPet = new Pet("Treyvor", 'M');
  assertFunc(isTainted(myPet), false, "Simple Object", "Untainted construction of simple object");
  assertFunc(isTainted(myPet.prototype), false, "Simple Object", "Untainted prototype of untained simple object");

  var taintedName = "Justine";
  taintedName = taint(taintedName);
  var myTaintedPropertyPet = new Pet(taintedName, 'F');
  var myTPPHello = myTaintedPropertyPet.hello();
  assertFunc(isTainted(myTaintedPropertyPet.name), true, "Simple Object", "Tainted object property");
  assertFunc(isTainted(myTaintedPropertyPet.hello), false, "Simple Object", "Untainted property of object w/ tainted property");
  assertFunc(isTainted(myTPPHello), true, "Simple Object", "Tainted return value of object property which uses tainted property");
  assertFunc(isTainted(myTaintedPropertyPet), false, "Simple Object", "Untainted object with tainted property");

  assertFunc(isTainted(myPet.hello()), false, "Simple Object", "Untainted name property of untainted Object Pet");

  var myTaintedPet = new Pet("Param", 'M');
  myTaintedPet = taint(myTaintedPet);
  assertFunc(isTainted(myTaintedPet), true, "Simple Object", "Tainted object");
  assertFunc(isTainted(myTaintedPet.name), false, "Simple Object", "Property of tainted object");
  assertFunc(isTainted(myTaintedPet.prototype), false, "Simple Object", "Untainted prototype of tainted object");

  // Prototypes
  var x = "Aaron";
  x = taint(x);
  Pet.prototype.owner = x;

  var newPet = new Pet("Spike", 'M');
  assertFunc(isTainted(newPet.owner), true, "Prototype", "Tainted prototype property");
  assertFunc(isTainted(newPet), false, "Prototype", "Object instance untainted");
  assertFunc(isTainted(myPet.owner), true, "Prototype", "Shared tainted prototype property");

  Pet.prototype = taint(Pet.prototype);
  assertFunc(isTainted(newPet), false, "Prototype", "Object instance w/ tainted prototype untainted");
  assertFunc(isTainted(newPet.prototype), false, "Prototype", "Tainted prototype property of object");

  Pet.prototype.greet = function () { return "Hi, I'm " + this.name + " and I belong to " + this.owner; };
  var g = newPet.greet();
  assertFunc(isTainted(g), true, "Prototype", "Return value of prototype method which uses tainted data");

  // Array
  var arrElement = "foo";
  arrElement = taint(arrElement);

  var simpleArr = new Array(1, 2, "duck", arrElement, "orange");
  assertFunc(isTainted(simpleArr), false, "Array", "Array after initialization w/ tainted value");
  assertFunc(isTainted(simpleArr[3]), true, "Array", "Array access of tainted value");

  var isInArr = arrElement in simpleArr;
  // FIXME
  //assertFunc(isTainted(isInArr), true, "In", "Result of in operation looking for tainted value");

  var joinedArrContents = simpleArr.join();
  assertFunc(isTainted(joinedArrContents), true, "Array", "Joined array contents containing tainted data");

  var concatArr = simpleArr.concat("stuff");
  assertFunc(isTainted(concatArr), false, "Array", "Result of array concatentation");
  assertFunc(isTainted(concatArr[3]), true, "Array", "Element in result of array concatentation");

  var assocArr = new Array();
  assocArr[arrElement] = "Hello World!";
  assertFunc(isTainted(assocArr[arrElement]), false, "Array", "Associative array access w/ tainted key");
  assocArr["asdf"] = arrElement;
  assertFunc(isTainted(assocArr["asdf"]), true, "Array", "Associative array access of tainted value");
  assertFunc(isTainted(assocArr), false, "Array", "Associative array with tainted key and tainted value");

  // Elementary closure test
  function Scoping(a, b) {
      var exposedProperty = "";
      var taintedClosure = taint("taint");
      var untaintedClosure = "untaint";
      var boolValue = false;
      function InsideScope() {
         if (boolValue) {
             exposedProperty = taintedClosure;
         }
         else {
             exposedProperty = untaintedClosure;
         }
         //print("Boolean is: " + boolValue);
         //print("Taint is: " + isTainted(exposedProperty));
         boolValue = !boolValue;

         return exposedProperty;
      }

      return InsideScope;
  }

  var closedScope = Scoping(2, 3);
  assertFunc(isTainted(closedScope()), false, "Closure Test", "First time return value is untainted");
  assertFunc(isTainted(closedScope()), true, "Closure Test", "Next time return value IS tainted");
  assertFunc(isTainted(closedScope()), false, "Closure Test", "Third time return value is again untainted");
  assertFunc(isTainted(closedScope()), true, "Closure Test", "Next time return value IS tainted (and the cycle repeats)");

  // Elementary closure test: setting/getting private object properties

  // Code taken from JavaScript the Definitive Guide, 5E, David Flanagan

  function makeProperty(o, name, predicate) {
    var value; // This is the property value

    // The getter method simply returns the value.
    o["get" + name] = function () { return value;
        };

    // The setter method stores the value or throws an exception if the predicate rejects the value.
    o["set" + name] = function (v) {
      if (predicate && !predicate(v))
        throw "set" + name + ": invalid value " + v;
      else
        value = v;
    };
  }

  // The following code demonstrates the makeProperty() method.
  var o = {}; // Here is an empty object

  // Add property accessor methods getName and setName()
  // Ensure that only string values are allowed
  makeProperty(o, "Name", function (x) { return true; }); // checking type for a string fails with a tainted string

  o.setName("Frank"); // Set the property value
  //print(o.getName()); // Get the property value

  taintedName = "Tainted Name";
  taintedName = taint(taintedName);

  assertFunc(isTainted(o.getName()), false, "Private Scoped Props", "Setting prop to untainted value");

  o.setName(taintedName);
  assertFunc(isTainted(o.getName()), true, "Private Scoped Props", "Setting prop to tainted value");

  o.setName("Frank");
  assertFunc(isTainted(o.getName()), false, "Private Scoped Props", "Setting prop back to untainted value");

  // Conditionals

  function TestTaintReturn() {
    var a = taint("taint");
    return a;
  }
  var abc = isTainted(TestTaintReturn());
  assertFunc(abc, true, "Function tainted return", "Function returns a tainted value");

  a = "if taint test";
  b = true;
  b = taint(b);

  if (a && b) {
    c = a;
  } else if (b) {
    c = b;
  }
  assertFunc(isTainted(c), false, "Conditionals", "Dynamically");
}

for (i = 0; i < 100000; i++) {
  runTests();
  //print("iteration " + i);
}
