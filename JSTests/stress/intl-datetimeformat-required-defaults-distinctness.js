// toLocaleDateString, toLocaleTimeString, toLocaleString, and the no-arg
// constructor must each produce their own distinct, stable output regardless
// of how often the others are invoked.

function expect(label, got, want)
{
    if (got !== want)
        throw new Error(`${label}: expected ${JSON.stringify(want)}, got ${JSON.stringify(got)}`);
}

const ts = 0; // 1970-01-01T00:00:00Z renders distinctly across all three.
const date = new Date(ts);

const refDate = date.toLocaleDateString();
const refTime = date.toLocaleTimeString();
const refAll = date.toLocaleString();

// Constructor-path constructions must not perturb the toLocale* outputs.
new Intl.DateTimeFormat();
new Intl.DateTimeFormat("en-US");
new Intl.DateTimeFormat();

expect("toLocaleDateString stable after cache warming",
    date.toLocaleDateString(), refDate);
expect("toLocaleTimeString stable after cache warming",
    date.toLocaleTimeString(), refTime);
expect("toLocaleString stable after cache warming",
    date.toLocaleString(), refAll);

// All three toLocale* outputs are different by construction.
if (refDate === refTime)
    throw new Error(`toLocaleDateString and toLocaleTimeString collapsed: "${refDate}"`);
if (refDate === refAll)
    throw new Error(`toLocaleDateString and toLocaleString collapsed: "${refDate}"`);
if (refTime === refAll)
    throw new Error(`toLocaleTimeString and toLocaleString collapsed: "${refTime}"`);

// The constructor's no-arg form is also self-consistent.
const ctorFormatted = new Intl.DateTimeFormat().format(ts);
const ctorAgain = new Intl.DateTimeFormat().format(ts);
expect("ctor no-arg format is self-consistent", ctorFormatted, ctorAgain);

// Repeated alternation must keep each output stable.
for (let i = 0; i < 50; ++i) {
    expect(`iter ${i}: toLocaleDateString stable`, date.toLocaleDateString(), refDate);
    expect(`iter ${i}: toLocaleTimeString stable`, date.toLocaleTimeString(), refTime);
    expect(`iter ${i}: toLocaleString stable`, date.toLocaleString(), refAll);
    expect(`iter ${i}: ctor no-arg stable`, new Intl.DateTimeFormat().format(ts), ctorFormatted);
}

// Same shape with an explicit "en-US" locale.
const refDateEn = date.toLocaleDateString("en-US");
const refTimeEn = date.toLocaleTimeString("en-US");
const refAllEn = date.toLocaleString("en-US");
const ctorEn = new Intl.DateTimeFormat("en-US").format(ts);
for (let i = 0; i < 20; ++i) {
    expect(`en-US iter ${i}: toLocaleDateString stable`, date.toLocaleDateString("en-US"), refDateEn);
    expect(`en-US iter ${i}: toLocaleTimeString stable`, date.toLocaleTimeString("en-US"), refTimeEn);
    expect(`en-US iter ${i}: toLocaleString stable`, date.toLocaleString("en-US"), refAllEn);
    expect(`en-US iter ${i}: ctor("en-US") stable`, new Intl.DateTimeFormat("en-US").format(ts), ctorEn);
}
