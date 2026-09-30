# `feature/reader-enhancements`

A branch of [ahrm/sioyek](https://github.com/ahrm/sioyek) that turns Sioyek's
freetext bookmarks into a general annotation tool: persistent rectangles,
note boxes you can edit in place, curved arrows, and LaTeX typeset inside the
notes.

Everything here is additive. Nothing changes how stock Sioyek behaves until you
use one of the new commands, and documents annotated with this build stay
readable by an unpatched Sioyek.

## What it adds

**Annotations**

- Persistent vector rectangle annotations, drawn and deleted with the same
  shortcut family as highlights.
- One unified note/rectangle object rather than two parallel representations.
- Note boxes that can be edited in place, moved, and resized with the mouse.
- Per-note text colour, background colour, and border width, adjustable from
  the keyboard.
- Curved arrows attached to a note, whose tail snaps to any of the note's eight
  handles.
- Note colours taken from the existing `a`–`z` highlight palette.
- Cut, copy, and paste for notes. Paste creates a new note at the pointer.

**LaTeX in notes**

- `$...$` for inline math and `$$...$$` for display math, rendered with
  [JKQTMathText](https://github.com/jkriege2/JKQtPlotter).
- `\mathcal` rewritten to the Unicode script letters so the math font draws its
  real script glyphs, including the dozen letters that live in Letterlike
  Symbols rather than Mathematical Alphanumeric Symbols.
- `\boldsymbol` and `\bm` rewritten to `\mathbfit`, which JKQTMathText does
  implement.
- Math variables pointed at a text family with a real italic cut, naming the
  10pt Latin Modern optical master specifically (see *Fonts* below).

**Interface**

- A searchable list of the key bindings actually in effect.
- Exact-match-first command palette.
- Config files open in a real text editor via a `text_editor_command` preference.
- Context-sensitive shortcuts, with the active mode shown in the status bar.

## Building

### Dependencies

Stock Sioyek's dependencies, plus — for the LaTeX support only —
[JKQtPlotter](https://github.com/jkriege2/JKQtPlotter)'s JKQTMathText library
(`JKQTMathText6_Release` and `JKQTCommon6_Release`). The rest of the branch
builds without it.

### The three qmake flags

**The `.pro` file cannot enable LaTeX on its own.** It adds the JKQT libraries
only inside `contains(DEFINES, SIOYEK_JKQT_MATHTEXT_SUPPORT)` — that is, only if
the define *already* exists — and it adds no include path for
`<jkqtmathtext/jkqtmathtext.h>`. Pass all three yourself:

```sh
qmake \
  DEFINES+=SIOYEK_JKQT_MATHTEXT_SUPPORT \
  INCLUDEPATH+=/path/to/jkqtplotter/include \
  "LIBS+=-L/path/to/jkqtplotter/lib -lJKQTMathText6_Release -lJKQTCommon6_Release" \
  pdf_viewer_build_config.pro
make -j"$(nproc)"
```

Build without them and `$...$` renders as literal text, with no error and no
warning.

**`make` does not track compiler-flag changes.** After adding or changing
`DEFINES` you must `make clean` first, or stale object files link with no JKQT
and the feature is silently absent. Always confirm before believing it works:

```sh
ldd build/sioyek | grep -i jkqt
```

Note that `mupdf`'s `make libs` does not build `libmupdf-threads.a`, which
Sioyek links; run `make libmupdf-threads` as well.

### Fonts

The note renderer looks for these families, in order, and falls back gracefully:

| Role | Preferred | Fallbacks |
| --- | --- | --- |
| Roman and math | `Latin Modern Math` | `XITS Math`, `STIX Two Math` |
| Math variables | `LM Roman 10` | `Latin Modern Roman`, `STIX Two Text`, `XITS` |
| Calligraphic | `TeX Gyre Chorus` | `URW Chancery L`, `Z003`, … |

On most distributions `Latin Modern Math` and `LM Roman 10` both come from the
`lmodern` package (Debian/Ubuntu `fonts-lmodern`, Fedora `texlive-lm`, Arch
`otf-latin-modern`, Nix `pkgs.lmodern`).

**Name the optical master, not the family.** Latin Modern ships one master per
design size and Qt merges all of them into a single family called
`Latin Modern Roman`; asking that family for its Regular style can hand back the
5pt or 6pt cut. Those are drawn wider and set much looser on purpose — that is
what small optical sizes are for — which tracks operator names like `\cos` and
ordinary words about a third too wide. `LM Roman 10` addresses the 10pt master
directly and measures M/x-height 2.13, the same as `Latin Modern Math`.

### Optional: TeX-fidelity math

JKQTMathText's own metrics differ from TeX's in four places that are obvious
when a formula is set beside the same one from a TeX renderer. If you want the
math to match TeX, patch JKQtPlotter with
[`jkqtplotter-tex-metrics.patch`](https://github.com/dragonleopardpig/Projects/blob/main/NixOS/patches/jkqtplotter-tex-metrics.patch):

- **Rule thickness.** Every rule — fraction and matrix rules, radicals,
  decorations like `\vec`, boxes, and `\left..\right` delimiters — is drawn with
  a pen whose width is the font's underline thickness. A text family reports
  about 2.8% of the font size there, under TeX's default rule thickness of
  0.04em, so strokes come out light.
- **Curly braces.** A `\left\{` takes its height from its contents but its width
  from a font metric, growing only with the square root of the oversize factor,
  and its stem from the text rule thickness. Neither grows with the brace, so
  one around a two-line block is a hairline a third of TeX's width.
- **Operator spacing.** A math operator is padded by a factor of its own glyph,
  which leaves a wide relation like `\leq` or a multi-letter name like `\cos`
  with almost no room. TeX inserts a fixed skip that does not depend on the
  glyph: 3mu beside an operator, 5mu beside a relation.
- **`aligned` / `align`.** Parsed as a plain matrix with an empty column spec,
  so every column is left aligned and each `&` gets the generic matrix column
  separation. amsmath numbers the columns in pairs, right then left — that is
  what stacks the relation signs of successive lines — and the `&` inside a pair
  is an alignment point that adds no space.

The patch applies to JKQtPlotter at `5.0.0-unstable-2025-12-26` with zero fuzz.
Always dry-run it; `patch` at its default fuzz will silently apply hunks by
guessing their location:

```sh
patch -p1 -F0 --dry-run < jkqtplotter-tex-metrics.patch
```

## NixOS

The flake that builds this branch lives in
[dragonleopardpig/Projects](https://github.com/dragonleopardpig/Projects) under
`NixOS/`. It carries the branch as one generated patch over the nixpkgs Sioyek
snapshot, injects the three qmake flags, and patches JKQtPlotter.

### Building for another host

Every host in the flake exposes the package, so you can build any of them from
any machine of the same architecture:

```sh
cd ~/Projects/NixOS
nix build ".#nixosConfigurations.<HOST>.pkgs.sioyek" --print-out-paths
```

`<HOST>` is one of `X299`, `X299-SSD`, `M90aPro`, `PortableSSD`; `nix flake show`
lists them. Derive the current machine's own name from `/run/current-system`
rather than guessing — it is the `nixos-system-<HOST>-<version>` component of
the store path:

```sh
readlink -f /run/current-system
```

To activate on the current host:

```sh
sudo nixos-rebuild switch --flake ~/Projects/NixOS#<HOST>
```

There is no `/etc/nixos` here, so a bare `nixos-rebuild switch` fails with
`file 'nixos-config' was not found in the Nix search path`. Only the plain
`X299` host needs `--impure`. Never run the switch in the background — it
hijacks the default boot entry.

To build on this machine and activate on another over SSH:

```sh
nixos-rebuild switch --flake ~/Projects/NixOS#<HOST> \
  --build-host localhost --target-host root@<address>
```

### Regenerating the Sioyek patch

**Generate it against the materialised tree, never from a branch diff.** The
nixpkgs Sioyek snapshot is *older upstream* than this branch's base: it has no
`db_mutex`, and it predates upstream `3719c182`, which normalises the freetext
rectangle differently. A `git diff` of the branch therefore does not apply, and
branch code touching newer upstream will not compile.

```sh
SRC=$(nix build ".#nixosConfigurations.<HOST>.pkgs.sioyek.src" --no-link --print-out-paths)
cp -r "$SRC" base && chmod -R u+w base
cd base
patch -p1 -F0 < ../patches/sioyek-dual-page.patch
patch -p1     < ../patches/sioyek-native-djvu.patch
cd ..
cp -r base tree            # edit `tree`, or apply your branch commits to it
diff -ruN base tree > ../patches/sioyek-reader-enhancements.patch
```

Then rewrite the headers to `--- a/<path>` / `+++ b/<path>` and dry-run at zero
fuzz before committing. Nix flakes ignore untracked files, so `git add` the
patch before building or the old one ships.

## Testing without touching your annotations

Sioyek's `.pro` puts `DEFINES += LINUX_STANDARD_PATHS` in the branch that is
*not* `linux_app_image`, and that code path uses `$HOME/.local/share/sioyek` and
ignores `XDG_DATA_HOME` entirely. The nixpkgs build is not a `linux_app_image`
build, so **override `HOME`** to isolate a test run — setting `XDG_DATA_HOME`
alone silently opens your real annotation database.

Pass `--instance-name` **space separated**. `get_argv_value` matches
`key == argv[i]` and then takes `argv[i+1]`, so the `--instance-name=value` form
is ignored; RunGuard then falls back to the default key and forwards your
command into the already-running Sioyek.

`--execute-command` only acts on an already-running instance. At startup
`handle_args` runs it before the document is open, so it is dropped. To script a
command, start a server instance and then invoke Sioyek a second time with the
same `--instance-name`.

## Diagnostics

Set `SIOYEK_UI_TRACE` to a file path to log UI lifecycle events — useful for
catching a transient popup that appears and closes too quickly to see. Nothing
is written, and no file is created, unless the variable is set:

```sh
SIOYEK_UI_TRACE=/tmp/sioyek-ui.log sioyek
```

It also records note LaTeX that JKQTMathText only partly understood. This
matters more than it sounds: `parse()` returns **true** for input it did not
fully handle — an unknown instruction is dropped, and a construct given the
wrong number of arguments collapses together with its contents — and the
complaint goes only to `getErrorList()`. A note can therefore lose a whole
term with nothing on screen to say so. The trace line carries the source, the
rewritten form, and the error:

```
15:46:48.960  noteMathParseError  |  \frobnicate{x}  ->  \frobnicate{x}  ||  error @ ch. 11: unknown instruction \frobnicate
```

If a formula comes out wrong, look here first.

### LaTeX spellings JKQTMathText does not accept

These are rewritten before parsing, so you can write ordinary LaTeX. Listed
because the failure mode is silent, and anything not on this list that
JKQTMathText does not know will fail the same way:

| You write | JKQTMathText wants | Without the rewrite |
| --- | --- | --- |
| `\mathcal{E}` | a script glyph, not a font switch | upright roman `E` |
| `\boldsymbol{E}`, `\bm{E}` | `\mathbfit{E}` | bold silently dropped |
| `\overbrace{x}^{n}` | `\overbrace{x}{n}` | **whole construct collapses** |
| `\underbrace{x}_{m}` | `\underbrace{x}{m}` | **whole construct collapses** |
| `\overbracket`, `\underbracket` | same two-group form | **whole construct collapses** |
| `\overbrace{x}` (no label) | `\overbrace{x}{}` | **whole construct collapses** |

`\overset`, `\underset` and `\stackrel` already agree with LaTeX and are left
alone.

## Upstreaming

The branch is kept rebasable on `ahrm/sioyek` `main`. Changes are grouped one
concern per commit so individual features can be proposed separately.
