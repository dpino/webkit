function foo() {
    return fiatInt52(bar($vm.dfgTrue())) + 1;
}

var thingy = false;
function bar(p) {
    if (thingy)
        return "hello";
    return p ? 42 : 5.5;
}

noInline(foo);
noInline(bar);

for (var i = 0; i < testLoopCount; ++i) {
    var result = foo();
    if (result != 43 && result != 6.5)
        throw "Error: bad result: " + result;
}

thingy = true;
var result = foo();
if (result != "hello1")
    throw "Error: bad result at end: " + result;
