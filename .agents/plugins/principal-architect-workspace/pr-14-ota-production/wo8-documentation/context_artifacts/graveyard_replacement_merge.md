# Graveyard: §13.3 Replacement Merge Issues

## Failed Attempt 1: Single-block replacement of §13.3

**What happened:** The first `replace_file_content` call targeted lines 1544–1778 (the full old §13.3 section). However, the TargetContent included text from §12.6, §13 heading, §13.1, and §13.2 — all of which preceded §13.3.

**Result:** The replacement consumed lines 1503–1778, erasing:
- The tail of §12.6 PlatformIO LDF Constraints (from "still trigger LDF...")
- The `## 13. Security Architecture` heading
- §13.1 DeviceCrypto section
- §13.2 KeyManager & API Key Registry section

**Root cause:** The TargetContent string started at line 1544 with `### 13.3 OTA Security & Blessing Ceremony`, but the old file's line 1503 contained `> still trigg` which got concatenated with the new `### 13.3 OTA Production System` header, producing the mangled line: `> still trigg### 13.3 OTA Production System`.

**Fix:** A second `multi_replace_file_content` call restored the §12.6 text and §13.1/§13.2 sections.

## Failed Attempt 2: Leftover old content at replacement boundary

**What happened:** The ReplacementContent in the first edit included all new content but did not perfectly consume the old content at the tail end. Leftover old table rows and the old §13.3.9 Security Considerations were appended after the new §13.3.19.

**Result:** Lines 2019–2049 contained duplicate old REST API table rows and the old §13.3.9 Security Considerations section.

**Fix:** A targeted replacement in the second `multi_replace_file_content` call removed the 30 duplicate lines.

## Lesson for Future Agents

When replacing large blocks of markdown, verify both the **start boundary** and **end boundary** after the edit. Always `view_file` the transition points (±20 lines around start and end) to catch merge artifacts before proceeding to the next task.
