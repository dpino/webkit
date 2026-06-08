//@ runDefault("--useDollarVM=1")

// Subsequent IntlDateTimeFormat constructions must observe a host TZ change.
// Driven across real setTimeout boundaries so the change goes through the
// production VMEntryScope service path, not a synthetic flush. Chained
// setTimeout (not async/await) — async rejections get swallowed in jsc.cpp.

function expect(label, got, want)
{
    if (got !== want)
        throw new Error(`${label}: expected "${want}", got "${got}"`);
}

const tzs = ["Europe/Paris", "Asia/Tokyo", "America/Los_Angeles", "UTC", "Australia/Sydney"];
const formattedByTz = new Map();

const epoch = 0; // 1970-01-01T00:00:00Z lands on different calendar days east vs west of UTC.

const cycles = [];
for (let pass = 0; pass < 2; ++pass)
    for (const tz of tzs)
        cycles.push({ pass, tz });

function runCycle(i)
{
    if (i >= cycles.length) {
        afterCycles();
        return;
    }
    const { pass, tz } = cycles[i];
    $vm.setTimeZoneOverride(tz);
    setTimeout(() => {
        expect(`pass${pass} [${tz}] string locale sees new host`,
            new Intl.DateTimeFormat("en-US").resolvedOptions().timeZone, tz);
        expect(`pass${pass} [${tz}] repeated construction agrees`,
            new Intl.DateTimeFormat("en-US").resolvedOptions().timeZone, tz);
        expect(`pass${pass} [${tz}] distinct locale sees host`,
            new Intl.DateTimeFormat("ja").resolvedOptions().timeZone, tz);
        expect(`pass${pass} [${tz}] no-args ctor sees host`,
            new Intl.DateTimeFormat().resolvedOptions().timeZone, tz);
        expect(`pass${pass} [${tz}] options-bearing ctor sees host`,
            new Intl.DateTimeFormat("en-US", { hour: "numeric" }).resolvedOptions().timeZone, tz);

        // Explicit timeZone option always wins.
        expect(`pass${pass} [${tz}] explicit timeZone wins`,
            new Intl.DateTimeFormat("en-US", { timeZone: "Asia/Kolkata" }).resolvedOptions().timeZone,
            "Asia/Kolkata");

        if (pass === 0)
            formattedByTz.set(tz, new Intl.DateTimeFormat("en-US").format(epoch));

        runCycle(i + 1);
    }, 0);
}

function afterCycles()
{
    // Distinct host TZs must format the same epoch differently.
    if (new Set(formattedByTz.values()).size < 2)
        throw new Error(`format() output never differed across host TZs: ${JSON.stringify([...formattedByTz.entries()])}`);

    // Host-TZ path: drive the platform's resolution + change-notification channels,
    // distinct from the WTF in-process override exercised above.
    $vm.setTimeZoneOverride("");
    $vm.setHostTimeZoneForTesting("America/New_York");
    $vm.timeZoneDidChange();
    setTimeout(() => {
        expect("host-TZ change cacheable path",
            new Intl.DateTimeFormat("en-US").resolvedOptions().timeZone, "America/New_York");

        $vm.setHostTimeZoneForTesting("Australia/Sydney");
        $vm.timeZoneDidChange();
        setTimeout(() => {
            expect("host-TZ change second cycle",
                new Intl.DateTimeFormat("en-US").resolvedOptions().timeZone, "Australia/Sydney");
            stableFields();
        }, 100);
    }, 100);
}

function stableFields()
{
    // Non-TZ resolved-options fields are unaffected by TZ changes.
    $vm.setTimeZoneOverride("UTC");
    setTimeout(() => {
        const r = new Intl.DateTimeFormat("en-US").resolvedOptions();
        expect("locale stable across TZ changes", r.locale, "en-US");
        expect("calendar stable across TZ changes", r.calendar, "gregory");
        expect("numberingSystem stable across TZ changes", r.numberingSystem, "latn");
    }, 0);
}

runCycle(0);
