function assert(bool, message) {
  if (!bool) {
    print(message + "\n");
    quit();
  } else {
    print(".");
  }
}

assert(!isTainted(3), "3 should not be tainted");

// setting i to 1 and tainting
i = 1;
i = taint(i);
assert(isTainted(i), "i should be tainted");
assert(isTainted(i+1), "i+1 should be tainted");

// setting i to 1 untainted
i = 1;
assert(!isTainted(i), "i should not be tainted");
assert(!isTainted(i+1), "i+1 should not be tainted");

// setting s to "foobar"
s = taint("foobar");
assert(isTainted(s), "s should be tainted");
assert(isTainted(s+ "baz"), "s concatted with \"baz\" should be tainted");
assert(isTainted("baz" + s), "\"baz\" concatted with s should be tainted");

print("subscripting");
assert(isTainted(s[1]), "all chars of s should be tainted");
