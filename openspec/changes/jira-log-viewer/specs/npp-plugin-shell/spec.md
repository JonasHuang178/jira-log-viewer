## ADDED Requirements

### Requirement: DLL exports for Notepad++ plugin

The plugin DLL SHALL export the 6 required functions for Notepad++ plugin integration: `setInfo`, `getName`, `getFuncsArray`, `beNotified`, `messageProc`, and `isUnicode`. The `isUnicode` function SHALL return `TRUE`.

#### Scenario: Plugin loads in Notepad++

- **WHEN** the DLL is placed in Notepad++'s plugins directory
- **THEN** Notepad++ loads the plugin and displays its menu under the Plugins menu

### Requirement: Plugin menu items

The plugin SHALL register 3 menu items under `Plugins` → `jira-log-viewer`:
1. `Open Jira Attachment` — triggers the main workflow
2. `Settings` — opens the Settings dialog
3. `About` — displays version information

#### Scenario: Menu appears

- **WHEN** Notepad++ starts with the plugin installed
- **THEN** the Plugins menu contains a `jira-log-viewer` submenu with `Open Jira Attachment`, `Settings`, and `About`

### Requirement: Configurable keyboard shortcut

The `Open Jira Attachment` command SHALL be bound to a keyboard shortcut configurable via `config/JiraConfig.h`. The default shortcut SHALL be `Ctrl+Shift+J`. The configuration SHALL use 4 compile-time constants: `SHORTCUT_CTRL` (bool), `SHORTCUT_ALT` (bool), `SHORTCUT_SHIFT` (bool), and `SHORTCUT_KEY` (UCHAR, virtual key code).

#### Scenario: Default shortcut triggers command

- **WHEN** the user presses Ctrl+Shift+J (default configuration)
- **THEN** the Jira Key input dialog is displayed

#### Scenario: Custom shortcut configuration

- **WHEN** the user changes `SHORTCUT_KEY` to `'K'` and `SHORTCUT_ALT` to `true` in JiraConfig.h and rebuilds
- **THEN** the command is triggered by Ctrl+Alt+Shift+K instead

### Requirement: About dialog

The `About` command SHALL display a MessageBox showing the plugin name, version, and shortcut key.

#### Scenario: About dialog content

- **WHEN** the user clicks `About`
- **THEN** a MessageBox shows `jira-log-viewer v1.0` and the shortcut `Ctrl+Shift+J`

### Requirement: Compile-time configuration via config/JiraConfig.h

Compile-time configuration SHALL be defined as `constexpr` constants in `config/JiraConfig.h`:
- `JIRA_URL` — Jira Server base URL (no trailing slash)
- `JIRA_PAT` — Personal Access Token
- `SEVEN_ZIP_PATH` — path to 7z.exe
- `SHORTCUT_CTRL` — Ctrl modifier key enabled (bool)
- `SHORTCUT_ALT` — Alt modifier key enabled (bool)
- `SHORTCUT_SHIFT` — Shift modifier key enabled (bool)
- `SHORTCUT_KEY` — main key virtual key code (UCHAR)
- `FOCUSED_EXTENSION` — focused file extension filter (default `.log`)
- `FOCUSED_ARCHIVE_EXT` — focused archive extension (default `.7z`)
- `FOCUSED_PREFIX` — archive filename prefix filter (default `Pattern_`)
- `FOCUSED_RECURSIVE` — recursive extraction search (default `false`)

The `FOCUSED_*` constants serve as default values for the runtime Settings.

#### Scenario: Configuration applied at compile time

- **WHEN** the user modifies `JIRA_URL` in JiraConfig.h and rebuilds the plugin
- **THEN** the plugin connects to the newly configured Jira server

#### Scenario: Shortcut key configuration

- **WHEN** the user changes `SHORTCUT_KEY` to `VK_F5` and sets `SHORTCUT_SHIFT` to `false` in JiraConfig.h and rebuilds
- **THEN** the Open Jira Attachment command is bound to Ctrl+F5

### Requirement: Runtime settings via Settings dialog

The plugin SHALL provide a Settings dialog accessible from the plugin menu. The dialog SHALL allow the user to modify:
- Focused Extension (text input)
- Archive Extension (text input)
- Archive Prefix (text input)
- Search recursively (checkbox)

Settings SHALL be persisted in `jira-log-viewer.ini` in the Notepad++ plugin config directory (obtained via `NPPM_GETPLUGINSCONFIGDIR`). Changes SHALL take effect immediately after clicking OK, without restarting Notepad++. On first launch (no INI file), settings SHALL use compile-time defaults from `JiraConfig.h`.

#### Scenario: Change focused extension at runtime

- **WHEN** the user opens Settings, changes Focused Extension to `.txt`, and clicks OK
- **THEN** the next Open Jira Attachment command filters for `.txt` files instead of `.log`

#### Scenario: Settings persist across sessions

- **WHEN** the user modifies settings and restarts Notepad++
- **THEN** the plugin loads the saved settings from the INI file

#### Scenario: First launch defaults

- **WHEN** the plugin loads for the first time (no INI file exists)
- **THEN** settings use compile-time defaults: `.log`, `.7z`, `Pattern_`, non-recursive

### Requirement: Plugin name for Notepad++ registration

The `getName` export SHALL return the string `jira-log-viewer`.

#### Scenario: Plugin name displayed

- **WHEN** Notepad++ queries the plugin name
- **THEN** the plugin returns `jira-log-viewer`
