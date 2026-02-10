$ErrorActionPreference = 'Stop'

function Get-FontCoverage([string]$fontPath) {
    $raw = Get-Content $fontPath -Raw -Encoding UTF8

    $coverage = [System.Collections.Generic.HashSet[int]]::new()

    $cmapMatches = [regex]::Matches($raw, '\.range_start\s*=\s*(\d+),\s*\.range_length\s*=\s*(\d+),\s*\.glyph_id_start\s*=\s*\d+,\s*\.unicode_list\s*=\s*(NULL|unicode_list_\d+),[^\n]*\.list_length\s*=\s*(\d+),', [System.Text.RegularExpressions.RegexOptions]::Singleline)

    $unicodeLists = @{}
    $listMatches = [regex]::Matches($raw, 'static const uint16_t (unicode_list_\d+)\[\]\s*=\s*\{([^}]*)\};', [System.Text.RegularExpressions.RegexOptions]::Singleline)
    foreach ($m in $listMatches) {
        $name = $m.Groups[1].Value
        $body = $m.Groups[2].Value
        $vals = @()
        foreach ($num in [regex]::Matches($body, '0x[0-9a-fA-F]+|\d+')) {
            $s = $num.Value
            if ($s.StartsWith('0x')) { $vals += [Convert]::ToInt32($s.Substring(2), 16) }
            else { $vals += [int]$s }
        }
        $unicodeLists[$name] = $vals
    }

    foreach ($m in $cmapMatches) {
        $start = [int]$m.Groups[1].Value
        $len = [int]$m.Groups[2].Value
        $listName = $m.Groups[3].Value
        if ($listName -eq 'NULL') {
            for ($cp = $start; $cp -lt ($start + $len); $cp++) { [void]$coverage.Add($cp) }
        } else {
            if (-not $unicodeLists.ContainsKey($listName)) { continue }
            foreach ($ofs in $unicodeLists[$listName]) { [void]$coverage.Add($start + $ofs) }
        }
    }

    return $coverage
}

$keys = Get-Content 'src/ui/strings/UiStrings.keys.inc' -Encoding UTF8 | ForEach-Object {
    if ($_ -match 'UI_STR_ID\(([^\)]+)\)') { $matches[1] }
}
$vals = Get-Content 'src/ui/strings/UiStrings.zh.inc' -Encoding UTF8 | ForEach-Object {
    $line = $_.Trim()
    if ($line -match '^"(.*)",$') {
        $s = $matches[1]
        $s = $s -replace '\\n', "`n"
        $s
    }
}

if ($keys.Count -ne $vals.Count) {
    throw "keys/values mismatch: $($keys.Count)/$($vals.Count)"
}

$map = @{}
for ($i=0; $i -lt $keys.Count; $i++) { $map[$keys[$i]] = $vals[$i] }

$mrKeys = @('InfoMrText','InfoMrExcellent','InfoMrAcceptable','InfoMrUncomfortable','InfoMrPoor','SensorInfoTitleMr')
$mrText = ($mrKeys | ForEach-Object { $map[$_] }) -join "`n"

$chars = [System.Collections.Generic.HashSet[string]]::new()
foreach ($ch in $mrText.ToCharArray()) {
    if ([char]::IsWhiteSpace($ch)) { continue }
    [void]$chars.Add([string]$ch)
}

$cover18 = Get-FontCoverage 'src/ui/ui_font_noto_sans_sc_reg_18.c'
$cover14 = Get-FontCoverage 'src/ui/ui_font_noto_sans_sc_reg_14.c'

$missing18 = @()
$missing14 = @()
foreach ($ch in $chars) {
    $cp = [int][char]$ch
    if (-not $cover18.Contains($cp)) { $missing18 += [pscustomobject]@{ char=$ch; code=('U+{0:X4}' -f $cp) } }
    if (-not $cover14.Contains($cp)) { $missing14 += [pscustomobject]@{ char=$ch; code=('U+{0:X4}' -f $cp) } }
}

$missing18 = $missing18 | Sort-Object code -Unique
$missing14 = $missing14 | Sort-Object code -Unique

"MR keys: $($mrKeys -join ', ')"
"Unique chars in MR texts: $($chars.Count)"
"Missing in 18: $($missing18.Count)"
$missing18 | ForEach-Object { "  $($_.char) $($_.code)" }
"Missing in 14: $($missing14.Count)"
$missing14 | ForEach-Object { "  $($_.char) $($_.code)" }
