// Repeated constructions with the same arguments must produce the same observable
// side-effect trace. An indexed accessor on Object.prototype is a way to surface
// engine slow paths through the spec's CanonicalizeLocaleList step; the test only
// asserts call-to-call consistency, so engines that legitimately take a path
// that never touches the setter still pass.

function expect(label, got, want)
{
    if (got !== want)
        throw new Error(`${label}: expected ${JSON.stringify(want)}, got ${JSON.stringify(got)}`);
}

let trapped = 0;
let stored = "en-US";

Object.defineProperty(Object.prototype, "0", {
    get() { return stored; },
    set(v) { stored = v; ++trapped; },
    configurable: true,
});

function trapsFor(fn)
{
    const before = trapped;
    fn();
    return trapped - before;
}

const c1 = trapsFor(() => new Intl.DateTimeFormat("en-US"));
const c2 = trapsFor(() => new Intl.DateTimeFormat("en-US"));
const c3 = trapsFor(() => new Intl.DateTimeFormat("en-US"));

expect("repeated construction is idempotent in side effects (call 1 vs 2)", c2, c1);
expect("repeated construction is idempotent in side effects (call 2 vs 3)", c3, c2);

// Resolution must also be consistent across calls, even with the proto trap in place.
const a = new Intl.DateTimeFormat("en-US");
const b = new Intl.DateTimeFormat("en-US");
expect("locale agrees across calls",
    a.resolvedOptions().locale, b.resolvedOptions().locale);
expect("format agrees across calls",
    a.format(0), b.format(0));

// And the format output must match a slow-path construction (one with options),
// which the cache gate is required to exclude.
const slow = new Intl.DateTimeFormat("en-US", {});
expect("cacheable and non-cacheable agree on locale",
    a.resolvedOptions().locale, slow.resolvedOptions().locale);
expect("cacheable and non-cacheable agree on format(0)",
    a.format(0), slow.format(0));
