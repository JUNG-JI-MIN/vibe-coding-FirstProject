[CmdletBinding(DefaultParameterSetName = 'Read')]
param(
    [Parameter(ParameterSetName = 'Read')][string]$Base,
    [Parameter(ParameterSetName = 'Read')][ValidateRange(1, 200)][int]$MaxCommits = 10,
    [Parameter(ParameterSetName = 'Read')][ValidateRange(1, 500)][int]$MaxFiles = 80,
    [Parameter(ParameterSetName = 'Read')][ValidateRange(1, 100)][int]$MaxHandoffs = 5,
    [Parameter(Mandatory = $true, ParameterSetName = 'Mark')][switch]$MarkReviewed,
    [Parameter(Mandatory = $true, ParameterSetName = 'Mark')][string]$ExpectedHead
)

$ErrorActionPreference = 'Stop'
$rootResult = & git -C $PSScriptRoot rev-parse --show-toplevel 2>&1
if ($LASTEXITCODE -ne 0) { throw "Cannot locate Git repository: $rootResult" }
$repoRoot = [IO.Path]::GetFullPath([string]$rootResult)
$cursorPath = Join-Path $repoRoot '.local/codex-context.json'

function Read-Git {
    param([string[]]$GitArgs)
    $result = & git -C $repoRoot -c core.quotePath=false -c core.pager=cat @GitArgs 2>&1
    if ($LASTEXITCODE -ne 0) { throw "Git failed ($($GitArgs -join ' ')): $result" }
    return $result
}

function Show-Limited {
    param([string[]]$Lines, [int]$Limit)
    @($Lines | Select-Object -First $Limit) | ForEach-Object { Write-Output $_ }
    if ($Lines.Count -gt $Limit) {
        Write-Output "[OMITTED: $($Lines.Count - $Limit) lines/items. Read the relevant remainder before marking reviewed.]"
    }
}

$head = [string](Read-Git -GitArgs @('rev-parse', 'HEAD'))
$branch = [string](Read-Git -GitArgs @('rev-parse', '--abbrev-ref', 'HEAD'))
$status = @(Read-Git -GitArgs @('status', '--short', '--untracked-files=all'))

if ($MarkReviewed) {
    if ($ExpectedHead -cne $head) { throw 'ExpectedHead must equal the full current HEAD SHA. Recheck changes before saving.' }
    if ($status.Count -gt 0) { throw 'Local changes or untracked files exist. Preserve and commit them before marking reviewed.' }
    $record = [ordered]@{
        schemaVersion = 1
        computer = [Environment]::MachineName
        repoRoot = $repoRoot
        head = $head
        branch = $branch
        reviewedAtUtc = [DateTime]::UtcNow.ToString('o')
    }
    # Check again so a concurrent commit/change is not silently acknowledged.
    if ([string](Read-Git -GitArgs @('rev-parse', 'HEAD')) -cne $head -or
        @(Read-Git -GitArgs @('status', '--short', '--untracked-files=all')).Count -gt 0) {
        throw 'Repository changed while marking reviewed. Retry after reviewing the new state.'
    }
    [void][IO.Directory]::CreateDirectory((Split-Path -Parent $cursorPath))
    $json = $record | ConvertTo-Json
    [IO.File]::WriteAllText($cursorPath, $json + [Environment]::NewLine, (New-Object Text.UTF8Encoding($false)))
    Write-Output "Saved review cursor: $head ($branch). This file is local to this checkout and computer."
    return
}

$notice = ''
$baseline = $null
if ($Base) {
    if ($Base.StartsWith('-')) { throw 'Base must be a Git revision, not an option.' }
    $baseline = [string](Read-Git -GitArgs @('rev-parse', '--verify', "$Base^{commit}"))
} elseif (Test-Path -LiteralPath $cursorPath) {
    try {
        $cursor = Get-Content -LiteralPath $cursorPath -Raw -Encoding UTF8 | ConvertFrom-Json
        if ($cursor.schemaVersion -eq 1 -and $cursor.computer -eq [Environment]::MachineName -and
            $cursor.repoRoot -eq $repoRoot -and $cursor.head -match '^[0-9a-f]{40,64}$') {
            $resolved = & git -C $repoRoot rev-parse --verify "$($cursor.head)^{commit}" 2>$null
            if ($LASTEXITCODE -eq 0) { $baseline = [string]$resolved }
            else { $notice = 'Saved commit is unavailable; using initial context.' }
        } else { $notice = 'Cursor belongs to another computer/checkout or has invalid data; using initial context.' }
    } catch { $notice = 'Cursor could not be read; using initial context.' }
}
if ($baseline) {
    & git -C $repoRoot merge-base --is-ancestor $baseline $head 2>$null
    if ($LASTEXITCODE -eq 1) {
        $notice = 'Baseline is not an ancestor of HEAD (branch switch or rewritten history); using initial context.'
        $baseline = $null
    } elseif ($LASTEXITCODE -ne 0) { throw 'Cannot check baseline ancestry.' }
}

Write-Output "HEAD: $head | branch: $branch | computer: $([Environment]::MachineName)"
Write-Output 'Scope: LOCAL checkout only; this command does not fetch, merge or change the review cursor.'
if ($notice) { Write-Output "NOTICE: $notice" }
if ($baseline) { Write-Output "MODE: changes since $baseline" }
else { Write-Output 'MODE: initial context (no usable review cursor). Older history is not automatically reviewed.' }

Write-Output "`nCURRENT BASELINE: docs/context/current.md"
$currentPath = Join-Path $repoRoot 'docs/context/current.md'
if (Test-Path -LiteralPath $currentPath) {
    Show-Limited @(Get-Content -LiteralPath $currentPath -Encoding UTF8) 60
} else { Write-Output 'Missing current.md; inspect README and relevant source.' }

Write-Output "`nLOCAL CHANGES (always shown, including untracked files):"
if ($status.Count) { Show-Limited $status $MaxFiles }
else { Write-Output '(clean)' }

$changedPaths = @()
if ($baseline) {
    $range = "$baseline..$head"
    $count = [int](Read-Git -GitArgs @('rev-list', '--count', $range))
    Write-Output "`nNEW COMMITS: $count (showing up to $MaxCommits)"
    Read-Git -GitArgs @('log', "-n$MaxCommits", '--format=%h %s', $range)
    if ($count -gt $MaxCommits) { Write-Output "[OMITTED: $($count - $MaxCommits) commits. Inspect: git log --oneline $range]" }
    Write-Output "`nCOMMITTED FILE CHANGES:"
    Show-Limited @(Read-Git -GitArgs @('diff', '--name-status', '-M', $baseline, $head, '--')) $MaxFiles
    $changedPaths += @(Read-Git -GitArgs @('diff', '--name-only', $baseline, $head, '--'))
}
$changedPaths += @(Read-Git -GitArgs @('diff', '--name-only', $head, '--'))
$changedPaths += @(Read-Git -GitArgs @('ls-files', '--others', '--exclude-standard'))
$changedPaths = @($changedPaths | Sort-Object -Unique)

Write-Output "`nMODULE ROUTES (read relevant paths/docs only):"
$modulePath = Join-Path $repoRoot 'docs/context/modules.json'
if (Test-Path -LiteralPath $modulePath) {
    $modules = (Get-Content -LiteralPath $modulePath -Raw -Encoding UTF8 | ConvertFrom-Json).modules
    foreach ($module in $modules) {
        $matched = -not $baseline
        foreach ($pattern in @($module.paths) + @($module.docs)) {
            if (@($changedPaths | Where-Object { $_ -like $pattern }).Count -gt 0) { $matched = $true }
        }
        if ($matched) {
            Write-Output "[$($module.id)] $($module.description)"
            Write-Output "  Paths: $($module.paths -join ', ')"
            Write-Output "  Docs: $($module.docs -join ', ')"
        }
    }
}
Write-Output 'Changes outside these routes still require inspecting the file list. The routes are navigation, not a complete review.'

Write-Output "`nHANDOFFS (current contents, up to $MaxHandoffs files, 24 lines each):"
if ($baseline) {
    $handoffs = @($changedPaths | Where-Object {
        $_ -like 'docs/handoffs/*.md' -and (Test-Path -LiteralPath (Join-Path $repoRoot $_))
    } | Sort-Object -Descending)
} else {
    $handoffRoot = Join-Path $repoRoot 'docs/handoffs'
    $handoffs = @()
    if (Test-Path -LiteralPath $handoffRoot) {
        $handoffs = @(Get-ChildItem -LiteralPath $handoffRoot -Filter '*.md' -File |
            Sort-Object Name -Descending | ForEach-Object { "docs/handoffs/$($_.Name)" })
    }
}
foreach ($path in @($handoffs | Select-Object -First $MaxHandoffs)) {
    Write-Output "--- $path"
    Show-Limited @(Get-Content -LiteralPath (Join-Path $repoRoot $path) -Encoding UTF8) 24
}
if ($handoffs.Count -eq 0) { Write-Output '(none)' }
if ($handoffs.Count -gt $MaxHandoffs) {
    Write-Output "[OMITTED: $($handoffs.Count - $MaxHandoffs) handoffs. Increase -MaxHandoffs or read relevant files.]"
}
if ($baseline) { Write-Output "`nDETAIL: git diff $baseline $head -- <relevant-path>" }
Write-Output "AFTER REVIEW (clean checkout required): ./tools/read-context.ps1 -MarkReviewed -ExpectedHead $head"
