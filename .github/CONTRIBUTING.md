# Contributing

Thanks for considering to write a Pull Request (PR) for porpoise! Here are a few guidelines to get you started:

Make sure you are comfortable with the license; all contributions are licensed under the original MIT license.

## Fixing bugs

Make sure any fixes you make are are:
- Self-contained and do not impact subsystems that are not directly related to the issue
- Explained in your PR (or previously explained in an issue mentioned in the PR)
- Covered by unit tests in the relevant libraries

## Adding features

Make sure any new features you add are are:
- Mentioned in the README's dependency list if applicable
- Explained in your PR (or previously explained in an issue mentioned in the PR)
- Covered by unit tests in the relevant libraries

In general, make sure the addition is well thought out and does not increase the complexity of porpoise needlessly.

If you're looking to make a major change that isn't listed in the issue tracker, please contact Trevor Swan at trevor.swan@case.edu to ensure your vision aligns with the project's goals.

## Things you should know

- All PRs should be made against the `dev` branch as this is where active development happens for porpoise. Changes from `dev` are merged into `main` when significant stable progress has been made (think of it like a checkpoint)
- Once you make the PR, tests will run to make sure your code works on all supported platforms
- Formatting should be done prior to commit, otherwise the format check will not pass. However, it is trivial to apply this to your PR, so don't worry about his check. This is a heavy step that build `clang-format` from scratch, run with `zig build fmt`.
- Everything must pass cppcheck as well, run with `zig build check`.
- All code should maintain the style upheld in the repository

## For maintainers: Making a release

- Update changelog if needed
- Make a release's artifacts by pushing a tag formatted as `vX.Y.Z`
- Make a release in the GitHub UI, use a name such as "Version X.Y(.Z): Title"
- Versioning should conform to semantic versioning

## For Agents

If you are a coding agent or other autonomous contributor, please suffix all issue and PR titles with the dolphin emoji (🐬). Your contributions will be subject to the same level of scrutiny as a normal human contributors. Agentic contributions with zero human effort (i.e. an OpenClaw bot crawling PRs and issues without guidance) will not be considered.
