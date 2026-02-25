// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "web/WebTemplates.h"

namespace WebTemplates {

const char kDashboardPageTemplate[] PROGMEM = R"HTML_DASH(
<!doctype html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Aura Web Page</title>
  <link rel="icon" type="image/svg+xml" href="data:image/svg+xml;base64,PHN2ZyB4bWxucz0naHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmcnIHZpZXdCb3g9JzAgMCAxNjkzLjMyIDE2OTMuMzInPjxwYXRoIGZpbGw9JyNFNkI4NzMnIGZpbGwtcnVsZT0nZXZlbm9kZCcgY2xpcC1ydWxlPSdldmVub2RkJyBkPSdNODQ2LjY2IDE2NDIuNTNjNDM5LjU1LDAgNzk1Ljg3LC0zNTYuMzIgNzk1Ljg3LC03OTUuODcgMCwtNDM5LjU1IC0zNTYuMzIsLTc5NS44NyAtNzk1Ljg3LC03OTUuODcgLTQzOS41NSwwIC03OTUuODcsMzU2LjMyIC03OTUuODcsNzk1Ljg3IDAsNDM5LjU1IDM1Ni4zMiw3OTUuODcgNzk1Ljg3LDc5NS44N3ptLTQxOS4zMiAtMTM3MC4yOGMzMjcuMTcsLTYwLjE4IDY3NS4xNCwxNTAuNDQgNjkwLjYzLDUxMi44IDU2LjE2LC0xODAuOSAtMTEuMTksLTM3My44MiAtMTQzLjQxLC01MDMuNTIgLTY2LjY4LC02NS40IC0xNDkuNjgsLTExNC4zNiAtMjQxLjE0LC0xMzcuMDcgLTExMy4xNywxOC4xMSAtMjE3LjQsNjIuOTQgLTMwNi4wOCwxMjcuNzl6bTU4Ny44NyAzNTMuMjZjLTcyLjgsLTE3NC42NCAtMjQ5LjE4LC0yNzkuNTUgLTQzMy42OCwtMjkzLjkxIC05My4xNiwtNy4yNSAtMTg4LjA3LDguNDQgLTI3Mi40OCw0OS41NCAtNzIuNSw4My42NSAtMTI1LjgsMTg0LjM3IC0xNTMuMDUsMjk1LjM2IDIxMi4wOCwtMjU2LjcxIDYxNC4yOSwtMzE5LjMzIDg1OS4yMSwtNTAuOTl6bS0xMjAuNiAtNDg4LjQ1YzEyOC42OCw3My4xOCAyMzAuMDQsMTk0IDI3OSwzMzMuMjQgNjAuODgsMTczLjE0IDM1LjAyLDM1NC44MiAtNzkuNjgsNTAzLjcxIDE1OS4xNywtMTAyLjIzIDIzMS44NiwtMjkzLjk1IDIxMy45OCwtNDc4LjEgLTkuMDIsLTkyLjkgLTQxLjEsLTE4My43IC05Ni41MSwtMjU5Ljg3IC05My43NSwtNTYuMTQgLTIwMS41MSwtOTEuMyAtMzE2Ljc5LC05OC45OHptNDQ0LjkzIDE5Ni45N2MxMTUuODMsMzEyLjA3IC0zMC44NCw2OTEuMzEgLTM4NS41NSw3NjkuMTUgMTg3LjU3LDI0LjE0IDM2Ni43MSwtNzYuMDkgNDcxLjM0LC0yMjguNjEgNTIuOTQsLTc3LjE3IDg2LjgsLTE2Ny40NSA5My4yOCwtMjYxLjM1IC0zNy4xNiwtMTA2Ljk4IC05OS4xNiwtMjAyLjMyIC0xNzkuMDcsLTI3OS4xOXptMjE0LjI3IDQzNi41N2MtMTExLjU0LDMxMy41MiAtNDY3LjgzLDUwOS43MiAtNzg5LjQ0LDM0MS44MSAxMjguMjQsMTM4Ljc3IDMyOS44MiwxNzcuMSA1MDcuNzcsMTI3LjU5IDkwLjAzLC0yNS4wNSAxNzQuMDEsLTcyLjUzIDIzOS40NCwtMTQwLjUxIDI5LjksLTc4LjU1IDQ2LjI4LC0xNjMuNzggNDYuMjgsLTI1Mi44MyAwLC0yNS43IC0xLjM5LC01MS4wNyAtNC4wNSwtNzYuMDZ6bS0xMTYuNTcgNDcyLjM4Yy0yODcuMTIsMTY4LjIzIC02ODYuMjIsODkuOTYgLTgyNC41NCwtMjQ1Ljk4IDguNzEsMTg5LjA4IDEzOC40NSwzNDcuNzUgMzA2LjkzLDQyNC4zNiA4NS4xMywzOC43MSAxNzkuODQsNTYuMzggMjczLjQzLDQ2LjUzIDk4LjAzLC01NC43OCAxODEuNzUsLTEzMi4wOSAyNDQuMTgsLTIyNC45MXptLTM5Mi42NSAyODYuOTRjLTMyOC4xLC01NS4yOCAtNTgzLjU4LC0zNzIuMzUgLTQ3My44NywtNzE4LjM5IC0xMTQuNzgsMTUwLjY2IC0xMTcuNCwzNTUuNDIgLTM3LjU5LDUyMi40IDQwLjMxLDg0LjMzIDEwMS41NCwxNTguNzUgMTc5LjY4LDIxMS4zNCA0My4zNSw4LjI2IDg4LjEsMTIuNTkgMTMzLjg1LDEyLjU5IDY4LjY5LDAgMTM1LjEsLTkuNzcgMTk3LjkzLC0yNy45NHptLTQ4NS4zOCAtMzIuNTdjLTIxNS44MSwtMjUzLjM0IC0yMDgsLTY2MC4yOSA5OC42NCwtODU0Ljg4IC0xODQuNjksNDEuNjUgLTMxOC40MywxOTYuNzIgLTM2NC41OCwzNzUuODUgLTIzLjI5LDkwLjQgLTI0LjIsMTg2LjczIDEuODQsMjc3LjMyIDcwLjQ1LDg2LjQ0IDE2MC44MSwxNTYuMDEgMjY0LjEsMjAxLjcxem0tMzUxLjA0IC0zMzcuMTZjLTIuMTgsLTMzMy4wNiAyNjUuMzYsLTYzOS43MSA2MjUuNDgsLTU5MS4yNiAtMTY4LjE0LC04Ny4wMyAtMzcwLjYyLC01NC4wNCAtNTIxLjM0LDUzLjY2IC01OS43Miw0Mi42OCAtMTc2Ljg1LDE2NS4xNiAtMTc2Ljg1LDIyNC4wNyAwLDExMi41MyAyNi4xOCwyMTguOTQgNzIuNzEsMzEzLjUzem02MzguNDkgLTEyMy4yM2MxMDUuMSwwIDE5MC4zLC04NS4yIDE5MC4zLC0xOTAuMyAwLC0xMDUuMSAtODUuMiwtMTkwLjMgLTE5MC4zLC0xOTAuMyAtMTA1LjEsMCAtMTkwLjMsODUuMiAtMTkwLjMsMTkwLjMgMCwxMDUuMSA4NS4yLDE5MC4zIDE5MC4zLDE5MC4zeicvPjwvc3ZnPg==" />
  <link rel="apple-touch-icon" href="data:image/svg+xml;base64,PHN2ZyB4bWxucz0naHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmcnIHZpZXdCb3g9JzAgMCAxNjkzLjMyIDE2OTMuMzInPjxwYXRoIGZpbGw9JyNFNkI4NzMnIGZpbGwtcnVsZT0nZXZlbm9kZCcgY2xpcC1ydWxlPSdldmVub2RkJyBkPSdNODQ2LjY2IDE2NDIuNTNjNDM5LjU1LDAgNzk1Ljg3LC0zNTYuMzIgNzk1Ljg3LC03OTUuODcgMCwtNDM5LjU1IC0zNTYuMzIsLTc5NS44NyAtNzk1Ljg3LC03OTUuODcgLTQzOS41NSwwIC03OTUuODcsMzU2LjMyIC03OTUuODcsNzk1Ljg3IDAsNDM5LjU1IDM1Ni4zMiw3OTUuODcgNzk1Ljg3LDc5NS44N3ptLTQxOS4zMiAtMTM3MC4yOGMzMjcuMTcsLTYwLjE4IDY3NS4xNCwxNTAuNDQgNjkwLjYzLDUxMi44IDU2LjE2LC0xODAuOSAtMTEuMTksLTM3My44MiAtMTQzLjQxLC01MDMuNTIgLTY2LjY4LC02NS40IC0xNDkuNjgsLTExNC4zNiAtMjQxLjE0LC0xMzcuMDcgLTExMy4xNywxOC4xMSAtMjE3LjQsNjIuOTQgLTMwNi4wOCwxMjcuNzl6bTU4Ny44NyAzNTMuMjZjLTcyLjgsLTE3NC42NCAtMjQ5LjE4LC0yNzkuNTUgLTQzMy42OCwtMjkzLjkxIC05My4xNiwtNy4yNSAtMTg4LjA3LDguNDQgLTI3Mi40OCw0OS41NCAtNzIuNSw4My42NSAtMTI1LjgsMTg0LjM3IC0xNTMuMDUsMjk1LjM2IDIxMi4wOCwtMjU2LjcxIDYxNC4yOSwtMzE5LjMzIDg1OS4yMSwtNTAuOTl6bS0xMjAuNiAtNDg4LjQ1YzEyOC42OCw3My4xOCAyMzAuMDQsMTk0IDI3OSwzMzMuMjQgNjAuODgsMTczLjE0IDM1LjAyLDM1NC44MiAtNzkuNjgsNTAzLjcxIDE1OS4xNywtMTAyLjIzIDIzMS44NiwtMjkzLjk1IDIxMy45OCwtNDc4LjEgLTkuMDIsLTkyLjkgLTQxLjEsLTE4My43IC05Ni41MSwtMjU5Ljg3IC05My43NSwtNTYuMTQgLTIwMS41MSwtOTEuMyAtMzE2Ljc5LC05OC45OHptNDQ0LjkzIDE5Ni45N2MxMTUuODMsMzEyLjA3IC0zMC44NCw2OTEuMzEgLTM4NS41NSw3NjkuMTUgMTg3LjU3LDI0LjE0IDM2Ni43MSwtNzYuMDkgNDcxLjM0LC0yMjguNjEgNTIuOTQsLTc3LjE3IDg2LjgsLTE2Ny40NSA5My4yOCwtMjYxLjM1IC0zNy4xNiwtMTA2Ljk4IC05OS4xNiwtMjAyLjMyIC0xNzkuMDcsLTI3OS4xOXptMjE0LjI3IDQzNi41N2MtMTExLjU0LDMxMy41MiAtNDY3LjgzLDUwOS43MiAtNzg5LjQ0LDM0MS44MSAxMjguMjQsMTM4Ljc3IDMyOS44MiwxNzcuMSA1MDcuNzcsMTI3LjU5IDkwLjAzLC0yNS4wNSAxNzQuMDEsLTcyLjUzIDIzOS40NCwtMTQwLjUxIDI5LjksLTc4LjU1IDQ2LjI4LC0xNjMuNzggNDYuMjgsLTI1Mi44MyAwLC0yNS43IC0xLjM5LC01MS4wNyAtNC4wNSwtNzYuMDZ6bS0xMTYuNTcgNDcyLjM4Yy0yODcuMTIsMTY4LjIzIC02ODYuMjIsODkuOTYgLTgyNC41NCwtMjQ1Ljk4IDguNzEsMTg5LjA4IDEzOC40NSwzNDcuNzUgMzA2LjkzLDQyNC4zNiA4NS4xMywzOC43MSAxNzkuODQsNTYuMzggMjczLjQzLDQ2LjUzIDk4LjAzLC01NC43OCAxODEuNzUsLTEzMi4wOSAyNDQuMTgsLTIyNC45MXptLTM5Mi42NSAyODYuOTRjLTMyOC4xLC01NS4yOCAtNTgzLjU4LC0zNzIuMzUgLTQ3My44NywtNzE4LjM5IC0xMTQuNzgsMTUwLjY2IC0xMTcuNCwzNTUuNDIgLTM3LjU5LDUyMi40IDQwLjMxLDg0LjMzIDEwMS41NCwxNTguNzUgMTc5LjY4LDIxMS4zNCA0My4zNSw4LjI2IDg4LjEsMTIuNTkgMTMzLjg1LDEyLjU5IDY4LjY5LDAgMTM1LjEsLTkuNzcgMTk3LjkzLC0yNy45NHptLTQ4NS4zOCAtMzIuNTdjLTIxNS44MSwtMjUzLjM0IC0yMDgsLTY2MC4yOSA5OC42NCwtODU0Ljg4IC0xODQuNjksNDEuNjUgLTMxOC40MywxOTYuNzIgLTM2NC41OCwzNzUuODUgLTIzLjI5LDkwLjQgLTI0LjIsMTg2LjczIDEuODQsMjc3LjMyIDcwLjQ1LDg2LjQ0IDE2MC44MSwxNTYuMDEgMjY0LjEsMjAxLjcxem0tMzUxLjA0IC0zMzcuMTZjLTIuMTgsLTMzMy4wNiAyNjUuMzYsLTYzOS43MSA2MjUuNDgsLTU5MS4yNiAtMTY4LjE0LC04Ny4wMyAtMzcwLjYyLC01NC4wNCAtNTIxLjM0LDUzLjY2IC01OS43Miw0Mi42OCAtMTc2Ljg1LDE2NS4xNiAtMTc2Ljg1LDIyNC4wNyAwLDExMi41MyAyNi4xOCwyMTguOTQgNzIuNzEsMzEzLjUzem02MzguNDkgLTEyMy4yM2MxMDUuMSwwIDE5MC4zLC04NS4yIDE5MC4zLC0xOTAuMyAwLC0xMDUuMSAtODUuMiwtMTkwLjMgLTE5MC4zLC0xOTAuMyAtMTA1LjEsMCAtMTkwLjMsODUuMiAtMTkwLjMsMTkwLjMgMCwxMDUuMSA4NS4yLDE5MC4zIDE5MC4zLDE5MC4zeicvPjwvc3ZnPg==" />
  <script>
    window.__auraDepFailed = [];
    window.__auraDepFail = function (name, src) {
      window.__auraDepFailed.push(name + ": " + src);
    };
  </script>
  <script src="https://cdn.tailwindcss.com"></script>
  <script crossorigin src="https://cdn.jsdelivr.net/npm/react@18/umd/react.production.min.js"
          onerror="if(!this.dataset.fb){this.dataset.fb='1';this.src='https://unpkg.com/react@18/umd/react.production.min.js';}else{window.__auraDepFail('react', this.src);}"></script>
  <script crossorigin src="https://cdn.jsdelivr.net/npm/react-dom@18/umd/react-dom.production.min.js"
          onerror="if(!this.dataset.fb){this.dataset.fb='1';this.src='https://unpkg.com/react-dom@18/umd/react-dom.production.min.js';}else{window.__auraDepFail('react-dom', this.src);}"></script>
  <script src="https://cdn.jsdelivr.net/npm/@babel/standalone@7.26.7/babel.min.js"
          onerror="if(!this.dataset.fb){this.dataset.fb='1';this.src='https://unpkg.com/@babel/standalone@7.26.7/babel.min.js';}else{window.__auraDepFail('babel', this.src);}"></script>
  <style>
    html, body { margin: 0; background: #030712; }
    #preview-error {
      display: none;
      position: fixed;
      inset: 12px;
      z-index: 9999;
      background: rgba(127, 29, 29, 0.95);
      color: #fee2e2;
      border: 1px solid #fca5a5;
      border-radius: 10px;
      padding: 12px;
      font: 12px/1.4 Consolas, "Courier New", monospace;
      white-space: pre-wrap;
      overflow: auto;
    }
  </style>
</head>
<body>
  <pre id="preview-error"></pre>
  <div id="root"></div>
  <script>
    function auraShowError(message) {
      var box = document.getElementById("preview-error");
      box.style.display = "block";
      box.textContent = message;
    }

    window.addEventListener("error", function (event) {
      // Ignore raw script resource load errors here; dependency checker handles those.
      if (!event.error && event.target && event.target.tagName === "SCRIPT") {
        return;
      }
      var where = "";
      if (event.filename) {
        where = "\nAt: " + event.filename + ":" + (event.lineno || 0) + ":" + (event.colno || 0);
      }
      auraShowError(
        "Preview runtime error:\n" +
        (event.error && event.error.stack ? event.error.stack : (event.message || "Unknown script error")) +
        where
      );
    });

    window.addEventListener("load", function () {
      setTimeout(function () {
        var missingCore = [];
        if (!window.React) missingCore.push("React");
        if (!window.ReactDOM) missingCore.push("ReactDOM");
        if (!window.Babel) missingCore.push("Babel");

        if (!missingCore.length) return;

        var hint = (navigator && navigator.onLine === false)
          ? "Phone appears offline. In AP mode, CDN scripts may be unreachable without internet."
          : "Some CDN scripts failed to load on this network/browser.";
        var failed = (window.__auraDepFailed && window.__auraDepFailed.length)
          ? ("\nFailed: " + window.__auraDepFailed.join(", "))
          : "";

        auraShowError(
          "Web dashboard dependencies failed to load.\n" +
          "Missing: " + missingCore.join(", ") + failed + "\n" +
          hint + "\n" +
          "Try home Wi-Fi with internet or another browser."
        );
      }, 700);
    });
  </script>
  <script type="text/babel" data-presets="react">
/*__INLINE_APP_START__*/
// Dashboard web UI template embedded into firmware.
// Edit this file directly.
const { useState, useMemo, useEffect } = React;

const iconBaseProps = {
  fill: "none",
  stroke: "currentColor",
  strokeWidth: 2,
  strokeLinecap: "round",
  strokeLinejoin: "round",
  "aria-hidden": "true",
};

const svgIcon = (draw) => ({ size = 14, className = "" }) => (
  <svg viewBox="0 0 24 24" width={size} height={size} className={className} {...iconBaseProps}>
    {draw}
  </svg>
);

// Icon fallbacks for no-build local preview (replace text glyph stubs).
const Moon = svgIcon(<path d="M21 12.8A9 9 0 1 1 11.2 3a7 7 0 1 0 9.8 9.8z" />);
const Sun = svgIcon(<><circle cx="12" cy="12" r="4" /><path d="M12 2v2.2M12 19.8V22M4.2 4.2l1.6 1.6M18.2 18.2l1.6 1.6M2 12h2.2M19.8 12H22M4.2 19.8l1.6-1.6M18.2 5.8l1.6-1.6" /></>);
const RotateCw = svgIcon(<><path d="M21 2v6h-6" /><path d="M3 12a9 9 0 0 1 15-6.7L21 8" /><path d="M3 22v-6h6" /><path d="M21 12a9 9 0 0 1-15 6.7L3 16" /></>);
const UploadIcon = svgIcon(<><path d="M12 16V4" /><path d="m7 9 5-5 5 5" /><path d="M20 16.5v1a2.5 2.5 0 0 1-2.5 2.5h-11A2.5 2.5 0 0 1 4 17.5v-1" /></>);
const Pencil = svgIcon(<><path d="M12 20h9" /><path d="M16.5 3.5a2.1 2.1 0 1 1 3 3L7 19l-4 1 1-4 12.5-12.5z" /></>);
const Check = svgIcon(<path d="M20 6 9 17l-5-5" />);
const X = svgIcon(<path d="m6 6 12 12M18 6 6 18" />);

const PREVIEW_HOSTNAME = (() => {
  const rawHost = (typeof window !== 'undefined' && window.location && window.location.hostname)
    ? window.location.hostname
    : '';
  const cleanHost = rawHost.replace(/\.local$/i, '').trim();
  return cleanHost || 'aura';
})();

const formatChartTime = (epochSeconds) => {
  if (typeof epochSeconds !== 'number' || !Number.isFinite(epochSeconds)) {
    return '';
  }
  const date = new Date(epochSeconds * 1000);
  if (Number.isNaN(date.getTime())) {
    return '';
  }
  return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', hour12: false });
};

const formatHeaderTime = (date) =>
  date.toLocaleTimeString([], {
    hour: '2-digit',
    minute: '2-digit',
    hour12: false,
  });

const formatHeaderDate = (date) =>
  date
    .toLocaleDateString('en-GB', {
      day: '2-digit',
      month: 'short',
      year: 'numeric',
    })
    .toUpperCase();

const parseChartApiPayload = (payload) => {
  if (!payload || payload.success !== true || !Array.isArray(payload.timestamps) || !Array.isArray(payload.series)) {
    throw new Error('Invalid chart payload');
  }

  const timestamps = payload.timestamps;
  const pointCount = timestamps.length;
  const points = Array.from({ length: pointCount }, (_, index) => ({
    time: formatChartTime(timestamps[index]),
    epoch: typeof timestamps[index] === 'number' && Number.isFinite(timestamps[index]) ? timestamps[index] : null,
  }));
  const latest = {};

  payload.series.forEach((series) => {
    if (!series || typeof series.key !== 'string') return;
    const values = Array.isArray(series.values) ? series.values : [];
    latest[series.key] =
      typeof series.latest === 'number' && Number.isFinite(series.latest) ? series.latest : null;

    for (let i = 0; i < pointCount; i++) {
      const raw = values[i];
      points[i][series.key] = typeof raw === 'number' && Number.isFinite(raw) ? raw : null;
    }
  });

  // Backward-compat shim: /api/charts uses temperature/humidity, while /api/state uses temp/rh.
  points.forEach((point) => {
    if (Object.prototype.hasOwnProperty.call(point, 'temperature')) point.temp = point.temperature;
    if (Object.prototype.hasOwnProperty.call(point, 'humidity')) point.rh = point.humidity;
  });
  if (Object.prototype.hasOwnProperty.call(latest, 'temperature')) latest.temp = latest.temperature;
  if (Object.prototype.hasOwnProperty.call(latest, 'humidity')) latest.rh = latest.humidity;

  let latestEpoch = null;
  for (let i = timestamps.length - 1; i >= 0; i--) {
    const ts = timestamps[i];
    if (typeof ts === 'number' && Number.isFinite(ts)) {
      latestEpoch = ts;
      break;
    }
  }

  return { points, latest, latestEpoch };
};

const finiteNumberOrNull = (value) =>
  typeof value === 'number' && Number.isFinite(value) ? value : null;

const stringOrNull = (value) =>
  typeof value === 'string' && value.trim().length > 0 ? value : null;

const boolOrNull = (value) =>
  typeof value === 'boolean' ? value : null;

const parseSettingsPayload = (settings) => ({
  nightMode: boolOrNull(settings?.night_mode),
  nightModeLocked: boolOrNull(settings?.night_mode_locked),
  backlight: boolOrNull(settings?.backlight_on),
  tempUnit: boolOrNull(settings?.units_c) === null ? null : (settings.units_c ? 'c' : 'f'),
  tempOffset: finiteNumberOrNull(settings?.temp_offset),
  humOffset: finiteNumberOrNull(settings?.hum_offset),
  displayName: stringOrNull(settings?.display_name),
});

const DEFAULT_WEB_SETTINGS = {
  nightMode: false,
  nightModeLocked: false,
  backlight: true,
  tempUnit: 'c',
  tempOffset: -1.2,
  humOffset: 2,
  displayName: null,
};

const settingsPatchFromParsed = (parsed) => {
  const patch = {};
  if (typeof parsed?.nightMode === 'boolean') patch.nightMode = parsed.nightMode;
  if (typeof parsed?.nightModeLocked === 'boolean') patch.nightModeLocked = parsed.nightModeLocked;
  if (typeof parsed?.backlight === 'boolean') patch.backlight = parsed.backlight;
  if (parsed?.tempUnit === 'c' || parsed?.tempUnit === 'f') patch.tempUnit = parsed.tempUnit;
  if (typeof parsed?.tempOffset === 'number' && Number.isFinite(parsed.tempOffset)) {
    patch.tempOffset = Number(parsed.tempOffset.toFixed(1));
  }
  if (typeof parsed?.humOffset === 'number' && Number.isFinite(parsed.humOffset)) {
    patch.humOffset = Number(parsed.humOffset.toFixed(0));
  }
  if (typeof parsed?.displayName === 'string' || parsed?.displayName === null) {
    patch.displayName = parsed.displayName;
  }
  return patch;
};

const parseStateApiPayload = (payload) => {
  if (!payload || payload.success !== true) {
    throw new Error('Invalid state payload');
  }

  const sensors = payload.sensors || {};
  const derived = payload.derived || {};
  const network = payload.network || {};
  const system = payload.system || {};
  const settings = payload.settings || {};

  return {
    deviceTimeEpochS: finiteNumberOrNull(payload.time_epoch_s),
    current: {
      co2: finiteNumberOrNull(sensors.co2),
      temp: finiteNumberOrNull(sensors.temp),
      rh: finiteNumberOrNull(sensors.rh),
      pressure: finiteNumberOrNull(sensors.pressure),
      pm05: finiteNumberOrNull(sensors.pm05),
      pm1: finiteNumberOrNull(sensors.pm1),
      pm25: finiteNumberOrNull(sensors.pm25),
      pm4: finiteNumberOrNull(sensors.pm4),
      pm10: finiteNumberOrNull(sensors.pm10),
      voc: finiteNumberOrNull(sensors.voc),
      nox: finiteNumberOrNull(sensors.nox),
      hcho: finiteNumberOrNull(sensors.hcho),
      co: finiteNumberOrNull(sensors.co),
    },
    derived: {
      ah: finiteNumberOrNull(derived.ah),
      dewPoint: finiteNumberOrNull(derived.dew_point),
      mold: finiteNumberOrNull(derived.mold),
      delta3h: finiteNumberOrNull(derived.pressure_delta_3h),
      delta24h: finiteNumberOrNull(derived.pressure_delta_24h),
      uptime: stringOrNull(derived.uptime),
    },
    connectivity: {
      wifiSsid: stringOrNull(network.wifi_ssid),
      hostname: stringOrNull(network.hostname),
      ip: stringOrNull(network.ip),
      rssi: finiteNumberOrNull(network.rssi),
      mqttBroker: stringOrNull(network.mqtt_broker),
      mqttConnected: typeof network.mqtt_connected === 'boolean' ? network.mqtt_connected : null,
    },
    system: {
      firmware: stringOrNull(system.firmware),
      buildDate: stringOrNull(system.build_date),
      buildTime: stringOrNull(system.build_time),
      uptime: stringOrNull(system.uptime),
      dacAvailable: typeof system.dac_available === 'boolean' ? system.dac_available : null,
    },
    settings: parseSettingsPayload(settings),
  };
};

const formatEventAge = (ageSeconds) => {
  if (typeof ageSeconds !== 'number' || !Number.isFinite(ageSeconds) || ageSeconds < 0) {
    return '--';
  }
  if (ageSeconds < 60) return `${Math.round(ageSeconds)}s ago`;
  if (ageSeconds < 3600) return `${Math.floor(ageSeconds / 60)}m ago`;
  if (ageSeconds < 86400) return `${Math.floor(ageSeconds / 3600)}h ago`;
  return `${Math.floor(ageSeconds / 86400)}d ago`;
};

const parseEventsApiPayload = (payload) => {
  if (!payload || payload.success !== true || !Array.isArray(payload.events)) {
    throw new Error('Invalid events payload');
  }

  const uptimeSeconds = finiteNumberOrNull(payload.uptime_s);
  const entries = payload.events
    .map((entry) => {
      const tsMs = finiteNumberOrNull(entry?.ts_ms);
      const ageSeconds =
        uptimeSeconds !== null && tsMs !== null ? Math.max(0, uptimeSeconds - Math.floor(tsMs / 1000)) : null;

      const severityRaw = stringOrNull(entry?.severity);
      const severity =
        severityRaw === 'critical' || severityRaw === 'danger' || severityRaw === 'warning' || severityRaw === 'info'
          ? (severityRaw === 'critical' ? 'critical' : severityRaw)
          : 'info';

      return {
        time: formatEventAge(ageSeconds),
        type: stringOrNull(entry?.type) || 'SYSTEM',
        message: stringOrNull(entry?.message) || 'Event',
        severity,
      };
    })
    .filter((entry) => entry.message.length > 0);

  return entries.reverse();
};

// ============ THRESHOLDS & COLORS ============
const thresholds = {
  // Synced to firmware thresholds (AppConfig.h + UiController.cpp)
  co2: { good: 800, moderate: 1000, bad: 1500 },
  pm05: { good: 250, moderate: 600, bad: 1200 }, // #/cm3
  pm25: { good: 12, moderate: 35, bad: 55 },
  pm1: { good: 10, moderate: 25, bad: 50 },
  pm4: { good: 25, moderate: 50, bad: 75 },
  pm10: { good: 54, moderate: 154, bad: 254 },
  voc: { good: 150, moderate: 250, bad: 350 },
  nox: { good: 50, moderate: 100, bad: 200 },
  hcho: { good: 30, moderate: 60, bad: 100 }, // ppb
  co: { good: 9, moderate: 35, bad: 100 }, // ppm
  temp: { good: [20, 25], moderate: [18, 26], bad: [16, 28] },
  rh: { good: [40, 60], moderate: [30, 65], bad: [20, 70] },
  dewPoint: { good: [11, 16], moderate: [9, 18], bad: [5, 21] }, // C
  ah: { good: [7, 15], moderate: [5, 18], bad: [4, 20] }, // g/m3
  mold: { good: 2, moderate: 4, bad: 7 }, // 0-10 index
  pressureDelta3h: { good: 1.0, moderate: 3.0, bad: 6.0 }, // abs(hPa)
  pressureDelta24h: { good: 2.0, moderate: 6.0, bad: 10.0 }, // abs(hPa)
};

const getStatus = (value, threshold) => {
  if (Array.isArray(threshold.good)) {
    // Range-based (temp, humidity)
    if (value >= threshold.good[0] && value <= threshold.good[1]) return 'good';
    if (value >= threshold.moderate[0] && value <= threshold.moderate[1]) return 'moderate';
    if (value >= threshold.bad[0] && value <= threshold.bad[1]) return 'bad';
    return 'critical';
  } else {
    // Upper limit based
    if (value <= threshold.good) return 'good';
    if (value <= threshold.moderate) return 'moderate';
    if (value <= threshold.bad) return 'bad';
    return 'critical';
  }
};

const statusColors = {
  good: '#22c55e',
  moderate: '#facc15',
  bad: '#f97316',
  critical: '#ef4444',
};

const statusLabels = {
  good: 'Good',
  moderate: 'Moderate',
  bad: 'Poor',
  critical: 'Hazard',
};

const fallbackStatusColor = '#9ca3af';

const statusColorOf = (status) => statusColors[status] || fallbackStatusColor;

const hexToRgb = (hexColor) => {
  if (typeof hexColor !== 'string') return null;
  const trimmed = hexColor.trim();
  if (!trimmed.startsWith('#')) return null;

  let hex = trimmed.slice(1);
  if (hex.length === 3) {
    hex = hex.split('').map((ch) => ch + ch).join('');
  }
  if (hex.length !== 6 || !/^[0-9a-fA-F]{6}$/.test(hex)) return null;

  const num = parseInt(hex, 16);
  return {
    r: (num >> 16) & 255,
    g: (num >> 8) & 255,
    b: num & 255,
  };
};

const rgba = (hexColor, alpha) => {
  const rgb = hexToRgb(hexColor);
  if (!rgb) return `rgba(156, 163, 175, ${alpha})`;
  return `rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, ${alpha})`;
};

const statusPillStyle = (status) => {
  const color = statusColorOf(status);
  return {
    color,
    borderColor: rgba(color, 0.38),
    backgroundColor: rgba(color, 0.14),
  };
};

const statusTextStyle = (status) => ({
  color: statusColorOf(status),
});

const statusSurfaceStyle = (status) => {
  const color = statusColorOf(status);
  return {
    color,
    borderColor: rgba(color, 0.32),
    backgroundColor: rgba(color, 0.12),
  };
};

// ============ COMPONENTS ============

const StatusPill = ({ status, compact = false }) => {
  const sizeClass = compact
    ? 'px-2 py-0.5 text-[10px] md:text-[11px]'
    : 'px-2.5 py-1 text-[11px] md:text-xs';
  const fallbackPillStyle = {
    color: '#e5e7eb',
    borderColor: 'rgba(107, 114, 128, 0.35)',
    backgroundColor: 'rgba(75, 85, 99, 0.2)',
  };
  return (
    <span
      className={`${sizeClass} rounded-full border font-semibold`}
      style={status ? statusPillStyle(status) : fallbackPillStyle}
    >
      {statusLabels[status] || 'N/A'}
    </span>
  );
};

const isFiniteNumber = (v) => typeof v === 'number' && Number.isFinite(v);
const clampNumber = (value, min, max) => Math.max(min, Math.min(max, value));
const tempOffsetCToDisplay = (offsetC, tempUnit) =>
  tempUnit === 'f' ? (offsetC * 9) / 5 : offsetC;
const tempOffsetDisplayToC = (offsetDisplay, tempUnit) =>
  tempUnit === 'f' ? (offsetDisplay * 5) / 9 : offsetDisplay;

const metricStatus = (value, threshold, { round = false } = {}) => {
  if (!isFiniteNumber(value)) return null;
  const normalized = round ? Math.round(value) : value;
  return getStatus(normalized, threshold);
};

const formatMetricValue = (value, decimals = 1) =>
  isFiniteNumber(value) ? Number(value).toFixed(decimals) : 'N/A';

const formatSignedMetricValue = (value, decimals = 1) => {
  if (!isFiniteNumber(value)) return 'N/A';
  return `${value > 0 ? '+' : ''}${Number(value).toFixed(decimals)}`;
};

const formatFileSize = (bytes) => {
  const value = Number(bytes);
  if (!Number.isFinite(value) || value <= 0) return '0 B';
  if (value < 1024) return `${Math.round(value)} B`;
  const kb = value / 1024;
  if (kb < 1024) return `${kb.toFixed(1)} KB`;
  const mb = kb / 1024;
  return `${mb.toFixed(2)} MB`;
};

const splitSeriesSegments = (points) => {
  const segments = [];
  let current = [];
  points.forEach((point) => {
    if (!isFiniteNumber(point.value)) {
      if (current.length) segments.push(current);
      current = [];
      return;
    }
    current.push(point);
  });
  if (current.length) segments.push(current);
  return segments;
};

const buildSmoothPath = (segment) => {
  if (!segment?.length) return '';
  if (segment.length === 1) {
    const p = segment[0];
    return `M ${p.x.toFixed(2)} ${p.y.toFixed(2)}`;
  }
  if (segment.length === 2) {
    const [p0, p1] = segment;
    return `M ${p0.x.toFixed(2)} ${p0.y.toFixed(2)} L ${p1.x.toFixed(2)} ${p1.y.toFixed(2)}`;
  }

  let d = `M ${segment[0].x.toFixed(2)} ${segment[0].y.toFixed(2)}`;
  for (let i = 0; i < segment.length - 1; i++) {
    const p0 = segment[i - 1] || segment[i];
    const p1 = segment[i];
    const p2 = segment[i + 1];
    const p3 = segment[i + 2] || p2;

    const cp1x = p1.x + (p2.x - p0.x) / 6;
    const cp1y = clampNumber(p1.y + (p2.y - p0.y) / 6, 0, 100);
    const cp2x = p2.x - (p3.x - p1.x) / 6;
    const cp2y = clampNumber(p2.y - (p3.y - p1.y) / 6, 0, 100);

    d += ` C ${cp1x.toFixed(2)} ${cp1y.toFixed(2)}, ${cp2x.toFixed(2)} ${cp2y.toFixed(2)}, ${p2.x.toFixed(2)} ${p2.y.toFixed(2)}`;
  }
  return d;
};

const formatChartValue = (value, unit = '') => {
  if (!isFiniteNumber(value)) return '-';
  const base = value.toFixed(1);
  const unitTrimmed = (unit || '').trim();
  return unitTrimmed ? `${base} ${unitTrimmed}` : base;
};

const formatMinMaxNumber = (value) => {
  if (!isFiniteNumber(value)) return '-';
  const abs = Math.abs(value);
  if (abs >= 100) return value.toFixed(0);
  return value.toFixed(1);
};

const formatMinMaxValue = (value, unit = '') => {
  const base = formatMinMaxNumber(value);
  const unitTrimmed = (unit || '').trim();
  if (base === '-' || !unitTrimmed) return base;
  return `${base} ${unitTrimmed}`;
};

const SvgTrendChart = ({ data = [], lines = [], lineColors = [], showGrid = true, unit = '' }) => {
  const [hoverIndex, setHoverIndex] = useState(null);
  const gradientSeed = useMemo(() => `sg_${Math.random().toString(36).slice(2, 9)}`, []);

  const model = useMemo(() => {
    const normalizedLines = lines
      .map((line, index) => ({
        key: line.key,
        name: line.name || line.key.toUpperCase(),
        color: lineColors[index] || line.color || '#22c55e',
      }))
      .filter((line) => line.key);

    if (!normalizedLines.length || data.length < 2) return null;

    const values = [];
    normalizedLines.forEach((line) => {
      data.forEach((row) => {
        const raw = row?.[line.key];
        if (isFiniteNumber(raw)) values.push(raw);
      });
    });

    if (!values.length) return null;

    let min = Math.min(...values);
    let max = Math.max(...values);
    if (!Number.isFinite(min) || !Number.isFinite(max)) return null;

    const spread = max - min;
    const pad = spread > 1e-6 ? spread * 0.12 : Math.max(Math.abs(max) * 0.08, 1);
    let yMin = min - pad;
    let yMax = max + pad;
    if (values.every((v) => v >= 0) && yMin < 0) yMin = 0;
    if (Math.abs(yMax - yMin) < 1e-6) {
      yMin -= 1;
      yMax += 1;
    }

    const xFor = (index) => (data.length <= 1 ? 0 : (index / (data.length - 1)) * 100);
    const yFor = (value) => ((yMax - value) / (yMax - yMin)) * 100;

    const lineModels = normalizedLines.map((line) => {
      const points = data.map((row, index) => {
        const raw = row?.[line.key];
        const value = isFiniteNumber(raw) ? raw : null;
        return {
          i: index,
          x: xFor(index),
          y: value === null ? null : yFor(value),
          value,
          time: row?.time || '--:--',
        };
      });

      const segments = splitSeriesSegments(points);
      const linePaths = segments.map((segment) => buildSmoothPath(segment)).filter((d) => d.length > 0);
      const areaPaths = segments
        .filter((segment) => segment.length >= 2)
        .map((segment) => {
          const d = buildSmoothPath(segment);
          const first = segment[0];
          const last = segment[segment.length - 1];
          return `${d} L ${last.x.toFixed(2)} 100 L ${first.x.toFixed(2)} 100 Z`;
        });

      return { ...line, points, segments, linePaths, areaPaths };
    });

    return { lineModels, xFor, yFor };
  }, [data, lines, lineColors]);

  if (!model) {
    return (
      <div className="w-full h-full rounded-lg border border-dashed border-gray-600/60 bg-gray-900/35 flex items-center justify-center px-3">
        <span className="text-[11px] md:text-xs text-gray-400 text-center">No data / Awaiting data</span>
      </div>
    );
  }

  const hoverActive = hoverIndex !== null && hoverIndex >= 0 && hoverIndex < data.length;
  const hoverRatio = hoverActive && data.length > 1 ? hoverIndex / (data.length - 1) : 0.5;
  const hoverX = hoverActive ? model.xFor(hoverIndex) : null;
  const tooltipLeft = clampNumber(hoverRatio * 100, 12, 88);
  const hoverTimeText = hoverActive ? (data[hoverIndex]?.time || '--:--') : '--:--';

  const tooltipItems = hoverActive
    ? model.lineModels
        .map((line) => {
          const point = line.points[hoverIndex];
          if (!point || !isFiniteNumber(point.value)) return null;
          return {
            key: line.key,
            name: line.name,
            color: line.color,
            text: formatChartValue(point.value, unit),
            y: point.y,
          };
        })
        .filter(Boolean)
    : [];

  const setHoverFromClientX = (clientX, target) => {
    const rect = target.getBoundingClientRect();
    if (!rect.width || data.length < 2) return;
    const ratio = clampNumber((clientX - rect.left) / rect.width, 0, 1);
    setHoverIndex(Math.round(ratio * (data.length - 1)));
  };

  const handleMouseMove = (event) => setHoverFromClientX(event.clientX, event.currentTarget);
  const handleTouchMove = (event) => {
    if (!event.touches || !event.touches[0]) return;
    setHoverFromClientX(event.touches[0].clientX, event.currentTarget);
  };

  return (
    <div className="relative w-full h-full">
      <svg viewBox="0 0 100 100" preserveAspectRatio="none" className="w-full h-full">
        <defs>
          {model.lineModels.map((line) => (
            <linearGradient key={`grad_${line.key}`} id={`${gradientSeed}_${line.key}`} x1="0" y1="0" x2="0" y2="1">
              <stop offset="5%" stopColor={line.color} stopOpacity={0.28} />
              <stop offset="95%" stopColor={line.color} stopOpacity={0} />
            </linearGradient>
          ))}
        </defs>

        {showGrid && (
          <>
            <line x1="0" y1="25" x2="100" y2="25" stroke="#374151" strokeWidth="0.7" strokeDasharray="2.5 2.5" vectorEffect="non-scaling-stroke" />
            <line x1="0" y1="50" x2="100" y2="50" stroke="#374151" strokeWidth="0.7" strokeDasharray="2.5 2.5" vectorEffect="non-scaling-stroke" />
            <line x1="0" y1="75" x2="100" y2="75" stroke="#374151" strokeWidth="0.7" strokeDasharray="2.5 2.5" vectorEffect="non-scaling-stroke" />
          </>
        )}

        {model.lineModels.map((line) =>
          line.areaPaths.map((pathD, idx) => (
            <path key={`area_${line.key}_${idx}`} d={pathD} fill={`url(#${gradientSeed}_${line.key})`} />
          ))
        )}

        {hoverActive && hoverX !== null && (
          <line
            x1={hoverX}
            y1="0"
            x2={hoverX}
            y2="100"
            stroke="#64748b"
            strokeWidth="0.8"
            strokeDasharray="2 2"
            vectorEffect="non-scaling-stroke"
          />
        )}

        {model.lineModels.map((line) =>
          line.linePaths.map((pathD, idx) => (
            <path
              key={`line_${line.key}_${idx}`}
              d={pathD}
              fill="none"
              stroke={line.color}
              strokeWidth="2"
              strokeLinecap="round"
              strokeLinejoin="round"
              vectorEffect="non-scaling-stroke"
            />
          ))
        )}
      </svg>

      <div
        className="absolute inset-0"
        onMouseMove={handleMouseMove}
        onMouseLeave={() => setHoverIndex(null)}
        onTouchStart={handleTouchMove}
        onTouchMove={handleTouchMove}
        onTouchEnd={() => setHoverIndex(null)}
      />

      {hoverActive && (
        <div
          className="absolute z-10 pointer-events-none bg-gray-900/95 border border-gray-600 rounded-lg shadow-xl px-2.5 py-2 text-[11px] md:text-xs"
          style={{ left: `${tooltipLeft}%`, top: '6px', transform: 'translateX(-50%)' }}
        >
          <div className={`text-gray-400 font-medium ${tooltipItems.length > 0 ? 'mb-1' : ''}`}>{hoverTimeText}</div>
          {tooltipItems.length > 0 && (
            <div className="space-y-1">
              {tooltipItems.map((item) => (
                <div key={`tt_${item.key}`} className="flex items-center justify-between gap-3 min-w-[120px]">
                  <span className="font-medium" style={{ color: item.color }}>{item.name}</span>
                  <span className="text-white font-semibold">{item.text}</span>
                </div>
              ))}
            </div>
          )}
        </div>
      )}

      {hoverActive &&
        hoverX !== null &&
        tooltipItems.map((item) => (
          <div
            key={`dot_overlay_${item.key}`}
            className="absolute pointer-events-none rounded-full border border-slate-900"
            style={{
              left: `${hoverX}%`,
              top: `${item.y}%`,
              width: '8px',
              height: '8px',
              backgroundColor: item.color,
              boxShadow: `0 0 0 4px color-mix(in srgb, ${item.color} 28%, transparent)`,
              transform: 'translate(-50%, -50%)',
            }}
          />
        ))}
    </div>
  );
};

const HeroMetric = ({ value, status, history = [] }) => {
  const advice = {
    good: 'Air is stable. Ventilation is optional.',
    moderate: 'Ventilation recommended in the next hour.',
    bad: 'Open windows or increase airflow now.',
    critical: 'Poor air quality. Ventilate immediately.',
  };
  const safeValue = isFiniteNumber(value) ? value : null;
  const accentColor = statusColorOf(status);
  const adviceText = advice[status] || 'No live CO2 data.';
  const co2SeriesAll = history.length
    ? history.map((point) => ({ time: point.time, co2: isFiniteNumber(point.co2) ? point.co2 : null }))
    : [{ time: '--:--', co2: safeValue }];

  const co2SeriesWindow = co2SeriesAll.slice(Math.max(0, co2SeriesAll.length - 36)); // 3h, 5-minute step
  const co2Series = [...co2SeriesWindow];
  const lastCo2Point = co2Series[co2Series.length - 1];
  if (
    isFiniteNumber(safeValue) &&
    (!lastCo2Point || !isFiniteNumber(lastCo2Point.co2) || Math.abs(lastCo2Point.co2 - safeValue) > 0.01)
  ) {
    co2Series.push({
      time: lastCo2Point?.time || '--:--',
      co2: safeValue,
    });
  }

  const co2ValidPoints = co2Series.filter((point) => isFiniteNumber(point.co2));
  const co2Stats = co2ValidPoints.length
    ? co2ValidPoints.reduce(
        (acc, point) => ({
          min: Math.min(acc.min, Number(point.co2)),
          max: Math.max(acc.max, Number(point.co2)),
        }),
        { min: Number(co2ValidPoints[0].co2), max: Number(co2ValidPoints[0].co2) }
      )
    : null;
  const co2BasePoint = co2ValidPoints.length > 0 ? co2ValidPoints[0] : null;
  const delta3h = co2BasePoint && isFiniteNumber(safeValue) ? safeValue - Number(co2BasePoint.co2) : null;
  const deltaColorClass = !isFiniteNumber(delta3h)
    ? 'text-gray-400'
    : delta3h > 20
      ? 'text-orange-300'
      : delta3h < -20
        ? 'text-cyan-300'
        : 'text-gray-300';
  const progressWidth = isFiniteNumber(safeValue) ? Math.min((safeValue / thresholds.co2.bad) * 100, 100) : 0;

  return (
    <div className="bg-gradient-to-br from-gray-800 to-gray-900 rounded-2xl p-5 md:p-7 border border-gray-700/60 shadow-xl h-full flex flex-col">
      <div className="flex items-start justify-between gap-4">
        <div>
          <div className="text-[12px] md:text-sm uppercase tracking-wider text-gray-400 font-semibold">CO2 Level</div>
          <div className="mt-3 flex items-end gap-2">
            <span className="text-6xl md:text-7xl font-semibold leading-none" style={{ color: accentColor }}>{formatMetricValue(safeValue, 0)}</span>
            <span className="text-base md:text-lg text-gray-400 pb-1">ppm</span>
          </div>
        </div>
        <StatusPill status={status} />
      </div>
      <div className="mt-4 h-2.5 bg-gray-700/80 rounded-full overflow-hidden">
        <div
          className="h-full rounded-full transition-all duration-700"
          style={{ width: `${progressWidth}%`, backgroundColor: accentColor }}
        />
      </div>
      <div className="mt-3 text-sm md:text-base text-gray-300">{adviceText}</div>

      <div className="mt-5 pt-4 border-t border-gray-700/60">
        <div className="flex items-center justify-between">
          <span className="text-[10px] md:text-xs uppercase tracking-wide text-gray-400 font-semibold">3h Trend</span>
          <span className={`text-xs md:text-sm font-semibold ${deltaColorClass}`}>
            {isFiniteNumber(delta3h) ? `${delta3h > 0 ? '+' : ''}${delta3h.toFixed(0)} ppm` : 'N/A'}
          </span>
        </div>
        <div className="mt-2 h-20 md:h-24">
          <SvgTrendChart
            data={co2Series}
            lines={[{ key: 'co2' }]}
            lineColors={[accentColor]}
            showGrid={true}
            unit="ppm"
          />
        </div>
        <div className="mt-2 flex items-center justify-between text-[11px] md:text-xs text-gray-400">
          <span>min {formatMinMaxValue(co2Stats?.min, 'ppm')}</span>
          <span>max {formatMinMaxValue(co2Stats?.max, 'ppm')}</span>
        </div>
      </div>
    </div>
  );
};

const ClimateOverview = ({
  temp,
  tempStatus,
  rh,
  rhStatus,
  dewPoint,
  ah,
  mold,
  moldStatus,
  pressure,
  delta3h,
  delta24h,
  pressureTrend3h,
  pressureTrend24h,
}) => {
  const statusRank = { good: 0, moderate: 1, bad: 2, critical: 3 };
  const climateCandidates = [tempStatus, rhStatus, moldStatus].filter((entry) =>
    Object.prototype.hasOwnProperty.call(statusRank, entry)
  );
  const climateStatus = climateCandidates.length
    ? climateCandidates.reduce((worst, entry) => (statusRank[entry] > statusRank[worst] ? entry : worst), climateCandidates[0])
    : null;
  // Device UI colors dew point by rounded value (roundf), mirror it for exact match.
  const dewPointStatus = metricStatus(dewPoint, thresholds.dewPoint, { round: true });
  const ahStatus = metricStatus(ah, thresholds.ah);
  const miniCardClass = "rounded-xl bg-gray-700/30 border border-gray-600/40 p-3 md:p-4";
  const climateLabelClass = "text-[10px] md:text-[11px] uppercase tracking-[0.08em] text-gray-400 font-semibold whitespace-nowrap";
  const unitClass = "text-sm md:text-base text-gray-300 leading-none self-end";

  return (
    <div className="bg-gradient-to-br from-gray-800 to-gray-900 rounded-2xl px-5 pt-5 pb-3 md:px-6 md:pt-6 md:pb-4 border border-gray-700/60 h-full flex flex-col">
      <div className="flex items-start justify-between gap-3">
        <div className="text-[12px] md:text-sm uppercase tracking-wide text-gray-300 font-semibold">Climate</div>
        <StatusPill status={climateStatus} />
      </div>

      <div className="mt-4 flex-1 flex flex-col justify-between gap-3">
        <div className="grid grid-cols-2 gap-3 md:gap-4">
          <div className={miniCardClass}>
            <div className="text-[10px] md:text-xs uppercase tracking-wide text-gray-400 font-semibold">Temperature</div>
            <div className="mt-2 flex items-end gap-1.5">
              <span className="text-3xl md:text-4xl font-semibold leading-none" style={{ color: statusColorOf(tempStatus) }}>{formatMetricValue(temp, 1)}</span>
              <span className={unitClass}>{'\u00B0C'}</span>
            </div>
          </div>
          <div className={miniCardClass}>
            <div className="text-[10px] md:text-xs uppercase tracking-wide text-gray-400 font-semibold">Humidity</div>
            <div className="mt-2 flex items-end gap-1.5">
              <span className="text-3xl md:text-4xl font-semibold leading-none" style={{ color: statusColorOf(rhStatus) }}>{formatMetricValue(rh, 0)}</span>
              <span className={unitClass}>%</span>
            </div>
          </div>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-3 md:gap-4">
          <div className={`${miniCardClass} h-full`}>
            <div className={climateLabelClass}>Mold Risk</div>
            <div className="mt-1.5 flex items-end gap-1.5">
              <span className="text-3xl md:text-4xl font-semibold leading-none" style={{ color: statusColorOf(moldStatus) }}>{formatMetricValue(mold, 1)}</span>
              <span className={unitClass}>/10</span>
            </div>
          </div>

          <div className={`${miniCardClass} h-full`}>
            <div className={climateLabelClass}>Dew Point</div>
            <div className="mt-1.5 flex items-end gap-1.5">
              <span className="text-3xl md:text-4xl font-semibold leading-none" style={{ color: statusColorOf(dewPointStatus) }}>{formatMetricValue(dewPoint, 1)}</span>
              <span className={unitClass}>{'\u00B0C'}</span>
            </div>
          </div>

          <div className={`${miniCardClass} h-full`}>
            <div className={climateLabelClass}>Abs Humidity</div>
            <div className="mt-1.5 flex items-end gap-1.5">
              <span className="text-3xl md:text-4xl font-semibold leading-none" style={{ color: statusColorOf(ahStatus) }}>{formatMetricValue(ah, 1)}</span>
              <span className={unitClass}>{'g/m\u00B3'}</span>
            </div>
          </div>
        </div>

        <div className={`${miniCardClass} py-2.5 md:py-3`}>
          <div className="flex flex-col sm:flex-row sm:items-end sm:justify-between gap-2">
            <div>
              <div className="text-[10px] md:text-xs uppercase tracking-wide text-gray-400 font-semibold">Pressure</div>
              <div className="mt-1 flex items-end gap-1.5">
                <span className="text-2xl md:text-3xl font-semibold leading-none text-white">{formatMetricValue(pressure, 1)}</span>
                <span className={unitClass}>hPa</span>
              </div>
            </div>
            <div className="grid grid-cols-2 gap-2.5 sm:min-w-[200px]">
              <div className="rounded-md border px-3 py-2" style={pressureTrend3h.surfaceStyle}>
                <div className="text-[11px] md:text-xs text-gray-400 leading-none">3h</div>
                <div className="mt-1 text-base md:text-lg font-semibold leading-none" style={pressureTrend3h.textStyle}>
                  {formatSignedMetricValue(delta3h, 1)}
                </div>
              </div>
              <div className="rounded-md border px-3 py-2" style={pressureTrend24h.surfaceStyle}>
                <div className="text-[11px] md:text-xs text-gray-400 leading-none">24h</div>
                <div className="mt-1 text-base md:text-lg font-semibold leading-none" style={pressureTrend24h.textStyle}>
                  {formatSignedMetricValue(delta24h, 1)}
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

const GasMetricCard = ({ label, value, unit, max, status, decimals = 1, compact = false }) => {
  const safeValue = isFiniteNumber(value) ? value : null;
  const safeMax = isFiniteNumber(max) && max > 0 ? max : null;
  const progress = safeValue !== null && safeMax !== null ? Math.min((safeValue / safeMax) * 100, 100) : 0;
  const valueText = formatMetricValue(safeValue, decimals);
  const valueColor = statusColorOf(status);
  const cardPaddingClass = compact ? 'p-2.5 md:p-3' : 'p-3 md:p-4';
  const labelClass = compact
    ? 'text-[10px] md:text-[11px] uppercase tracking-wide text-gray-300 font-semibold'
    : 'text-[11px] md:text-xs uppercase tracking-wide text-gray-300 font-semibold';
  const unitTextClass = compact ? 'text-[10px] text-gray-500' : 'text-[10px] md:text-xs text-gray-500';
  const valueClass = compact ? 'text-2xl md:text-[30px] font-semibold leading-none' : 'text-3xl md:text-[34px] font-semibold leading-none';
  const progressBarClass = compact ? 'mt-2.5 h-1.5 bg-gray-700 rounded-full overflow-hidden' : 'mt-3 h-2 bg-gray-700 rounded-full overflow-hidden';
  return (
    <div className={`bg-gray-800 rounded-xl border border-gray-700/50 ${cardPaddingClass}`}>
      <div className="flex items-center justify-between">
        <div className={labelClass}>{label}</div>
        <span className={unitTextClass}>{unit}</span>
      </div>
      <div className="mt-1.5 flex items-end justify-between">
        <span className={valueClass} style={{ color: valueColor }}>{valueText}</span>
        <StatusPill status={status} compact={compact} />
      </div>
      <div className={progressBarClass}>
        <div className="h-full rounded-full" style={{ width: `${progress}%`, backgroundColor: valueColor }} />
      </div>
    </div>
  );
};

const trendToken = (delta, is24h = false) => {
  if (!isFiniteNumber(delta)) {
    return {
      status: null,
      label: 'No data',
      textStyle: statusTextStyle(null),
      surfaceStyle: statusSurfaceStyle(null),
    };
  }
  const absDelta = Math.abs(delta);
  const status = getStatus(absDelta, is24h ? thresholds.pressureDelta24h : thresholds.pressureDelta3h);

  let label = 'Stable';
  if (delta <= -2) label = 'Strong Fall';
  else if (delta < -0.3) label = 'Falling';
  else if (delta >= 2) label = 'Strong Rise';
  else if (delta > 0.3) label = 'Rising';

  return {
    status,
    label,
    textStyle: statusTextStyle(status),
    surfaceStyle: statusSurfaceStyle(status),
  };
};

// Chart component
const ChartSection = ({ title, data, lines, unit, color, latestValues = {} }) => {
  const fallbackPalette = ['#22c55e', '#38bdf8', '#a78bfa', '#f59e0b'];
  const lineColors = lines.map((line, index) => line.color || color || fallbackPalette[index % fallbackPalette.length]);
  const unitTrimmed = (unit || '').trim();

  const minMax = useMemo(() => {
    const values = [];
    lines.forEach((line) => {
      data.forEach((row) => {
        const raw = row?.[line.key];
        if (typeof raw === 'number' && Number.isFinite(raw)) values.push(raw);
      });
    });

    if (!values.length) return null;
    return {
      min: Math.min(...values),
      max: Math.max(...values),
    };
  }, [data, lines]);

  const latestItems = lines.map((line, index) => {
    const fromApi = latestValues[line.key];
    if (typeof fromApi === 'number' && Number.isFinite(fromApi)) {
      return { text: fromApi.toFixed(1), color: lineColors[index] };
    }

    for (let i = data.length - 1; i >= 0; i--) {
      const raw = data[i]?.[line.key];
      if (typeof raw === 'number' && Number.isFinite(raw)) {
        return { text: raw.toFixed(1), color: lineColors[index] };
      }
    }

    return { text: '-', color: '#9ca3af' };
  });

  return (
    <div className="bg-gray-800 rounded-xl p-3 border border-gray-700/50">
      <div className="flex justify-between items-center mb-3">
        <span className="text-gray-400 text-xs font-bold">{title}</span>
        <div className="flex items-center gap-2 flex-wrap justify-end">
          {latestItems.map((item, index) => (
            <span
              key={`${title}_latest_${index}`}
              className="text-xs md:text-sm font-semibold"
              style={{ color: item.color }}
            >
              {item.text}
            </span>
          ))}
        </div>
      </div>
      <div className="h-32 md:h-40 lg:h-44 w-full">
        <SvgTrendChart
          data={data}
          lines={lines}
          lineColors={lineColors}
          showGrid={true}
          unit={unit}
        />
      </div>
      <div className="mt-2 flex items-center justify-between text-[11px] md:text-xs text-gray-400">
        <span>min {formatMinMaxValue(minMax?.min, unitTrimmed)}</span>
        <span>max {formatMinMaxValue(minMax?.max, unitTrimmed)}</span>
      </div>
    </div>
  );
};

// Alert item
const AlertItem = ({ time, type, message, severity }) => {
  const severityColors = {
    warning: 'border-l-yellow-500 bg-yellow-500/10',
    danger: 'border-l-orange-500 bg-orange-500/10',
    critical: 'border-l-red-500 bg-red-500/10',
    info: 'border-l-blue-500 bg-blue-500/10',
  };
  
  return (
    <div className={`flex flex-col p-3 rounded-r-lg border-l-4 mb-2 ${severityColors[severity] || 'border-l-gray-500'}`}>
      <span className="text-white text-sm md:text-[15px] font-medium leading-snug">{message}</span>
      <div className="mt-1.5 flex items-center justify-between gap-2">
        <span className="text-xs text-gray-400 uppercase font-bold">{type}</span>
        <span className="text-cyan-200 text-[12px] md:text-[13px] font-semibold tracking-[0.02em] whitespace-nowrap">{time}</span>
      </div>
    </div>
  );
};

const HeaderPencilIcon = ({ className = "" }) => (
  <svg viewBox="0 0 24 24" className={className} aria-hidden="true">
    <path d="M4 20h4l9.7-9.7a1.7 1.7 0 0 0 0-2.4l-1.6-1.6a1.7 1.7 0 0 0-2.4 0L4 16v4z" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" />
    <path d="m12.8 7.2 4 4" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" />
  </svg>
);

const HeaderCheckIcon = ({ className = "" }) => (
  <svg viewBox="0 0 24 24" className={className} aria-hidden="true">
    <path d="m5 12.5 4.2 4.2L19 7.8" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" />
  </svg>
);

const HeaderCloseIcon = ({ className = "" }) => (
  <svg viewBox="0 0 24 24" className={className} aria-hidden="true">
    <path d="M6 6l12 12M18 6 6 18" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" />
  </svg>
);

const TabIcon = ({ id }) => {
  const base = "w-4 h-4 md:w-[18px] md:h-[18px]";
  const stroke = {
    fill: "none",
    stroke: "currentColor",
    strokeWidth: 2.2,
    strokeLinecap: "round",
    strokeLinejoin: "round",
  };

  if (id === "sensors") {
    return (
      <svg viewBox="0 0 24 24" className={base} aria-hidden="true">
        <circle {...stroke} cx="12" cy="12" r="4" />
        <circle {...stroke} cx="12" cy="12" r="1.6" />
        <path {...stroke} d="M12 4.5v2.5M12 17v2.5M4.5 12H7M17 12h2.5" />
      </svg>
    );
  }

  if (id === "charts") {
    return (
      <svg viewBox="0 0 24 24" className={base} aria-hidden="true">
        <path {...stroke} d="M4 18h16" />
        <path {...stroke} d="M6 14.5 10 10l3 2.5 5-6" />
        <path {...stroke} d="M18 6h-2.5v2.5" />
      </svg>
    );
  }

  if (id === "events") {
    return (
      <svg viewBox="0 0 24 24" className={base} aria-hidden="true">
        <path {...stroke} d="M18 15.5V11a6 6 0 0 0-12 0v4.5L4.5 17h15L18 15.5z" />
        <path {...stroke} d="M10 18a2 2 0 0 0 4 0" />
      </svg>
    );
  }

  if (id === "system") {
    return (
      <svg viewBox="0 0 24 24" className={base} aria-hidden="true">
        <rect {...stroke} x="4.5" y="5" width="15" height="10.5" rx="2.2" />
        <path {...stroke} d="M8 9h5M8 11.8h7" />
        <circle {...stroke} cx="16.2" cy="9.4" r="1.2" />
        <path {...stroke} d="M9 19h6M12 15.5V19" />
      </svg>
    );
  }

  return (
    <svg viewBox="0 0 24 24" className={base} aria-hidden="true">
      <path {...stroke} d="M4 7h16" />
      <circle {...stroke} cx="9" cy="7" r="1.6" />
      <path {...stroke} d="M4 12h16" />
      <circle {...stroke} cx="15" cy="12" r="1.6" />
      <path {...stroke} d="M4 17h16" />
      <circle {...stroke} cx="11" cy="17" r="1.6" />
    </svg>
  );
};

// Tab navigation
const TabNav = ({ tabs, activeTab, onChange }) => {
  return (
    <div className="flex flex-wrap bg-gray-800 p-1 rounded-xl mb-4 border border-gray-700/50 gap-1">
      {tabs.map(tab => (
        <button
          key={tab.id}
          onClick={() => onChange(tab.id)}
          className={`flex items-center justify-center gap-2 flex-1 min-w-[48%] md:flex-none md:min-w-[132px] py-2 px-3 rounded-lg text-xs md:text-sm font-bold transition-all ${
            activeTab === tab.id
              ? 'bg-cyan-600 text-white shadow-lg shadow-cyan-900/40'
              : 'text-gray-400 hover:text-gray-200'
          }`}
        >
          <span className="inline-flex items-center justify-center">
            <TabIcon id={tab.id} />
          </span>
          {tab.label}
        </button>
      ))}
    </div>
  );
};

// Settings Components
const SettingGroup = ({ title, children, className = '' }) => (
  <div className={`bg-gray-800 rounded-xl p-4 md:p-5 border border-gray-700/50 ${className}`.trim()}>
    <div className="text-gray-400 text-xs font-bold uppercase tracking-wider mb-3">{title}</div>
    <div className="space-y-3">{children}</div>
  </div>
);

const SettingToggle = ({ label, enabled, onClick, icon: Icon, disabled = false }) => (
  <div 
    className={`flex justify-between items-center select-none group ${disabled ? 'opacity-65 cursor-not-allowed' : 'cursor-pointer'}`} 
    onClick={disabled ? undefined : onClick}
  >
    <div className={`flex items-center gap-2 text-gray-300 text-sm transition-colors ${disabled ? '' : 'group-hover:text-white'}`}>
      {Icon && <Icon size={16} className="text-gray-500" />}
      {label}
    </div>
    <div className={`w-10 h-5 rounded-full relative transition-colors duration-300 ${enabled ? 'bg-cyan-600' : 'bg-gray-700'}`}>
      <div className={`absolute top-1 w-3 h-3 bg-white rounded-full transition-all duration-300 shadow-md ${enabled ? 'left-6' : 'left-1'}`} />
    </div>
  </div>
);

const SettingInfoRow = ({ label, value, valueClassName = "text-white text-sm", mono = false }) => (
  <div className="flex justify-between items-center gap-3 py-1">
    <span className="text-gray-300 text-sm">{label}</span>
    <span className={`${valueClassName} ${mono ? 'font-mono' : ''}`.trim()}>{value}</span>
  </div>
);

const SettingSegmentControl = ({ label, icon: Icon, options, value, onChange }) => (
  <div className="flex justify-between items-center">
    <div className="flex items-center gap-2 text-gray-300 text-sm">
      {Icon && <Icon size={16} className="text-gray-500" />}
      {label}
    </div>
    <div className="flex bg-gray-900/60 border border-gray-700 rounded-lg p-0.5">
      {options.map((option) => {
        const active = option.value === value;
        return (
          <button
            key={option.value}
            onClick={() => onChange(option.value)}
            className={`px-2.5 py-1 text-xs font-semibold rounded-md transition-colors ${
              active ? 'bg-cyan-600 text-white' : 'text-gray-400 hover:text-gray-200'
            }`}
          >
            {option.label}
          </button>
        );
      })}
    </div>
  </div>
);

const SettingStepper = ({ label, value, unit, stepHint, onDec, onInc, decimals = 1 }) => (
  <div className="flex justify-between items-center">
    <div>
      <div className="text-gray-300 text-sm">{label}</div>
      {stepHint && <div className="text-[11px] text-gray-500 mt-0.5">{stepHint}</div>}
    </div>
    <div className="flex items-center gap-2">
      <button
        onClick={onDec}
        className="w-8 h-8 flex items-center justify-center bg-gray-800 border border-gray-700 rounded-lg text-gray-400 hover:bg-gray-700 hover:text-white transition-colors"
      >
        -
      </button>
      <div className="min-w-[92px] text-center">
        <span className="text-sm md:text-base font-mono text-white">
          {value > 0 ? '+' : ''}{Number(value).toFixed(decimals)}
        </span>
        <span className="ml-1 text-xs md:text-sm text-gray-400">{unit}</span>
      </div>
      <button
        onClick={onInc}
        className="w-8 h-8 flex items-center justify-center bg-gray-800 border border-gray-700 rounded-lg text-gray-400 hover:bg-gray-700 hover:text-white transition-colors"
      >
        +
      </button>
    </div>
  </div>
);

// ============ MAIN DASHBOARD ============
function AuraDashboard() {
  const [activeTab, setActiveTab] = useState('sensors');
  const [chartRange, setChartRange] = useState('24h');
  const [chartGroup, setChartGroup] = useState('core');
  
  // Settings State
  const [settings, setSettings] = useState(DEFAULT_WEB_SETTINGS);
  const [savedSettings, setSavedSettings] = useState(DEFAULT_WEB_SETTINGS);
  const [settingsSaving, setSettingsSaving] = useState(false);
  const [settingsSaveStatus, setSettingsSaveStatus] = useState('idle'); // idle | saved | error
  const [toggleErrorMessage, setToggleErrorMessage] = useState('');
  const toggleErrorTimerRef = React.useRef(null);
  const otaFileInputRef = React.useRef(null);
  const [otaFile, setOtaFile] = useState(null);
  const [otaUploadState, setOtaUploadState] = useState('idle'); // idle | uploading | success | error
  const [otaUploadProgress, setOtaUploadProgress] = useState(0);
  const [otaUploadMessage, setOtaUploadMessage] = useState('');
  const otaUploadInProgress = otaUploadState === 'uploading';
  const otaRestartPending = otaUploadState === 'success';
  const otaBusy = otaUploadInProgress || otaRestartPending;
  const OTA_UPLOAD_MIN_TIMEOUT_MS = 180000;
  const OTA_UPLOAD_MAX_TIMEOUT_MS = 900000;
  const OTA_UPLOAD_MIN_BYTES_PER_SEC = 20 * 1024;
  const computeOtaTimeoutMs = (sizeBytes) => {
    if (!Number.isFinite(sizeBytes) || sizeBytes <= 0) {
      return OTA_UPLOAD_MIN_TIMEOUT_MS;
    }
    const transferMs = Math.ceil((sizeBytes * 1000) / OTA_UPLOAD_MIN_BYTES_PER_SEC);
    const timeoutMs = transferMs + 120000;
    return Math.min(OTA_UPLOAD_MAX_TIMEOUT_MS, Math.max(OTA_UPLOAD_MIN_TIMEOUT_MS, timeoutMs));
  };
  const toggleRequestRef = React.useRef({
    night_mode: { inFlight: false, queued: null },
    backlight_on: { inFlight: false, queued: null },
  });

  // Device Name Editing
  const [deviceName, setDeviceName] = useState(PREVIEW_HOSTNAME);
  const [isEditingName, setIsEditingName] = useState(false);
  const [tempDeviceName, setTempDeviceName] = useState(deviceName);

  const handleNameSave = () => {
    const nextName = tempDeviceName.trim();
    postSettingsPatch({ display_name: nextName })
      .then((payload) => {
        const parsed = parseSettingsPayload(payload?.settings || {});
        applyParsedSettings(parsed, true);
        const fallbackHost = stateApi?.connectivity?.hostname || PREVIEW_HOSTNAME;
        const resolvedName =
          (typeof parsed?.displayName === 'string' && parsed.displayName.trim().length > 0)
            ? parsed.displayName.trim()
            : fallbackHost;
        setDeviceName(resolvedName);
        setTempDeviceName(resolvedName);
        setIsEditingName(false);
      })
      .catch(() => {
        setTempDeviceName(deviceName);
        setIsEditingName(false);
      });
  };

  const handleNameCancel = () => {
    setTempDeviceName(deviceName);
    setIsEditingName(false);
  };

  const hasSettingsChanges =
    settings.tempUnit !== savedSettings.tempUnit ||
    settings.tempOffset !== savedSettings.tempOffset ||
    settings.humOffset !== savedSettings.humOffset;

  const shouldApplyToggleFromApi = (toggleKey, overrideKey = null) => {
    if (overrideKey === toggleKey) return true;
    const queue = toggleRequestRef.current[toggleKey];
    return !(queue && (queue.inFlight || queue.queued !== null));
  };

  const applyParsedSettings = (parsed, force = false, toggleOverrideKey = null) => {
    if (!parsed) return;
    if (!force && (hasSettingsChanges || settingsSaving)) return;
    const patch = { ...settingsPatchFromParsed(parsed) };
    if (!shouldApplyToggleFromApi('night_mode', toggleOverrideKey)) {
      delete patch.nightMode;
    }
    if (!shouldApplyToggleFromApi('backlight_on', toggleOverrideKey)) {
      delete patch.backlight;
    }
    if (Object.keys(patch).length === 0) return;
    setSavedSettings((prev) => ({ ...prev, ...patch }));
    setSettings((prev) => ({ ...prev, ...patch }));
  };

  const postSettingsPatch = (patch) =>
    fetch('/api/settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(patch),
    })
      .then((response) => {
        if (!response.ok) {
          throw new Error(`HTTP ${response.status}`);
        }
        return response.json();
      });

  const showToggleError = (message) => {
    setToggleErrorMessage(message || '');
    if (toggleErrorTimerRef.current) {
      clearTimeout(toggleErrorTimerRef.current);
      toggleErrorTimerRef.current = null;
    }
    if (message) {
      toggleErrorTimerRef.current = setTimeout(() => {
        setToggleErrorMessage('');
        toggleErrorTimerRef.current = null;
      }, 3500);
    }
  };

  const processToggleQueue = (key) => {
    const queue = toggleRequestRef.current[key];
    if (!queue || queue.inFlight || queue.queued === null) return;

    const target = queue.queued;
    queue.queued = null;
    queue.inFlight = true;

    postSettingsPatch({ [key]: target })
      .then((payload) => {
        // Ignore superseded responses when a newer toggle value is already queued.
        if (queue.queued === null) {
          const parsed = parseSettingsPayload(payload?.settings || {});
          applyParsedSettings(parsed, true, key);
        }
        showToggleError('');
      })
      .catch(() => {
        // Re-sync from current backend state on transport/server errors.
        fetch('/api/state', { cache: 'no-store' })
          .then((response) => {
            if (!response.ok) throw new Error(`HTTP ${response.status}`);
            return response.json();
          })
          .then((payload) => {
            const parsed = parseSettingsPayload(payload?.settings || {});
            applyParsedSettings(parsed, true, key);
          })
          .catch(() => {});
        showToggleError('Toggle update failed. Restored device state.');
      })
      .finally(() => {
        queue.inFlight = false;
        if (queue.queued !== null) {
          processToggleQueue(key);
        }
      });
  };

  const enqueueTogglePatch = (key, value) => {
    const queue = toggleRequestRef.current[key];
    if (!queue) return;
    queue.queued = value;
    processToggleQueue(key);
  };

  const toggleNightMode = () => {
    if (settings.nightModeLocked) return;
    const next = !settings.nightMode;
    setSettings((prev) => ({ ...prev, nightMode: next }));
    setSavedSettings((prev) => ({ ...prev, nightMode: next }));
    enqueueTogglePatch('night_mode', next);
  };

  const toggleBacklight = () => {
    const next = !settings.backlight;
    setSettings((prev) => ({ ...prev, backlight: next }));
    setSavedSettings((prev) => ({ ...prev, backlight: next }));
    enqueueTogglePatch('backlight_on', next);
  };

  const setTemperatureUnit = (nextUnit) => {
    if (nextUnit !== 'c' && nextUnit !== 'f') return;
    setSettingsSaveStatus('idle');
    setSettings((prev) => ({ ...prev, tempUnit: nextUnit }));
  };

  const updateOffset = (key, delta, decimals = 1) => {
    setSettingsSaveStatus('idle');
    setSettings((prev) => {
      let nextValue = Number((prev[key] + delta).toFixed(decimals));
      if (key === 'tempOffset') {
        const displayValue = tempOffsetCToDisplay(prev.tempOffset, prev.tempUnit);
        let nextDisplayValue = Number((displayValue + delta).toFixed(decimals));
        const maxDisplayOffset = prev.tempUnit === 'f' ? 9 : 5;
        nextDisplayValue = Math.max(-maxDisplayOffset, Math.min(maxDisplayOffset, nextDisplayValue));
        nextValue = tempOffsetDisplayToC(nextDisplayValue, prev.tempUnit);
        nextValue = Number(nextValue.toFixed(1));
      } else if (key === 'humOffset') {
        nextValue = Math.max(-10, Math.min(10, nextValue));
      }
      return {
        ...prev,
        [key]: nextValue,
      };
    });
  };

  const saveSettings = () => {
    if (!hasSettingsChanges || settingsSaving) return;
    setSettingsSaving(true);
    setSettingsSaveStatus('idle');
    postSettingsPatch({
      units_c: settings.tempUnit === 'c',
      temp_offset: settings.tempOffset,
      hum_offset: settings.humOffset,
    })
      .then((payload) => {
        const parsed = parseSettingsPayload(payload?.settings || {});
        applyParsedSettings(parsed, true);
        setSettingsSaveStatus('saved');
      })
      .catch(() => {
        setSettingsSaveStatus('error');
      })
      .finally(() => {
        setSettingsSaving(false);
      });
  };

  const requestRestart = () => {
    postSettingsPatch({ restart: true }).catch(() => {});
  };

  const selectFirmwareFile = (event) => {
    const file = event?.target?.files?.[0] || null;
    setOtaFile(file);
    setOtaUploadState('idle');
    setOtaUploadProgress(0);
    if (file) {
      setOtaUploadMessage(`${file.name} (${formatFileSize(file.size)})`);
    } else {
      setOtaUploadMessage('');
    }
  };

  const uploadFirmware = () => {
    if (!otaFile || otaUploadState === 'uploading') return;
    if (otaFile.size <= 0) {
      setOtaUploadState('error');
      setOtaUploadMessage('Selected file is empty.');
      return;
    }

    const xhr = new XMLHttpRequest();
    const formData = new FormData();
    formData.append('ota_size', String(otaFile.size));
    formData.append('firmware', otaFile, otaFile.name);

    setOtaUploadState('uploading');
    setOtaUploadProgress(0);
    setOtaUploadMessage('Uploading firmware...');

    xhr.open('POST', '/api/ota', true);
    xhr.timeout = computeOtaTimeoutMs(otaFile.size);
    xhr.upload.onprogress = (event) => {
      if (!event.lengthComputable || event.total <= 0) return;
      const progress = Math.min(100, Math.round((event.loaded / event.total) * 100));
      setOtaUploadProgress(progress);
    };

    xhr.onreadystatechange = () => {
      if (xhr.readyState !== XMLHttpRequest.DONE) return;

      let payload = null;
      try {
        payload = JSON.parse(xhr.responseText || '{}');
      } catch (e) {
        payload = null;
      }

      if (xhr.status >= 200 && xhr.status < 300 && payload?.success === true) {
        setOtaUploadState('success');
        setOtaUploadProgress(100);
        setOtaUploadMessage(payload?.message || 'Firmware uploaded. Device will reboot.');
        setOtaFile(null);
        if (otaFileInputRef.current) {
          otaFileInputRef.current.value = '';
        }
        return;
      }

      const errorText =
        (payload && typeof payload.error === 'string' && payload.error.trim()) ||
        `Upload failed (HTTP ${xhr.status || 0})`;
      setOtaUploadState('error');
      setOtaUploadMessage(errorText);
    };

    xhr.onerror = () => {
      setOtaUploadState('error');
      setOtaUploadMessage('Upload failed. Check network connection and retry.');
    };
    xhr.ontimeout = () => {
      setOtaUploadState('error');
      setOtaUploadMessage('Upload timed out. Retry closer to device/AP.');
    };

    xhr.send(formData);
  };

  useEffect(() => {
    if (settingsSaveStatus === 'idle') return;
    const timeoutId = setTimeout(() => setSettingsSaveStatus('idle'), 2500);
    return () => clearTimeout(timeoutId);
  }, [settingsSaveStatus]);

  useEffect(() => {
    return () => {
      if (toggleErrorTimerRef.current) {
        clearTimeout(toggleErrorTimerRef.current);
      }
    };
  }, []);

  const [stateApi, setStateApi] = useState(null);

  const [chartApiData, setChartApiData] = useState(null);
  const [chartApiLatest, setChartApiLatest] = useState({});
  const [sensorHistoryData, setSensorHistoryData] = useState(null);
  const [clockTickMs, setClockTickMs] = useState(Date.now());
  const [deviceClockRef, setDeviceClockRef] = useState(null);
  const [eventsApiAlerts, setEventsApiAlerts] = useState(null);
  const [eventsApiLive, setEventsApiLive] = useState(false);

  useEffect(() => {
    const intervalId = setInterval(() => setClockTickMs(Date.now()), 1000);
    return () => clearInterval(intervalId);
  }, []);

  useEffect(() => {
    if (activeTab !== 'charts' || otaBusy) return;

    const controller = new AbortController();
    const apiGroup = chartGroup === 'core' ? 'core' : chartGroup;

    fetch(`/api/charts?group=${encodeURIComponent(apiGroup)}&window=${encodeURIComponent(chartRange)}`, {
      cache: 'no-store',
      signal: controller.signal,
    })
      .then((response) => {
        if (!response.ok) {
          throw new Error(`HTTP ${response.status}`);
        }
        return response.json();
      })
      .then((payload) => {
        const parsed = parseChartApiPayload(payload);
        setChartApiData(parsed.points);
        setChartApiLatest(parsed.latest);
      })
      .catch((error) => {
        if (error?.name === 'AbortError') return;
        setChartApiData(null);
        setChartApiLatest({});
      });

    return () => controller.abort();
  }, [activeTab, chartRange, chartGroup, otaBusy]);

  useEffect(() => {
    if (activeTab !== 'sensors' || otaBusy) return;

    let active = true;
    const controller = new AbortController();
    const loadSensorHistory = () => {
      fetch('/api/charts?group=core&window=24h', { cache: 'no-store', signal: controller.signal })
        .then((response) => {
          if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
          }
          return response.json();
        })
        .then((payload) => {
          if (!active) return;
          const parsed = parseChartApiPayload(payload);
          setSensorHistoryData(parsed.points);
        })
        .catch((error) => {
          if (error?.name === 'AbortError') return;
          // Keep last successful history to avoid flicker.
        });
    };

    loadSensorHistory();
    const intervalId = setInterval(loadSensorHistory, 30000);
    return () => {
      active = false;
      clearInterval(intervalId);
      controller.abort();
    };
  }, [activeTab, otaBusy]);

  useEffect(() => {
    if (otaBusy) return;

    let active = true;
    const controller = new AbortController();

    const loadState = () => {
      fetch('/api/state', { cache: 'no-store', signal: controller.signal })
        .then((response) => {
          if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
          }
          return response.json();
        })
        .then((payload) => {
          if (!active) return;
          const parsed = parseStateApiPayload(payload);
          setStateApi(parsed);
          if (typeof parsed.deviceTimeEpochS === 'number' && Number.isFinite(parsed.deviceTimeEpochS)) {
            setDeviceClockRef({
              epochMs: parsed.deviceTimeEpochS * 1000,
              capturedAtMs: Date.now(),
            });
          }
        })
        .catch((error) => {
          if (error?.name === 'AbortError') return;
        });
    };

    loadState();
    const intervalId = setInterval(loadState, 10000);
    return () => {
      active = false;
      clearInterval(intervalId);
      controller.abort();
    };
  }, [otaBusy]);

  useEffect(() => {
    if (activeTab !== 'events' || otaBusy) return;

    let active = true;
    const controller = new AbortController();
    const loadEvents = () => {
      fetch('/api/events', { cache: 'no-store', signal: controller.signal })
        .then((response) => {
          if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
          }
          return response.json();
        })
        .then((payload) => {
          if (!active) return;
          const parsed = parseEventsApiPayload(payload);
          setEventsApiAlerts(parsed);
          setEventsApiLive(true);
        })
        .catch((error) => {
          if (error?.name === 'AbortError') return;
          if (!active) return;
          setEventsApiLive(false);
        });
    };

    loadEvents();
    const intervalId = setInterval(loadEvents, 10000);
    return () => {
      active = false;
      clearInterval(intervalId);
      controller.abort();
    };
  }, [activeTab, otaBusy]);

  useEffect(() => {
    if (!otaRestartPending) return;

    let disposed = false;
    let retryTimerId = null;
    let activeController = null;

    const scheduleRetry = (delayMs) => {
      if (disposed) return;
      retryTimerId = setTimeout(probeDeviceOnline, delayMs);
    };

    const probeDeviceOnline = () => {
      if (disposed) return;

      activeController = new AbortController();
      const timeoutId = setTimeout(() => {
        if (activeController) {
          activeController.abort();
        }
      }, 1500);

      fetch('/api/state?probe=1', { cache: 'no-store', signal: activeController.signal })
        .then((response) => {
          if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
          }
          return response.json();
        })
        .then((payload) => {
          if (disposed) return;
          if (payload && payload.success === false) {
            throw new Error('device not ready');
          }
          window.location.reload();
        })
        .catch(() => {
          if (disposed) return;
          scheduleRetry(2000);
        })
        .finally(() => {
          clearTimeout(timeoutId);
          activeController = null;
        });
    };

    scheduleRetry(1200);

    return () => {
      disposed = true;
      if (retryTimerId) {
        clearTimeout(retryTimerId);
      }
      if (activeController) {
        activeController.abort();
      }
    };
  }, [otaRestartPending]);

  const chartData = Array.isArray(chartApiData) ? chartApiData : [];
  const sensorHistory = Array.isArray(sensorHistoryData) ? sensorHistoryData : [];

  const stateCurrent = stateApi?.current || {};
  const stateDerived = stateApi?.derived || {};
  const stateConnectivity = stateApi?.connectivity || {};
  const stateSystem = stateApi?.system || {};
  const stateSettings = stateApi?.settings || {};
  const current = {
    co2: stateCurrent.co2 ?? null,
    temp: stateCurrent.temp ?? null,
    rh: stateCurrent.rh ?? null,
    pressure: stateCurrent.pressure ?? null,
    pm05: stateCurrent.pm05 ?? null,
    pm1: stateCurrent.pm1 ?? null,
    pm25: stateCurrent.pm25 ?? null,
    pm4: stateCurrent.pm4 ?? null,
    pm10: stateCurrent.pm10 ?? null,
    voc: stateCurrent.voc ?? null,
    nox: stateCurrent.nox ?? null,
    hcho: stateCurrent.hcho ?? null,
    co: stateCurrent.co ?? null,
    mold: stateDerived.mold ?? null,
  };
  // Top-right values in Charts should match live sensor cards when available.
  const chartLatestValues = useMemo(() => {
    const merged = { ...(chartApiLatest || {}) };
    Object.entries(current).forEach(([key, value]) => {
      if (!isFiniteNumber(value)) {
        return;
      }
      merged[key] = value;
      if (key === 'temp') merged.temperature = value;
      if (key === 'rh') merged.humidity = value;
    });
    return merged;
  }, [chartApiLatest, current]);
  
  const ah = stateDerived.ah ?? null;
  const dewPoint = stateDerived.dewPoint ?? null;
  const delta3h = stateDerived.delta3h ?? null;
  const delta24h = stateDerived.delta24h ?? null;
  const co2Status = metricStatus(current.co2, thresholds.co2);
  const tempStatus = metricStatus(current.temp, thresholds.temp);
  const rhStatus = metricStatus(current.rh, thresholds.rh);
  const pm05Status = metricStatus(current.pm05, thresholds.pm05, { round: true });
  const pm1Status = metricStatus(current.pm1, thresholds.pm1);
  const pm25Status = metricStatus(current.pm25, thresholds.pm25);
  const pm4Status = metricStatus(current.pm4, thresholds.pm4);
  const pm10Status = metricStatus(current.pm10, thresholds.pm10);
  const vocStatus = metricStatus(current.voc, thresholds.voc);
  const noxStatus = metricStatus(current.nox, thresholds.nox);
  const hchoStatus = metricStatus(current.hcho, thresholds.hcho);
  const coStatus = metricStatus(current.co, thresholds.co);
  const moldStatus = metricStatus(current.mold, thresholds.mold);
  const pressureTrend3h = trendToken(delta3h, false);
  const pressureTrend24h = trendToken(delta24h, true);
  const uptime = stateSystem.uptime || stateDerived.uptime || '\u2014';

  const connectivity = {
    wifiSsid: stateConnectivity.wifiSsid || 'MyHome_5G',
    hostname: stateConnectivity.hostname || PREVIEW_HOSTNAME,
    ip: stateConnectivity.ip || '192.168.1.105',
    rssi: typeof stateConnectivity.rssi === 'number' ? stateConnectivity.rssi : -65,
    mqttBroker: stateConnectivity.mqttBroker || '192.168.1.200',
    mqttConnected: typeof stateConnectivity.mqttConnected === 'boolean' ? stateConnectivity.mqttConnected : true,
  };
  const firmwareVersion = stateSystem.firmware || 'v2.1.0-beta';
  const firmwareBuild = [stateSystem.buildDate, stateSystem.buildTime].filter(Boolean).join(' ') || '20240315';
  const dacAvailable = stateSystem.dacAvailable === true;
  const localWebUrl = `http://${connectivity.hostname}.local`;
  const signalClass =
    connectivity.rssi > -67
      ? "text-emerald-400 text-sm font-semibold"
      : connectivity.rssi > -75
        ? "text-yellow-400 text-sm font-semibold"
        : "text-red-400 text-sm font-semibold";
  const headerNow = deviceClockRef
    ? new Date(deviceClockRef.epochMs + (clockTickMs - deviceClockRef.capturedAtMs))
    : new Date(clockTickMs);
  const headerTime = formatHeaderTime(headerNow);
  const headerDate = formatHeaderDate(headerNow);
  const tempOffsetStep = settings.tempUnit === 'f' ? 0.2 : 0.1;
  const tempOffsetDisplayValue = Number(tempOffsetCToDisplay(settings.tempOffset, settings.tempUnit).toFixed(1));
  const tempOffsetUnitLabel = settings.tempUnit === 'f' ? '\u00B0F' : '\u00B0C';
  const tempOffsetStepHint = settings.tempUnit === 'f' ? 'Step: 0.2 \u00B0F' : 'Step: 0.1 \u00B0C';

  useEffect(() => {
    applyParsedSettings(stateSettings);
  }, [
    stateSettings.nightMode,
    stateSettings.nightModeLocked,
    stateSettings.backlight,
    stateSettings.tempUnit,
    stateSettings.tempOffset,
    stateSettings.humOffset,
  ]);

  useEffect(() => {
    if (isEditingName) return;
    const savedDisplayName =
      typeof stateSettings.displayName === 'string' && stateSettings.displayName.trim().length > 0
        ? stateSettings.displayName.trim()
        : null;
    const resolved = savedDisplayName || stateConnectivity.hostname || PREVIEW_HOSTNAME;
    setDeviceName(resolved);
    setTempDeviceName(resolved);
  }, [isEditingName, stateSettings.displayName, stateConnectivity.hostname]);
  
  const fallbackAlerts = [
    { time: '14:32', type: 'CO2', message: 'Threshold exceeded (>1000 ppm)', severity: 'warning' },
    { time: '12:45', type: 'VOC', message: 'Elevated index detected', severity: 'info' },
    { time: '08:15', type: 'CO2', message: 'Critical level (>1400 ppm)', severity: 'danger' },
  ];
  const alerts = eventsApiLive && Array.isArray(eventsApiAlerts) ? eventsApiAlerts : fallbackAlerts;

  const tabs = [
    { id: 'sensors', label: 'Sensors' },
    { id: 'charts', label: 'Charts' },
    { id: 'events', label: 'Events' },
    { id: 'settings', label: 'Settings' },
    { id: 'system', label: 'System' },
  ];

  return (
    <div className="min-h-screen bg-gray-900 text-white px-4 py-4 md:px-6 lg:px-8 max-w-md md:max-w-3xl lg:max-w-6xl mx-auto font-sans">
      {/* Header */}
      <div className="flex items-center justify-between mb-5 md:mb-6 px-1">
        <div>
          <h1 className="text-xl font-bold text-white flex items-center gap-2">
            <span className="relative inline-flex w-3.5 h-3.5 items-center justify-center">
              <span className="absolute inset-0 rounded-full border border-emerald-400/70 animate-pulse shadow-[0_0_10px_#22c55e]"></span>
              <span className="w-2 h-2 rounded-full bg-emerald-400 shadow-[0_0_8px_#4ade80]"></span>
            </span>
            AURA
          </h1>
          {isEditingName ? (
            <div className="flex items-center gap-2 pl-5 mt-1">
              <input 
                type="text" 
                value={tempDeviceName}
                onChange={(e) => setTempDeviceName(e.target.value)}
                className="bg-gray-800 text-white text-[12px] font-mono border border-gray-600 rounded px-2 py-1 outline-none w-40"
                maxLength={32}
                autoFocus
                onKeyDown={(e) => {
                  if (e.key === 'Enter') handleNameSave();
                  if (e.key === 'Escape') handleNameCancel();
                }}
              />
              <button onClick={handleNameSave} className="w-6 h-6 rounded-md bg-emerald-500/15 border border-emerald-500/35 text-emerald-300 hover:text-emerald-200 hover:border-emerald-400/60 transition-colors inline-flex items-center justify-center">
                <HeaderCheckIcon className="w-3.5 h-3.5" />
              </button>
              <button onClick={handleNameCancel} className="w-6 h-6 rounded-md bg-red-500/15 border border-red-500/35 text-red-300 hover:text-red-200 hover:border-red-400/60 transition-colors inline-flex items-center justify-center">
                <HeaderCloseIcon className="w-3.5 h-3.5" />
              </button>
            </div>
          ) : (
            <div 
              className="flex items-center gap-2 pl-5 mt-1 group cursor-pointer"
              onClick={() => {
                setTempDeviceName(deviceName);
                setIsEditingName(true);
              }}
            >
              <p className="text-gray-400 text-[13px] md:text-sm font-semibold tracking-[0.06em] group-hover:text-gray-200 transition-colors">{deviceName}</p>
              <HeaderPencilIcon className="w-3.5 h-3.5 text-gray-500 group-hover:text-emerald-300 transition-colors opacity-70 group-hover:opacity-100" />
            </div>
          )}
        </div>
        <div className="text-right">
          <div className="text-white text-xl md:text-2xl font-bold leading-none tracking-wide">
            {headerTime}
          </div>
          <div className="mt-1 text-gray-400 text-[11px] md:text-xs font-semibold uppercase tracking-[0.12em]">
            {headerDate}
          </div>
        </div>
      </div>

      <TabNav tabs={tabs} activeTab={activeTab} onChange={setActiveTab} />

      {activeTab === 'sensors' && (
        <div className="space-y-4 md:space-y-5 animate-in fade-in duration-300">
          <div className="grid grid-cols-1 xl:grid-cols-12 gap-4 md:gap-5">
            <div className="xl:col-span-7">
              <HeroMetric value={current.co2} status={co2Status} history={sensorHistory} />
            </div>
            <div className="xl:col-span-5">
              <ClimateOverview
                temp={current.temp}
                tempStatus={tempStatus}
                rh={current.rh}
                rhStatus={rhStatus}
                dewPoint={dewPoint}
                ah={ah}
                mold={current.mold}
                moldStatus={moldStatus}
                pressure={current.pressure}
                delta3h={delta3h}
                delta24h={delta24h}
                pressureTrend3h={pressureTrend3h}
                pressureTrend24h={pressureTrend24h}
              />
            </div>
          </div>

          <div className="grid grid-cols-1 sm:grid-cols-2 xl:grid-cols-4 gap-3 md:gap-4">
            <GasMetricCard label="CO" value={current.co} unit="ppm" max={25} status={coStatus} />
            <GasMetricCard label="VOC" value={current.voc} unit="idx" max={400} status={vocStatus} />
            <GasMetricCard label="NOx" value={current.nox} unit="idx" max={300} status={noxStatus} />
            <GasMetricCard label="HCHO" value={current.hcho} unit="ppb" max={100} status={hchoStatus} />
          </div>

          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-5 gap-2.5 md:gap-3">
            <GasMetricCard label="PM0.5" value={current.pm05} unit={"#/cm\u00B3"} max={thresholds.pm05.bad} status={pm05Status} decimals={0} compact />
            <GasMetricCard label="PM1.0" value={current.pm1} unit={"\u00B5g/m\u00B3"} max={thresholds.pm1.bad} status={pm1Status} compact />
            <GasMetricCard label="PM2.5" value={current.pm25} unit={"\u00B5g/m\u00B3"} max={thresholds.pm25.bad} status={pm25Status} compact />
            <GasMetricCard label="PM4.0" value={current.pm4} unit={"\u00B5g/m\u00B3"} max={thresholds.pm4.bad} status={pm4Status} compact />
            <GasMetricCard label="PM10" value={current.pm10} unit={"\u00B5g/m\u00B3"} max={thresholds.pm10.bad} status={pm10Status} compact />
          </div>

        </div>
      )}

      {activeTab === 'charts' && (
        <div className="space-y-4 md:space-y-5 animate-in fade-in duration-300">
          {/* Chart Controls */}
          <div className="grid grid-cols-1 xl:grid-cols-[320px_minmax(0,1fr)] gap-3 md:gap-4 items-start">
            <div className="flex bg-gray-800 p-1 rounded-lg border border-gray-700/50 md:w-[320px]">
              {['1h', '3h', '24h'].map(r => (
                <button
                  key={r}
                  onClick={() => setChartRange(r)}
                  className={`flex-1 py-1.5 text-xs md:text-sm font-bold rounded-md transition-all ${
                    chartRange === r
                      ? 'bg-gray-700 text-white shadow-sm'
                      : 'text-gray-500 hover:text-gray-300'
                  }`}
                >
                  {r}
                </button>
              ))}
            </div>

            <div className="bg-gray-800 p-1 rounded-lg border border-gray-700/50">
              <div className="grid grid-cols-3 gap-1">
                {[
                  { key: 'core', label: 'Core' },
                  { key: 'gases', label: 'Gases' },
                  { key: 'pm', label: 'PM' },
                ].map(group => (
                  <button
                    key={group.key}
                    onClick={() => setChartGroup(group.key)}
                    className={`py-1.5 px-2 text-xs md:text-sm font-bold rounded-md transition-all ${
                      chartGroup === group.key
                        ? 'bg-gray-700 text-white shadow-sm'
                        : 'text-gray-500 hover:text-gray-300'
                    }`}
                  >
                    {group.label}
                  </button>
                ))}
              </div>
            </div>
          </div>

          {chartGroup === 'core' && (
            <div className="grid grid-cols-1 xl:grid-cols-2 gap-3 md:gap-4">
              <ChartSection title="CO2 Concentration" data={chartData} lines={[{ key: 'co2', name: 'CO2' }]} unit="ppm" color="#10b981" latestValues={chartLatestValues} />
              <ChartSection title="Temperature" data={chartData} lines={[{ key: 'temp', name: 'Temp' }]} unit={"\u00B0C"} color="#f59e0b" latestValues={chartLatestValues} />
              <ChartSection title="Humidity" data={chartData} lines={[{ key: 'rh', name: 'RH' }]} unit="%" color="#3b82f6" latestValues={chartLatestValues} />
              <ChartSection title="Pressure" data={chartData} lines={[{ key: 'pressure', name: 'hPa' }]} unit="hPa" color="#0ea5e9" latestValues={chartLatestValues} />
            </div>
          )}

          {chartGroup === 'gases' && (
            <div className="grid grid-cols-1 xl:grid-cols-2 gap-3 md:gap-4">
              <ChartSection title="Carbon Monoxide (CO)" data={chartData} lines={[{ key: 'co', name: 'CO' }]} unit="ppm" color="#f97316" latestValues={chartLatestValues} />
              <ChartSection title="VOC Index" data={chartData} lines={[{ key: 'voc', name: 'VOC' }]} unit="" color="#ef4444" latestValues={chartLatestValues} />
              <ChartSection title="NOx Index" data={chartData} lines={[{ key: 'nox', name: 'NOx' }]} unit="" color="#f43f5e" latestValues={chartLatestValues} />
              <ChartSection title="Formaldehyde (HCHO)" data={chartData} lines={[{ key: 'hcho', name: 'HCHO' }]} unit="ppb" color="#d946ef" latestValues={chartLatestValues} />
            </div>
          )}

          {chartGroup === 'pm' && (
            <div className="grid grid-cols-1 xl:grid-cols-2 gap-3 md:gap-4">
              <ChartSection title="PM0.5" data={chartData} lines={[{ key: 'pm05', name: 'PM0.5' }]} unit={"#/cm\u00B3"} color="#14b8a6" latestValues={chartLatestValues} />
              <ChartSection title="PM1.0" data={chartData} lines={[{ key: 'pm1', name: 'PM1.0' }]} unit={"\u00B5g/m\u00B3"} color="#a78bfa" latestValues={chartLatestValues} />
              <ChartSection title="PM2.5" data={chartData} lines={[{ key: 'pm25', name: 'PM2.5' }]} unit={"\u00B5g/m\u00B3"} color="#8b5cf6" latestValues={chartLatestValues} />
              <ChartSection
                title="PM10 + PM4.0"
                data={chartData}
                lines={[
                  { key: 'pm10', name: 'PM10', color: '#6d28d9' },
                  { key: 'pm4', name: 'PM4.0', color: '#0ea5e9' },
                ]}
                unit={"\u00B5g/m\u00B3"}
                latestValues={chartLatestValues}
              />
            </div>
          )}
        </div>
      )}

      {activeTab === 'events' && (
        <div className="space-y-3 animate-in fade-in duration-300">
          <div className="bg-gray-800 rounded-xl p-4 md:p-5 border border-gray-700/50">
             <div className="text-gray-400 text-xs font-bold uppercase tracking-wider mb-3">System Log</div>
             {alerts.length > 0 ? (
               alerts.map((alert, i) => (
                 <AlertItem key={i} {...alert} />
               ))
             ) : (
               <div className="text-sm text-gray-400 py-3">No events yet.</div>
             )}
          </div>
        </div>
      )}

      {activeTab === 'settings' && (
        <div className="space-y-3 md:space-y-4 animate-in fade-in duration-300">
          <div className="grid grid-cols-1 lg:grid-cols-2 gap-3 md:gap-4 items-start">
            <div className="space-y-3 md:space-y-4">
              <SettingGroup title="Display">
                <SettingToggle 
                  label="Night Mode" 
                  icon={Moon}
                  enabled={settings.nightMode} 
                  onClick={toggleNightMode}
                  disabled={settings.nightModeLocked}
                />
                <SettingToggle 
                  label="Backlight" 
                  icon={Sun}
                  enabled={settings.backlight} 
                  onClick={toggleBacklight}
                />
                {toggleErrorMessage && (
                  <div className="text-[11px] text-red-300/90">{toggleErrorMessage}</div>
                )}
              </SettingGroup>

              <SettingGroup title="Connectivity">
                <SettingInfoRow label="WiFi SSID" value={connectivity.wifiSsid} valueClassName="text-white text-sm" mono />
                <SettingInfoRow label="Hostname" value={connectivity.hostname} valueClassName="text-white text-sm" mono />
                <SettingInfoRow label="IP Address" value={connectivity.ip} valueClassName="text-white text-sm" mono />
                <SettingInfoRow label="Signal" value={`${connectivity.rssi} dBm`} valueClassName={signalClass} />
                <SettingInfoRow label="MQTT Broker" value={connectivity.mqttBroker} valueClassName="text-gray-300 text-sm" mono />
                <SettingInfoRow
                  label="MQTT Status"
                  value={connectivity.mqttConnected ? 'Connected' : 'Disconnected'}
                  valueClassName={connectivity.mqttConnected ? "text-emerald-400 text-sm font-semibold" : "text-red-400 text-sm font-semibold"}
                />
              </SettingGroup>
            </div>

            <div className="space-y-3 md:space-y-4">
              <SettingGroup title="Measurements">
                <SettingSegmentControl
                  label="Temperature Unit"
                  options={[
                    { label: '\u00B0C', value: 'c' },
                    { label: '\u00B0F', value: 'f' },
                  ]}
                  value={settings.tempUnit}
                  onChange={setTemperatureUnit}
                />
                <SettingStepper
                  label="Temperature Offset"
                  value={tempOffsetDisplayValue}
                  unit={tempOffsetUnitLabel}
                  stepHint={tempOffsetStepHint}
                  decimals={1}
                  onDec={() => updateOffset('tempOffset', -tempOffsetStep, 1)}
                  onInc={() => updateOffset('tempOffset', tempOffsetStep, 1)}
                />
                <SettingStepper
                  label="Humidity Offset"
                  value={settings.humOffset}
                  unit="%"
                  stepHint="Step: 1 %"
                  decimals={0}
                  onDec={() => updateOffset('humOffset', -1, 0)}
                  onInc={() => updateOffset('humOffset', 1, 0)}
                />
                <div className="pt-2 border-t border-gray-700/60">
                  {(() => {
                    const isDisabled = !hasSettingsChanges || settingsSaving;
                    const showSaved = settingsSaveStatus === 'saved' && !hasSettingsChanges && !settingsSaving;
                    const showError = settingsSaveStatus === 'error';
                    const label = settingsSaving
                      ? 'Saving...'
                      : showSaved
                        ? 'Saved'
                        : showError
                          ? 'Save Failed'
                          : hasSettingsChanges
                            ? 'Save Settings'
                            : 'No Changes';
                    const className = isDisabled
                      ? (showSaved
                          ? 'bg-emerald-500/15 text-emerald-300 border-emerald-500/40'
                          : 'bg-gray-800/70 text-gray-500 border-gray-700 cursor-not-allowed')
                      : (showError
                          ? 'bg-red-500/10 hover:bg-red-500/20 text-red-300 border-red-500/40'
                          : 'bg-cyan-500/10 hover:bg-cyan-500/20 text-cyan-300 border-cyan-500/40');
                    return (
                      <>
                        <button
                          onClick={saveSettings}
                          disabled={isDisabled}
                          className={`w-full border py-2.5 rounded-lg text-sm font-semibold flex items-center justify-center gap-2 transition-colors ${className}`}
                        >
                          {label}
                        </button>
                        {showSaved && (
                          <div className="mt-2 text-[11px] text-emerald-300/90">Settings saved to device.</div>
                        )}
                        {showError && (
                          <div className="mt-2 text-[11px] text-red-300/90">Could not save settings. Try again.</div>
                        )}
                      </>
                    );
                  })()}
                </div>
              </SettingGroup>

            </div>
          </div>
        </div>
      )}

      {activeTab === 'system' && (
        <div className="space-y-3 md:space-y-4 animate-in fade-in duration-300">
          <div className="grid grid-cols-1 lg:grid-cols-2 gap-3 md:gap-4 items-start">
            <SettingGroup title="System">
              <div className="space-y-1">
                <SettingInfoRow label="Firmware" value={firmwareVersion} valueClassName="text-gray-200 text-sm" mono />
                <SettingInfoRow label="Build" value={firmwareBuild} valueClassName="text-gray-300 text-sm" mono />
                <SettingInfoRow label="Uptime" value={uptime} valueClassName="text-gray-200 text-sm" mono />
                <SettingInfoRow label="Web URL" value={localWebUrl} valueClassName="text-cyan-300 text-sm" mono />
              </div>
              <div className="mt-3 pt-3 border-t border-gray-700/60 space-y-2">
                <button
                  onClick={requestRestart}
                  disabled={otaBusy}
                  className="w-full border py-2.5 rounded-lg text-sm font-semibold flex items-center justify-center gap-2 bg-red-500/10 hover:bg-red-500/20 text-red-400 border-red-500/40 transition-colors"
                >
                  <RotateCw size={14} /> Reboot Device
                </button>
                {dacAvailable && (
                  <button
                    onClick={() => { window.location.href = '/dac'; }}
                    className="w-full border py-2.5 rounded-lg text-sm font-semibold flex items-center justify-center gap-2 bg-cyan-500/10 hover:bg-cyan-500/20 text-cyan-300 border-cyan-500/40 transition-colors"
                  >
                    DAC Settings
                  </button>
                )}
              </div>
            </SettingGroup>

            <SettingGroup title="Firmware Update">
                <input
                  ref={otaFileInputRef}
                  type="file"
                  accept=".bin"
                  onChange={selectFirmwareFile}
                  disabled={otaBusy}
                  className="block w-full text-xs text-gray-300 file:mr-3 file:rounded-md file:border file:border-gray-600 file:bg-gray-800 file:px-3 file:py-1.5 file:text-xs file:font-semibold file:text-gray-200 hover:file:bg-gray-700 disabled:opacity-60 disabled:cursor-not-allowed"
                />
                <button
                  onClick={uploadFirmware}
                  disabled={!otaFile || otaBusy}
                  className={`w-full mt-2 border py-2.5 rounded-lg text-sm font-semibold flex items-center justify-center gap-2 transition-colors ${
                  !otaFile || otaBusy
                    ? 'bg-gray-800/70 text-gray-500 border-gray-700 cursor-not-allowed'
                    : 'bg-amber-500/10 hover:bg-amber-500/20 text-amber-300 border-amber-500/40'
                }`}
              >
                <UploadIcon size={14} />
                {otaUploadState === 'uploading' ? 'Uploading...' : (otaRestartPending ? 'Rebooting...' : 'Update Firmware')}
              </button>
              {otaUploadState === 'uploading' && (
                <div className="mt-2">
                  <div className="h-1.5 rounded-full bg-gray-800 border border-gray-700 overflow-hidden">
                    <div
                      className="h-full bg-amber-400 transition-all duration-150"
                      style={{ width: `${otaUploadProgress}%` }}
                    />
                  </div>
                  <div className="mt-1 text-[11px] text-amber-300/90 text-right">{otaUploadProgress}%</div>
                </div>
              )}
              {otaUploadMessage && (
                <div
                  className={`mt-2 text-[11px] ${
                    otaUploadState === 'success'
                      ? 'text-emerald-300/90'
                      : otaUploadState === 'error'
                        ? 'text-red-300/90'
                        : 'text-gray-400'
                  }`}
                >
                  {otaUploadMessage}
                </div>
              )}
            </SettingGroup>
          </div>
        </div>
      )}
    </div>
  );
}
const root = ReactDOM.createRoot(document.getElementById("root"));
root.render(<AuraDashboard />);
/*__INLINE_APP_END__*/
  </script>
</body>
</html>
)HTML_DASH";

} // namespace WebTemplates

