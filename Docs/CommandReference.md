# Command Reference

> Exact, ready-to-paste commands for the things Claude runs most often on this project — build, IDE project-file regen, and the git commit/push cycle. Not a workflow change: run these yourself whenever it's more convenient than asking. PowerShell syntax (this machine's primary shell); same commands work from Rider's own terminal or `cmd.exe` with minor quoting differences. Run from the repo root: `C:\Users\aaron\Documents\Unreal Projects\ZombieShooter`. Update the engine path below if the project ever moves to a newer UE version.

## Before building: check nothing's holding it open

The editor or Live Coding holding the module locked is the most common cause of a failed build. Bash's `tasklist` is unreliable for this on this machine — use PowerShell:

```powershell
Get-Process | Where-Object { $_.ProcessName -match "Unreal|LiveCoding|Rider" } | Select-Object ProcessName, Id
```

`Rider`/`Rider.Backend` showing up is fine. `UnrealEditor`/`LiveCoding` showing up means close the editor first.

## Build the editor (Development)

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ZombieShooterEditor Win64 Development "-project=C:\Users\aaron\Documents\Unreal Projects\ZombieShooter\ZombieShooter.uproject" -waitmutex
```

Use after any change, but it's mandatory (not just Live Coding) after a header change — see `CLAUDE.md`'s Live Coding corruption lesson.

## Regenerate IDE project files (Rider/VS)

Needed after adding new `.h`/`.cpp` files, or after a session with a lot of file churn:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" -projectfiles "-project=C:\Users\aaron\Documents\Unreal Projects\ZombieShooter\ZombieShooter.uproject" -game -engine
```

Rider picks up the regenerated `.sln`/`.slnx` automatically; if it doesn't prompt to reload, use Rider's own "Reload Project."

## Git: check state before committing

```powershell
git status
git diff --stat
git log --oneline -10
```

## Git: stage, commit, push

Stage specific files rather than `git add -A`/`git add .` — safer against accidentally picking up something unintended:

```powershell
git add <path1> <path2> ...
```

Multi-line commit message (PowerShell here-string — the closing `'@` must sit at column 0, no leading whitespace):

```powershell
git commit -m @'
Short summary line.

Longer explanation if needed.
'@
```

```powershell
git push
```

## Typical end-of-session sequence

1. Check nothing's locking the build (above).
2. Build.
3. Regenerate project files, if new files were added this session.
4. `git status` / `git diff --stat` — confirm what actually changed before staging anything.
5. `git add` the specific files, `git commit`, `git push`.

## If something doesn't compile

Read the actual error, fix it, rebuild with the same command above — don't reach for Live Coding to patch a header change, and don't force past a failure (`--no-verify`, `-c commit.gpgsign=false`, force-push). See `CLAUDE.md`'s Workflow Efficiency section for the Live Coding corruption pattern if a rebuild "fixes" something that then behaves strangely in the editor afterward.

## See also

`Docs/AsyncSessionProtocol.md` — the behavioral protocol for a full away-session (when to build, how to tag commits `[compiled]`/`[uncompiled]`, when to stop and hand back). This doc is just the underlying commands; that one is when/why to run them unsupervised.
