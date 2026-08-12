# Backup-Project.ps1
# Mirrors the ZombieShooter working tree to D:\Dev\Backups\ZombieShooter\Live.
# Safety net for whatever isn't committed yet - Gitea/GitHub only capture committed history.
# Run manually anytime, or via the "ZS Nightly Backup" scheduled task.

$SourcePath = "C:\Users\aaron\Documents\Unreal Projects\ZombieShooter"
$DestPath   = "D:\Dev\Backups\ZombieShooter\Live"
$LogDir     = "D:\Dev\Backups\ZombieShooter\Logs"

if (-not (Test-Path "D:\")) {
    Write-Output "$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss') - D: drive not available, skipping backup."
    exit 0
}

New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

# Prune logs older than 30 days so these don't accumulate forever
Get-ChildItem $LogDir -Filter "mirror_*.log" -ErrorAction SilentlyContinue |
    Where-Object { $_.LastWriteTime -lt (Get-Date).AddDays(-30) } |
    Remove-Item -Force -ErrorAction SilentlyContinue

$Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$LogFile = Join-Path $LogDir "mirror_$Timestamp.log"

# Regeneratable build/IDE junk only - everything authored or licensed still gets mirrored.
$ExcludeDirs = @(
    (Join-Path $SourcePath ".git"),
    (Join-Path $SourcePath "Binaries"),
    (Join-Path $SourcePath "Build"),
    (Join-Path $SourcePath "DerivedDataCache"),
    (Join-Path $SourcePath "Intermediate"),
    (Join-Path $SourcePath ".vs"),
    (Join-Path $SourcePath ".idea"),
    (Join-Path $SourcePath "Saved\Logs"),
    (Join-Path $SourcePath "Saved\Crashes"),
    (Join-Path $SourcePath "Saved\ShaderDebugInfo"),
    (Join-Path $SourcePath "Saved\SemanticSearch"),
    (Join-Path $SourcePath "Saved\UnrealBuildTool")
)

$StartTime = Get-Date
& robocopy.exe $SourcePath $DestPath /MIR /XD @ExcludeDirs /R:2 /W:5 /MT:8 /NP /NFL /NDL "/LOG+:$LogFile"
$ExitCode = $LASTEXITCODE
$Duration = (Get-Date) - $StartTime

# Robocopy exit codes 0-7 are all success (varying "what happened"); 8+ is a real failure.
if ($ExitCode -ge 8) {
    Write-Output "Backup FAILED (robocopy exit code $ExitCode) after $($Duration.ToString('mm\:ss')). See $LogFile"
    exit 1
} else {
    Write-Output "Backup completed (robocopy exit code $ExitCode) in $($Duration.ToString('mm\:ss')). Log: $LogFile"
    exit 0
}
