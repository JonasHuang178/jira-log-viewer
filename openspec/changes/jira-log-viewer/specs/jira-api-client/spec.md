## ADDED Requirements

### Requirement: Fetch issue attachments via REST API

The system SHALL call `GET {JIRA_URL}/rest/api/2/issue/{issueKey}` to retrieve the attachment list for a given Jira issue. The request SHALL include an `Authorization: Bearer {JIRA_PAT}` header. The system SHALL parse the JSON response and extract `fields.attachment[]`, returning each attachment's `id`, `filename`, `size`, `created`, `author.displayName`, and `content` (download URL).

#### Scenario: Successful attachment retrieval

- **WHEN** the user provides a valid Jira Key (e.g., `PROJ-123`) and the API returns HTTP 200
- **THEN** the system returns a list of attachment objects with filename, size, created date, author display name, and download URL

#### Scenario: Issue not found

- **WHEN** the API returns HTTP 404
- **THEN** the system displays an error message indicating the Jira Key does not exist

#### Scenario: Authentication failure

- **WHEN** the API returns HTTP 401 or 403
- **THEN** the system displays an error message prompting the user to check the PAT in config.h

### Requirement: Download attachment binary

The system SHALL download an attachment by sending `GET {content URL}` with the same Bearer token. The response body SHALL be saved as a binary file to the designated download directory.

#### Scenario: Successful download

- **WHEN** the download URL returns HTTP 200
- **THEN** the system saves the file to `%TEMP%\jira-log-viewer\{JIRA-KEY}\{filename}`

#### Scenario: PAT redirect detection (JRASERVER-72019)

- **WHEN** the download request returns HTTP 302 with a Location header containing `/login`
- **THEN** the system displays an error message indicating the Jira Server version may not support PAT for attachment downloads

### Requirement: Handle self-signed certificates

The system SHALL configure WinHTTP to ignore certificate validation errors, including `SECURITY_FLAG_IGNORE_UNKNOWN_CA` and `SECURITY_FLAG_IGNORE_CERT_CN_INVALID`, to support corporate Jira servers using self-signed certificates.

#### Scenario: Self-signed certificate

- **WHEN** the Jira server uses a self-signed SSL certificate
- **THEN** the HTTPS connection succeeds without certificate errors

### Requirement: Connection timeout

The system SHALL set a WinHTTP connection timeout of 30 seconds and a receive timeout of 60 seconds to prevent indefinite UI blocking on network issues.

#### Scenario: Network timeout

- **WHEN** the Jira server does not respond within the timeout period
- **THEN** the system displays a timeout error message and returns control to the user
