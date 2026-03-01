---
type: archive
status: archived
last_review: 2026-02-19
owner: dylan
audience: humans-agents
---

# Archive - CORRECTIONS_PRIORITAIRES

Ce document est conserve pour historique.

Remplacements actuels:

- [[WORKFLOW_PHASE0_BMAD]]
- [[ROADMAP]]
- [[IMPACT_MIGRATION]]

---

# 🚀 DÉMARRAGE RAPIDE - Corrections Prioritaires

**Objectif:** Corriger la base AVANT de commencer le développement  
**Durée:** 1-2 heures  
**Priorité:** CRITIQUE

---

## ⚡ CORRECTIONS À FAIRE MAINTENANT

### 1️⃣ VÉRIFIER/CRÉER LES STRUCTS (15 min)

**Localisation:** `/Game/Game/Data/Structs/`

#### ST_KeyFloat
```
Si n'existe pas, créer:

Blueprint → Structure
Name: ST_KeyFloat
Location: /Game/Game/Data/Structs/

Members:
  - Key (Name)
  - Value (Float)
```

#### ST_CooldownOverrideEntry
```
Si n'existe pas, créer:

Blueprint → Structure
Name: ST_CooldownOverrideEntry
Location: /Game/Game/Data/Structs/

Members:
  - AbilityId (GameplayTag)
  - NewCooldown (Float)
```

#### ST_ParamOverrideEntry
```
Si n'existe pas, créer:

Blueprint → Structure  
Name: ST_ParamOverrideEntry
Location: /Game/Game/Data/Structs/

Members:
  - AbilityId (GameplayTag)
  - Params (Array<ST_KeyFloat>)
```

**✅ Résultat:** GameState peut utiliser ces structs pour le balancing

---

### 2️⃣ CORRIGER GI_SpaceLeague (20 min)

**Localisation:** `/Game/Game/Core/GI_SpaceLeague`

**Problème actuel:** Pas de fonction pour récupérer les data

**Actions:**
1. Ouvrir GI_SpaceLeague
2. Vérifier que ces variables existent:
   - ✅ LegendRegistry (object)
   - ✅ SelectedLegendId (byte)
   - ✅ SelectedSkin (object)

3. Créer fonction **GetSelectedLegendData**

**Graph:**
```
Function: GetSelectedLegendData
Return Type: DA_LegendBase (Object Reference)

Nodes:
1. [FunctionEntry] GetSelectedLegendData

2. [VariableGet] LegendRegistry
   ↓
3. [IsValid] LegendRegistry
   ↓ (True)
4. [CallFunction] Get Legend By ID
   - Target: LegendRegistry
   - Input: SelectedLegendId (from variable)
   ↓
5. [FunctionResult] Return Value
   - Return Node: Legend Data

(False from IsValid):
6. [Print String] "ERROR: LegendRegistry is null!"
7. [FunctionResult] Return Value: None
```

4. Créer fonction **SetSelectedLegend**

**Graph:**
```
Function: SetSelectedLegend
Inputs:
  - LegendId (Byte)
  - Skin (DA_LegendSkin, object)

Nodes:
1. [FunctionEntry] SetSelectedLegend
   - LegendId
   - Skin
   ↓
2. [VariableSet] SelectedLegendId
   - Value: LegendId (input)
   ↓
3. [VariableSet] SelectedSkin
   - Value: Skin (input)
   ↓
4. [Print String] "Legend selected: {0}"
   - Format: Conv_ByteToString(LegendId)
```

**✅ Résultat:** GameInstance peut stocker et récupérer le choix du joueur

---

### 3️⃣ CORRIGER BP_Paradoxe (30 min)

**Localisation:** `/Game/Legends/BP_Paradoxe`

**Problème actuel:** Utilise DataTable au lieu de DA_LegendBase

**Actions:**

#### 3.1 Supprimer anciennes variables
```
À SUPPRIMER:
❌ DatatableLegends (DataTable)
❌ ChoosenLegendData (struct)
```

#### 3.2 Ajouter nouvelle variable
```
À AJOUTER:
✅ LegendData
   Type: DA_LegendBase (Object Reference)
   Category: Legend
   Editable: False
   Replicated: Replicated
```

#### 3.3 Modifier fonction SetLegendAndData

**AVANT (ancien code avec DataTable):**
```
Probablement quelque chose comme:
1. Get Row from DataTable
2. Break struct
3. Set variables
```

**APRÈS (nouveau code avec DA_LegendBase):**
```
Function: SetLegendAndData
Inputs:
  - NewLegendData (DA_LegendBase)

Graph:
1. [FunctionEntry] SetLegendAndData
   - NewLegendData
   ↓
2. [VariableSet] LegendData
   - Value: NewLegendData
   ↓
3. [IsValid] LegendData
   ↓ (True)
4. [VariableGet] LegendData
5. [BreakStruct/GetProperty] Abilities
   - Target: LegendData
   - Property: Abilities (TMap)
   ↓
6. [ForEachLoop] Abilities Map
   For each entry (Slot → Ability):
   
   a. [Switch on Enum] Slot
      Case Passive:
        → [VariableSet] S_Passif = Ability
      Case Skill:
        → [VariableSet] S_FirstSpell = Ability
      Case Ultimate:
        → [VariableSet] S_Ult = Ability
   ↓
7. [CallFunction] SetSkinHUD
   ↓
8. [Print String] "Legend data set: {0}"
   - LegendData.DisplayName
```

**✅ Résultat:** BP_Paradoxe utilise le nouveau système

---

### 4️⃣ CRÉER BP_GameMode_SpaceLeague (30 min)

**Localisation:** `/Game/Game/Gameplay/Gamemodes/BP_GameMode_SpaceLeague`

**Actions:**
1. Content Browser → RMB → Blueprint Class
2. Parent Class: Game Mode Base
3. Name: BP_GameMode_SpaceLeague
4. Save to: `/Game/Game/Gameplay/Gamemodes/`

**Class Defaults (Details Panel):**
```
Classes:
  Default Pawn Class: None
  Player Controller Class: BPPC_Paradoxe
  Game State Class: BP_GameState_SpaceLeague
  Player State Class: PlayerState (pour l'instant)
  HUD Class: None
  Spectator Class: Default
```

**Variables:**
```
✅ SpawnPoints
   Type: Array<Actor>
   Category: Spawning
   Editable: True

✅ NextSpawnIndex
   Type: Integer
   Category: Spawning
   Default: 0
   Private: True
```

**Fonctions à créer:**

#### ChoosePlayerStart (Override)
```
Function: ChoosePlayerStart (Override from GameModeBase)
Inputs:
  - Player (Controller)
Return: Actor (PlayerStart)

Graph:
1. [FunctionEntry] ChoosePlayerStart
   ↓
2. [VariableGet] SpawnPoints
3. [Array] Length
   ↓
4. [Branch] Length == 0?
   ↓ (True)
5. [GetAllActorsOfClass] PlayerStart
6. [VariableSet] SpawnPoints = Found Actors
   ↓ (False - continue)
7. [VariableGet] NextSpawnIndex
8. [VariableGet] SpawnPoints
9. [Array] Length
10. [Math] NextSpawnIndex >= Length?
    ↓ (True)
11. [VariableSet] NextSpawnIndex = 0
    ↓
12. [Array] GET SpawnPoints[NextSpawnIndex]
    ↓
13. [Math] NextSpawnIndex + 1
14. [VariableSet] NextSpawnIndex
    ↓
15. [FunctionResult] Return: PlayerStart
```

#### SpawnDefaultPawnFor (Override)
```
Function: SpawnDefaultPawnFor (Override from GameModeBase)
Inputs:
  - NewPlayer (Controller)
  - StartSpot (Actor)
Return: Pawn

Graph:
1. [FunctionEntry] SpawnDefaultPawnFor
   ↓
2. [Cast] NewPlayer → BPPC_Paradoxe
   ↓ (Success)
3. [VariableGet] SelectedCharacter
   ↓
4. [IsValid] SelectedCharacter
   ↓ (False)
5. [GetGameInstance] 
6. [Cast] → GI_SpaceLeague
7. [CallFunction] GetSelectedLegendData
   ↓
8. [IsValid] Legend Data
   ↓ (True)
9. [GetProperty] CharacterClass
   - Target: Legend Data
   ↓
10. [VariableSet] SelectedCharacter (temp)
    ↓
11. [GetActorTransform] StartSpot
    ↓
12. [SpawnActor] SelectedCharacter
    - Transform: from StartSpot
    - Owner: NewPlayer
    ↓
13. [Cast] Spawned → BP_Paradoxe
    ↓
14. [CallFunction] SetLegendAndData
    - Target: Spawned Pawn
    - Input: Legend Data (from step 7)
    ↓
15. [FunctionResult] Return: Pawn
```

**✅ Résultat:** GameMode spawn le bon personnage selon choix

---

### 5️⃣ CONFIGURER LA MAP DE JEU (15 min)

**Localisation:** `/Game/Levels/FloatingIsland` (ou ta map principale)

**Actions:**

1. Ouvrir le Level
2. Placer 4 PlayerStart dans la map
   - Position: Espacés de 500-1000 units
   - Rotation: Vers le centre de la map
   - Tag: Aucun (ou "Team1", "Team2" pour teams)

3. World Settings (Toolbar → Settings → World Settings)
   ```
   Game Mode:
     Game Mode Override: BP_GameMode_SpaceLeague
     Default Pawn Class: None (laissé vide)
   ```

4. Sauvegarder le level

**✅ Résultat:** Level prêt pour spawn multi-joueur

---

### 6️⃣ VÉRIFIER DA_LegendRegistry (10 min)

**Localisation:** `/Game/Game/Data/Registry/DA_LegendRegistry_Main`

**Actions:**

1. Ouvrir DA_LegendRegistry_Main
2. Vérifier qu'il contient un array/map de legends
3. Vérifier que les 2-4 legends sont référencées:
   - DA_LegendBase_Raijin
   - DA_LegendBase_Keplar
   - (Senna, Jhin si créés)

**Si structure incorrecte:**
```
Variables attendues:
  - Legends (Array<DA_LegendBase>) ou
  - LegendsMap (TMap<Byte, DA_LegendBase>)
```

**Si absente, créer:**
```
Function: GetLegendByID
Inputs:
  - LegendId (Byte)
Return: DA_LegendBase

Graph:
1. FOR EACH Legend in Legends:
   a. IF Legend.LegendId == Input LegendId:
      Return Legend
2. Return None
```

**✅ Résultat:** Registry peut trouver legends par ID

---

## ✅ CHECKLIST CORRECTIONS

Coche au fur et à mesure :

- [ ] **1. Structs vérifiés/créés**
  - [ ] ST_KeyFloat
  - [ ] ST_CooldownOverrideEntry
  - [ ] ST_ParamOverrideEntry

- [ ] **2. GI_SpaceLeague corrigé**
  - [ ] Variables existent
  - [ ] GetSelectedLegendData() créée
  - [ ] SetSelectedLegend() créée

- [ ] **3. BP_Paradoxe corrigé**
  - [ ] DatatableLegends supprimé
  - [ ] LegendData ajouté
  - [ ] SetLegendAndData() modifiée
  - [ ] Replication configurée

- [ ] **4. BP_GameMode créé**
  - [ ] Blueprint créé
  - [ ] Class Defaults configurées
  - [ ] ChoosePlayerStart() créée
  - [ ] SpawnDefaultPawnFor() créée

- [ ] **5. Map configurée**
  - [ ] 4 PlayerStarts placés
  - [ ] GameMode Override set
  - [ ] Level sauvegardé

- [ ] **6. Registry vérifié**
  - [ ] DA_LegendRegistry_Main ouvert
  - [ ] Legends présentes
  - [ ] GetLegendByID() existe

---

## 🧪 TEST DE VALIDATION

**Une fois toutes les corrections faites:**

1. Lancer PIE (Play In Editor)
2. Ouvrir Output Log
3. Vérifier aucune erreur
4. Si erreurs → Les corriger avant de continuer

**Test spawn manuel:**
```
1. Ouvrir BP_GameMode_SpaceLeague
2. Aller dans Class Defaults
3. Default Pawn Class: BP_ParadoxeRaijin (temporaire pour test)
4. Lancer PIE
5. Vérifier qu'un personnage Raijin spawn
6. Si oui → ✅ Base correcte !
7. Remettre Default Pawn Class: None
```

---

## 📝 NOTES IMPORTANTES

### Si GameInstance ne se charge pas

**Problème:** GI_SpaceLeague non utilisé par le projet

**Solution:**
```
1. Project Settings → Maps & Modes
2. Game Instance Class: GI_SpaceLeague
3. Restart Editor
```

### Si DA_LegendRegistry_Main ne se trouve pas

**Solution:**
```
1. Content Browser → Search: "Registry"
2. Si absent, créer nouveau DataAsset:
   - Parent: PrimaryDataAsset
   - Name: DA_LegendRegistry_Main
   - Location: /Game/Game/Data/Registry/
```

### Si PlayerStarts invisibles

**Solution:**
```
1. Viewport → Show → Navigation
2. Les PlayerStarts apparaissent en bleu
3. Utiliser "P" pour placer rapidement
```

---

## ⏭️ APRÈS CES CORRECTIONS

**Une fois toutes les corrections validées:**

1. ✅ Ton système a une base saine
2. ✅ Tu peux commencer le PLAN_MULTI_COMPLET_A_Z
3. ✅ Commence par la Phase 1 du plan complet

**Prochaine étape:** Phase 2 du plan complet (GameMode/PlayerState)

---

## 💡 CONSEILS

**Sauvegarde AVANT de modifier:**
```
1. File → Save All
2. Git commit -m "Avant corrections base"
3. Fais les modifs
4. Test
5. Si OK → Git commit -m "Corrections base OK"
6. Si problème → Git revert
```

**Teste après CHAQUE modification:**
```
Pas besoin de tout finir d'un coup.
Fais 1 correction → Compile → Test → Next
```

**Demande de l'aide si bloqué:**
```
N'hésite pas à revenir me voir à chaque étape
si tu rencontres un problème !
```

---

**Prêt ? Go ! 🚀**
