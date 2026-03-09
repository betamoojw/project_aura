import gzip
import io
import re
from pathlib import Path

Import("env")

PROJECT_DIR = Path(env.subst("$PROJECT_DIR"))
SOURCE_CPP = PROJECT_DIR / "src" / "web" / "WebTemplatesDashboardAp.cpp"
OUT_INC = PROJECT_DIR / "src" / "web" / "generated" / "WebTemplatesDashboardApGzip.inc"

START_MARKER = 'const char kDashboardPageTemplateAp[] PROGMEM = R"HTML_DASH_AP(\n'
END_MARKER = ')HTML_DASH_AP";'

SHELL_BOOT_HTML = """
<div id="shellBoot" class="shell-boot" aria-live="polite">
  <div class="shell-boot-head">
    <div>
      <div class="shell-boot-title">AURA Dashboard</div>
      <div class="shell-boot-sub">Loading live device data...</div>
    </div>
    <div class="shell-boot-chip">Skeleton</div>
  </div>
  <div class="shell-boot-grid">
    <div class="shell-boot-card shell-boot-card-hero"></div>
    <div class="shell-boot-card"></div>
    <div class="shell-boot-card"></div>
    <div class="shell-boot-card"></div>
  </div>
</div>
""".strip()

SHELL_CRITICAL_CSS = """
:root{color-scheme:dark}
*{box-sizing:border-box}
html,body{margin:0;min-height:100%;background:#111827;color:#f9fafb;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif}
body{padding:16px}
body.shell-loading .wrap{display:none}
body.shell-loading #otaGlobalOverlay{display:none}
.shell-boot{display:none;max-width:1152px;margin:0 auto}
body.shell-loading .shell-boot{display:block}
.shell-boot-head{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:18px;flex-wrap:wrap}
.shell-boot-title{font-size:24px;font-weight:800;letter-spacing:.02em}
.shell-boot-sub{margin-top:4px;color:#9ca3af;font-size:13px}
.shell-boot-chip{padding:8px 12px;border-radius:999px;border:1px solid rgba(61,214,198,.35);background:rgba(17,24,39,.82);color:#9ae6dc;font-size:12px;font-weight:700;text-transform:uppercase;letter-spacing:.08em}
.shell-boot-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:14px}
.shell-boot-card{height:132px;border-radius:18px;border:1px solid rgba(255,255,255,.08);background:linear-gradient(110deg,rgba(31,41,55,.95) 8%,rgba(55,65,81,.82) 18%,rgba(31,41,55,.95) 33%);background-size:200% 100%;animation:shell-shimmer 1.2s linear infinite}
.shell-boot-card-hero{min-height:196px;grid-column:span 2}
@keyframes shell-shimmer{to{background-position-x:-200%}}
@media (max-width:720px){body{padding:14px}.shell-boot-title{font-size:21px}.shell-boot-card-hero{grid-column:span 1;min-height:164px}}
""".strip()

SPLIT_STYLE_RE = re.compile(r"<style>\n?(.*?)\n?</style>", re.DOTALL)
SPLIT_SCRIPT_RE = re.compile(r"<script>\n?(.*?)\n?</script>", re.DOTALL)


def extract_dashboard_html(source_path: Path) -> str:
    text = source_path.read_text(encoding="utf-8")
    start = text.find(START_MARKER)
    if start < 0:
        raise RuntimeError(
            f"Dashboard template start marker not found in {source_path}"
        )
    start += len(START_MARKER)

    end = text.find(END_MARKER, start)
    if end < 0:
        raise RuntimeError(
            f"Dashboard template end marker not found in {source_path}"
        )

    return text[start:end]


def get_app_version() -> str:
    for item in env.get("CPPDEFINES", []):
        if isinstance(item, tuple) and item[0] == "APP_VERSION":
            return str(item[1]).strip('"')
    return "dev"


def make_version_token(version: str) -> str:
    filtered = "".join(ch.lower() for ch in version if ch.isalnum())
    if not filtered:
        filtered = "dev"
    return f"v{filtered}"


def split_dashboard_assets(html: str, css_path: str, js_path: str) -> tuple[str, str, str]:
    style_match = SPLIT_STYLE_RE.search(html)
    if style_match is None:
        raise RuntimeError("Dashboard template <style> block not found")

    script_matches = list(SPLIT_SCRIPT_RE.finditer(html))
    if not script_matches:
        raise RuntimeError("Dashboard template <script> block not found")
    script_match = script_matches[-1]

    css = style_match.group(1).strip() + "\n"
    js = (
        "document.documentElement.classList.remove('shell-loading');\n"
        "if (document.body) document.body.classList.remove('shell-loading');\n"
        "var shellBoot = document.getElementById('shellBoot');\n"
        "if (shellBoot && shellBoot.parentNode) shellBoot.parentNode.removeChild(shellBoot);\n\n"
        + script_match.group(1).strip()
        + "\n"
    )

    bootstrap = f"""
(function() {{
  var cssHref = "{css_path}";
  var jsHref = "{js_path}";
  var scriptLoaded = false;
  function loadScript() {{
    if (scriptLoaded) return;
    scriptLoaded = true;
    var script = document.createElement('script');
    script.src = jsHref;
    script.defer = true;
    script.async = false;
    document.body.appendChild(script);
  }}
  function loadAssets() {{
    var link = document.createElement('link');
    link.rel = 'stylesheet';
    link.href = cssHref;
    link.onload = loadScript;
    link.onerror = loadScript;
    document.head.appendChild(link);
    window.setTimeout(loadScript, 1500);
  }}
  if (window.requestAnimationFrame) {{
    window.requestAnimationFrame(function() {{
      window.setTimeout(loadAssets, 0);
    }});
  }} else {{
    window.setTimeout(loadAssets, 0);
  }}
}})();
""".strip()

    shell = (
        html[: style_match.start()]
        + "<style>\n"
        + SHELL_CRITICAL_CSS
        + "\n</style>"
        + html[style_match.end() : script_match.start()]
        + "<script>\n"
        + bootstrap
        + "\n</script>"
        + html[script_match.end() :]
    )
    shell = shell.replace("<body>", '<body class="shell-loading">', 1)
    shell = shell.replace('<div class="wrap">', SHELL_BOOT_HTML + "\n<div class=\"wrap\">", 1)
    return shell, css, js


def gzip_bytes(data: bytes) -> bytes:
    buffer = io.BytesIO()
    with gzip.GzipFile(
        filename="", mode="wb", fileobj=buffer, compresslevel=9, mtime=0
    ) as gz:
        gz.write(data)
    return buffer.getvalue()


def render_bytes(lines: list[str], symbol: str, payload: bytes) -> None:
    lines.append(f"const uint8_t {symbol}[] PROGMEM = {{")
    bytes_per_line = 12
    for index in range(0, len(payload), bytes_per_line):
        chunk = payload[index : index + bytes_per_line]
        lines.append("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    lines.append("};")
    lines.append(f"const size_t {symbol}Size = {len(payload)};")
    lines.append("")


def render_inc(version_token: str, shell_payload: bytes, css_payload: bytes, js_payload: bytes) -> str:
    lines = [
        "// Auto-generated by scripts/generate_dashboard_gzip.py. Do not edit manually.",
        "",
        f'const char kDashboardAssetVersion[] = "{version_token}";',
        f'const char kDashboardStylesCssPath[] = "/assets/dashboard/styles.{version_token}.css";',
        f'const char kDashboardAppJsPath[] = "/assets/dashboard/app.{version_token}.js";',
        "",
    ]
    render_bytes(lines, "kDashboardShellHtmlGzip", shell_payload)
    render_bytes(lines, "kDashboardStylesCssGzip", css_payload)
    render_bytes(lines, "kDashboardAppJsGzip", js_payload)
    return "\n".join(lines)


def write_if_changed(path: Path, content: str) -> bool:
    if path.exists():
        existing = path.read_text(encoding="utf-8")
        if existing == content:
            return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    return True


def main() -> None:
    html = extract_dashboard_html(SOURCE_CPP)
    app_version = get_app_version()
    version_token = make_version_token(app_version)
    css_path = f"/assets/dashboard/styles.{version_token}.css"
    js_path = f"/assets/dashboard/app.{version_token}.js"
    shell_html, css_text, js_text = split_dashboard_assets(html, css_path, js_path)

    shell_bytes = shell_html.encode("utf-8")
    css_bytes = css_text.encode("utf-8")
    js_bytes = js_text.encode("utf-8")

    shell_payload = gzip_bytes(shell_bytes)
    css_payload = gzip_bytes(css_bytes)
    js_payload = gzip_bytes(js_bytes)
    inc_content = render_inc(version_token, shell_payload, css_payload, js_payload)
    changed = write_if_changed(OUT_INC, inc_content)

    status = "updated" if changed else "up-to-date"
    print(
        "[dashboard-gzip] "
        f"{status}: shell={len(shell_bytes)}->{len(shell_payload)} bytes, "
        f"css={len(css_bytes)}->{len(css_payload)} bytes, "
        f"js={len(js_bytes)}->{len(js_payload)} bytes"
    )


main()
