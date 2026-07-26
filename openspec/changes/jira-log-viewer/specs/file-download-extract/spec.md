## ADDED Requirements

### Requirement: Download to temp directory

The system SHALL download attachment files to `%TEMP%\jira-log-viewer\{JIRA-KEY}\`. The directory SHALL be created automatically if it does not exist. The filename SHALL match the original attachment filename.

#### Scenario: Directory creation

- **WHEN** the user downloads an attachment for `PROJ-123` for the first time
- **THEN** the system creates `%TEMP%\jira-log-viewer\PROJ-123\` and saves the file there

#### Scenario: File already exists

- **WHEN** a file with the same name already exists in the download directory
- **THEN** the system overwrites the existing file

### Requirement: Detect and extract .7z archives

The system SHALL detect files with the `.7z` extension after download. For .7z files, the system SHALL invoke `7z.exe x -o"{outputDir}" -y "{archivePath}"` via `CreateProcess` to extract the archive contents. The output directory SHALL be `%TEMP%\jira-log-viewer\{JIRA-KEY}\{archive-name-without-extension}\`.

#### Scenario: Successful .7z extraction

- **WHEN** the downloaded file is `server-logs.7z`
- **THEN** the system extracts its contents to `%TEMP%\jira-log-viewer\PROJ-123\server-logs\`

#### Scenario: 7z.exe not found

- **WHEN** the path configured in `config.h` for 7z.exe does not exist
- **THEN** the system displays an error message indicating 7z.exe was not found at the configured path

#### Scenario: Extraction failure

- **WHEN** 7z.exe returns a non-zero exit code
- **THEN** the system displays an error message including the stderr output from 7z.exe

### Requirement: Configurable extraction search depth

The system SHALL support a configurable `recursive` flag for post-extraction file enumeration. When `false` (default), only top-level files in the extraction directory are listed. When `true`, files in subdirectories are included recursively. The flag is controlled via runtime Settings.

#### Scenario: Non-recursive search (default)

- **WHEN** the .7z extracts to a directory with files at top level and in subdirectories, and recursive is `false`
- **THEN** only top-level files are considered for opening

#### Scenario: Recursive search

- **WHEN** recursive is set to `true` in Settings
- **THEN** files in all subdirectories are also included

### Requirement: Post-extraction focused file filtering

After extraction, the system SHALL filter extracted files by the configured focused extension (e.g., `.log`). If matches are found, only those are presented. If no matches, all extracted files are shown. A single match SHALL be opened directly; multiple matches SHALL show a selection dialog.

#### Scenario: Single .log in extracted files

- **WHEN** the .7z contains 5 files and only 1 is `.log`
- **THEN** the .log file is opened directly without a selection dialog

#### Scenario: Multiple .log files extracted

- **WHEN** the .7z contains 3 `.log` files
- **THEN** a file selection dialog displays those 3 files

#### Scenario: No .log files in archive

- **WHEN** the .7z contains no `.log` files
- **THEN** all extracted files are displayed in the selection dialog

### Requirement: Open files in Notepad++

The system SHALL open selected files in Notepad++ by sending the `NPPM_DOOPEN` message to the Notepad++ window handle for each file path.

#### Scenario: Open multiple files

- **WHEN** the user selects 3 files to open
- **THEN** all 3 files are opened as separate tabs in Notepad++

### Requirement: Non-.7z files open directly

For attachments that are not .7z archives (e.g., .log, .txt), the system SHALL open them directly in Notepad++ after download without any extraction step.

#### Scenario: Open .log file

- **WHEN** the downloaded file is `app.log`
- **THEN** the system opens it directly in Notepad++ without extraction
