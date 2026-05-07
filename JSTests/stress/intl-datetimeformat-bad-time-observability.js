// Under "having a bad time" (an indexed accessor on Object.prototype), a string locale
// must still resolve to the argument and fire no user callback: the spec canonicalizes it
// through own array elements, so Object.prototype[0] is never reached. Matches V8.

function expect(label, got, want)
{
    if (got !== want)
        throw new Error(`${label}: expected ${JSON.stringify(want)}, got ${JSON.stringify(got)}`);
}

let getCount = 0;
let setCount = 0;
// If the accessor were ever consulted, this value would leak in as the locale.
let poison = "fr";
Object.defineProperty(Object.prototype, "0", {
    get() { ++getCount; return poison; },
    set(v) { ++setCount; },
    configurable: true,
});

for (let i = 0; i < 5; ++i) {
    const f = new Intl.DateTimeFormat("en-US");
    expect(`construction ${i}: getter untouched`, getCount, 0);
    expect(`construction ${i}: setter untouched`, setCount, 0);
    expect(`construction ${i}: resolves to the argument`, f.resolvedOptions().locale, "en-US");
}
