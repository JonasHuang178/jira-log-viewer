## ADDED Requirements

### Requirement: Jira Key input dialog

The system SHALL display a modal dialog with a text input field when the user triggers the Open Jira Attachment command. The dialog SHALL accept a Jira Key string (e.g., `PROJ-123`). Pressing OK or Enter SHALL proceed; pressing Cancel or Escape SHALL abort. The dialog SHALL use a custom WndProc (`InputWndProc`) to handle `WM_COMMAND` from OK/Cancel buttons, and the window height SHALL be sufficient to display all controls without clipping (350x120 pixels).

#### Scenario: Valid Jira Key entered

- **WHEN** the user types `PROJ-123` and presses OK
- **THEN** the system proceeds to fetch attachments for `PROJ-123`

#### Scenario: Empty input

- **WHEN** the user presses OK without entering text
- **THEN** the dialog remains open (does not proceed)

#### Scenario: User cancels

- **WHEN** the user presses Cancel or Escape
- **THEN** the operation is aborted and no API call is made

### Requirement: Attachment list dialog with checkboxes

The system SHALL display a modal dialog with a ListView control in Report mode with checkboxes. The ListView SHALL show columns: No (row number), Date (formatted), and Filename. The dialog title SHALL include the Jira Key and issue summary.

#### Scenario: Display attachments

- **WHEN** the API returns 8 attachments
- **THEN** the ListView displays 8 rows with row number, formatted date (e.g., `2024-01-15 10:30`), and filename

#### Scenario: User selects and opens

- **WHEN** the user checks 3 attachments and clicks Open
- **THEN** the system downloads and opens those 3 files

#### Scenario: Double-click to open

- **WHEN** the user double-clicks a row in the ListView
- **THEN** the system selects only that item and proceeds to download/open it

#### Scenario: No selection

- **WHEN** the user clicks Open without checking any attachment
- **THEN** the system does nothing (Open button is disabled when no item is checked)

### Requirement: Smart attachment filtering

The system SHALL filter attachments using a two-path priority system based on runtime settings:

**Path A (archive priority)**: If attachments matching the configured archive extension AND filename prefix exist, only those are shown. A single match SHALL be auto-selected without showing the dialog.

**Path B (fallback)**: If no archive matches, attachments matching the configured focused extension are shown. A single match SHALL be auto-selected. If no focused matches, all attachments are shown.

#### Scenario: Single matching archive

- **WHEN** the Jira issue has one attachment `Pattern_server.7z` and the prefix is `Pattern_`
- **THEN** the archive is auto-downloaded and extracted without showing a selection dialog

#### Scenario: Multiple matching archives

- **WHEN** the issue has `Pattern_app.7z` and `Pattern_db.7z`
- **THEN** the selection dialog is displayed with these 2 archives

#### Scenario: No matching archives, single .log file

- **WHEN** the issue has no `Pattern_*.7z` files but has one `app.log`
- **THEN** the .log file is auto-downloaded and opened directly

#### Scenario: No focused matches at all

- **WHEN** no attachments match either the archive filter or the focused extension
- **THEN** all attachments are displayed in the selection dialog

### Requirement: Text filter for attachment list

The dialog SHALL include a text input field that filters the ListView in real time. Only attachments whose filename contains the filter text (case-insensitive) SHALL be displayed.

#### Scenario: Filter by keyword

- **WHEN** the user types `error` in the filter field
- **THEN** only attachments with `error` in their filename are shown

#### Scenario: Clear filter

- **WHEN** the user clears the filter field
- **THEN** all attachments are displayed again

### Requirement: Select All button

The dialog SHALL include a Select All button that toggles all visible (filtered) items' checkboxes. If all visible items are checked, clicking it SHALL uncheck all.

#### Scenario: Select all visible

- **WHEN** the user clicks Select All and 5 of 8 items are visible (filtered)
- **THEN** those 5 visible items are checked

#### Scenario: Toggle off

- **WHEN** all visible items are already checked and the user clicks Select All
- **THEN** all visible items are unchecked

### Requirement: Status bar shows selection count

The dialog SHALL display a status text showing the number of checked items and total items (e.g., `Selected: 3 / 8`).

#### Scenario: Update count on selection change

- **WHEN** the user checks or unchecks an item
- **THEN** the status text updates immediately to reflect the new count
