# CONTRIBUTING.md

Welcome to Samaya! Thank you for the interest in contributing to this minimalist Pomodoro timer.

This document outlines the guidelines and workflow for contributing to the `samaya` project.

---

## Table of Contents

1.  [Code Acquisition (Source Cloning)](#1-code-acquisition-source-cloning)
2.  [Official Contribution Paths](#2-official-contribution-paths)
3.  [Translation Contributions](#3-translation-contributions)
4.  [Pull Request (PR) Submission Workflow](#4-pull-request-pr-submission-workflow)
5.  [Merging Policy and Authorship](#5-merging-policy-and-authorship)
6.  [Reporting Issues and Getting Help](#6-reporting-issues-and-getting-help)
7.  [Legal Notice](#7-legal-notice)
8.  [Out-of-Band Contributions (Patches)](#8-out-of-band-contributions-patches)

---

## 1. Code Acquisition (Source Cloning)

The primary repository is on Codeberg, but read-only mirrors are maintained for faster cloning speeds and reliability. **Mirrors can only be used to clone the source code locally.**

| Platform                      | URL                                           | Purpose                                       |
| :---------------------------- | :-------------------------------------------- | :-------------------------------------------- |
| **Codeberg (Primary Source)** | `https://codeberg.org/lockedmutex/samaya.git` | Required for Contribution Forking             |
| **GitHub Mirror**             | `https://github.com/lockedmutex/samaya.git`   | Read-only local cloning (if Codeberg is slow) |
| **GitLab Mirror**             | `https://gitlab.com/lockedmutex/samaya.git`   | Read-only local cloning (if Codeberg is slow) |
| **Personal Mirror**           | `https://git.suyogtandel.in/samaya/`          | Read-only viewing and cloning (Last Resort)   |

---

## 2. Official Contribution Paths

Contributions are accepted only via three formal methods:

1.  **Codeberg Translate (Weblate):** The automated and recommended path for translations.
2.  **Pull Request (PR) on Codeberg:** For all code, documentation, and manual translation changes. This requires forking the repository on Codeberg.
3.  **Patches via Email:** For submitting changes when all necessary repository platforms are inaccessible.

### Branch Policy

- The **`main`** branch is the only branch permitted for forking and cloning for contribution purposes.
- **All Pull Requests (PRs) must be created against the `main` branch.**
- No other branches (such as `release`) are used for external contributions.

---

## 3. Translation Contributions

Translating Samaya into different languages is highly appreciated. We use **GNU gettext** for localization. You can contribute translations in two ways:

### Option A: Codeberg Translate (Recommended)

The easiest way to translate is using our hosted Weblate instance. You do not need to know Git or install any tools.

1.  Visit the project on **[Codeberg Translate](https://translate.codeberg.org/projects/samaya/)**.
2.  Log in with your Codeberg account.
3.  Select your language and start translating directly in the browser.
4.  Your changes will be automatically formatted and submitted as a Pull Request by the translation bot.

### Option B: Manual Pull Request

If you prefer working offline or with your own tools (like Poedit, Gtranslator, or a text editor):

1.  Follow the **[Code Acquisition](#1-code-acquisition-source-cloning)** steps to fork and clone the repository.
2.  Navigate to the `po/` directory in the source code.
3.  **For existing languages:** Open the corresponding `.po` file (e.g., `es.po`) and add your translations.
4.  **For new languages:** Create a new file named `[language_code].po` (e.g., `fr.po` or `pt_BR.po`) using `samaya.pot` as your template, and add the new language code to the `LINGUAS` file.
5.  Commit your changes and follow the **[Pull Request Workflow](#4-pull-request-pr-submission-workflow)** below to submit them against the `main` branch.

---

## 4. Pull Request (PR) Submission Workflow

### Standard Contribution

1.  The contributor must **fork** the repository **on Codeberg** to a personal account.
2.  The Codeberg fork is cloned locally, or an existing local clone is redirected to use the Codeberg fork as the push remote.
3.  The contributor commits their changes directly to the **`main` branch of their local fork**.
4.  Changes are made and tested.
5.  The `main` branch of the local fork is pushed to the Codeberg remote.
6.  A **Pull Request** is opened from the contributor's Codeberg fork's `main` branch to the **`main`** branch of the original repository.

### Active Development (Draft PR)

If changes are **under active development** and not ready for review or merging, the Pull Request must be opened as a **Draft PR**. The maintainer will not merge a Draft PR. The contributor should convert it to "Ready for Review" when development is complete.

### Complex Features or Documentation

For any complex features, major documentation updates, or large refactoring efforts, the following protocol must be followed:

1.  An **Issue must be opened first** to propose and discuss the change.
2.  Work on the feature should only commence after maintainer approval to prevent duplicated effort.

---

## 5. Merging Policy and Authorship

Pull Requests are typically merged by the maintainer using the Command Line Interface (CLI) for precise control.

### Default Merging Strategy (Squash)

The default merging strategy is **Squash and Merge**.

- **Result:** All commits in the PR are combined into a **single new commit** on the `main` branch.
- **Authorship:** The **Author** of the final squashed commit will be the person who opened the PR (the contributor). The **Committer** will be the maintainer.

### Exception Merging Strategy (Preserved Commits)

If all commits within a Pull Request are important and should be preserved (as determined by the maintainer), a standard merge will be performed.

### Requesting a Specific Merge

A request for a specific merging method can be clearly **stated in the PR comments**. The final decision regarding the merge strategy will be taken by the maintainer after discussion.

---

## 6. Reporting Issues and Getting Help

For bugs, feature requests, or general help, the following channels are available:

- **Open an Issue:** For formal bug reports, feature requests, and detailed asynchronous discussion, use the Codeberg issue tracker.
- **Matrix Discussion:** For quick help, questions, or informal discussion about development, join the Matrix channel: `#samaya:matrix.org`.

### General Guidelines

- Existing issues should be searched first.
- Reports must be descriptive, explaining the expected and observed behavior.
- Reproduction steps and context (OS, platform, etc.) must be provided.
- Communication must be civil and respectful.

---

## 7. Legal Notice

By contributing code, documentation, or other assets to the Samaya project, the contributor agrees that the content is 100% authored by them, that they have the necessary rights to the content, and that the content may be provided under the project's existing license.

---

## 8. Out-of-Band Contributions (Patches)

In the event that **Codeberg, GitHub, AND GitLab are all inaccessible** simultaneously, but a copy of the source code can still be obtained (e.g., from the personal mirror):

- Changes may be sent as a Git patch via email to `git@suyogtandel.in`.
- **Note:** This email address is for receiving **incoming changes only**. The maintainer is unable to reply via this email.
- **The maintainer will not solve merge conflicts.** Patches must apply cleanly to the tip of the `main` branch. The maintainer will apply the changes only if the patch is approved and applies without any conflicts.
