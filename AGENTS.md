# AGENTS.md

## Project goal

This fork replaces Sway's manual i3-style tiling with a built-in layout that
matches Hyprland's dwindle layout. Dwindle behavior is the default and only
tiling behavior. Preserve floating, fullscreen, sticky, scratchpad, and
multi-output behavior.

Behavioral compatibility includes at least:

- extending the existing spawn spiral independently of later focus changes;
- alternating split orientation from the geometry of the split being divided;
- preserving the binary split tree when windows are closed or removed;
- moving tiled windows within a workspace;
- moving tiled windows between workspaces and outputs;
- pointer-driven tiled-window moves;
- directional focus and movement;
- resizing adjacent tiled windows;
- fullscreen and floating transitions.

Treat Hyprland's current `dwindle` implementation and documentation as the
behavioral reference. When Sway and Hyprland semantics conflict, retain Sway's
public IPC/config compatibility where practical, but make the resulting window
placement match Hyprland.

## Current local implementation

The worktree is a source-only fork of upstream Sway. The dwindle changes are
currently local worktree changes, not a separate library or plugin.

### Tree model and insertion

- `struct sway_container` has `is_dwindle` and `dwindle_split_ratio`.
- A dwindle internal node owns exactly two children. Its ratio uses Hyprland's
  `0.1..1.9` scale, where `1.0` is an even split.
- `workspace_add_tiling_at()` is the canonical tiled insertion helper. It
  detaches the moving container, normalizes the old branch, chooses a target
  leaf, creates a binary parent, sets insertion order, and preserves focus.
- `workspace_get_dwindle_tail()` returns the terminal leaf. Newly mapped views
  split this leaf with the new window on the second side, so changing focus
  does not restart the spawn spiral.
- Pointer-based insertion may choose the closest leaf. Directional insertion
  can force the new window before or after its target.
- `container_reap_empty()` collapses redundant one-child dwindle parents after
  removal. Do not leave unary `is_dwindle` nodes in the live tree.

### Arrangement and resizing

- `prepare_dwindle_split()` in `sway/tree/arrange.c` derives horizontal versus
  vertical orientation from the split node's current geometry before arranging
  its children.
- Resize fractions are converted to and retained in
  `dwindle_split_ratio`. Missing or zero child fractions retain the parent's
  existing ratio instead of creating a near-`95/5` split.
- Normal Sway inner and outer gaps still apply. Gap defaults in the config only
  affect newly created workspaces; use the runtime `gaps ... all set ...`
  command for workspaces that already exist.

### Floating and fullscreen transitions

- Tiled-to-floating uses Sway's normal floating sizing and centering.
- Floating-to-tiled clears stale floating fractions *before*
  `workspace_add_tiling_at()`. The insertion helper then owns the new pair's
  fractions. Never clear them after insertion: doing so makes the returned
  window approximately 5% of the split on the next arrange pass.
- Scratchpad removal, CSD/SSD transitions, floating state, and fullscreen state
  must remain intact through reparenting.
- `view_center_and_clip_surface()` only centers floating views. Fullscreen views
  must not be centered from stale XDG geometry; doing so caused fullscreen
  video surfaces to render tiled and off-center.

### Dynamic floating-window sizing

- `dynamic_resize <criteria> <width> <height>` is a custom config-time window
  rule implemented by this fork.
- Rules are re-evaluated on title changes and when a view enters floating mode.
  Only floating, visible, non-fullscreen views are resized.
- The first matching rule wins. On the first dynamic match, the container saves
  its prior floating size. If no rule later matches, that saved size is restored.
  Direct transitions between matching rules retain the same saved base size.
- Every applied or restored size is clamped to Sway's floating constraints and
  centered on the current workspace.
- Rule state is owned by `struct sway_container`; criteria dimensions are owned
  by `struct criteria`. Pure state transitions live in
  `include/sway/tree/dynamic_resize.h`.
- The active config mirrors the old Hyprland Lua sizing scripts:
  - Ghostty `btop`: `810x700`
  - Ghostty `nv` or `vim`: `1220x730`
  - other Ghostty titles: `700x460`
  - Helium: `1200x800`
  - Nautilus: `975x615`
  - Discord: `1200x900`

### Moves and command compatibility

- Command-driven moves and pointer-driven tiled drops both route through
  `workspace_add_tiling_at()`.
- Directional moves reinsert relative to the requested edge and can cross to
  the active workspace on an adjacent output.
- A tiled view touching a workspace edge must use the full output boundary for
  its directional focal point. Do not probe only one pixel beyond the gapped
  container geometry: outer gaps would keep that point on the source output
  and prevent Hyprland-style cross-monitor movement.
- Interior directional focal points must cross the complete inner gap before
  choosing a target leaf. Sampling inside the gap can select an expanding
  sibling after detachment and recreate the original split instead of moving.
- `layout`, `split`, `splith`, `splitv`, `unsplit`, and `workspace_layout` are
  accepted for Sway/i3 config and IPC compatibility but intentionally do not
  replace dwindle.
- IPC move and floating events must still be emitted after successful changes.

### Tests

- Pure dwindle geometry helpers live in `include/sway/tree/dwindle.h`.
- `tests/dwindle.c` covers orientation, insertion order, forced sides, and
  split-ratio fallback behavior.
- `tests/dynamic_resize.c` covers eligibility, first-match state capture,
  transitions between special sizes, and restoration.
- `tests/meson.build` registers them as `dwindle` and `dynamic-resize`.

## Implementation guidelines

- Keep changes focused on the tree/layout model; do not rewrite unrelated
  compositor subsystems.
- Prefer small helpers with explicit ownership rules over duplicating tree
  mutation logic across commands and input handlers.
- Maintain valid parent/workspace pointers and normalize redundant one-child
  containers after every mutation.
- Do not silently discard existing floating or fullscreen state.
- Add focused tests for pure tree and geometry behavior whenever the relevant
  code can be exercised without starting a compositor.
- Follow the surrounding C style: tabs for indentation, braces on the same
  line, and existing naming/logging conventions.

## Validation

- Configure with Meson in `build/`.
- Build with `ninja -C build`.
- Run the focused test with
  `meson test -C build dwindle --print-errorlogs`.
- Run the dynamic sizing test with
  `meson test -C build dynamic-resize --print-errorlogs`.
- Run the full relevant suite with
  `meson test -C build --print-errorlogs`.
- Run `git diff --check`.
- Never start Sway as part of automated validation. The user will launch and
  test the resulting binary from their current Hyprland session.

## Local runtime and deployment

- Built binaries are:
  - `build/sway/sway`
  - `build/swaymsg/swaymsg`
  - `build/swaybar/swaybar`
- The user runs this worktree through symlinks:
  - `~/.local/bin/sway` -> `build/sway/sway`
  - `~/.local/bin/swaymsg` -> `build/swaymsg/swaymsg`
  - `~/.local/bin/swaybar` -> `build/swaybar/swaybar`
- There is no need to install the fork as an AUR or system package. Rebuilding
  updates the binaries reached by those symlinks.
- The active user config is `~/.config/sway/config`. It is outside the
  repository and must not be overwritten unless the user explicitly asks for
  a config change.
- The current bar/window theme intentionally uses only `#141318` and
  `#cbbeff`, with `Maple Mono NF ExtraBold 11` as the global Sway font.
- Use `swaymsg reload` only when a config change needs to be applied. A source
  rebuild requires the user to exit and launch Sway again; never restart their
  session automatically.
- Do not reload a config containing newly added custom commands into an older
  running Sway process. Restart into the rebuilt binary first.

## Repository layout

- Public tree/layout interfaces: `include/sway/tree/`
- Pure dwindle helpers: `include/sway/tree/dwindle.h`
- Dynamic resize state helper: `include/sway/tree/dynamic_resize.h`
- Tree mutation and arrangement: `sway/tree/`
- Interactive tiled moves: `sway/input/seatop_move_tiling.c`
- Command-driven moves/resizes/layout: `sway/commands/`
- View mapping and tiled insertion: `sway/tree/view.c`
- Protocol-specific view lifecycle: `sway/desktop/`
- IPC tree serialization: `sway/ipc-json.c`
