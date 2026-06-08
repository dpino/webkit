//@ runDefault

// Two IntlDateTimeFormat instances built the same way must behave independently:
// formatting one doesn't perturb the other, resolvedOptions() returns fresh
// objects, and per-instance state (.format binding, formatRange) is hermetic.
// An engine sharing internal resolution state across instances passes only if
// that sharing stays read-only.

function expect(label, got, want)
{
    if (got !== want)
        throw new Error(`${label}: expected "${want}", got "${got}"`);
}

const A = new Intl.DateTimeFormat("en-US");
const B = new Intl.DateTimeFormat("en-US");

if (A === B)
    throw new Error("two constructions returned the same JS object");

const ts1 = 0;                    // 1970-01-01T00:00:00Z
const ts2 = 1577836800000;        // 2020-01-01T00:00:00Z
const ts3 = 1593561600000;        // 2020-07-01T00:00:00Z

// (1) Same input → same output across instances.
expect("A.format(ts1) == B.format(ts1)", A.format(ts1), B.format(ts1));
expect("A.format(ts2) == B.format(ts2)", A.format(ts2), B.format(ts2));

// (2) Interleaved formatting on B must not perturb A's results.
const aTs1Before = A.format(ts1);
B.format(ts2);
B.format(ts3);
B.format(ts2);
const aTs1After = A.format(ts1);
expect("A.format(ts1) idempotent under interleaved B.format()", aTs1After, aTs1Before);

// (3) resolvedOptions returns independent JS objects.
const oa = A.resolvedOptions();
const ob = B.resolvedOptions();
if (oa === ob)
    throw new Error("A.resolvedOptions() and B.resolvedOptions() returned the same JS object");
oa.locale = "MUTATED";
oa.timeZone = "MUTATED";
expect("mutating oa doesn't affect ob.locale", ob.locale, "en-US");
const oc = B.resolvedOptions();
expect("mutating oa doesn't poison subsequent A.resolvedOptions()",
    A.resolvedOptions().locale, "en-US");
expect("mutating oa doesn't poison subsequent B.resolvedOptions()",
    oc.locale, "en-US");

// (4) formatRange / formatRangeToParts hermeticity (gated on engine support).
if (typeof Intl.DateTimeFormat.prototype.formatRange === "function") {
    const aRangeBefore = A.formatRange(ts1, ts2);
    B.formatRange(ts2, ts3);
    B.formatRangeToParts(ts1, ts3);
    const aRangeAfter = A.formatRange(ts1, ts2);
    expect("A.formatRange idempotent under interleaved B range ops",
        aRangeAfter, aRangeBefore);

    const aPartsBefore = JSON.stringify(A.formatRangeToParts(ts1, ts2));
    B.formatRangeToParts(ts2, ts3);
    const aPartsAfter = JSON.stringify(A.formatRangeToParts(ts1, ts2));
    expect("A.formatRangeToParts idempotent under interleaved B range ops",
        aPartsAfter, aPartsBefore);
}

// (5) The .format accessor lazily creates a per-instance bound function.
const boundA = A.format;
const boundB = B.format;
if (boundA === boundB)
    throw new Error("A.format and B.format returned the same bound function");
expect("boundA(ts1) == A.format(ts1)", boundA(ts1), A.format(ts1));
expect("boundB(ts2) == B.format(ts2)", boundB(ts2), B.format(ts2));

// A third construction must still produce yet another fresh object.
const C = new Intl.DateTimeFormat("en-US");
if (C === A || C === B)
    throw new Error("third construction returned an existing JS object");
expect("C.format == A.format for same ts", C.format(ts1), A.format(ts1));
expect("C.resolvedOptions().locale matches A", C.resolvedOptions().locale, A.resolvedOptions().locale);
expect("C.resolvedOptions().timeZone matches A",
    C.resolvedOptions().timeZone, A.resolvedOptions().timeZone);
