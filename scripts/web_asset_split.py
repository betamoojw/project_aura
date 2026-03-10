import gzip
import hashlib
import io
import re
from dataclasses import dataclass
from pathlib import Path

SPLIT_STYLE_RE = re.compile(r"<style>\n?(.*?)\n?</style>", re.DOTALL)
SPLIT_SCRIPT_RE = re.compile(r"<script>\n?(.*?)\n?</script>", re.DOTALL)
CSS_PATH_PLACEHOLDER = "__AURA_CSS_ASSET_PATH__"
JS_PATH_PLACEHOLDER = "__AURA_JS_ASSET_PATH__"


@dataclass(frozen=True)
class ShellConfig:
    shell_boot_html: str
    shell_critical_css: str
    insert_anchor: str
    body_original: str = "<body>"
    body_replacement: str = '<body class="shell-loading">'
    load_script_timeout_ms: int = 1500
    asset_retry_delay_ms: int = 800


@dataclass(frozen=True)
class SplitSymbols:
    version_symbol: str
    css_path_symbol: str
    js_path_symbol: str
    shell_symbol: str
    css_symbol: str
    js_symbol: str


def extract_template(source_path: Path, start_marker: str, end_marker: str, label: str) -> str:
    text = source_path.read_text(encoding="utf-8")
    start = text.find(start_marker)
    if start < 0:
        raise RuntimeError(f"{label} template start marker not found in {source_path}")
    start += len(start_marker)

    end = text.find(end_marker, start)
    if end < 0:
        raise RuntimeError(f"{label} template end marker not found in {source_path}")

    return text[start:end]


def get_app_version(env) -> str:
    for item in env.get("CPPDEFINES", []):
        if isinstance(item, tuple) and item[0] == "APP_VERSION":
            return str(item[1]).replace('\\"', '"').strip('"')
        if isinstance(item, str) and item.startswith("APP_VERSION="):
            return item.split("=", 1)[1].replace('\\"', '"').strip('"')

    build_flags = env.GetProjectOption("build_flags", [])
    if isinstance(build_flags, str):
        build_flags = [build_flags]
    for item in build_flags:
        value = str(item).strip()
        if value.startswith("-DAPP_VERSION="):
            return value.split("=", 1)[1].replace('\\"', '"').strip('"')
    return "dev"


def make_version_token(version: str, fingerprint_source: str = "") -> str:
    filtered = "".join(ch.lower() for ch in version if ch.isalnum())
    if not filtered:
        filtered = "dev"
    if not fingerprint_source:
        return f"v{filtered}"

    digest = hashlib.sha1(fingerprint_source.encode("utf-8")).hexdigest()[:8]
    return f"v{filtered}-{digest}"


def split_assets(html: str, css_path: str, js_path: str, shell: ShellConfig, label: str) -> tuple[str, str, str]:
    style_match = SPLIT_STYLE_RE.search(html)
    if style_match is None:
        raise RuntimeError(f"{label} template <style> block not found")

    script_matches = list(SPLIT_SCRIPT_RE.finditer(html))
    if not script_matches:
        raise RuntimeError(f"{label} template <script> block not found")
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
  var scriptRequested = false;
  var shellBootSub = document.querySelector('.shell-boot-sub');
  function withRetryToken(url, attempt) {{
    if (!attempt) return url;
    return url + (url.indexOf('?') >= 0 ? '&' : '?') + 'retry=' + attempt;
  }}
  function retryOnce(makeEl, mount, url, onSuccess, onFinalError) {{
    var attempt = 0;
    function start() {{
      var el = makeEl(withRetryToken(url, attempt));
      el.onload = function() {{
        el.onload = null;
        el.onerror = null;
        if (onSuccess) onSuccess();
      }};
      el.onerror = function() {{
        el.onload = null;
        el.onerror = null;
        if (el.parentNode) {{
          el.parentNode.removeChild(el);
        }}
        if (attempt < 1) {{
          attempt++;
          window.setTimeout(start, {shell.asset_retry_delay_ms});
          return;
        }}
        if (onFinalError) onFinalError();
      }};
      mount.appendChild(el);
    }}
    start();
  }}
  function loadScript() {{
    if (scriptRequested) return;
    scriptRequested = true;
    retryOnce(function(src) {{
      var script = document.createElement('script');
      script.src = src;
      script.defer = true;
      script.async = false;
      return script;
    }}, document.body, jsHref, null, function() {{
      if (shellBootSub) {{
        shellBootSub.textContent = 'Load failed. Reload to retry.';
      }}
    }});
  }}
  function loadAssets() {{
    retryOnce(function(href) {{
      var link = document.createElement('link');
      link.rel = 'stylesheet';
      link.href = href;
      return link;
    }}, document.head, cssHref, loadScript, loadScript);
    window.setTimeout(loadScript, {shell.load_script_timeout_ms});
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

    shell_html = (
        html[: style_match.start()]
        + "<style>\n"
        + shell.shell_critical_css.strip()
        + "\n</style>"
        + html[style_match.end() : script_match.start()]
        + "<script>\n"
        + bootstrap
        + "\n</script>"
        + html[script_match.end() :]
    )

    if shell.body_original not in shell_html:
        raise RuntimeError(f"{label} template body marker not found")
    shell_html = shell_html.replace(shell.body_original, shell.body_replacement, 1)

    if shell.insert_anchor not in shell_html:
        raise RuntimeError(f"{label} template insert anchor not found")
    shell_html = shell_html.replace(
        shell.insert_anchor,
        shell.shell_boot_html.strip() + "\n" + shell.insert_anchor,
        1,
    )

    return shell_html, css, js


def apply_asset_paths(shell_html: str, css_path: str, js_path: str) -> str:
    if CSS_PATH_PLACEHOLDER not in shell_html:
        raise RuntimeError("CSS asset placeholder not found in shell HTML")
    if JS_PATH_PLACEHOLDER not in shell_html:
        raise RuntimeError("JS asset placeholder not found in shell HTML")
    return shell_html.replace(CSS_PATH_PLACEHOLDER, css_path).replace(JS_PATH_PLACEHOLDER, js_path)


def gzip_bytes(data: bytes) -> bytes:
    buffer = io.BytesIO()
    with gzip.GzipFile(filename="", mode="wb", fileobj=buffer, compresslevel=9, mtime=0) as gz:
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


def render_split_inc(
    generated_by: str,
    symbols: SplitSymbols,
    version_token: str,
    css_path: str,
    js_path: str,
    shell_payload: bytes,
    css_payload: bytes,
    js_payload: bytes,
) -> str:
    lines = [
        f"// Auto-generated by {generated_by}. Do not edit manually.",
        "",
        f'const char {symbols.version_symbol}[] = "{version_token}";',
        f'const char {symbols.css_path_symbol}[] = "{css_path}";',
        f'const char {symbols.js_path_symbol}[] = "{js_path}";',
        "",
    ]
    render_bytes(lines, symbols.shell_symbol, shell_payload)
    render_bytes(lines, symbols.css_symbol, css_payload)
    render_bytes(lines, symbols.js_symbol, js_payload)
    return "\n".join(lines)


def write_if_changed(path: Path, content: str) -> bool:
    if path.exists():
        existing = path.read_text(encoding="utf-8")
        if existing == content:
            return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    return True
