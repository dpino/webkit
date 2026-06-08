// Spec-mandated user-visible side effects (Proxy traps, getters, indexed reads
// on arrays, String wrapper iteration, ToObject on null) must fire on every
// construction — no engine optimization may silently elide them on a repeated
// call.

function expect(label, got, want)
{
    if (got !== want)
        throw new Error(`${label}: expected ${JSON.stringify(want)}, got ${JSON.stringify(got)}`);
}

// (1) Proxy on the locales argument — every construction must see the proxy reads.
{
    let calls = 0;
    const target = ["en-US"];
    const handler = {
        get(t, p, r) {
            ++calls;
            return Reflect.get(t, p, r);
        },
    };
    const proxied = new Proxy(target, handler);

    new Intl.DateTimeFormat(proxied);
    const after1 = calls;
    if (after1 === 0)
        throw new Error("proxy on locales: no traps fired on first construction");
    new Intl.DateTimeFormat(proxied);
    if (calls === after1)
        throw new Error("proxy on locales: cache hit elided traps on second construction");
}

// (2) Proxy on the options argument — same.
{
    let calls = 0;
    const opts = new Proxy({}, {
        get(t, p, r) {
            ++calls;
            return Reflect.get(t, p, r);
        },
    });

    new Intl.DateTimeFormat("en-US", opts);
    const after1 = calls;
    if (after1 === 0)
        throw new Error("proxy on options: no traps fired on first construction");
    new Intl.DateTimeFormat("en-US", opts);
    if (calls === after1)
        throw new Error("proxy on options: cache hit elided traps on second construction");
}

// (3) Getter on a plain options object — must run on each call.
{
    let calls = 0;
    const opts = {};
    Object.defineProperty(opts, "hour", {
        enumerable: true,
        get() { ++calls; return "numeric"; },
    });

    new Intl.DateTimeFormat("en-US", opts);
    const after1 = calls;
    if (after1 === 0)
        throw new Error("getter on options: getter never fired");
    new Intl.DateTimeFormat("en-US", opts);
    if (calls === after1)
        throw new Error("getter on options: cache hit elided getter on second construction");
}

// (4) `{}` options must resolve identically to absent options — a regression
// that swallowed `options` would also break this.
{
    expect("empty options preserves resolution",
        new Intl.DateTimeFormat("en-US", {}).resolvedOptions().locale,
        new Intl.DateTimeFormat("en-US").resolvedOptions().locale);
}

// (5) Array locales — every call must hit the indexed getter.
{
    let indexReads = 0;
    const arr = ["en-US"];
    Object.defineProperty(arr, "0", {
        get() { ++indexReads; return "en-US"; },
        configurable: true,
    });

    new Intl.DateTimeFormat(arr);
    const after1 = indexReads;
    if (after1 === 0)
        throw new Error("array locales: indexed getter never fired");
    new Intl.DateTimeFormat(arr);
    if (indexReads === after1)
        throw new Error("array locales: cache hit elided indexed getter");
}

// (6) String wrapper — iterated character-by-character, so "e" throws as an
// invalid tag every time.
{
    let firstThrow = null, secondThrow = null;
    try { new Intl.DateTimeFormat(new String("en-US")); } catch (e) { firstThrow = e; }
    try { new Intl.DateTimeFormat(new String("en-US")); } catch (e) { secondThrow = e; }
    if (!firstThrow || !secondThrow)
        throw new Error("String wrapper locales: expected throw on both calls");
}

// (7) Object locales — the spec walks .length / indexed entries and throws on
// the bad-tag content; both calls must take the trap path.
{
    let calls = 0;
    let threw = false;
    const obj = {
        get length() { ++calls; return 0; },
    };
    try { new Intl.DateTimeFormat(obj); } catch { threw = true; }
    if (calls === 0)
        throw new Error("object locales: length getter never fired");
    const callsAfter1 = calls;
    try { new Intl.DateTimeFormat(obj); } catch { /* second try */ }
    if (calls === callsAfter1)
        throw new Error("object locales: cache hit elided length getter on second call");
}

// (8) Number locales fall through to the default-locale resolution, identical
// to the no-arg form.
{
    const a = new Intl.DateTimeFormat(42).resolvedOptions().locale;
    const b = new Intl.DateTimeFormat(42).resolvedOptions().locale;
    const ref = new Intl.DateTimeFormat().resolvedOptions().locale;
    if (a !== b || a !== ref)
        throw new Error(`Number locales did not resolve consistently: a=${a} b=${b} ref=${ref}`);
}

// (9) null locales must throw on every call (no cached short-circuit).
{
    let firstThrow = false, secondThrow = false;
    try { new Intl.DateTimeFormat(null); } catch { firstThrow = true; }
    try { new Intl.DateTimeFormat(null); } catch { secondThrow = true; }
    if (!firstThrow || !secondThrow)
        throw new Error("null locales: should throw on every call");
}
