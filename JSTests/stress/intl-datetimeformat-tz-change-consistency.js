// Within a single engine, the two construction shapes (`new Intl.DateTimeFormat()`
// and `new Intl.DateTimeFormat(undefined, {})`) must always agree on the
// resolved time zone, including across host TZ changes. Skipped on engines
// without a TZ-override hook.

const setTZ = (typeof setTimeZone === "function")
    ? setTimeZone
    : (typeof $vm !== "undefined" && typeof $vm.setTimeZoneOverride === "function")
        ? (tz) => $vm.setTimeZoneOverride(tz)
        : null;
if (!setTZ)
    quit();

function expect(label, got, want)
{
    if (got !== want)
        throw new Error(`${label}: expected ${JSON.stringify(want)}, got ${JSON.stringify(got)}`);
}

function cacheableTZ()
{
    return new Intl.DateTimeFormat().resolvedOptions().timeZone;
}

function slowTZ()
{
    return new Intl.DateTimeFormat(undefined, {}).resolvedOptions().timeZone;
}

function cacheableFormat(ts)
{
    return new Intl.DateTimeFormat("en-US").format(ts);
}

function slowFormat(ts)
{
    return new Intl.DateTimeFormat("en-US", {}).format(ts);
}

const ts = 0; // 1970-01-01T00:00:00Z lands on different calendar days east vs west of UTC.
const baselineCacheable = cacheableTZ();
const baselineSlow = slowTZ();
expect("baseline cacheable matches slow", baselineCacheable, baselineSlow);
expect("baseline format cacheable matches slow",
    cacheableFormat(ts), slowFormat(ts));

const tzs = ["Asia/Tokyo", "America/New_York", "Europe/Paris", "UTC", "Australia/Sydney"];
for (const tz of tzs) {
    setTZ(tz);
    expect(`after setTZ(${tz}): cacheable matches slow on TZ resolution`,
        cacheableTZ(), slowTZ());
    expect(`after setTZ(${tz}): cacheable matches slow on en-US format`,
        cacheableFormat(ts), slowFormat(ts));
    // Repeated cacheable construction agrees with itself.
    expect(`after setTZ(${tz}): cacheable is self-consistent on TZ`,
        cacheableTZ(), cacheableTZ());
    expect(`after setTZ(${tz}): cacheable is self-consistent on format`,
        cacheableFormat(ts), cacheableFormat(ts));
}

// Reverting to UTC must keep the two paths in sync as well.
setTZ("UTC");
expect("after revert to UTC: cacheable matches slow on TZ",
    cacheableTZ(), slowTZ());
expect("after revert to UTC: cacheable matches slow on format",
    cacheableFormat(ts), slowFormat(ts));
