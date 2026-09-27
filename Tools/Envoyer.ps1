# ============================================================
#  ORA - Envoi en une commande (Perforce + Git)
# ============================================================
# A lancer depuis n'importe ou, apres chaque changement verifie :
#   powershell -ExecutionPolicy Bypass -File Tools\Envoyer.ps1
#
# 1. Perforce : cree un changelist, y met les fichiers modifies du projet actif
#    (Source, Config, plugin GameplayVariables par defaut), l'affiche, puis le soumet.
# 2. Git : fusionne la branche de travail des agents dans main et pousse.
#
# Options :
#   -Message "feat: ..."     description (par defaut : les commits Git en attente)
#   -Paths "chemin/...", ... chemins Perforce a envoyer (relatifs a la racine ORA)
#   -Branch "claude/..."     branche Git a fusionner (par defaut : la plus recente claude/*)
#   -Oui                     ne pas demander de confirmation avant le submit
#   -SansGit / -SansPerforce sauter une des deux etapes

param(
    [string]$Message = "",
    [string[]]$Paths = @(
        "ProjetUE5/MovementORA 5.8/Source/...",
        "ProjetUE5/MovementORA 5.8/Config/...",
        "ProjetUE5/MovementORA 5.8/Plugins/GameplayVariables/...",
        "ProjetUE5/MovementORA 5.8/AGENTS.md"
    ),
    [string]$Branch = "",
    [switch]$Oui,
    [switch]$SansGit,
    [switch]$SansPerforce
)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
# Depuis un worktree .claude/worktrees/<nom>, remonter a la racine du depot principal.
if ($Root -match '\\\.claude\\worktrees\\') { $Root = $Root -replace '\\\.claude\\worktrees\\.*$', '' }
Set-Location $Root

function Step($Text) { Write-Host "`n== $Text" -ForegroundColor Cyan }

# --- Branche Git et message ---------------------------------------------------------------
if (-not $SansGit -and -not $Branch) {
    $Branch = (git for-each-ref --sort=-committerdate --format="%(refname:short)" refs/heads/claude/ | Select-Object -First 1)
}
$pending = @()
if ($Branch) { $pending = @(git log --format="- %s" "main..$Branch" 2>$null) }
if (-not $Message) {
    $Message = if ($pending.Count -gt 0) { "ORA - " + ($pending.Count) + " changement(s)`n" + ($pending -join "`n") } else { "ORA - mise a jour" }
}

# --- Perforce -----------------------------------------------------------------------------
if (-not $SansPerforce) {
    Step "Perforce : connexion"
    cmd /c "p4 login -s >nul 2>&1"
    if ($LASTEXITCODE -ne 0) { p4 login; if ($LASTEXITCODE -ne 0) { throw "Connexion Perforce impossible." } }

    Step "Perforce : creation du changelist"
    $desc = ($Message -split "`n" | ForEach-Object { "`t$_" }) -join "`n"
    $spec = "Change: new`nDescription:`n$desc`n"
    $out = $spec | p4 change -i
    $cl = [regex]::Match(($out -join " "), "Change (\d+) created").Groups[1].Value
    if (-not $cl) { throw "Creation du changelist impossible : $out" }
    Write-Host "Changelist $cl"

    p4 reconcile -c $cl @Paths | Out-Null
    $opened = @(p4 opened -c $cl 2>$null)
    if ($opened.Count -eq 0) {
        Write-Host "Aucun fichier modifie cote Perforce." -ForegroundColor Yellow
        p4 change -d $cl | Out-Null
    } else {
        $opened | ForEach-Object { Write-Host "  $_" }
        $go = $Oui -or ((Read-Host "Soumettre ces $($opened.Count) fichier(s) ? (o/n)") -eq "o")
        if ($go) {
            p4 submit -c $cl
            if ($LASTEXITCODE -ne 0) { throw "Submit Perforce echoue (changelist $cl conserve)." }
        } else {
            Write-Host "Submit annule : changelist $cl conserve." -ForegroundColor Yellow
        }
    }
}

# --- Git ----------------------------------------------------------------------------------
if (-not $SansGit) {
    Step "Git : fusion de $Branch dans main"
    if ($Branch -and $pending.Count -gt 0) {
        git checkout -q main
        git merge --ff-only $Branch
        if ($LASTEXITCODE -ne 0) { git merge --no-edit $Branch; if ($LASTEXITCODE -ne 0) { throw "Fusion Git impossible." } }
    } else {
        Write-Host "Rien a fusionner."
    }
    Step "Git : envoi"
    git push origin main
    if ($LASTEXITCODE -ne 0) { throw "git push a echoue." }
}

Write-Host "`nEnvoi termine." -ForegroundColor Green
