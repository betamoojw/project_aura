// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "web/WebTemplates.h"

namespace WebTemplates {

const char kDashboardPageTemplateAp[] PROGMEM = R"HTML_DASH_AP(
<!doctype html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Aura Dashboard (AP Offline)</title>
  <link rel="icon" type="image/svg+xml" href="data:image/svg+xml;base64,PHN2ZyB4bWxucz0naHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmcnIHZpZXdCb3g9JzAgMCAxNjkzLjMyIDE2OTMuMzInPjxwYXRoIGZpbGw9JyNFNkI4NzMnIGZpbGwtcnVsZT0nZXZlbm9kZCcgY2xpcC1ydWxlPSdldmVub2RkJyBkPSdNODQ2LjY2IDE2NDIuNTNjNDM5LjU1LDAgNzk1Ljg3LC0zNTYuMzIgNzk1Ljg3LC03OTUuODcgMCwtNDM5LjU1IC0zNTYuMzIsLTc5NS44NyAtNzk1Ljg3LC03OTUuODcgLTQzOS41NSwwIC03OTUuODcsMzU2LjMyIC03OTUuODcsNzk1Ljg3IDAsNDM5LjU1IDM1Ni4zMiw3OTUuODcgNzk1Ljg3LDc5NS44N3ptLTQxOS4zMiAtMTM3MC4yOGMzMjcuMTcsLTYwLjE4IDY3NS4xNCwxNTAuNDQgNjkwLjYzLDUxMi44IDU2LjE2LC0xODAuOSAtMTEuMTksLTM3My44MiAtMTQzLjQxLC01MDMuNTIgLTY2LjY4LC02NS40IC0xNDkuNjgsLTExNC4zNiAtMjQxLjE0LC0xMzcuMDcgLTExMy4xNywxOC4xMSAtMjE3LjQsNjIuOTQgLTMwNi4wOCwxMjcuNzl6bTU4Ny44NyAzNTMuMjZjLTcyLjgsLTE3NC42NCAtMjQ5LjE4LC0yNzkuNTUgLTQzMy42OCwtMjkzLjkxIC05My4xNiwtNy4yNSAtMTg4LjA3LDguNDQgLTI3Mi40OCw0OS41NCAtNzIuNSw4My42NSAtMTI1LjgsMTg0LjM3IC0xNTMuMDUsMjk1LjM2IDIxMi4wOCwtMjU2LjcxIDYxNC4yOSwtMzE5LjMzIDg1OS4yMSwtNTAuOTl6bS0xMjAuNiAtNDg4LjQ1YzEyOC42OCw3My4xOCAyMzAuMDQsMTk0IDI3OSwzMzMuMjQgNjAuODgsMTczLjE0IDM1LjAyLDM1NC44MiAtNzkuNjgsNTAzLjcxIDE1OS4xNywtMTAyLjIzIDIzMS44NiwtMjkzLjk1IDIxMy45OCwtNDc4LjEgLTkuMDIsLTkyLjkgLTQxLjEsLTE4My43IC05Ni41MSwtMjU5Ljg3IC05My43NSwtNTYuMTQgLTIwMS41MSwtOTEuMyAtMzE2Ljc5LC05OC45OHptNDQ0LjkzIDE5Ni45N2MxMTUuODMsMzEyLjA3IC0zMC44NCw2OTEuMzEgLTM4NS41NSw3NjkuMTUgMTg3LjU3LDI0LjE0IDM2Ni43MSwtNzYuMDkgNDcxLjM0LC0yMjguNjEgNTIuOTQsLTc3LjE3IDg2LjgsLTE2Ny40NSA5My4yOCwtMjYxLjM1IC0zNy4xNiwtMTA2Ljk4IC05OS4xNiwtMjAyLjMyIC0xNzkuMDcsLTI3OS4xOXptMjE0LjI3IDQzNi41N2MtMTExLjU0LDMxMy41MiAtNDY3LjgzLDUwOS43MiAtNzg5LjQ0LDM0MS44MSAxMjguMjQsMTM4Ljc3IDMyOS44MiwxNzcuMSA1MDcuNzcsMTI3LjU5IDkwLjAzLC0yNS4wNSAxNzQuMDEsLTcyLjUzIDIzOS40NCwtMTQwLjUxIDI5LjksLTc4LjU1IDQ2LjI4LC0xNjMuNzggNDYuMjgsLTI1Mi44MyAwLC0yNS43IC0xLjM5LC01MS4wNyAtNC4wNSwtNzYuMDZ6bS0xMTYuNTcgNDcyLjM4Yy0yODcuMTIsMTY4LjIzIC02ODYuMjIsODkuOTYgLTgyNC41NCwtMjQ1Ljk4IDguNzEsMTg5LjA4IDEzOC40NSwzNDcuNzUgMzA2LjkzLDQyNC4zNiA4NS4xMywzOC43MSAxNzkuODQsNTYuMzggMjczLjQzLDQ2LjUzIDk4LjAzLC01NC43OCAxODEuNzUsLTEzMi4wOSAyNDQuMTgsLTIyNC45MXptLTM5Mi42NSAyODYuOTRjLTMyOC4xLC01NS4yOCAtNTgzLjU4LC0zNzIuMzUgLTQ3My44NywtNzE4LjM5IC0xMTQuNzgsMTUwLjY2IC0xMTcuNCwzNTUuNDIgLTM3LjU5LDUyMi40IDQwLjMxLDg0LjMzIDEwMS41NCwxNTguNzUgMTc5LjY4LDIxMS4zNCA0My4zNSw4LjI2IDg4LjEsMTIuNTkgMTMzLjg1LDEyLjU5IDY4LjY5LDAgMTM1LjEsLTkuNzcgMTk3LjkzLC0yNy45NHptLTQ4NS4zOCAtMzIuNTdjLTIxNS44MSwtMjUzLjM0IC0yMDgsLTY2MC4yOSA5OC42NCwtODU0Ljg4IC0xODQuNjksNDEuNjUgLTMxOC40MywxOTYuNzIgLTM2NC41OCwzNzUuODUgLTIzLjI5LDkwLjQgLTI0LjIsMTg2LjczIDEuODQsMjc3LjMyIDcwLjQ1LDg2LjQ0IDE2MC44MSwxNTYuMDEgMjY0LjEsMjAxLjcxem0tMzUxLjA0IC0zMzcuMTZjLTIuMTgsLTMzMy4wNiAyNjUuMzYsLTYzOS43MSA2MjUuNDgsLTU5MS4yNiAtMTY4LjE0LC04Ny4wMyAtMzcwLjYyLC01NC4wNCAtNTIxLjM0LDUzLjY2IC01OS43Miw0Mi42OCAtMTc2Ljg1LDE2NS4xNiAtMTc2Ljg1LDIyNC4wNyAwLDExMi41MyAyNi4xOCwyMTguOTQgNzIuNzEsMzEzLjUzem02MzguNDkgLTEyMy4yM2MxMDUuMSwwIDE5MC4zLC04NS4yIDE5MC4zLC0xOTAuMyAwLC0xMDUuMSAtODUuMiwtMTkwLjMgLTE5MC4zLC0xOTAuMyAtMTA1LjEsMCAtMTkwLjMsODUuMiAtMTkwLjMsMTkwLjMgMCwxMDUuMSA4NS4yLDE5MC4zIDE5MC4zLDE5MC4zeicvPjwvc3ZnPg==" />
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
    html, body {
      color: #f9fafb;
      font-family: "Segoe UI", Roboto, Arial, sans-serif;
      font-size: 14px;
      line-height: 1.5;
      background-color: #020712;
      background-image:
        radial-gradient(1200px 700px at 18% -12%, rgba(8, 145, 178, 0.22), transparent 62%),
        radial-gradient(900px 520px at 84% -6%, rgba(59, 130, 246, 0.14), transparent 58%),
        linear-gradient(180deg, #050d1f 0%, #030918 48%, #020712 100%);
      background-attachment: fixed;
    }
    body { padding: 12px; }
    .wrap { max-width: 1100px; margin: 0 auto; }

    /* ── Header ── */
    .hdr { display: flex; align-items: center; justify-content: space-between; gap: 12px; margin-bottom: 16px; flex-wrap: wrap; }
    .hdr-left { display: flex; flex-direction: column; gap: 2px; }
    .brand { display: flex; align-items: center; gap: 8px; }
    .brand-dot { width: 10px; height: 10px; border-radius: 999px; background: #34d399; box-shadow: 0 0 8px #34d399; flex: 0 0 auto; }
    .brand-name { font-size: 22px; font-weight: 700; letter-spacing: .02em; color: #f9fafb; }
    .brand-sub { font-size: 12px; color: #9ca3af; }
    .hdr-right { display: flex; align-items: center; gap: 12px; flex-wrap: wrap; justify-content: flex-end; }
    .clock { text-align: right; }
    .clock-time { font-size: 22px; font-weight: 700; line-height: 1; letter-spacing: .03em; }
    .clock-date { margin-top: 3px; color: #9ca3af; font-size: 11px; font-weight: 600; letter-spacing: .08em; text-transform: uppercase; }
    .hdr-actions { display: flex; gap: 8px; }
    .btn { cursor: pointer; border-radius: 10px; padding: 8px 14px; font-size: 13px; font-weight: 600; border: 1px solid #374151; background: #1f2937; color: #d1d5db; transition: border-color .15s; }
    .btn:hover { border-color: #4b5563; color: #f9fafb; }
    .btn:disabled { opacity: .5; cursor: not-allowed; }
    .btn-cyan { background: #0e7490; border-color: #0891b2; color: #fff; }
    .btn-cyan:hover { background: #0891b2; }
    .btn-danger { background: #1c0a0a; border-color: #7f1d1d; color: #fca5a5; }
    .btn-danger:hover { border-color: #dc2626; }
    .btn-amber { background: #1c1100; border-color: #78350f; color: #fcd34d; }
    .btn-amber:hover { border-color: #d97706; }
    .link-btn { text-decoration: none; display: inline-flex; align-items: center; }

    /* ── Tab nav ── */
    .tab-nav { display: flex; flex-wrap: wrap; background: #1f2937; padding: 4px; border-radius: 12px; margin-bottom: 16px; border: 1px solid rgba(55,65,81,.5); gap: 4px; }
    .tab-btn { display: flex; align-items: center; justify-content: center; gap: 7px; flex: 1; min-width: 48%; padding: 8px 12px; border-radius: 8px; border: none; background: transparent; color: #9ca3af; font-size: 13px; font-weight: 700; cursor: pointer; transition: background .15s, color .15s; }
    @media (min-width:640px) { .tab-btn { min-width: 0; flex: none; } }
    .tab-btn:hover { color: #e5e7eb; }
    .tab-btn.active { background: #0891b2; color: #fff; box-shadow: 0 4px 16px rgba(8,145,178,.3); }
    .tab-btn svg { width: 16px; height: 16px; flex-shrink: 0; }
    .tab-panel { display: none; }
    .tab-panel.active { display: block; }

    /* ── Cards & grids ── */
    .card-g8 { background: linear-gradient(135deg, #1f2937, #111827); border: 1px solid rgba(55,65,81,.6); border-radius: 16px; }
    .card-g7 { background: #1f2937; border: 1px solid rgba(55,65,81,.5); border-radius: 12px; }
    .mini-card { background: rgba(55,65,81,.3); border: 1px solid rgba(75,85,99,.4); border-radius: 12px; padding: 12px 14px; }

    /* ── HeroMetric ── */
    .hero-wrap { padding: 20px 22px 16px; }
    .hero-label { font-size: 11px; text-transform: uppercase; letter-spacing: .08em; color: #9ca3af; font-weight: 600; }
    .hero-val-row { display: flex; align-items: flex-end; gap: 8px; margin-top: 10px; }
    .hero-val { font-size: 64px; font-weight: 600; line-height: 1; }
    .hero-unit { font-size: 16px; color: #9ca3af; padding-bottom: 6px; }
    .hero-pill { margin-top: 2px; }
    .hero-progress-track { margin-top: 14px; height: 10px; background: rgba(55,65,81,.8); border-radius: 999px; overflow: hidden; }
    .hero-progress-fill { height: 100%; border-radius: 999px; transition: width .7s; }
    .hero-advice { margin-top: 10px; font-size: 14px; color: #d1d5db; }
    .hero-trend-sep { margin-top: 18px; padding-top: 14px; border-top: 1px solid rgba(55,65,81,.6); }
    .hero-trend-hdr { display: flex; align-items: center; justify-content: space-between; }
    .hero-trend-label { font-size: 10px; text-transform: uppercase; letter-spacing: .08em; color: #9ca3af; font-weight: 600; }
    .hero-trend-delta { font-size: 13px; font-weight: 600; }
    .hero-minmax { display: flex; justify-content: space-between; margin-top: 6px; font-size: 11px; color: #6b7280; }

    /* ── Status pill ── */
    .pill { display: inline-flex; align-items: center; padding: 3px 9px; border-radius: 999px; border: 1px solid; font-size: 11px; font-weight: 600; white-space: nowrap; }
    .pill-sm { font-size: 10px; padding: 2px 7px; }

    /* ── ClimateOverview ── */
    .climate-wrap { padding: 20px 22px 14px; display: flex; flex-direction: column; height: 100%; }
    .climate-head { display: flex; align-items: center; justify-content: space-between; gap: 8px; }
    .climate-title { font-size: 12px; text-transform: uppercase; letter-spacing: .06em; color: #d1d5db; font-weight: 600; }
    .climate-grid2 { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-top: 14px; }
    .climate-grid3 { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 10px; margin-top: 10px; }
    .mini-label { font-size: 10px; text-transform: uppercase; letter-spacing: .08em; color: #9ca3af; font-weight: 600; }
    .mini-val-row { display: flex; align-items: flex-end; gap: 6px; margin-top: 7px; }
    .mini-val { font-size: 28px; font-weight: 600; line-height: 1; }
    .mini-unit { font-size: 14px; color: #d1d5db; padding-bottom: 2px; }
    .pressure-mini { margin-top: 10px; }
    .pressure-row { display: flex; flex-direction: column; gap: 6px; }
    @media (min-width: 560px) { .pressure-row { flex-direction: row; align-items: flex-end; justify-content: space-between; } }
    .pressure-delta-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; min-width: 180px; }
    .pdelta-box { border-radius: 6px; border: 1px solid; padding: 8px 12px; }
    .pdelta-lbl { font-size: 11px; color: #9ca3af; line-height: 1; }
    .pdelta-val { font-size: 17px; font-weight: 600; line-height: 1; margin-top: 4px; }

    /* ── GasMetricCard grid ── */
    .gas-grid { display: grid; grid-template-columns: 1fr; gap: 10px; margin-top: 14px; }
    @media (min-width: 640px) { .gas-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); } }
    @media (min-width: 1280px) { .gas-grid { grid-template-columns: repeat(4, minmax(0, 1fr)); } }
    .gas-card { padding: 12px 14px; }
    .gas-head { display: flex; align-items: center; justify-content: space-between; }
    .gas-name { font-size: 11px; text-transform: uppercase; letter-spacing: .06em; color: #d1d5db; font-weight: 600; }
    .gas-unit { font-size: 10px; color: #6b7280; }
    .gas-val-row { display: flex; align-items: flex-end; justify-content: space-between; margin-top: 6px; }
    .gas-val { font-size: 28px; font-weight: 600; line-height: 1; }
    .gas-progress { margin-top: 10px; height: 6px; background: #374151; border-radius: 999px; overflow: hidden; }
    .gas-bar { height: 100%; border-radius: 999px; }

    /* ── PM compact grid ── */
    .pm-grid { display: grid; grid-template-columns: 1fr; gap: 8px; margin-top: 12px; }
    @media (min-width: 640px) { .pm-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); } }
    @media (min-width: 1024px) { .pm-grid { grid-template-columns: repeat(3, minmax(0, 1fr)); } }
    @media (min-width: 1280px) { .pm-grid { grid-template-columns: repeat(5, minmax(0, 1fr)); } }
    .pm-card { padding: 10px 12px; }
    .pm-head { display: flex; align-items: center; justify-content: space-between; gap: 8px; }
    .pm-label { font-size: 10px; text-transform: uppercase; letter-spacing: .06em; color: #9ca3af; font-weight: 600; }
    .pm-val { font-size: 22px; font-weight: 600; line-height: 1; }
    .pm-unit { font-size: 10px; color: #6b7280; padding-bottom: 0; }
    .pm-val-row { display: flex; align-items: flex-end; justify-content: space-between; gap: 5px; margin-top: 5px; }
    .pm-val-left { display: flex; align-items: flex-end; gap: 5px; }
    .pm-progress { margin-top: 6px; height: 5px; background: #374151; border-radius: 999px; overflow: hidden; }
    .pm-bar { height: 100%; border-radius: 999px; }

    /* ── Section title (sensors tab) ── */
    .sec-title { font-size: 11px; text-transform: uppercase; letter-spacing: .08em; color: #6b7280; font-weight: 700; margin-bottom: 12px; }

    /* ── Sensors layout ── */
    .sensor-grid { display: grid; grid-template-columns: 1fr; gap: 14px; margin-top: 0; }
    .sensor-hero, .sensor-climate { min-width: 0; }
    @media (min-width: 1280px) {
      .sensor-grid { grid-template-columns: repeat(12, minmax(0, 1fr)); }
      .sensor-hero { grid-column: span 7 / span 7; }
      .sensor-climate { grid-column: span 5 / span 5; }
    }

    /* ── Chart section ── */
    .chart-controls { display: flex; gap: 14px; flex-wrap: wrap; align-items: center; margin-bottom: 14px; }
    .chart-ctrl-label { font-size: 11px; text-transform: uppercase; letter-spacing: .07em; color: #6b7280; font-weight: 600; margin-bottom: 6px; }
    .seg-ctrl { display: flex; background: rgba(17,24,39,.6); border: 1px solid #374151; border-radius: 8px; padding: 2px; gap: 2px; }
    .seg-btn { border: none; background: transparent; color: #9ca3af; padding: 5px 11px; font-size: 12px; font-weight: 600; border-radius: 6px; cursor: pointer; transition: background .12s, color .12s; }
    .seg-btn.active { background: #0891b2; color: #fff; }
    .chart-grid { display: grid; grid-template-columns: 1fr; gap: 12px; }
    @media (min-width: 1280px) { .chart-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); } }
    .chart-box { padding: 12px; }
    .chart-head { display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }
    .chart-name { font-size: 12px; font-weight: 700; color: #9ca3af; }
    .chart-latest-row { display: flex; align-items: center; gap: 8px; flex-wrap: wrap; justify-content: flex-end; }
    .chart-latest-item { font-size: 13px; font-weight: 600; }
    .chart-svg-wrap { position: relative; height: 140px; width: 100%; }
    .chart-svg-wrap svg { width: 100%; height: 100%; display: block; overflow: visible; }
    .chart-point { fill: transparent; stroke: transparent; pointer-events: none; }
    .chart-tooltip {
      position: absolute;
      z-index: 5;
      pointer-events: none;
      background: rgba(3, 7, 18, 0.96);
      border: 1px solid #4b5563;
      border-radius: 8px;
      color: #f9fafb;
      font-size: 11px;
      line-height: 1.3;
      padding: 6px 8px;
      white-space: pre-line;
      transform: translate(-50%, -100%);
      display: none;
    }
    .chart-tooltip.show { display: block; }
    .chart-minmax { display: flex; justify-content: space-between; margin-top: 6px; font-size: 11px; color: #6b7280; }
    .no-data { color: #6b7280; font-size: 13px; padding: 24px 0; text-align: center; }

    /* ── Events ── */
    .events-list { display: flex; flex-direction: column; gap: 0; }
    .alert-item { display: flex; flex-direction: column; padding: 10px 14px; border-left: 4px solid; border-radius: 0 8px 8px 0; margin-bottom: 8px; }
    .alert-item.sev-warning { border-color: #eab308; background: rgba(234,179,8,.1); }
    .alert-item.sev-danger { border-color: #f97316; background: rgba(249,115,22,.1); }
    .alert-item.sev-critical { border-color: #ef4444; background: rgba(239,68,68,.1); }
    .alert-item.sev-info { border-color: #3b82f6; background: rgba(59,130,246,.1); }
    .alert-msg { font-size: 14px; color: #f9fafb; font-weight: 500; line-height: 1.4; }
    .alert-meta { display: flex; align-items: center; justify-content: space-between; gap: 8px; margin-top: 6px; }
    .alert-type { font-size: 11px; color: #9ca3af; font-weight: 700; text-transform: uppercase; }
    .alert-time { font-size: 12px; color: #a5f3fc; font-weight: 600; white-space: nowrap; }

    /* ── Settings ── */
    .sg { background: #1f2937; border: 1px solid rgba(55,65,81,.5); border-radius: 12px; padding: 16px 18px; }
    .sg + .sg { margin-top: 12px; }
    .sg-title { font-size: 11px; text-transform: uppercase; letter-spacing: .08em; color: #6b7280; font-weight: 700; margin-bottom: 12px; }
    .sg-rows { display: flex; flex-direction: column; gap: 12px; }
    .toggle-row { display: flex; align-items: center; justify-content: space-between; cursor: pointer; user-select: none; }
    .toggle-row.disabled { opacity: .6; cursor: not-allowed; }
    .toggle-label { font-size: 14px; color: #d1d5db; }
    .toggle-sw { width: 40px; height: 20px; border-radius: 999px; background: #374151; position: relative; transition: background .2s; flex-shrink: 0; }
    .toggle-sw.on { background: #0891b2; }
    .toggle-knob { position: absolute; top: 3px; left: 3px; width: 14px; height: 14px; border-radius: 50%; background: #fff; transition: left .2s; box-shadow: 0 1px 3px rgba(0,0,0,.4); }
    .toggle-sw.on .toggle-knob { left: 23px; }
    .toggle-msg { min-height: 14px; font-size: 11px; color: #6b7280; }
    .toggle-msg.err { color: #fca5a5; }
    .seg-row { display: flex; align-items: center; justify-content: space-between; }
    .seg-label { font-size: 14px; color: #d1d5db; }
    .stepper-row { display: flex; align-items: center; justify-content: space-between; }
    .stepper-label { font-size: 14px; color: #d1d5db; }
    .stepper-ctrl { display: flex; align-items: center; gap: 8px; }
    .stepper-btn { width: 30px; height: 30px; border-radius: 8px; border: 1px solid #374151; background: #111827; color: #d1d5db; font-size: 18px; cursor: pointer; display: flex; align-items: center; justify-content: center; line-height: 1; }
    .stepper-btn:hover { border-color: #4b5563; }
    .stepper-val { min-width: 52px; text-align: center; font-size: 14px; font-weight: 600; color: #f9fafb; }
    .text-field-row { display: flex; flex-direction: column; gap: 6px; }
    .text-field-lbl { font-size: 12px; color: #9ca3af; }
    .text-input { width: 100%; background: #111827; border: 1px solid #374151; border-radius: 8px; padding: 8px 12px; color: #f9fafb; font-size: 14px; }
    .text-input:focus { outline: none; border-color: #0891b2; }
    .save-btn { margin-top: 14px; padding: 9px 18px; border-radius: 10px; border: none; font-size: 13px; font-weight: 700; cursor: pointer; transition: background .15s; }
    .save-btn.idle { background: #1f2937; color: #9ca3af; border: 1px solid #374151; cursor: default; }
    .save-btn.dirty { background: #0891b2; color: #fff; }
    .save-btn.dirty:hover { background: #0e7490; }
    .save-btn.saving { background: #374151; color: #9ca3af; cursor: not-allowed; }
    .save-btn.saved { background: #14532d; color: #86efac; }
    .save-btn.error { background: #7f1d1d; color: #fca5a5; }
    .settings-grid { display: grid; grid-template-columns: 1fr; gap: 12px; align-items: start; }
    .settings-col { display: flex; flex-direction: column; gap: 12px; }
    .settings-col .sg + .sg { margin-top: 0; }
    @media (min-width: 1024px) { .settings-grid { grid-template-columns: 1fr 1fr; } }

    /* ── System tab ── */
    .system-grid { display: grid; grid-template-columns: 1fr; gap: 12px; align-items: start; }
    @media (min-width: 1024px) { .system-grid { grid-template-columns: 1fr 1fr; } }
    .info-rows { display: flex; flex-direction: column; gap: 0; }
    .info-row { display: flex; align-items: center; justify-content: space-between; gap: 12px; padding: 8px 0; border-bottom: 1px solid rgba(55,65,81,.4); font-size: 13px; }
    .info-row:last-child { border-bottom: none; }
    .info-key { color: #9ca3af; flex-shrink: 0; }
    .info-val { color: #f9fafb; text-align: right; font-weight: 500; }
    .info-val.ok { color: #4ade80; }
    .info-val.err { color: #f87171; }
    .sys-btn-row { display: flex; gap: 10px; flex-wrap: wrap; margin-top: 14px; }

    /* ── OTA progress ── */
    .progress-track { height: 8px; background: #1f2937; border-radius: 999px; overflow: hidden; border: 1px solid #374151; margin-top: 10px; }
    .progress-fill { height: 100%; background: linear-gradient(90deg, #0891b2, #67e8f9); border-radius: 999px; width: 0%; transition: width .25s; }
    .ota-status { font-size: 12px; color: #9ca3af; margin-top: 6px; }
    .ota-status.ok { color: #4ade80; }
    .ota-status.err { color: #f87171; }

    /* ── Misc ── */
    .muted { color: #6b7280; }
    @media (max-width: 560px) { .brand-name { font-size: 19px; } .clock-time { font-size: 19px; } .hero-val { font-size: 52px; } .mini-val { font-size: 24px; } }
  </style>
</head>
<body>
<div class="wrap">

  <!-- Header -->
  <div class="hdr">
    <div class="hdr-left">
      <div class="brand">
        <span class="brand-dot"></span>
        <span class="brand-name">AURA</span>
      </div>
      <div id="deviceNameLabel" class="brand-sub">Device</div>
    </div>
    <div class="hdr-right">
      <div class="clock">
        <div id="headerTime" class="clock-time">--:--</div>
        <div id="headerDate" class="clock-date">-- --- ----</div>
      </div>
    </div>
  </div>

  <!-- Tab nav -->
  <div class="tab-nav">
    <button class="tab-btn active" type="button" data-tab="sensors">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="4"/><circle cx="12" cy="12" r="1.6"/><path d="M12 4.5v2.5M12 17v2.5M4.5 12H7M17 12h2.5"/></svg>
      Sensors
    </button>
    <button class="tab-btn" type="button" data-tab="charts">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 18h16"/><path d="M6 14.5 10 10l3 2.5 5-6"/><path d="M18 6h-2.5v2.5"/></svg>
      Charts
    </button>
    <button class="tab-btn" type="button" data-tab="events">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M18 15.5V11a6 6 0 0 0-12 0v4.5L4.5 17h15L18 15.5z"/><path d="M10 18a2 2 0 0 0 4 0"/></svg>
      Events
    </button>
    <button class="tab-btn" type="button" data-tab="settings">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 7h16"/><circle cx="9" cy="7" r="1.6"/><path d="M4 12h16"/><circle cx="15" cy="12" r="1.6"/><path d="M4 17h16"/><circle cx="11" cy="17" r="1.6"/></svg>
      Settings
    </button>
    <button class="tab-btn" type="button" data-tab="system">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><rect x="4.5" y="5" width="15" height="10.5" rx="2.2"/><path d="M8 9h5M8 11.8h7"/><circle cx="16.2" cy="9.4" r="1.2"/><path d="M9 19h6M12 15.5V19"/></svg>
      System
    </button>
  </div>

  <!-- === SENSORS === -->
  <div id="tab-sensors" class="tab-panel active">
    <div class="sensor-grid">
      <div id="heroMetric" class="card-g8 sensor-hero"></div>
      <div id="climateOverview" class="card-g8 sensor-climate"></div>
    </div>
    <div style="margin-top:14px"><div id="gasGrid"></div></div>
    <div style="margin-top:14px"><div id="pmGrid"></div></div>
  </div>

  <!-- === CHARTS === -->
  <div id="tab-charts" class="tab-panel">
    <div class="chart-controls">
      <div>
        <div class="chart-ctrl-label">Group</div>
        <div class="seg-ctrl" id="cgroupSeg">
          <button class="seg-btn active" type="button" data-group="core">Core</button>
          <button class="seg-btn" type="button" data-group="gases">Gases</button>
          <button class="seg-btn" type="button" data-group="pm">PM</button>
        </div>
      </div>
      <div>
        <div class="chart-ctrl-label">Range</div>
        <div class="seg-ctrl" id="crangeSeg">
          <button class="seg-btn" type="button" data-range="1h">1h</button>
          <button class="seg-btn" type="button" data-range="3h">3h</button>
          <button class="seg-btn active" type="button" data-range="24h">24h</button>
        </div>
      </div>
    </div>
    <div id="chartGrid" class="chart-grid"></div>
  </div>

  <!-- === EVENTS === -->
  <div id="tab-events" class="tab-panel">
    <div id="eventsList" class="events-list"><div class="no-data">Loading...</div></div>
  </div>

  <!-- === SETTINGS === -->
  <div id="tab-settings" class="tab-panel">
    <div class="settings-grid">
      <div class="settings-col">
        <div class="sg">
          <div class="sg-title">Display</div>
          <div class="sg-rows">
            <div class="toggle-row" id="nightModeToggle">
              <span class="toggle-label">Night Mode</span>
              <div class="toggle-sw" id="nightModeSw"><div class="toggle-knob"></div></div>
            </div>
            <div class="toggle-row" id="backlightToggle">
              <span class="toggle-label">Backlight ON</span>
              <div class="toggle-sw on" id="backlightSw"><div class="toggle-knob"></div></div>
            </div>
            <div id="displayToggleMsg" class="toggle-msg" aria-live="polite"></div>
          </div>
        </div>
        <div class="sg">
          <div class="sg-title">Device</div>
          <div class="sg-rows">
            <div class="text-field-row">
              <label class="text-field-lbl" for="displayNameInput">Display name</label>
              <input class="text-input" type="text" id="displayNameInput" maxlength="32" placeholder="aura" />
            </div>
            <button class="save-btn idle" type="button" id="saveNameBtn">No changes</button>
          </div>
        </div>
      </div>
      <div class="settings-col">
        <div class="sg">
          <div class="sg-title">Measurements</div>
          <div class="sg-rows">
            <div class="seg-row">
              <span class="seg-label">Unit system</span>
              <div class="seg-ctrl" id="tempUnitSeg">
                <button class="seg-btn active" type="button" data-unit="c">Metric</button>
                <button class="seg-btn" type="button" data-unit="f">US</button>
              </div>
            </div>
            <div class="stepper-row">
              <span class="stepper-label" id="tempOffsetLabel">Temp offset (&deg;C)</span>
              <div class="stepper-ctrl">
                <button class="stepper-btn" type="button" id="tempOffsetMinus">&minus;</button>
                <span class="stepper-val" id="tempOffsetVal">0.0</span>
                <button class="stepper-btn" type="button" id="tempOffsetPlus">+</button>
              </div>
            </div>
            <div class="stepper-row">
              <span class="stepper-label">Humidity offset (%)</span>
              <div class="stepper-ctrl">
                <button class="stepper-btn" type="button" id="humOffsetMinus">&minus;</button>
                <span class="stepper-val" id="humOffsetVal">0</span>
                <button class="stepper-btn" type="button" id="humOffsetPlus">+</button>
              </div>
            </div>
            <button class="save-btn idle" type="button" id="saveSettingsBtn">No changes</button>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- === SYSTEM === -->
  <div id="tab-system" class="tab-panel">
    <div class="system-grid">
      <div class="sg">
        <div class="sg-title">System Info</div>
        <div class="info-rows" id="systemInfoRows">
          <div class="info-row"><span class="info-key">Mode</span><span class="info-val" id="si-mode">--</span></div>
          <div class="info-row"><span class="info-key">IP Address</span><span class="info-val" id="si-ip">--</span></div>
          <div class="info-row"><span class="info-key">Hostname</span><span class="info-val" id="si-hostname">--</span></div>
          <div class="info-row"><span class="info-key">Firmware</span><span class="info-val" id="si-firmware">--</span></div>
          <div class="info-row"><span class="info-key">Build</span><span class="info-val" id="si-build">--</span></div>
          <div class="info-row"><span class="info-key">Uptime</span><span class="info-val" id="si-uptime">--</span></div>
          <div class="info-row"><span class="info-key">DAC</span><span class="info-val" id="si-dac">--</span></div>
        </div>
        <div class="sys-btn-row">
          <button class="btn btn-danger" type="button" id="rebootBtn">Reboot Device</button>
          <button class="btn" type="button" id="openDacBtn" disabled>Open DAC Page</button>
          <a class="btn link-btn" href="/">WiFi Setup</a>
        </div>
      </div>
      <div class="sg">
        <div class="sg-title">Firmware OTA</div>
        <div class="sg-rows">
          <div class="text-field-row">
            <label class="text-field-lbl" for="otaFile">Firmware file (.bin)</label>
            <input type="file" id="otaFile" accept=".bin,application/octet-stream" style="color:#d1d5db;" />
          </div>
          <button class="btn" type="button" id="otaUploadBtn">Upload firmware</button>
          <div class="progress-track"><div class="progress-fill" id="otaProgress"></div></div>
          <div class="ota-status" id="otaStatus">No upload in progress.</div>
        </div>
      </div>
    </div>
  </div>

</div><!-- .wrap -->
<script>
// ─────────────────────────────────────────────
// Thresholds & status
// ─────────────────────────────────────────────
const thresholds = {
  co2:             { good: 800,  moderate: 1000, bad: 1500 },
  pm05:            { good: 250,  moderate: 600,  bad: 1200 },
  pm25:            { good: 12,   moderate: 35,   bad: 55 },
  pm1:             { good: 10,   moderate: 25,   bad: 50 },
  pm4:             { good: 25,   moderate: 50,   bad: 75 },
  pm10:            { good: 54,   moderate: 154,  bad: 254 },
  voc:             { good: 150,  moderate: 250,  bad: 350 },
  nox:             { good: 50,   moderate: 100,  bad: 200 },
  hcho:            { good: 30,   moderate: 60,   bad: 100 },
  co:              { good: 9,    moderate: 35,   bad: 100 },
  temp:            { good: [20, 25], moderate: [18, 26], bad: [16, 28] },
  rh:              { good: [40, 60], moderate: [30, 65], bad: [20, 70] },
  dewPoint:        { good: [11, 16], moderate: [9, 18],  bad: [5, 21] },
  ah:              { good: [7, 15],  moderate: [5, 18],  bad: [4, 20] },
  mold:            { good: 2,    moderate: 4,    bad: 7 },
  pressureDelta3h: { good: 1.0,  moderate: 3.0,  bad: 6.0 },
  pressureDelta24h:{ good: 2.0,  moderate: 6.0,  bad: 10.0 },
};

const statusColors  = { good:'#22c55e', moderate:'#facc15', bad:'#f97316', critical:'#ef4444' };
const statusLabels  = { good:'Good', moderate:'Moderate', bad:'Poor', critical:'Hazard' };
const fallbackColor = '#9ca3af';

function getStatus(value, thr) {
  if (Array.isArray(thr.good)) {
    if (value >= thr.good[0]     && value <= thr.good[1])     return 'good';
    if (value >= thr.moderate[0] && value <= thr.moderate[1]) return 'moderate';
    if (value >= thr.bad[0]      && value <= thr.bad[1])      return 'bad';
    return 'critical';
  }
  if (value <= thr.good)     return 'good';
  if (value <= thr.moderate) return 'moderate';
  if (value <= thr.bad)      return 'bad';
  return 'critical';
}

function statusColorOf(s) { return statusColors[s] || fallbackColor; }

function hexToRgb(hex) {
  if (typeof hex !== 'string' || !hex.startsWith('#')) return null;
  let h = hex.slice(1);
  if (h.length === 3) h = h.split('').map(c => c+c).join('');
  if (h.length !== 6) return null;
  const n = parseInt(h, 16);
  return { r:(n>>16)&255, g:(n>>8)&255, b:n&255 };
}

function rgba(hex, a) {
  const rgb = hexToRgb(hex);
  if (!rgb) return `rgba(156,163,175,${a})`;
  return `rgba(${rgb.r},${rgb.g},${rgb.b},${a})`;
}

function isNum(v) { return typeof v === 'number' && Number.isFinite(v); }
function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)); }

function metricStatus(value, thr, round) {
  if (!isNum(value)) return null;
  return getStatus(round ? Math.round(value) : value, thr);
}

function fmtVal(v, d) { return isNum(v) ? Number(v).toFixed(d) : 'N/A'; }
function fmtSigned(v, d) {
  if (!isNum(v)) return 'N/A';
  return (v > 0 ? '+' : '') + Number(v).toFixed(d);
}
function fmtMM(v, unit) {
  if (!isNum(v)) return '-';
  const s = unit === 'inHg'
    ? v.toFixed(2)
    : (Math.abs(v) >= 100 ? v.toFixed(0) : v.toFixed(1));
  return unit ? s + ' ' + unit : s;
}
function isUsUnitSystem(tempUnit) { return tempUnit === 'f'; }
function tempToDisplay(tempC, tempUnit) {
  return isUsUnitSystem(tempUnit) ? (tempC * 9 / 5) + 32 : tempC;
}
function pressureToDisplay(pressureHpa, tempUnit) {
  return isUsUnitSystem(tempUnit) ? (pressureHpa * 0.0295299830714) : pressureHpa;
}
function pressureDeltaToDisplay(deltaHpa, tempUnit) {
  return isUsUnitSystem(tempUnit) ? (deltaHpa * 0.0295299830714) : deltaHpa;
}
function temperatureUnitLabel(tempUnit) {
  return isUsUnitSystem(tempUnit) ? '\u00B0F' : '\u00B0C';
}
function pressureUnitLabel(tempUnit) {
  return isUsUnitSystem(tempUnit) ? 'inHg' : 'hPa';
}
function pressureDigits(tempUnit) {
  return isUsUnitSystem(tempUnit) ? 2 : 1;
}
function convertChartValueByKey(key, value, tempUnit) {
  if (!isNum(value)) return value;
  if (key === 'temp' || key === 'temperature') return tempToDisplay(value, tempUnit);
  if (key === 'pressure') return pressureToDisplay(value, tempUnit);
  return value;
}

function formatChartTime(ts) {
  if (!isNum(ts)) return '--:--';
  const ms = ts > 1000000000000 ? ts : ts * 1000;
  const d = new Date(ms);
  if (Number.isNaN(d.getTime())) return '--:--';
  return d.toLocaleTimeString([], { hour:'2-digit', minute:'2-digit', hour12:false });
}

function esc(v) {
  return String(v == null ? '' : v)
    .replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;')
    .replace(/"/g,'&quot;').replace(/'/g,'&#39;');
}

function pillHtml(status, compact) {
  const color = statusColorOf(status);
  const label = statusLabels[status] || 'N/A';
  const sizeClass = compact ? 'pill pill-sm' : 'pill';
  const style = `color:${color};border-color:${rgba(color,.38)};background:${rgba(color,.14)};`;
  return `<span class="${sizeClass}" style="${style}">${esc(label)}</span>`;
}

function trendToken(delta, is24h) {
  if (!isNum(delta)) {
    return { textStyle:'color:'+fallbackColor, surfaceStyle:'color:'+fallbackColor+';border-color:rgba(156,163,175,.32);background:rgba(75,85,99,.2)' };
  }
  const absDelta = Math.abs(delta);
  const status = getStatus(absDelta, is24h ? thresholds.pressureDelta24h : thresholds.pressureDelta3h);
  const color = statusColorOf(status);
  return {
    textStyle: 'color:' + color,
    surfaceStyle: 'color:' + color + ';border-color:' + rgba(color,.32) + ';background:' + rgba(color,.12),
  };
}

// ─────────────────────────────────────────────
// SVG bezier chart engine
// ─────────────────────────────────────────────
function splitSegments(points) {
  const segs = []; let cur = [];
  points.forEach(p => {
    if (!isNum(p.v)) { if (cur.length) segs.push(cur); cur = []; }
    else cur.push(p);
  });
  if (cur.length) segs.push(cur);
  return segs;
}

function buildSmoothPath(seg) {
  if (!seg || !seg.length) return '';
  if (seg.length === 1) return `M ${seg[0].x.toFixed(2)} ${seg[0].y.toFixed(2)}`;
  if (seg.length === 2) return `M ${seg[0].x.toFixed(2)} ${seg[0].y.toFixed(2)} L ${seg[1].x.toFixed(2)} ${seg[1].y.toFixed(2)}`;
  let d = `M ${seg[0].x.toFixed(2)} ${seg[0].y.toFixed(2)}`;
  for (let i = 0; i < seg.length - 1; i++) {
    const p0 = seg[i-1] || seg[i];
    const p1 = seg[i];
    const p2 = seg[i+1];
    const p3 = seg[i+2] || p2;
    const cp1x = p1.x + (p2.x - p0.x) / 6;
    const cp1y = clamp(p1.y + (p2.y - p0.y) / 6, 0, 100);
    const cp2x = p2.x - (p3.x - p1.x) / 6;
    const cp2y = clamp(p2.y - (p3.y - p1.y) / 6, 0, 100);
    d += ` C ${cp1x.toFixed(2)} ${cp1y.toFixed(2)}, ${cp2x.toFixed(2)} ${cp2y.toFixed(2)}, ${p2.x.toFixed(2)} ${p2.y.toFixed(2)}`;
  }
  return d;
}

function buildChartSvg(data, keys, colors, height, options) {
  const h = height || 140;
  const opts = options || {};
  const showPoints = opts.showPoints === true;
  const lineNames = Array.isArray(opts.lineNames) ? opts.lineNames : [];
  const lineUnits = Array.isArray(opts.lineUnits) ? opts.lineUnits : [];
  const lineDigits = Array.isArray(opts.lineDigits) ? opts.lineDigits : [];

  const allVals = [];
  keys.forEach(k => data.forEach(row => { if (isNum(row[k])) allVals.push(row[k]); }));
  if (!allVals.length) return '<div class="no-data">No data</div>';

  let yMin = Math.min(...allVals);
  let yMax = Math.max(...allVals);
  const spread = yMax - yMin;
  const pad = spread > 1e-6 ? spread * 0.12 : Math.max(Math.abs(yMax) * 0.08, 1);
  yMin -= pad;
  yMax += pad;
  if (allVals.every(v => v >= 0) && yMin < 0) yMin = 0;
  if (Math.abs(yMax - yMin) < 1e-6) { yMin -= 1; yMax += 1; }

  const xFor = i => data.length <= 1 ? 0 : (i / (data.length - 1)) * 100;
  const yFor = v => ((yMax - v) / (yMax - yMin)) * 100;

  const uid = Math.random().toString(36).slice(2, 9);
  let defs = '<defs>';
  keys.forEach((k, ki) => {
    const c = colors[ki];
    defs += `<linearGradient id="g${uid}${ki}" x1="0" y1="0" x2="0" y2="1">
      <stop offset="5%" stop-color="${c}" stop-opacity="0.28"/>
      <stop offset="95%" stop-color="${c}" stop-opacity="0"/>
    </linearGradient>`;
  });
  defs += '</defs>';

  const grid = `<line x1="0" y1="25" x2="100" y2="25" stroke="#374151" stroke-width="0.7" stroke-dasharray="2.5 2.5" vector-effect="non-scaling-stroke"/>
    <line x1="0" y1="50" x2="100" y2="50" stroke="#374151" stroke-width="0.7" stroke-dasharray="2.5 2.5" vector-effect="non-scaling-stroke"/>
    <line x1="0" y1="75" x2="100" y2="75" stroke="#374151" stroke-width="0.7" stroke-dasharray="2.5 2.5" vector-effect="non-scaling-stroke"/>`;

  let areas = '';
  let lines2 = '';
  let pointsHtml = '';
  keys.forEach((k, ki) => {
    const c = colors[ki];
    const digits = Number.isInteger(lineDigits[ki]) ? lineDigits[ki] : 1;
    const name = lineNames[ki] || k.toUpperCase();
    const unit = lineUnits[ki] || '';
    const pts = data.map((row, i) => ({
      x: xFor(i),
      y: isNum(row[k]) ? yFor(row[k]) : null,
      v: row[k],
      t: row._time || '--:--',
    }));

    const segs = splitSegments(pts);
    segs.forEach(seg => {
      const linePath = buildSmoothPath(seg);
      if (!linePath) return;
      lines2 += `<path d="${linePath}" fill="none" stroke="${c}" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" vector-effect="non-scaling-stroke"/>`;
      if (seg.length >= 4) {
        const first = seg[0];
        const last = seg[seg.length - 1];
        areas += `<path d="${linePath} L ${last.x.toFixed(2)} 100 L ${first.x.toFixed(2)} 100 Z" fill="url(#g${uid}${ki})"/>`;
      }
    });

    if (showPoints) {
      pts.forEach(p => {
        if (!isNum(p.v) || !isNum(p.y)) return;
        const valueText = Number(p.v).toFixed(digits);
        const tooltipText = `${name} ${p.t}: ${valueText}${unit ? ' ' + unit : ''}`;
        pointsHtml += `<circle class="chart-point" data-tip="${esc(tooltipText)}" data-x="${p.x.toFixed(2)}" data-y="${p.y.toFixed(2)}" cx="${p.x.toFixed(2)}" cy="${p.y.toFixed(2)}" r="3.0" fill="transparent" stroke="transparent" stroke-width="0"></circle>`;
      });
    }
  });

  return `<svg viewBox="0 0 100 100" preserveAspectRatio="none" style="width:100%;height:${h}px;display:block;overflow:visible;">${defs}${grid}${areas}${lines2}${pointsHtml}</svg>`;
}

function buildMiniChartSvg(data, key, color) {
  const meta = CHART_META[key] || {};
  return buildChartSvg(
    data,
    [key],
    [color],
    80,
    { lineNames:[meta.label || key.toUpperCase()], lineUnits:[meta.unit || ''], lineDigits:[Number.isInteger(meta.digits) ? meta.digits : 1], showPoints:false }
  );
}

// ─────────────────────────────────────────────
// Chart metadata
// ─────────────────────────────────────────────
const CHART_META = {
  co2:         { label:'CO2',         unit:'ppm',             digits:0, color:'#3dd6c6' },
  temperature: { label:'Temperature', unit:'\u00B0C',        digits:1, color:'#f59e0b' },
  humidity:    { label:'Humidity',    unit:'%',               digits:0, color:'#60a5fa' },
  pressure:    { label:'Pressure',    unit:'hPa',             digits:0, color:'#22c55e' },
  co:          { label:'CO',          unit:'ppm',             digits:1, color:'#fb7185' },
  voc:         { label:'VOC',         unit:'idx',             digits:0, color:'#f97316' },
  nox:         { label:'NOx',         unit:'idx',             digits:0, color:'#f43f5e' },
  hcho:        { label:'HCHO',        unit:'ppb',             digits:0, color:'#a78bfa' },
  pm05:        { label:'PM0.5',       unit:'#/cm\u00B3',     digits:0, color:'#fde047' },
  pm1:         { label:'PM1.0',       unit:'\u00B5g/m\u00B3', digits:1, color:'#38bdf8' },
  pm25:        { label:'PM2.5',       unit:'\u00B5g/m\u00B3', digits:1, color:'#34d399' },
  pm4:         { label:'PM4.0',       unit:'\u00B5g/m\u00B3', digits:1, color:'#22d3ee' },
  pm10:        { label:'PM10',        unit:'\u00B5g/m\u00B3', digits:1, color:'#f87171' },
};

// ─────────────────────────────────────────────
// Render: HeroMetric
// ─────────────────────────────────────────────
function renderHeroMetric(sensors, historyRows) {
  const v = sensors && isNum(sensors.co2) ? sensors.co2 : null;
  const status = v !== null ? getStatus(v, thresholds.co2) : null;
  const color = statusColorOf(status);
  const advice = {
    good: 'Air is stable. Ventilation is optional.',
    moderate: 'Ventilation recommended in the next hour.',
    bad: 'Open windows or increase airflow now.',
    critical: 'Poor air quality. Ventilate immediately.',
  }[status] || 'No live CO2 data.';

  const co2Data = (historyRows && historyRows.length) ? historyRows : (v !== null ? [{co2: v}] : []);
  const validPts = co2Data.filter(r => isNum(r.co2));
  const mmMin = validPts.length ? Math.min(...validPts.map(r => r.co2)) : null;
  const mmMax = validPts.length ? Math.max(...validPts.map(r => r.co2)) : null;
  const firstCo2 = validPts.length ? validPts[0].co2 : null;
  const delta3h = (isNum(v) && isNum(firstCo2)) ? v - firstCo2 : null;
  const deltaColor = !isNum(delta3h) ? '#9ca3af' : delta3h > 20 ? '#fdba74' : delta3h < -20 ? '#a5f3fc' : '#d1d5db';
  const progressW = v !== null
    ? Math.min(100, Math.max(0, ((v - 400) / (2000 - 400)) * 100))
    : 0;

  const miniSvg = co2Data.length >= 2 ? buildMiniChartSvg(co2Data, 'co2', color) : '';

  document.getElementById('heroMetric').innerHTML =
    `<div class="hero-wrap">
      <div class="hero-label">CO2 Level</div>
      <div style="display:flex;align-items:flex-start;justify-content:space-between;gap:12px">
        <div>
          <div class="hero-val-row">
            <span class="hero-val" style="color:${color}">${fmtVal(v, 0)}</span>
            <span class="hero-unit">ppm</span>
          </div>
        </div>
        ${pillHtml(status, false)}
      </div>
      <div class="hero-progress-track">
        <div class="hero-progress-fill" style="width:${progressW.toFixed(1)}%;background:${color};"></div>
      </div>
      <div class="hero-advice">${esc(advice)}</div>
      <div class="hero-trend-sep">
        <div class="hero-trend-hdr">
          <span class="hero-trend-label">3h Trend</span>
          <span class="hero-trend-delta" style="color:${deltaColor}">${isNum(delta3h) ? (delta3h > 0 ? '+' : '') + delta3h.toFixed(0) + ' ppm' : 'N/A'}</span>
        </div>
        <div style="margin-top:8px;height:80px;">${miniSvg || '<div class="no-data" style="height:100%;display:flex;align-items:center;justify-content:center;">Awaiting data</div>'}</div>
        <div class="hero-minmax">
          <span>min ${fmtMM(mmMin, 'ppm')}</span>
          <span>max ${fmtMM(mmMax, 'ppm')}</span>
        </div>
      </div>
    </div>`;
}

// ─────────────────────────────────────────────
// Render: ClimateOverview
// ─────────────────────────────────────────────
function renderClimateOverview(sensors, derived) {
  const temp     = sensors && isNum(sensors.temp)     ? sensors.temp     : null;
  const rh       = sensors && isNum(sensors.rh)       ? sensors.rh       : null;
  const pressure = sensors && isNum(sensors.pressure) ? sensors.pressure : null;
  const mold     = derived && isNum(derived.mold)     ? derived.mold     : null;
  const dewPoint = derived && isNum(derived.dew_point)? derived.dew_point: null;
  const ah       = derived && isNum(derived.ah)       ? derived.ah       : null;
  const delta3h  = derived && isNum(derived.pressure_delta_3h)  ? derived.pressure_delta_3h  : null;
  const delta24h = derived && isNum(derived.pressure_delta_24h) ? derived.pressure_delta_24h : null;

  const tempStatus  = metricStatus(temp, thresholds.temp);
  const rhStatus    = metricStatus(rh, thresholds.rh);
  const moldStatus  = metricStatus(mold, thresholds.mold);
  const dpStatus    = metricStatus(dewPoint, thresholds.dewPoint, true);
  const ahStatus    = metricStatus(ah, thresholds.ah);
  const statusRank  = { good:0, moderate:1, bad:2, critical:3 };
  const candidates  = [tempStatus, rhStatus, moldStatus].filter(s => s in statusRank);
  const climateStatus = candidates.length ? candidates.reduce((w, s) => statusRank[s] > statusRank[w] ? s : w, candidates[0]) : null;

  const t3  = trendToken(delta3h, false);
  const t24 = trendToken(delta24h, true);
  const tempDisplay = isNum(temp) ? tempToDisplay(temp, settings.tempUnit) : temp;
  const dewPointDisplay = isNum(dewPoint) ? tempToDisplay(dewPoint, settings.tempUnit) : dewPoint;
  const pressureDisplay = isNum(pressure) ? pressureToDisplay(pressure, settings.tempUnit) : pressure;
  const delta3hDisplay = isNum(delta3h) ? pressureDeltaToDisplay(delta3h, settings.tempUnit) : delta3h;
  const delta24hDisplay = isNum(delta24h) ? pressureDeltaToDisplay(delta24h, settings.tempUnit) : delta24h;
  const tempUnitText = temperatureUnitLabel(settings.tempUnit);
  const pressureUnitText = pressureUnitLabel(settings.tempUnit);
  const pressureValueDigits = pressureDigits(settings.tempUnit);

  document.getElementById('climateOverview').innerHTML =
    `<div class="climate-wrap">
      <div class="climate-head">
        <span class="climate-title">Climate</span>
        ${pillHtml(climateStatus, false)}
      </div>
      <div class="climate-grid2">
        <div class="mini-card">
          <div class="mini-label">Temperature</div>
          <div class="mini-val-row">
            <span class="mini-val" style="color:${statusColorOf(tempStatus)}">${fmtVal(tempDisplay,1)}</span>
            <span class="mini-unit">${tempUnitText}</span>
          </div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Humidity</div>
          <div class="mini-val-row">
            <span class="mini-val" style="color:${statusColorOf(rhStatus)}">${fmtVal(rh,0)}</span>
            <span class="mini-unit">%</span>
          </div>
        </div>
      </div>
      <div class="climate-grid3">
        <div class="mini-card">
          <div class="mini-label">Mold Risk</div>
          <div class="mini-val-row">
            <span class="mini-val" style="color:${statusColorOf(moldStatus)}">${fmtVal(mold,1)}</span>
            <span class="mini-unit">/10</span>
          </div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Dew Point</div>
          <div class="mini-val-row">
            <span class="mini-val" style="color:${statusColorOf(dpStatus)}">${fmtVal(dewPointDisplay,1)}</span>
            <span class="mini-unit">${tempUnitText}</span>
          </div>
        </div>
        <div class="mini-card">
          <div class="mini-label">Abs Humidity</div>
          <div class="mini-val-row">
            <span class="mini-val" style="color:${statusColorOf(ahStatus)}">${fmtVal(ah,1)}</span>
            <span class="mini-unit">g/m³</span>
          </div>
        </div>
      </div>
      <div class="pressure-mini mini-card" style="margin-top:10px;">
        <div class="pressure-row">
          <div>
            <div class="mini-label">Pressure</div>
            <div class="mini-val-row">
              <span class="mini-val" style="font-size:22px;color:#f9fafb">${fmtVal(pressureDisplay,pressureValueDigits)}</span>
              <span class="mini-unit">${pressureUnitText}</span>
            </div>
          </div>
          <div class="pressure-delta-grid">
            <div class="pdelta-box" style="${t3.surfaceStyle}">
              <div class="pdelta-lbl">3h</div>
              <div class="pdelta-val" style="${t3.textStyle}">${fmtSigned(delta3hDisplay,pressureValueDigits)}</div>
            </div>
            <div class="pdelta-box" style="${t24.surfaceStyle}">
              <div class="pdelta-lbl">24h</div>
              <div class="pdelta-val" style="${t24.textStyle}">${fmtSigned(delta24hDisplay,pressureValueDigits)}</div>
            </div>
          </div>
        </div>
      </div>
    </div>`;
}

// ─────────────────────────────────────────────
// Render: Gas grid
// ─────────────────────────────────────────────
const GAS_METRICS = [
  { key:'co',   label:'CO',   unit:'ppm', max:thresholds.co.bad,   thr:thresholds.co,   digits:1 },
  { key:'voc',  label:'VOC',  unit:'idx', max:thresholds.voc.bad,  thr:thresholds.voc,  digits:0 },
  { key:'nox',  label:'NOx',  unit:'idx', max:thresholds.nox.bad,  thr:thresholds.nox,  digits:0 },
  { key:'hcho', label:'HCHO', unit:'ppb', max:thresholds.hcho.bad, thr:thresholds.hcho, digits:0 },
];

function renderGasGrid(sensors) {
  const s = sensors || {};
  document.getElementById('gasGrid').innerHTML =
    `<div class="gas-grid">` +
    GAS_METRICS.map(m => {
      const v = isNum(s[m.key]) ? s[m.key] : null;
      const status = v !== null ? getStatus(v, m.thr) : null;
      const color = statusColorOf(status);
      const bar = v !== null ? Math.min((v / m.max) * 100, 100).toFixed(1) : 0;
      return `<div class="card-g7 gas-card">
        <div class="gas-head">
          <span class="gas-name">${esc(m.label)}</span>
          <span class="gas-unit">${esc(m.unit)}</span>
        </div>
        <div class="gas-val-row">
          <span class="gas-val" style="color:${color}">${fmtVal(v, m.digits)}</span>
          ${pillHtml(status, true)}
        </div>
        <div class="gas-progress"><div class="gas-bar" style="width:${bar}%;background:${color};"></div></div>
      </div>`;
    }).join('') +
    `</div>`;
}

// ─────────────────────────────────────────────
// Render: PM grid
// ─────────────────────────────────────────────
const PM_METRICS = [
  { key:'pm05', label:'PM0.5', unit:'#/cm\u00B3', max:thresholds.pm05.bad, thr:thresholds.pm05, digits:0 },
  { key:'pm1',  label:'PM1.0', unit:'\u00B5g/m\u00B3', max:thresholds.pm1.bad,  thr:thresholds.pm1,  digits:1 },
  { key:'pm25', label:'PM2.5', unit:'\u00B5g/m\u00B3', max:thresholds.pm25.bad, thr:thresholds.pm25, digits:1 },
  { key:'pm4',  label:'PM4.0', unit:'\u00B5g/m\u00B3', max:thresholds.pm4.bad,  thr:thresholds.pm4,  digits:1 },
  { key:'pm10', label:'PM10',  unit:'\u00B5g/m\u00B3', max:thresholds.pm10.bad, thr:thresholds.pm10, digits:1 },
];

function renderPMGrid(sensors) {
  const s = sensors || {};
  document.getElementById('pmGrid').innerHTML =
    `<div class="pm-grid">` +
    PM_METRICS.map(m => {
      const v = isNum(s[m.key]) ? s[m.key] : null;
      const status = v !== null ? getStatus(v, m.thr) : null;
      const color = statusColorOf(status);
      const bar = v !== null ? Math.min((v / m.max) * 100, 100).toFixed(1) : 0;
      return `<div class="card-g7 pm-card">
        <div class="pm-head">
          <span class="pm-label">${esc(m.label)}</span>
          <span class="pm-unit">${esc(m.unit)}</span>
        </div>
        <div class="pm-val-row">
          <div class="pm-val-left">
            <span class="pm-val" style="color:${color}">${fmtVal(v, m.digits)}</span>
          </div>
          ${pillHtml(status, true)}
        </div>
        <div class="pm-progress"><div class="pm-bar" style="width:${bar}%;background:${color};"></div></div>
      </div>`;
    }).join('') +
    `</div>`;
}

// ─────────────────────────────────────────────
// Render: Charts
// ─────────────────────────────────────────────
const CHART_LAYOUTS = {
  core: [
    { title:'CO2 Concentration', unit:'ppm', lines:[{ key:'co2', name:'CO2', color:'#10b981', digits:0, unit:'ppm' }] },
    { title:'Temperature', unit:'\u00B0C', lines:[{ key:'temperature', name:'Temperature', color:'#f59e0b', digits:1, unit:'\u00B0C' }] },
    { title:'Humidity', unit:'%', lines:[{ key:'humidity', name:'Humidity', color:'#60a5fa', digits:0, unit:'%' }] },
    { title:'Pressure', unit:'hPa', lines:[{ key:'pressure', name:'Pressure', color:'#22c55e', digits:1, unit:'hPa' }] },
  ],
  gases: [
    { title:'VOC', unit:'idx', lines:[{ key:'voc', name:'VOC', color:'#f97316', digits:0, unit:'idx' }] },
    { title:'NOx', unit:'idx', lines:[{ key:'nox', name:'NOx', color:'#f43f5e', digits:0, unit:'idx' }] },
    { title:'HCHO', unit:'ppb', lines:[{ key:'hcho', name:'HCHO', color:'#a78bfa', digits:0, unit:'ppb' }] },
    { title:'CO', unit:'ppm', lines:[{ key:'co', name:'CO', color:'#fb7185', digits:1, unit:'ppm' }] },
  ],
  pm: [
    { title:'PM0.5', unit:'#/cm\u00B3', lines:[{ key:'pm05', name:'PM0.5', color:'#fde047', digits:0, unit:'#/cm\u00B3' }] },
    { title:'PM1.0', unit:'\u00B5g/m\u00B3', lines:[{ key:'pm1', name:'PM1.0', color:'#38bdf8', digits:1, unit:'\u00B5g/m\u00B3' }] },
    { title:'PM2.5', unit:'\u00B5g/m\u00B3', lines:[{ key:'pm25', name:'PM2.5', color:'#34d399', digits:1, unit:'\u00B5g/m\u00B3' }] },
    {
      title:'PM10 + PM4.0',
      unit:'\u00B5g/m\u00B3',
      lines:[
        { key:'pm10', name:'PM10', color:'#f87171', digits:1, unit:'\u00B5g/m\u00B3' },
        { key:'pm4',  name:'PM4.0', color:'#22d3ee', digits:1, unit:'\u00B5g/m\u00B3' },
      ],
    },
  ],
};

function mapSeriesKeyToSensorKey(key) {
  if (key === 'temperature') return 'temp';
  if (key === 'humidity') return 'rh';
  return key;
}

function bindChartPointTooltips(root) {
  if (!root) return;
  root.querySelectorAll('.chart-svg-wrap').forEach(wrap => {
    const tooltip = wrap.querySelector('.chart-tooltip');
    if (!tooltip) return;
    const pointEls = Array.from(wrap.querySelectorAll('.chart-point'));
    if (!pointEls.length) return;

    const groups = new Map();
    pointEls.forEach(point => {
      const x = Number(point.getAttribute('data-x'));
      const y = Number(point.getAttribute('data-y'));
      const tip = point.getAttribute('data-tip') || '';
      if (!Number.isFinite(x) || !Number.isFinite(y) || !tip) return;
      const key = x.toFixed(2);
      if (!groups.has(key)) groups.set(key, []);
      groups.get(key).push({ x, y, tip });
    });
    const xKeys = Array.from(groups.keys()).map(Number).filter(Number.isFinite);
    if (!xKeys.length) return;

    const hide = () => {
      tooltip.classList.remove('show');
      tooltip.textContent = '';
    };

    const showAtClientX = clientX => {
      const rect = wrap.getBoundingClientRect();
      if (!rect.width) { hide(); return; }
      const xPct = clamp(((clientX - rect.left) / rect.width) * 100, 0, 100);

      let nearest = xKeys[0];
      let bestDist = Math.abs(xKeys[0] - xPct);
      for (let i = 1; i < xKeys.length; ++i) {
        const d = Math.abs(xKeys[i] - xPct);
        if (d < bestDist) { bestDist = d; nearest = xKeys[i]; }
      }

      const entries = groups.get(nearest.toFixed(2)) || [];
      if (!entries.length) { hide(); return; }

      tooltip.textContent = entries.map(e => e.tip).join('\n');
      const yMin = entries.reduce((acc, e) => Math.min(acc, e.y), entries[0].y);
      const xPx = clamp((nearest / 100) * rect.width, 32, Math.max(32, rect.width - 32));
      const yPx = clamp((yMin / 100) * rect.height - 8, 18, Math.max(18, rect.height - 18));
      tooltip.style.left = `${xPx}px`;
      tooltip.style.top = `${yPx}px`;
      tooltip.classList.add('show');
    };

    wrap.addEventListener('mousemove', e => showAtClientX(e.clientX));
    wrap.addEventListener('touchstart', e => {
      if (!e.touches || !e.touches[0]) return;
      showAtClientX(e.touches[0].clientX);
    }, { passive: true });
    wrap.addEventListener('touchmove', e => {
      if (!e.touches || !e.touches[0]) return;
      showAtClientX(e.touches[0].clientX);
    }, { passive: true });
    wrap.addEventListener('mouseleave', hide);
    wrap.addEventListener('touchend', hide);
    wrap.addEventListener('touchcancel', hide);
  });
}

function renderCharts(payload) {
  const timestamps = Array.isArray(payload && payload.timestamps) ? payload.timestamps : [];
  const series = Array.isArray(payload && payload.series) ? payload.series : [];
  const el = document.getElementById('chartGrid');
  if (!el) return;

  if (!series.length) { el.innerHTML = '<div class="no-data">No chart data</div>'; return; }

  const seriesByKey = {};
  series.forEach(s => { if (s && typeof s.key === 'string') seriesByKey[s.key] = s; });

  const rows = timestamps.map((ts, i) => {
    const row = { _ts: ts, _time: formatChartTime(ts) };
    series.forEach(s => {
      if (s && typeof s.key === 'string') {
        const rawValue = (Array.isArray(s.values) && isNum(s.values[i])) ? s.values[i] : null;
        row[s.key] = convertChartValueByKey(s.key, rawValue, settings.tempUnit);
      }
    });
    return row;
  });

  const layout = CHART_LAYOUTS[chartGroup] || CHART_LAYOUTS.core;
  el.innerHTML = layout.map(card => {
    const lineKeys = card.lines.map(line => line.key);
    const lineColors = card.lines.map(line => line.color || '#3dd6c6');
    const lineNames = card.lines.map(line => line.name || line.key.toUpperCase());
    const lineUnits = card.lines.map(line => {
      if (line.key === 'temperature') return temperatureUnitLabel(settings.tempUnit);
      if (line.key === 'pressure') return pressureUnitLabel(settings.tempUnit);
      return line.unit || card.unit || '';
    });
    const lineDigits = card.lines.map(line => {
      if (line.key === 'pressure') return pressureDigits(settings.tempUnit);
      return Number.isInteger(line.digits) ? line.digits : 1;
    });
    const cardUnit = card.lines.length === 1
      ? lineUnits[0]
      : (card.unit || '');

    const latestItems = card.lines.map((line, idx) => {
      const liveKey = mapSeriesKeyToSensorKey(line.key);
      const liveVal = stateCache && stateCache.sensors ? stateCache.sensors[liveKey] : null;
      const apiLatest = seriesByKey[line.key] && isNum(seriesByKey[line.key].latest) ? seriesByKey[line.key].latest : null;
      const rawLatest = isNum(liveVal) ? liveVal : apiLatest;
      const v = convertChartValueByKey(line.key, rawLatest, settings.tempUnit);
      return {
        value: v,
        digits: lineDigits[idx],
        color: lineColors[idx],
      };
    });

    const vals = [];
    rows.forEach(row => {
      lineKeys.forEach(k => { if (isNum(row[k])) vals.push(row[k]); });
    });
    const mmMin = vals.length ? Math.min(...vals) : null;
    const mmMax = vals.length ? Math.max(...vals) : null;

    const svgHtml = rows.length >= 2
      ? buildChartSvg(rows, lineKeys, lineColors, 140, { lineNames, lineUnits, lineDigits, showPoints: true })
      : '<div class="no-data">Awaiting data</div>';

    return `<div class="card-g7 chart-box">
      <div class="chart-head">
        <span class="chart-name">${esc(card.title)}</span>
        <div class="chart-latest-row">
          ${latestItems.map(item =>
            `<span class="chart-latest-item" style="color:${item.color}">${isNum(item.value) ? Number(item.value).toFixed(item.digits) : '-'}</span>`
          ).join('')}
        </div>
      </div>
      <div class="chart-svg-wrap">${svgHtml}<div class="chart-tooltip"></div></div>
      <div class="chart-minmax">
        <span>min ${fmtMM(mmMin, cardUnit)}</span>
        <span>max ${fmtMM(mmMax, cardUnit)}</span>
      </div>
    </div>`;
  }).join('');
  bindChartPointTooltips(el);
}

// ─────────────────────────────────────────────
// Render: Events
// ─────────────────────────────────────────────
function renderEvents(payload) {
  const events = Array.isArray(payload && payload.events) ? payload.events : [];
  const el = document.getElementById('eventsList');
  if (!events.length) { el.innerHTML = '<div class="no-data">No events</div>'; return; }

  const uptimeS = isNum(payload && payload.uptime_s) ? payload.uptime_s : null;
  const severityClass = { warning:'sev-warning', danger:'sev-danger', critical:'sev-critical', info:'sev-info' };

  const items = events.slice(0, 40).reverse().map(e => {
    const sev = (e && typeof e.severity === 'string') ? e.severity : 'info';
    const sevClass = severityClass[sev] || 'sev-info';
    const tsMs = isNum(e && e.ts_ms) ? e.ts_ms : null;
    let timeText = '--';
    if (uptimeS !== null && tsMs !== null) {
      const ageS = Math.max(0, uptimeS - Math.floor(tsMs / 1000));
      if (ageS < 60)    timeText = ageS + 's ago';
      else if (ageS < 3600)  timeText = Math.floor(ageS/60) + 'm ago';
      else if (ageS < 86400) timeText = Math.floor(ageS/3600) + 'h ago';
      else timeText = Math.floor(ageS/86400) + 'd ago';
    }
    const type = (e && e.type) || 'SYSTEM';
    const msg  = (e && e.message) || 'Event';
    return `<div class="alert-item ${sevClass}">
      <span class="alert-msg">${esc(msg)}</span>
      <div class="alert-meta">
        <span class="alert-type">${esc(type)}</span>
        <span class="alert-time">${esc(timeText)}</span>
      </div>
    </div>`;
  }).join('');

  el.innerHTML = items || '<div class="no-data">No events</div>';
}

// ─────────────────────────────────────────────
// Render: System info
// ─────────────────────────────────────────────
function renderSystemMeta(payload) {
  const network = (payload && payload.network) || {};
  const system  = (payload && payload.system)  || {};
  const derived = (payload && payload.derived) || {};

  const set = (id, val, cls) => {
    const el = document.getElementById(id);
    if (!el) return;
    el.textContent = val;
    if (cls) el.className = 'info-val ' + cls;
  };

  set('si-mode', String(network.mode || '--').toUpperCase());
  set('si-ip',   network.ip || '--');
  set('si-hostname', network.hostname || '--');
  set('si-firmware', system.firmware || '--');
  set('si-build', [system.build_date, system.build_time].filter(Boolean).join(' ') || '--');
  set('si-uptime', system.uptime || derived.uptime || '--');
  set('si-dac', system.dac_available ? 'Yes' : 'No');

  const dacBtn = document.getElementById('openDacBtn');
  if (dacBtn) dacBtn.disabled = system.dac_available !== true;
}

// ─────────────────────────────────────────────
// State
// ─────────────────────────────────────────────
let activeTab = 'sensors';
let chartGroup = 'core';
let chartRange = '24h';
let stateCache = null;
let historyCache = null;
let refreshBusy = false;
let otaUploadInFlight = false;
let otaRestartPending = false;
let otaRecoveryTimer = null;
let otaRecoveryActive = false;
let otaRecoveryProbeController = null;
const OTA_RECOVERY_PROBE_TIMEOUT_MS = 1500;
const OTA_UPLOAD_MIN_TIMEOUT_MS = 180000;
const OTA_UPLOAD_MAX_TIMEOUT_MS = 900000;
const OTA_UPLOAD_MIN_BYTES_PER_SEC = 20 * 1024;
let deviceClockRef = null;

// Settings state
const settings = {
  nightMode: false, nightModeLocked: false,
  backlight: true,
  tempUnit: 'c',
  tempOffset: 0, humOffset: 0,
  displayName: '',
};
const savedSettings = Object.assign({}, settings);
let settingsSaving = false;
let settingsSaveStatus = 'idle'; // idle|dirty|saving|saved|error
let nameDirty = false;
const toggleRequestState = {
  night_mode: { inFlight: false, queued: null },
  backlight_on: { inFlight: false, queued: null },
};
let toggleMsgTimer = null;
let chartsRefreshToken = 0;
let chartsRefreshController = null;

function resolveHeaderDeviceName() {
  const displayName =
    (typeof settings.displayName === 'string' && settings.displayName.trim())
      ? settings.displayName.trim()
      : '';
  const net = (stateCache && stateCache.network) || {};
  return displayName || net.hostname || 'aura';
}

function renderHeaderDeviceName() {
  const el = document.getElementById('deviceNameLabel');
  if (!el) return;
  el.textContent = resolveHeaderDeviceName();
}

function rerenderUnitDependentViews() {
  updateHeaderClock();
  if (stateCache) {
    renderClimateOverview(stateCache.sensors, stateCache.derived);
  }
  if (historyCache) {
    renderCharts(historyCache);
  }
}

// ─────────────────────────────────────────────
// API helpers
// ─────────────────────────────────────────────
async function getJson(url, init) {
  const requestInit = init ? Object.assign({}, init) : {};
  if (!Object.prototype.hasOwnProperty.call(requestInit, 'cache')) {
    requestInit.cache = 'no-store';
  }
  const r = await fetch(url, requestInit);
  if (!r.ok) throw new Error('HTTP ' + r.status + ' for ' + url);
  return r.json();
}

async function postJson(url, payload) {
  const r = await fetch(url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload),
  });
  let json = null;
  try { json = await r.json(); } catch (_) {}
  if (!r.ok) throw new Error((json && json.error) || ('HTTP ' + r.status));
  return json;
}

function stopOtaRecoveryWatcher() {
  otaRecoveryActive = false;
  if (otaRecoveryTimer) {
    clearTimeout(otaRecoveryTimer);
    otaRecoveryTimer = null;
  }
  if (otaRecoveryProbeController) {
    otaRecoveryProbeController.abort();
    otaRecoveryProbeController = null;
  }
}

function scheduleOtaRecoveryProbe(delayMs) {
  if (!otaRecoveryActive) return;
  if (otaRecoveryTimer) {
    clearTimeout(otaRecoveryTimer);
    otaRecoveryTimer = null;
  }
  if (otaRecoveryProbeController) {
    otaRecoveryProbeController.abort();
    otaRecoveryProbeController = null;
  }
  otaRecoveryTimer = setTimeout(async () => {
    if (!otaRecoveryActive) return;
    const controller = new AbortController();
    otaRecoveryProbeController = controller;
    const timeoutId = setTimeout(() => {
      controller.abort();
    }, OTA_RECOVERY_PROBE_TIMEOUT_MS);
    try {
      const r = await fetch('/api/state?probe=1', { cache: 'no-store', signal: controller.signal });
      if (r.ok) {
        window.location.reload();
        return;
      }
    } catch (_) {
    } finally {
      clearTimeout(timeoutId);
      if (otaRecoveryProbeController === controller) {
        otaRecoveryProbeController = null;
      }
    }
    scheduleOtaRecoveryProbe(2000);
  }, delayMs);
}

function computeOtaTimeoutMs(sizeBytes) {
  if (!isNum(sizeBytes) || sizeBytes <= 0) {
    return OTA_UPLOAD_MIN_TIMEOUT_MS;
  }
  const transferMs = Math.ceil((sizeBytes * 1000) / OTA_UPLOAD_MIN_BYTES_PER_SEC);
  const timeoutMs = transferMs + 120000;
  return Math.min(OTA_UPLOAD_MAX_TIMEOUT_MS, Math.max(OTA_UPLOAD_MIN_TIMEOUT_MS, timeoutMs));
}

function startOtaRecoveryWatcher() {
  if (otaRecoveryActive) return;
  otaRecoveryActive = true;
  scheduleOtaRecoveryProbe(1200);
}

// ─────────────────────────────────────────────
// Clock
// ─────────────────────────────────────────────
function updateHeaderClock() {
  const nowMs = Date.now();
  const now = deviceClockRef
    ? new Date(deviceClockRef.epochMs + (nowMs - deviceClockRef.capturedAtMs))
    : new Date(nowMs);
  document.getElementById('headerTime').textContent = now.toLocaleTimeString([], { hour:'2-digit', minute:'2-digit', hour12:false });
  document.getElementById('headerDate').textContent = now.toLocaleDateString(
    settings.tempUnit === 'f' ? 'en-US' : 'en-GB',
    { day:'2-digit', month:'short', year:'numeric' }
  ).toUpperCase();
}

// ─────────────────────────────────────────────
// Settings UI
// ─────────────────────────────────────────────
function updateToggleDom(swId, enabled) {
  const sw = document.getElementById(swId);
  if (!sw) return;
  sw.classList.toggle('on', !!enabled);
}

function showToggleError(message) {
  const el = document.getElementById('displayToggleMsg');
  if (!el) return;
  el.textContent = message || '';
  el.classList.toggle('err', !!message);
  if (toggleMsgTimer) {
    clearTimeout(toggleMsgTimer);
    toggleMsgTimer = null;
  }
  if (message) {
    toggleMsgTimer = setTimeout(() => {
      const msgEl = document.getElementById('displayToggleMsg');
      if (!msgEl) return;
      msgEl.textContent = '';
      msgEl.classList.remove('err');
      toggleMsgTimer = null;
    }, 3500);
  }
}

function updateSaveBtn() {
  const btn = document.getElementById('saveSettingsBtn');
  if (!btn) return;
  btn.className = 'save-btn ' + settingsSaveStatus;
  const labels = { idle:'No changes', dirty:'Save Settings', saving:'Saving…', saved:'Saved', error:'Save Failed' };
  btn.textContent = labels[settingsSaveStatus] || 'Save';
  btn.disabled = settingsSaveStatus === 'idle' || settingsSaveStatus === 'saving';
}

function markMeasurementsDirty() {
  settingsSaveStatus = 'dirty';
  updateSaveBtn();
}

function tempOffsetCToDisplay(offsetC, tempUnit) {
  return tempUnit === 'f' ? (offsetC * 9 / 5) : offsetC;
}

function tempOffsetDisplayToC(offsetDisplay, tempUnit) {
  return tempUnit === 'f' ? (offsetDisplay * 5 / 9) : offsetDisplay;
}

function nudgeTempOffset(deltaDisplay) {
  const maxDisplayOffset = settings.tempUnit === 'f' ? 9 : 5;
  const displayVal = tempOffsetCToDisplay(settings.tempOffset, settings.tempUnit);
  const nextDisplayVal = Math.max(
    -maxDisplayOffset,
    Math.min(maxDisplayOffset, Math.round((displayVal + deltaDisplay) * 10) / 10)
  );
  settings.tempOffset = Number(tempOffsetDisplayToC(nextDisplayVal, settings.tempUnit).toFixed(1));
}

function nudgeHumOffset(delta) {
  const next = Math.round(settings.humOffset + delta);
  settings.humOffset = Math.max(-10, Math.min(10, next));
}

function updateTempOffsetLabel() {
  const lbl = document.getElementById('tempOffsetLabel');
  if (lbl) lbl.textContent = 'Temp offset (' + (settings.tempUnit === 'f' ? '\u00B0F' : '\u00B0C') + ')';
}

function updateTempOffsetDisplay() {
  const el = document.getElementById('tempOffsetVal');
  if (!el) return;
  const displayVal = tempOffsetCToDisplay(settings.tempOffset, settings.tempUnit);
  el.textContent = displayVal.toFixed(1);
}

function updateHumOffsetDisplay() {
  const el = document.getElementById('humOffsetVal');
  if (el) el.textContent = settings.humOffset.toFixed(0);
}

function updateSegmentDom(segId, attrName, value) {
  const seg = document.getElementById(segId);
  if (!seg) return;
  seg.querySelectorAll('.seg-btn').forEach(btn => {
    btn.classList.toggle('active', btn.getAttribute(attrName) === value);
  });
}

function shouldApplyToggleFromApi(toggleKey, overrideKey) {
  if (overrideKey === toggleKey) return true;
  const queue = toggleRequestState[toggleKey];
  return !(queue && (queue.inFlight || queue.queued !== null));
}

function applySettingsToUI(apiSettings, force, toggleOverrideKey) {
  if (!apiSettings) return;
  if (settingsSaveStatus === 'dirty' && !force) return;
  const prevTempUnit = settings.tempUnit;

  if (typeof apiSettings.night_mode === 'boolean' &&
      shouldApplyToggleFromApi('night_mode', toggleOverrideKey)) {
    settings.nightMode = apiSettings.night_mode;
  }
  if (typeof apiSettings.night_mode_locked === 'boolean') settings.nightModeLocked = apiSettings.night_mode_locked;
  if (typeof apiSettings.backlight_on === 'boolean' &&
      shouldApplyToggleFromApi('backlight_on', toggleOverrideKey)) {
    settings.backlight = apiSettings.backlight_on;
  }
  if (typeof apiSettings.units_c === 'boolean') settings.tempUnit = apiSettings.units_c ? 'c' : 'f';
  if (isNum(apiSettings.temp_offset)) settings.tempOffset = Number(apiSettings.temp_offset.toFixed(1));
  if (isNum(apiSettings.hum_offset)) settings.humOffset = Number(apiSettings.hum_offset.toFixed(0));
  if (typeof apiSettings.display_name === 'string') settings.displayName = apiSettings.display_name;

  updateToggleDom('nightModeSw', settings.nightMode);
  updateToggleDom('backlightSw', settings.backlight);
  const nightRow = document.getElementById('nightModeToggle');
  if (nightRow) nightRow.classList.toggle('disabled', settings.nightModeLocked);
  updateSegmentDom('tempUnitSeg', 'data-unit', settings.tempUnit);
  updateTempOffsetLabel();
  updateTempOffsetDisplay();
  updateHumOffsetDisplay();
  if (prevTempUnit !== settings.tempUnit) {
    rerenderUnitDependentViews();
  }

  const nameInput = document.getElementById('displayNameInput');
  if (nameInput && (!nameDirty || force)) {
    nameInput.value = settings.displayName || '';
    nameDirty = false;
    updateNameBtn('idle');
  }

  renderHeaderDeviceName();

  Object.assign(savedSettings, settings);
  if (!force && settingsSaveStatus !== 'error') {
    settingsSaveStatus = 'idle';
    updateSaveBtn();
  }
}

function updateNameBtn(status) {
  const btn = document.getElementById('saveNameBtn');
  if (!btn) return;
  btn.className = 'save-btn ' + status;
  const labels = { idle:'No changes', dirty:'Save Name', saving:'Saving…', saved:'Saved', error:'Save Failed' };
  btn.textContent = labels[status] || 'Save';
  btn.disabled = status === 'idle' || status === 'saving';
}

function processToggleQueue(key) {
  const queue = toggleRequestState[key];
  if (!queue || queue.inFlight || queue.queued === null) return;

  const target = queue.queued;
  queue.queued = null;
  queue.inFlight = true;

  const payload = {};
  payload[key] = target;
  postJson('/api/settings', payload)
    .then(result => {
      // Avoid forcing a stale response into UI if a newer value is already queued.
      if (result && result.settings && queue.queued === null) {
        applySettingsToUI(result.settings, true, key);
      }
      showToggleError('');
    })
    .catch(() => {
      // Recover from transport/server errors by re-syncing current settings.
      getJson('/api/state')
        .then(statePayload => {
          if (statePayload && statePayload.settings) {
            applySettingsToUI(statePayload.settings, true, key);
          }
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
}

function enqueueToggle(key, value) {
  const queue = toggleRequestState[key];
  if (!queue) return;
  queue.queued = value;
  processToggleQueue(key);
}

async function saveMeasurements() {
  if (settingsSaving) return;
  settingsSaving = true;
  settingsSaveStatus = 'saving';
  updateSaveBtn();
  try {
    const result = await postJson('/api/settings', {
      units_c: settings.tempUnit === 'c',
      temp_offset: Number(settings.tempOffset.toFixed(1)),
      hum_offset: Math.round(settings.humOffset),
    });
    if (result && result.settings) applySettingsToUI(result.settings, true);
    settingsSaveStatus = 'saved';
    updateSaveBtn();
    setTimeout(() => { if (settingsSaveStatus === 'saved') { settingsSaveStatus = 'idle'; updateSaveBtn(); } }, 2500);
  } catch (_) {
    settingsSaveStatus = 'error';
    updateSaveBtn();
  }
  settingsSaving = false;
}

async function saveName() {
  const nameInput = document.getElementById('displayNameInput');
  if (!nameInput) return;
  updateNameBtn('saving');
  try {
    const result = await postJson('/api/settings', { display_name: nameInput.value.trim() });
    if (result && result.settings) applySettingsToUI(result.settings, true);
    updateNameBtn('saved');
    nameDirty = false;
    setTimeout(() => { if (!nameDirty) updateNameBtn('idle'); }, 2500);
  } catch (_) {
    updateNameBtn('error');
  }
}

function initSettingsUI() {
  // Night mode toggle
  const nightRow = document.getElementById('nightModeToggle');
  if (nightRow) nightRow.addEventListener('click', () => {
    if (settings.nightModeLocked) return;
    settings.nightMode = !settings.nightMode;
    updateToggleDom('nightModeSw', settings.nightMode);
    enqueueToggle('night_mode', settings.nightMode);
  });

  // Backlight toggle
  const blRow = document.getElementById('backlightToggle');
  if (blRow) blRow.addEventListener('click', () => {
    settings.backlight = !settings.backlight;
    updateToggleDom('backlightSw', settings.backlight);
    enqueueToggle('backlight_on', settings.backlight);
  });

  // Temp unit segment
  const unitSeg = document.getElementById('tempUnitSeg');
  if (unitSeg) unitSeg.querySelectorAll('.seg-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const unit = btn.getAttribute('data-unit');
      if (unit === settings.tempUnit) return;
      settings.tempUnit = unit;
      updateSegmentDom('tempUnitSeg', 'data-unit', unit);
      updateTempOffsetLabel();
      updateTempOffsetDisplay();
      rerenderUnitDependentViews();
      markMeasurementsDirty();
    });
  });

  // Temp offset stepper
  document.getElementById('tempOffsetMinus').addEventListener('click', () => {
    nudgeTempOffset(settings.tempUnit === 'f' ? -0.2 : -0.1);
    updateTempOffsetDisplay();
    markMeasurementsDirty();
  });
  document.getElementById('tempOffsetPlus').addEventListener('click', () => {
    nudgeTempOffset(settings.tempUnit === 'f' ? 0.2 : 0.1);
    updateTempOffsetDisplay();
    markMeasurementsDirty();
  });

  // Hum offset stepper
  document.getElementById('humOffsetMinus').addEventListener('click', () => {
    nudgeHumOffset(-1);
    updateHumOffsetDisplay();
    markMeasurementsDirty();
  });
  document.getElementById('humOffsetPlus').addEventListener('click', () => {
    nudgeHumOffset(1);
    updateHumOffsetDisplay();
    markMeasurementsDirty();
  });

  // Save measurements
  document.getElementById('saveSettingsBtn').addEventListener('click', () => {
    if (settingsSaveStatus === 'dirty') saveMeasurements().catch(() => {});
  });

  // Display name
  const nameInput = document.getElementById('displayNameInput');
  if (nameInput) nameInput.addEventListener('input', () => {
    nameDirty = true;
    updateNameBtn('dirty');
  });
  document.getElementById('saveNameBtn').addEventListener('click', () => {
    if (nameDirty) saveName().catch(() => {});
  });
}

// ─────────────────────────────────────────────
// OTA upload
// ─────────────────────────────────────────────
function initOtaUI() {
  document.getElementById('otaUploadBtn').addEventListener('click', () => {
    if (otaUploadInFlight || otaRestartPending) return;
    const fileInput = document.getElementById('otaFile');
    const file = fileInput.files && fileInput.files[0];
    const statusEl = document.getElementById('otaStatus');
    const progressEl = document.getElementById('otaProgress');
    const uploadBtn = document.getElementById('otaUploadBtn');

    if (!file) {
      statusEl.textContent = 'Select a firmware file first.';
      statusEl.className = 'ota-status';
      return;
    }

    stopOtaRecoveryWatcher();

    otaUploadInFlight = true;
    uploadBtn.disabled = true;
    statusEl.textContent = 'Uploading firmware…';
    statusEl.className = 'ota-status';
    progressEl.style.width = '0%';

    const form = new FormData();
    form.append('ota_size', String(file.size));
    form.append('firmware', file, file.name);

    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/api/ota', true);
    xhr.timeout = computeOtaTimeoutMs(file.size);
    xhr.upload.onprogress = ev => {
      if (!ev.lengthComputable || ev.total <= 0) return;
      const pct = Math.min(100, Math.round((ev.loaded / ev.total) * 100));
      progressEl.style.width = pct + '%';
    };
    xhr.onreadystatechange = () => {
      if (xhr.readyState !== XMLHttpRequest.DONE) return;
      otaUploadInFlight = false;
      let pl = null;
      try { pl = JSON.parse(xhr.responseText || '{}'); } catch (_) {}
      if (xhr.status >= 200 && xhr.status < 300 && pl && pl.success === true) {
        otaRestartPending = true;
        uploadBtn.disabled = true;
        progressEl.style.width = '100%';
        statusEl.textContent = pl.message || 'Firmware uploaded. Device will reboot.';
        statusEl.className = 'ota-status ok';
        fileInput.value = '';
        startOtaRecoveryWatcher();
        return;
      }
      otaRestartPending = false;
      stopOtaRecoveryWatcher();
      uploadBtn.disabled = false;
      statusEl.textContent = (pl && pl.error) || 'Upload failed (HTTP ' + (xhr.status || 0) + ')';
      statusEl.className = 'ota-status err';
    };
    xhr.onerror = () => {
      otaUploadInFlight = false;
      otaRestartPending = false;
      stopOtaRecoveryWatcher();
      uploadBtn.disabled = false;
      statusEl.textContent = 'Upload failed. Check AP connection and retry.';
      statusEl.className = 'ota-status err';
    };
    xhr.ontimeout = () => {
      otaUploadInFlight = false;
      otaRestartPending = false;
      stopOtaRecoveryWatcher();
      uploadBtn.disabled = false;
      statusEl.textContent = 'Upload timed out. Retry closer to AP.';
      statusEl.className = 'ota-status err';
    };
    xhr.send(form);
  });
}

// ─────────────────────────────────────────────
// Data refresh
// ─────────────────────────────────────────────
async function refreshState() {
  if (otaUploadInFlight || otaRestartPending) return;
  const payload = await getJson('/api/state');
  stateCache = payload;

  // Header clock
  if (isNum(payload && payload.time_epoch_s)) {
    deviceClockRef = { epochMs: payload.time_epoch_s * 1000, capturedAtMs: Date.now() };
    updateHeaderClock();
  }

  // Device name
  renderHeaderDeviceName();

  // Sensors tab
  renderHeroMetric(payload.sensors, historyCache);
  renderClimateOverview(payload.sensors, payload.derived);
  renderGasGrid(payload.sensors);
  renderPMGrid(payload.sensors);

  // System tab
  renderSystemMeta(payload);

  // Settings (non-destructive)
  if (payload.settings) applySettingsToUI(payload.settings, false);
}

async function refreshSensorHistory() {
  if (otaUploadInFlight || otaRestartPending) return;
  const payload = await getJson('/api/charts?group=core&window=3h');
  if (!payload || !Array.isArray(payload.timestamps)) return;
  // Extract co2 series into simple row array
  const co2Series = (payload.series || []).find(s => s && s.key === 'co2');
  if (!co2Series) return;
  historyCache = payload.timestamps.map((ts, i) => ({
    co2: Array.isArray(co2Series.values) && isNum(co2Series.values[i]) ? co2Series.values[i] : null,
  }));
  // Re-render hero
  if (stateCache) renderHeroMetric(stateCache.sensors, historyCache);
}

async function refreshCharts() {
  if (otaUploadInFlight || otaRestartPending) return;
  if (chartsRefreshController) {
    chartsRefreshController.abort();
    chartsRefreshController = null;
  }
  const token = ++chartsRefreshToken;
  const controller = new AbortController();
  chartsRefreshController = controller;

  try {
    const payload = await getJson(
      '/api/charts?group=' + encodeURIComponent(chartGroup) + '&window=' + encodeURIComponent(chartRange),
      { signal: controller.signal }
    );
    if (token !== chartsRefreshToken) {
      return;
    }
    renderCharts(payload);
  } catch (error) {
    if (error && error.name === 'AbortError') {
      return;
    }
    throw error;
  } finally {
    if (chartsRefreshController === controller) {
      chartsRefreshController = null;
    }
  }
}

async function refreshEvents() {
  if (otaUploadInFlight || otaRestartPending) return;
  const payload = await getJson('/api/events');
  renderEvents(payload);
}

let historyRefreshTimer = null;

async function refreshActive() {
  if (refreshBusy || otaUploadInFlight || otaRestartPending) return;
  refreshBusy = true;
  try { await refreshState(); } catch (_) {}
  if (activeTab === 'charts')  { try { await refreshCharts(); } catch (_) {} }
  if (activeTab === 'events')  { try { await refreshEvents(); } catch (_) {} }
  if (activeTab === 'sensors') { try { await refreshSensorHistory(); } catch (_) {} }
  refreshBusy = false;
}

// ─────────────────────────────────────────────
// Tab nav
// ─────────────────────────────────────────────
function selectTab(tab) {
  activeTab = tab;
  document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.classList.toggle('active', btn.getAttribute('data-tab') === tab);
  });
  ['sensors','charts','events','settings','system'].forEach(id => {
    const panel = document.getElementById('tab-' + id);
    if (panel) panel.classList.toggle('active', id === tab);
  });
  if (tab === 'charts') refreshCharts().catch(() => {});
  if (tab === 'events') refreshEvents().catch(() => {});
  if (tab === 'sensors') refreshSensorHistory().catch(() => {});
}

// ─────────────────────────────────────────────
// Init
// ─────────────────────────────────────────────
// Tab buttons
document.querySelectorAll('.tab-btn').forEach(btn => {
  btn.addEventListener('click', () => selectTab(btn.getAttribute('data-tab')));
});

// Chart group segment
document.querySelectorAll('#cgroupSeg .seg-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    chartGroup = btn.getAttribute('data-group') || 'core';
    document.querySelectorAll('#cgroupSeg .seg-btn').forEach(b => b.classList.toggle('active', b === btn));
    refreshCharts().catch(() => {});
  });
});

// Chart range segment
document.querySelectorAll('#crangeSeg .seg-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    chartRange = btn.getAttribute('data-range') || '24h';
    document.querySelectorAll('#crangeSeg .seg-btn').forEach(b => b.classList.toggle('active', b === btn));
    refreshCharts().catch(() => {});
  });
});

// System buttons
document.getElementById('rebootBtn').addEventListener('click', async () => {
  try { await postJson('/api/settings', { restart: true }); } catch (_) {}
});
document.getElementById('openDacBtn').addEventListener('click', () => { window.location.href = '/dac'; });

initSettingsUI();
initOtaUI();
updateHeaderClock();
setInterval(updateHeaderClock, 1000);
setInterval(refreshActive, 10000);

// Initial data load
refreshState().catch(() => {});
refreshSensorHistory().catch(() => {});
refreshCharts().catch(() => {});
refreshEvents().catch(() => {});
</script>
</body>
</html>
)HTML_DASH_AP";

} // namespace WebTemplates




