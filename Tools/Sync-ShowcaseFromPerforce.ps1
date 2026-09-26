# ============================================================
#  ORA - Synchronise la vitrine Git depuis le projet Perforce
# ============================================================
# Le projet Unreal actif (ProjetUE5/MovementORA 5.8) est versionne sur Perforce.
# Ce script en copie le code et la configuration dans ProjetUE5/MovementORA/,
# le dossier suivi par Git, puis masque les jetons locaux.
# Relancer apres chaque submit Perforce qui touche Source/ ou Config/, puis committer.

param(
    [string]$SourceProject = (Join-Path $PSScriptRoot "..\ProjetUE5\MovementORA 5.8"),
    [string]$Destination = (Join-Path $PSScriptRoot "..\ProjetUE5\MovementORA")
)

$ErrorActionPreference = "Stop"
$SourceProject = (Resolve-Path $SourceProject).Path
if (-not (Test-Path (Join-Path $SourceProject "MovementORA.uproject"))) {
    throw "MovementORA.uproject introuvable dans $SourceProject"
}
New-Item -ItemType Directory -Force -Path $Destination | Out-Null
$Destination = (Resolve-Path $Destination).Path

function Mirror {
    param([string]$Relative, [string[]]$Files)
    $from = Join-Path $SourceProject $Relative
    $to = Join-Path $Destination $Relative
    # /MIR supprime aussi les fichiers disparus cote Perforce.
    & robocopy $from $to $Files /MIR /NJH /NJS /NDL /NP /XD Intermediate Binaries | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy a echoue pour $Relative (code $LASTEXITCODE)" }
}

Mirror "Source" @("*.h", "*.cpp", "*.cs", "*.inl")
Mirror "Config" @("*.ini")
Mirror "Plugins\GameplayVariables" @("*.h", "*.cpp", "*.cs", "*.uplugin")
Copy-Item (Join-Path $SourceProject "MovementORA.uproject") $Destination -Force
Copy-Item (Join-Path $SourceProject "Documentation\GameplayVariablesConvention.md") $Destination -Force

# Jetons generes localement : jamais publies.
Get-ChildItem (Join-Path $Destination "Config") -Filter *.ini -Recurse | ForEach-Object {
    $text = [System.IO.File]::ReadAllText($_.FullName)
    $clean = [regex]::Replace($text, "(?m)^(SecurityToken=).*$", '${1}<local>')
    if ($clean -ne $text) { [System.IO.File]::WriteAllText($_.FullName, $clean) }
}

Write-Host "Vitrine synchronisee depuis $SourceProject vers $Destination" -ForegroundColor Green
