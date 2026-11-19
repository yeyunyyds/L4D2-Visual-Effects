# Logging levels and periodic diagnostics

The log output and rollover behavior are unchanged: events are appended to
`bin/neko/plugins/L4D2Visual/L4D2Visual.log`, with existing size-based
rotation into `L4D2Visual.previous.log`.

## Info (default)

**Info** is the normal operating mode. Startup, module changes, feature
toggles, configuration saves and failures continue to be recorded through the
existing `diagnostics::write` path, even when detailed diagnostics are off.
The live diagnostics menu remains available.

The plugin does not build or write the expensive periodic status snapshot,
repeated entity binding checks, entity-class inventory, or per-target draw
summary at Info level. This also removes the string-building overhead.

## Debug (opt-in)

Use **菜单 → 日志级别** to switch to **Debug**.
The old **实体缩放与拉伸 → 邦友的酒 → 接口与运行诊断** page
continues to display runtime status but no longer contains a log-level switch.
Detailed periodic snapshots preserve the previous format and nominal
two-second interval. Turning Debug off stops further periodic snapshots.
Normal Info events remain enabled in either mode.

Use **保存设置** in the diagnostics menu or the main menu to persist the
selection. The optional setting in `hud.json` is:

```json
{
  "logging": {
    "level": "info"
  }
}
```

Set `logging.level` to `"debug"` to opt in. A missing, malformed, or
unrecognized value defaults to Info; existing user configurations remain
valid without a migration. This setting is independent of all gameplay,
rendering, and first-person body settings.

This change only gates the previously high-volume *periodic* diagnostic
writer. It does **not** suppress logs emitted elsewhere, prevent individual
components from writing event messages, or change file rotation.
