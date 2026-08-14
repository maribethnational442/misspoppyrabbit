# Miss Poppy Rabbit Sync — Chrome extension

Keeps your Miss Poppy Rabbit device in sync with Google Calendar / Outlook.
No scraping, no OAuth: it fetches each calendar's **secret ICS address** in
the background and pushes the events to the device's
[import API](../docs/IMPORT_API.md). Re-syncing never duplicates events.

## Install (unpacked)

1. Open `chrome://extensions`
2. Enable **Developer mode** (top right)
3. **Load unpacked** → select this `extension/` folder
4. Pin the rabbit 🐰 to your toolbar

## Get your secret ICS addresses

- **Google Calendar**: Settings → *[your calendar]* → *Integrate calendar* →
  **Secret address in iCal format** (`https://calendar.google.com/.../basic.ics`)
- **Outlook**: Settings → Calendar → *Shared calendars* → publish a calendar →
  copy the **ICS** link

Treat these URLs like passwords — anyone who has one can read that calendar.

## Corporate calendar? (secret address disabled by your admin)

Google Workspace / Outlook admins often disable the secret ICS address. For
that case the extension has a second capture mode that **cannot be blocked**:
a content script that reads the events *you are already viewing* in your
calendar tab — via the accessibility layer (aria-labels), not the fragile
DOM, in English and Spanish UIs.

Just keep browsing your calendar normally (week by week with `n`): everything
you see gets captured. The popup shows the running count, you pick the target
device calendar, and captured events are pushed on every sync. Skipped:
all-day events and anything without a visible time range.

## Use

1. Click the rabbit → set your device URL (default `http://prabbit.local`)
2. Paste up to 4 ICS addresses and map each one to a device calendar
   (names are loaded live from the device) — and/or browse your corporate
   calendar tab to feed the capture mode
3. **Sync now**, or enable auto-sync (15m–3h; Chrome's minimum for background
   alarms is ~15 minutes)

What syncs: events in the next 60 days, including daily/weekly recurrences.
All-day events, cancelled events and monthly/yearly recurrence rules are
skipped (counted in the sync report). The browser must be on the same
network as the device.
