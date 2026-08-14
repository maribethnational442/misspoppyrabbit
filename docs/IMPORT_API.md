# Event Import API

Contract for pushing external calendar events into Miss Poppy Rabbit — used
by the Web UI's `.ics` import and by the companion Chrome extension
(separate project). One format, one endpoint, idempotent by design.

## Endpoint

```
POST http://prabbit.local/api/events/import
Content-Type: application/json
```

CORS is open (`Access-Control-Allow-Origin: *`) and `OPTIONS` preflight is
handled, so browser extensions can POST directly.

## Body

```json
{
  "calendarId": 0,
  "events": [
    {
      "uid":   "4f2ka9@google.com",
      "title": "Design sync w/ Client A",
      "start": 1786780200,
      "end":   1786783800,
      "cal":   1
    }
  ]
}
```

| Field | Type | Notes |
|---|---|---|
| `calendarId` | int 0-3 | Default target calendar for the batch |
| `events[].uid` | string | **Stable external ID** (ICS UID). Required |
| `events[].title` | string | Required; truncated to 47 chars on device |
| `events[].start` | int | Unix epoch, **seconds, UTC**. Required |
| `events[].end` | int | Epoch seconds; must be `> start`. Required |
| `events[].cal` | int 0-3 | Optional per-event calendar override |

Recommended batch size: **≤ 40 events per request** (the device has 512KB of
RAM and no PSRAM); send multiple requests for more. The device stores at most
**200 events** and prunes those that ended over 7 days ago at boot.

## Response

```json
{ "queued": 38, "dropped": 2, "free": 105 }
```

`queued` events are applied asynchronously by the device main loop (usually
within ~100ms). `dropped` counts invalid entries (missing fields,
`end <= start`) or a saturated command queue — retry those. `free` is the
remaining event capacity: if your batch exceeds it, the excess is silently
discarded on apply, so warn the user when it gets low.

## Idempotency

The device hashes `uid` (FNV-1a 32-bit) into its internal event id:

- Same `uid` re-imported → **updates** the existing event, never duplicates.
- Unchanged event re-imported → no-op (no storage write, no UI refresh).
- Changed times → event updated **and its alerts re-arm** for the new time.
- Unchanged times → already-fired alert state is preserved (no re-ringing).

Recurring events must be **expanded by the client** (one entry per
occurrence) with a per-occurrence uid such as `<uid>#<startEpoch>`. The Web
UI's ICS importer does this for DAILY/WEEKLY rules within a 60-day window.

## What the client is responsible for

- Expanding recurrences and resolving timezones to UTC epochs.
- Filtering: only send events the user cares about (upcoming window).
- Chunking to ≤40 events/request.

The device is intentionally dumb here: it validates, hashes, upserts.
