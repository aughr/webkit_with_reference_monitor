print("isTainted(3) == " + isTainted(3));

print("setting i to 1 and tainting");
i = 1;
i = taint(i);
print("isTainted(i) == " + isTainted(i));
print("isTainted(i + 1) == " + isTainted(i + 1));

print("setting i to 3, untainted");
i = 3;
print("isTainted(i) == " + isTainted(i));
print("isTainted(i + 1) == " + isTainted(i + 1));

print("setting s to \"foobar\"");
s = taint("foobar");
print("isTainted(s) == " + isTainted(s));
print("isTainted(s + \"baz\") == " + isTainted(s + "baz"));
print("isTainted(\"baz\" + s) == " + isTainted("baz" + s));
