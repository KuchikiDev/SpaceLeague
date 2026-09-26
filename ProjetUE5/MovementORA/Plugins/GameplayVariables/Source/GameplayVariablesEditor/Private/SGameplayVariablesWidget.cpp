#include "SGameplayVariablesWidget.h"

#include "GameplayVariablesSettings.h"
#include "HAL/PlatformProcess.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorDelegates.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SGameplayVariablesWidget"

void SGameplayVariablesWidget::Construct(const FArguments& InArgs)
{
	BuildTabs();
	ActiveTab = EGameplayVariablesTab::Match;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsArgs.bAllowSearch = true;
	DetailsArgs.bSearchInitialKeyFocus = false;
	DetailsArgs.bUpdatesFromSelection = false;

	DetailsView = PropertyEditorModule.CreateDetailView(DetailsArgs);
	DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &SGameplayVariablesWidget::IsPropertyVisible));
	DetailsView->SetObject(GetMutableDefault<UGameplayVariablesSettings>());
	DetailsView->OnFinishedChangingProperties().AddSP(this, &SGameplayVariablesWidget::HandleFinishedChangingProperties);

	StatusText = LOCTEXT("InitialStatus", "Les modifications sont enregistrees dans DefaultGame.ini.");
	TSharedRef<SWrapBox> TabBar = SNew(SWrapBox).UseAllottedSize(true);
	for (const FGameplayVariablesTabDefinition& Tab : Tabs)
	{
		TabBar->AddSlot()
		.Padding(0.0f, 0.0f, 6.0f, 6.0f)
		[
			BuildTabButton(Tab)
		];
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(10.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("HeaderTitle", "Centre de reglages gameplay"))
				.Font(FAppStyle::Get().GetFontStyle("HeadingSmall"))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("HeaderDescription", "Reglez le lancement du match et les principaux systemes depuis une seule fenetre. Chaque modification est sauvegardee automatiquement."))
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				TabBar
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				.Padding(12.0f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						SNew(SCheckBox)
						.IsChecked(this, &SGameplayVariablesWidget::GetIntroEnabledState)
						.OnCheckStateChanged(this, &SGameplayVariablesWidget::HandleIntroEnabledChanged)
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("IntroToggleTitle", "Cinematique d'introduction"))
							.Font(FAppStyle::Get().GetFontStyle("NormalFontBold"))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 3.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(this, &SGameplayVariablesWidget::GetIntroToggleSummary)
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				SNew(STextBlock)
				.Text(this, &SGameplayVariablesWidget::GetActiveTabDescription)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				DetailsView.ToSharedRef()
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 10.0f, 0.0f, 8.0f)
			[
				SNew(SSeparator)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(this, &SGameplayVariablesWidget::GetStatusText)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("OpenConfigButton", "Ouvrir config"))
					.OnClicked(this, &SGameplayVariablesWidget::HandleOpenConfigClicked)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("ReloadButton", "Recharger"))
					.OnClicked(this, &SGameplayVariablesWidget::HandleReloadClicked)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("SaveButton", "Sauvegarder"))
					.OnClicked(this, &SGameplayVariablesWidget::HandleSaveClicked)
				]
			]
		]
	];
}

FReply SGameplayVariablesWidget::HandleSaveClicked()
{
	UGameplayVariablesSettings* Settings = GetMutableDefault<UGameplayVariablesSettings>();
	Settings->SaveConfig();
	Settings->TryUpdateDefaultConfigFile();
	StatusText = FText::Format(
		LOCTEXT("SavedStatus", "Sauvegarde manuelle a {0}."),
		FText::FromString(FDateTime::Now().ToString(TEXT("%H:%M:%S"))));
	return FReply::Handled();
}

FReply SGameplayVariablesWidget::HandleReloadClicked()
{
	UGameplayVariablesSettings* Settings = GetMutableDefault<UGameplayVariablesSettings>();
	Settings->ReloadConfig();
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(Settings, true);
	}

	StatusText = LOCTEXT("ReloadedStatus", "Valeurs rechargees depuis la config.");
	return FReply::Handled();
}

FReply SGameplayVariablesWidget::HandleOpenConfigClicked()
{
	const FString ConfigFile = GetMutableDefault<UGameplayVariablesSettings>()->GetDefaultConfigFilename();
	FPlatformProcess::LaunchFileInDefaultExternalApplication(*ConfigFile);
	return FReply::Handled();
}

FReply SGameplayVariablesWidget::HandleTabClicked(EGameplayVariablesTab TabId)
{
	ActiveTab = TabId;
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(GetMutableDefault<UGameplayVariablesSettings>(), true);
	}

	return FReply::Handled();
}

ECheckBoxState SGameplayVariablesWidget::GetIntroEnabledState() const
{
	return GetDefault<UGameplayVariablesSettings>()->bEnablePreMatchIntro
		? ECheckBoxState::Checked
		: ECheckBoxState::Unchecked;
}

void SGameplayVariablesWidget::HandleIntroEnabledChanged(const ECheckBoxState NewState)
{
	UGameplayVariablesSettings* Settings = GetMutableDefault<UGameplayVariablesSettings>();
	Settings->bEnablePreMatchIntro = NewState == ECheckBoxState::Checked;
	Settings->SaveConfig();
	Settings->TryUpdateDefaultConfigFile();
	StatusText = Settings->bEnablePreMatchIntro
		? LOCTEXT("IntroEnabledStatus", "Cinematique d'introduction activee et sauvegardee.")
		: LOCTEXT("IntroDisabledStatus", "Cinematique d'introduction desactivee et sauvegardee.");
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(Settings, true);
	}
}

FText SGameplayVariablesWidget::GetIntroToggleSummary() const
{
	return GetDefault<UGameplayVariablesSettings>()->bEnablePreMatchIntro
		? LOCTEXT("IntroEnabledSummary", "Activee : vue aerienne, zoom, fondu, puis decompte.")
		: LOCTEXT("IntroDisabledSummary", "Desactivee : le test commence directement par le decompte.");
}

void SGameplayVariablesWidget::HandleFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	UGameplayVariablesSettings* Settings = GetMutableDefault<UGameplayVariablesSettings>();
	Settings->SaveConfig();
	Settings->TryUpdateDefaultConfigFile();
	StatusText = FText::Format(
		LOCTEXT("AutoSavedStatus", "Auto-save de la variable a {0}."),
		FText::FromString(FDateTime::Now().ToString(TEXT("%H:%M:%S"))));
}

void SGameplayVariablesWidget::BuildTabs()
{
	Tabs.Reset();
	Tabs.Add({
		EGameplayVariablesTab::Match,
		LOCTEXT("MatchTab", "Match"),
		LOCTEXT("MatchTabDescription", "Lancement du match : cinematique d'introduction, duree de la presentation, decompte 3-2-1 et regles de prison."),
		{ TEXT("Match") }
	});
	Tabs.Add({
		EGameplayVariablesTab::Grapple,
		LOCTEXT("GrappleTab", "Grappin"),
		LOCTEXT("GrappleTabDescription", "Reglages simples du grab. Commence par ces trois variables : Force horizontale, Force hauteur et Puissance distance."),
		{ TEXT("Grappin|Simple") }
	});
	Tabs.Add({
		EGameplayVariablesTab::GrappleAdvanced,
		LOCTEXT("GrappleAdvancedTab", "Grappin avance"),
		LOCTEXT("GrappleAdvancedTabDescription", "Reglages detailles du grab. A utiliser seulement pour corriger un cas precis apres le reglage simple."),
		{ TEXT("Grappin") }
	});
	Tabs.Add({
		EGameplayVariablesTab::Movement,
		LOCTEXT("MovementTab", "Deplacement"),
		LOCTEXT("MovementTabDescription", "Reglages du joueur : deplacement, dash et saut."),
		{ TEXT("Deplacement"), TEXT("Dash"), TEXT("Saut") }
	});
	Tabs.Add({
		EGameplayVariablesTab::Ball,
		LOCTEXT("BallTab", "Balle"),
		LOCTEXT("BallTabDescription", "Reglages lies a la balle, aux tirs et aux trajectoires enroulees."),
		{ TEXT("Balle") }
	});
	Tabs.Add({
		EGameplayVariablesTab::Camera,
		LOCTEXT("CameraTab", "Camera"),
		LOCTEXT("CameraTabDescription", "Reglages de camera, zoom, rotation et ressenti visuel."),
		{ TEXT("Camera") }
	});
	Tabs.Add({
		EGameplayVariablesTab::General,
		LOCTEXT("GeneralTab", "General"),
		LOCTEXT("GeneralTabDescription", "Variables globales ou transverses qui ne rentrent pas dans un systeme precis."),
		{ TEXT("General"), TEXT("Combat") }
	});
	Tabs.Add({
		EGameplayVariablesTab::All,
		LOCTEXT("AllTab", "Tout"),
		LOCTEXT("AllTabDescription", "Vue complete de toutes les variables, comme avant."),
		{}
	});
}

TSharedRef<SWidget> SGameplayVariablesWidget::BuildTabButton(const FGameplayVariablesTabDefinition& Tab)
{
	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "Button")
		.OnClicked(this, &SGameplayVariablesWidget::HandleTabClicked, Tab.Id)
		[
			SNew(STextBlock)
			.Text(Tab.Label)
			.ColorAndOpacity(this, &SGameplayVariablesWidget::GetTabTextColor, Tab.Id)
		];
}

bool SGameplayVariablesWidget::IsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	if (ActiveTab == EGameplayVariablesTab::Match
		&& PropertyAndParent.Property.GetFName() == GET_MEMBER_NAME_CHECKED(UGameplayVariablesSettings, bEnablePreMatchIntro))
	{
		return false;
	}

	const FGameplayVariablesTabDefinition* ActiveDefinition = FindActiveTab();
	if (!ActiveDefinition || ActiveDefinition->CategoryPrefixes.Num() == 0)
	{
		return true;
	}

	const FString Category = PropertyAndParent.Property.GetMetaData(TEXT("Category"));
	for (const FString& Prefix : ActiveDefinition->CategoryPrefixes)
	{
		if (Category == Prefix || Category.StartsWith(Prefix + TEXT("|")) || Category.StartsWith(Prefix + TEXT(" -")))
		{
			return true;
		}
	}

	return false;
}

const SGameplayVariablesWidget::FGameplayVariablesTabDefinition* SGameplayVariablesWidget::FindActiveTab() const
{
	for (const FGameplayVariablesTabDefinition& Tab : Tabs)
	{
		if (Tab.Id == ActiveTab)
		{
			return &Tab;
		}
	}

	return nullptr;
}

FText SGameplayVariablesWidget::GetActiveTabDescription() const
{
	if (const FGameplayVariablesTabDefinition* ActiveDefinition = FindActiveTab())
	{
		return ActiveDefinition->Description;
	}

	return FText::GetEmpty();
}

FSlateColor SGameplayVariablesWidget::GetTabTextColor(EGameplayVariablesTab TabId) const
{
	return ActiveTab == TabId ? FSlateColor(FLinearColor::White) : FSlateColor::UseForeground();
}

FText SGameplayVariablesWidget::GetStatusText() const
{
	return StatusText;
}

#undef LOCTEXT_NAMESPACE
