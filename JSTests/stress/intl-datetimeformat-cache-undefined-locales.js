// `new Intl.DateTimeFormat()` and `new Intl.DateTimeFormat(undefined, {})` must
// produce equivalent resolutions: anything else means an engine optimization
// path silently disagrees with its own slow path.

function expect(label, got, want)
{
    if (got !== want)
        throw new Error(`${label}: expected "${want}", got "${got}"`);
}

const a = new Intl.DateTimeFormat();
const b = new Intl.DateTimeFormat();
const c = new Intl.DateTimeFormat(undefined);

if (a === b || b === c || a === c)
    throw new Error("constructions returned the same JS wrapper");

const reference = new Intl.DateTimeFormat(undefined, {});
const oa = a.resolvedOptions();
const ob = b.resolvedOptions();
const oc = c.resolvedOptions();
const oref = reference.resolvedOptions();

for (const key of ["locale", "calendar", "numberingSystem", "timeZone"]) {
    expect(`a.${key} == reference.${key}`, oa[key], oref[key]);
    expect(`b.${key} == reference.${key}`, ob[key], oref[key]);
    expect(`c.${key} == reference.${key}`, oc[key], oref[key]);
}

expect("a.format == reference.format", a.format(0), reference.format(0));
expect("b.format == reference.format", b.format(0), reference.format(0));
expect("c.format == reference.format", c.format(0), reference.format(0));

// Distinct locale arguments must keep distinct resolutions.
const stringForm = new Intl.DateTimeFormat("en-US");
const stringAgain = new Intl.DateTimeFormat("en-US");
expect("explicit en-US locale", stringForm.resolvedOptions().locale, "en-US");
expect("explicit en-US locale (re-query)", stringAgain.resolvedOptions().locale, "en-US");
expect("explicit en-US format stable",
    stringForm.format(0), stringAgain.format(0));

// Many distinct locales — each must resolve to its own request.
const tags = ["en-US", "ja", "fr", "de", "ko", "es", "it", "pt", "ru", "zh", "ar", "hi"];
for (const tag of tags)
    expect(`flood: ${tag}`,
        new Intl.DateTimeFormat(tag).resolvedOptions().locale.startsWith(tag.split("-")[0]),
        true);

// Re-querying earlier locales must still recover the same resolution.
expect("re-query en-US after flood",
    new Intl.DateTimeFormat("en-US").format(0), stringForm.format(0));
expect("re-query undefined after flood",
    new Intl.DateTimeFormat().format(0), reference.format(0));
