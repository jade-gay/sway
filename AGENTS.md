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

This is a source-only fork of upstream Sway at
`https://github.com/jade-gay/sway.git`. The dwindle and dynamic-resize
implementation is committed on `master` (introduced by `e9c2b703`); it is not
a separate library or plugin. `origin` is the user's fork and `upstream` points
to `https://github.com/swaywm/sway.git`. The fork's default branch and
`origin/HEAD` are both `master`; it does not have a `main` branch.

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
- Rules are re-evaluated by `view_update_dynamic_resize()` after title changes
  and floating-state transitions. Only floating views attached to a workspace
  that are not hidden in the scratchpad or fullscreen are eligible.
- The first matching rule wins. On the first dynamic match, the container saves
  its prior floating size. If no rule later matches, that saved size is restored.
  Direct transitions between matching rules retain the same saved base size.
- Every applied or restored size is clamped to Sway's floating constraints and
  centered on the current workspace.
- Rule state is owned by `struct sway_container`; criteria dimensions are owned
  by `struct criteria`. Pure state transitions live in
  `include/sway/tree/dynamic_resize.h`.
- The parser and registration are in `sway/commands/dynamic_resize.c`,
  `sway/commands.c`, and `include/sway/commands.h`. The rule is documented in
  `sway/sway.5.scd`.
- Rule order is significant. Specific title rules must precede an app-wide
  fallback such as the ordinary Ghostty rule.

### Hyprland-style unaccelerated pointer motion

- `force_no_accel yes|no` is a custom global command implemented by this fork.
  It defaults to `no`.
- When enabled, physical relative pointer events move Sway's compositor cursor
  with the event's unaccelerated delta. This matches Hyprland 0.56's
  `input.force_no_accel` behavior.
- Relative-pointer clients still receive both the original accelerated and
  unaccelerated delta channels before Sway selects the compositor cursor delta.
  Do not replace both protocol values with the unaccelerated value; games need
  the standard protocol semantics.
- Absolute devices and non-pointer synthesized motion retain their normal
  deltas.
- The command parser is `sway/commands/force_no_accel.c`. The pure delta
  selector is `include/sway/input/pointer.h`, and the event integration is in
  `sway/input/cursor.c`.

### Immediate game presentation and tearing

- `immediate yes|no` is a custom per-view command implemented by this fork. It
  is an alias for Sway's existing per-view `allow_tearing` override and matches
  Hyprland 0.56's boolean `immediate` window-rule effect.
- Use it through Sway's native rule mechanism:
  `for_window <criteria> immediate yes`.
- `cmd_immediate()` and `cmd_allow_tearing()` share the same implementation in
  `sway/commands/allow_tearing.c`. Do not create a second tearing state.
- Immediate page flips require both the view override and
  `output <name> allow_tearing yes`. Sway only tears when the eligible view is
  fullscreen on that output.
- The active config enables output tearing globally with
  `output * allow_tearing yes` and keeps `output * max_render_time off`, then
  tags the Hyprland game criteria through the unified `class` identifier. A
  tag-based rule then moves those views to workspace 3, marks them immediate,
  and makes them fullscreen.

### Hyprland-style window tags

- The `class` criterion follows Hyprland semantics: it matches the X11 class
  for Xwayland views and falls back to `app_id` for native Wayland views.
  Existing X11 class rules remain compatible; use `app_id` when a rule should
  be Wayland-only.
- `tag +<name>`, `tag -<name>`, and `tag <name>` add, remove, and toggle a
  compositor-owned window tag. Unlike Sway marks, the same tag may belong to
  multiple windows.
- Tags are owned by `struct sway_container`, so they remain attached through
  workspace/output moves and floating or fullscreen transitions. The list must
  be freed with the container.
- The `tag` criterion matches every compositor-owned tag on both Wayland and
  Xwayland views. For upstream config compatibility, it also matches the
  client-supplied xdg-shell toplevel tag. Use `xdg_tag` when only that
  client-supplied value should match.
- The existing IPC `tag` string remains the xdg-shell tag. The new IPC `tags`
  array contains compositor-owned tags, and add/remove operations emit a
  window event with change `tag`.
- A successful tag change calls `view_execute_criteria()`. This allows an
  application/class rule that adds a tag to trigger later tag-based
  `for_window` rules immediately. Put tag assignment rules before rules that
  consume the tag.
- `for_window` retains normal Sway one-shot semantics; these are not Hyprland's
  continuously re-evaluated dynamic tags. Multiple effects for one tag can be
  compacted with a comma, for example:
  `for_window [tag="^game$"] move container to workspace number 3, immediate yes`.
- Pure tag modifier parsing lives in `include/sway/tree/tag.h`; the command is
  implemented in `sway/commands/tag.c`.
- `swaymsg clients` is shorthand for the custom `GET_CLIENTS` IPC request
  (`swaymsg -t get_clients`). Its concise output reports the unified `class`,
  raw `app_id` and `xwayland_class`, placement, state, and tags without making
  users traverse `GET_TREE`. Raw JSON remains available with `-r`.

### Realtime compositor scheduling

- Sway's existing `set_rr_scheduling()` in `sway/realtime.c` requests
  `SCHED_RR` at the minimum realtime priority and resets forked children to
  `SCHED_OTHER`; this is not a custom scheduling implementation.
- `/etc/security/limits.d/95-sway.conf` grants only `jade - rtprio 1`. The file
  is root-owned with mode `0644`, and the TTY login stack applies it through
  `pam_limits.so`.
- The limit is inherited only after a complete TTY logout and login. Restarting
  Sway from a shell created before the limits file was installed does not apply
  it.
- The active Sway process has been verified with `chrt -p` as `SCHED_RR`
  priority `1`, with both soft and hard `RLIMIT_RTPRIO` set to `1`.
- Keep the limit at priority `1`. Do not grant an unnecessarily high realtime
  priority or make game processes inherit Sway's scheduler policy.

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

### Regression invariants

- New windows extend `workspace_get_dwindle_tail()`; current keyboard or pointer
  focus must not select a new spawn root.
- Removing or moving a leaf must collapse its obsolete binary parent without
  corrupting the surviving branch, focus, fullscreen pointers, or workspace
  ownership.
- Directional movement must sample beyond the complete inner or outer gap.
  Sampling inside a gap can select the expanding source sibling and make the
  move appear to do nothing.
- Directional movement at the workspace boundary can target the active
  workspace on an adjacent output. Do not limit it to the source workspace.
- Floating-to-tiled must clear old fractions before insertion and must not clear
  the fractions assigned by insertion. Reversing this order makes the returned
  window extremely small.
- Fullscreen surface positioning must not reuse floating/tiled XDG geometry.
  Fullscreen video previously rendered clipped and off-center when
  `view_center_and_clip_surface()` centered non-floating views.

### Tests

- Pure dwindle geometry helpers live in `include/sway/tree/dwindle.h`.
- `tests/dwindle.c` covers orientation, insertion order, forced sides, and
  split-ratio fallback behavior.
- `tests/dynamic_resize.c` covers eligibility, first-match state capture,
  transitions between special sizes, and restoration.
- `tests/pointer_motion.c` covers accelerated, forced-unaccelerated, and
  non-pointer delta selection.
- `tests/tag.c` covers add, remove, toggle, and invalid tag modifier parsing.
- `tests/window_class.c` covers native Wayland, Xwayland, and missing unified
  class resolution.
- `tests/meson.build` registers them as `dwindle`, `dynamic-resize`,
  `pointer-motion`, `tag`, and `window-class`.

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
- Run the pointer delta test with
  `meson test -C build pointer-motion --print-errorlogs`.
- Run the tag parser test with
  `meson test -C build tag --print-errorlogs`.
- Run the unified class test with
  `meson test -C build window-class --print-errorlogs`.
- Run the full relevant suite with
  `meson test -C build --print-errorlogs`.
- Run `git diff --check`.
- Never start, exit, reload, or restart Sway as part of automated validation.
  The user controls the active compositor session and will launch or restart
  the rebuilt binary when ready.

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
- `mouse_warping container` makes the pointer follow keyboard focus. This is a
  normal Sway config behavior, not a separate source customization.
- Pointer input uses `accel_profile flat`, `pointer_accel 0`,
  `force_no_accel yes`, and disabled natural scrolling. This mirrors the active
  Hyprland input config. Inner and outer gaps are both `5`; borders are `3`
  pixels.
- TTY logins permanently grant `rtprio 1` through
  `/etc/security/limits.d/95-sway.conf`. Sway therefore enters its built-in
  `SCHED_RR` priority `1` policy on every normal launch. Verify the live process
  with `chrt -p $(pgrep -n -x sway)` from Bash or
  `chrt -p (pgrep -n -x sway)` from Fish.
- Use `swaymsg reload` only when a config change needs to be applied. A source
  rebuild requires the user to exit and launch Sway again; never restart their
  session automatically.
- Do not reload a config containing newly added custom commands into an older
  running Sway process. Restart into the rebuilt binary first.

### Active external configuration

These files are intentionally outside this repository. Inspect them before
making assumptions, and edit them only when the user explicitly asks.

- `~/.config/sway/config`:
  - DP-2 is at `0,0`, runs `1920x1080@239.760Hz`, and owns workspaces 1, 3,
    and 4.
  - DP-3 is at `1920,0`, runs `1920x1080@165.003Hz`, and owns workspace 2.
  - Sway has no useful Hyprland-style `primary` output flag for initial
    workspace placement; explicit `workspace <number> output <name>` mappings
    control it.
  - `assign [class="discord"] workspace number 2` moves Discord when it starts;
    it does not autostart Discord.
  - `workspace number 1` at the end of the config selects workspace 1 on DP-2
    at startup.
  - The native `bar {}` block launches this worktree's symlinked `swaybar`.
    It uses the two-color theme and disables the tray with `tray_output none`.
  - Tearing is allowed on all outputs, with output max render time disabled.
    Actual immediate page flips still require a fullscreen view whose
    per-view immediate override or tearing hint permits tearing.
- `~/.config/sway/config.d/50-systemd-user.conf` exports/imports
  `XDG_CURRENT_DESKTOP=sway`, `DISPLAY`, `SWAYSOCK`, and `WAYLAND_DISPLAY` into
  the systemd user and D-Bus activation environments. This supports
  `xdg-desktop-portal-wlr`.
- The active dynamic rules, in first-match order, are:

```
dynamic_resize [app_id="^com[.]mitchellh[.]ghostty$" title="(?i)btop"] 870 700
dynamic_resize [app_id="^com[.]mitchellh[.]ghostty$" title="(?i)(nv|vim)"] 1220 730
dynamic_resize [app_id="^com[.]mitchellh[.]ghostty$"] 700 460
dynamic_resize [app_id="^helium$"] 1200 800
dynamic_resize [app_id="^org[.]gnome[.]Nautilus$"] 975 615
dynamic_resize [class="^discord$"] 1200 900
```

- The Hyprland game match tags both Wayland and Xwayland views. The following
  tag-based rule applies the immediate/tearing override:

```
for_window [class="(org.vinegarhq.Sober|explorer.exe|steam_.*|Minecraft.*|Lunar.*)$"] tag +game
for_window [tag="^game$"] move container to workspace number 3, immediate yes, fullscreen
```

- Workspace 3 is mapped to DP-2. The tag-based compound rule moves games there
  without changing the visible workspace, permits immediate page flips, and
  makes the view fullscreen.
- Ghostty, Helium, and Nautilus are native Wayland applications. Discord is
  currently an Xwayland application. Confirm live identifiers with
  `swaymsg clients` before changing criteria.

## Repository layout

- Public tree/layout interfaces: `include/sway/tree/`
- Pure dwindle helpers: `include/sway/tree/dwindle.h`
- Dynamic resize state helper: `include/sway/tree/dynamic_resize.h`
- Dynamic resize command parser: `sway/commands/dynamic_resize.c`
- Pure tag modifier parser: `include/sway/tree/tag.h`
- Window tag command: `sway/commands/tag.c`
- Unified window class helper: `include/sway/tree/window_class.h`
- Focused client IPC serialization: `sway/ipc-json.c`
- Unaccelerated pointer helper: `include/sway/input/pointer.h`
- Relative pointer event handling: `sway/input/cursor.c`
- Immediate/tearing view command: `sway/commands/allow_tearing.c`
- Tree mutation and arrangement: `sway/tree/`
- Interactive tiled moves: `sway/input/seatop_move_tiling.c`
- Command-driven moves/resizes/layout: `sway/commands/`
- View mapping and tiled insertion: `sway/tree/view.c`
- Protocol-specific view lifecycle: `sway/desktop/`
- IPC tree serialization: `sway/ipc-json.c`
