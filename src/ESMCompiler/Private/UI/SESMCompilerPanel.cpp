#include "SESMCompilerPanel.h"

#include "../Unreal/ESMExportService.h"

#include "Async/Async.h"
#include "DesktopPlatformModule.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

namespace
{

const FLinearColor PanelTextColor(0.92F, 0.92F, 0.92F);
const FLinearColor SuccessColor(0.10F, 0.72F, 0.26F);
const FLinearColor ErrorColor(0.80F, 0.12F, 0.12F);

float StageProgress(EESMCompilerProgressStage Stage)
{
    switch (Stage)
    {
    case EESMCompilerProgressStage::ReadingWorld:
        return 0.20F;
    case EESMCompilerProgressStage::BuildingCellGrid:
        return 0.40F;
    case EESMCompilerProgressStage::BuildingLandscape:
        return 0.65F;
    case EESMCompilerProgressStage::WritingPlugin:
        return 0.90F;
    }

    return 0.0F;
}

} // namespace

void SESMCompilerPanel::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(0.025F, 0.025F, 0.025F, 1.0F))
        .Padding(16.0F)
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot().AutoHeight().Padding(0.0F, 0.0F, 0.0F, 5.0F)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Output")))
                .ColorAndOpacity(PanelTextColor)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0F, 0.0F, 0.0F, 14.0F)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0F).Padding(0.0F, 0.0F, 8.0F, 0.0F)
                [
                    SAssignNew(OutputPathTextBox, SEditableTextBox)
                    .OnTextChanged(this, &SESMCompilerPanel::OnOutputPathChanged)
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Browse")))
                    .OnClicked(this, &SESMCompilerPanel::BrowseOutput)
                ]
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(0.0F, 0.0F, 0.0F, 5.0F)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Plugin metadata")))
                .ColorAndOpacity(PanelTextColor)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0F, 0.0F, 0.0F, 14.0F)
            [
                SAssignNew(PluginNameTextBox, SEditableTextBox)
                .HintText(FText::FromString(TEXT("Name")))
                .OnTextChanged(this, &SESMCompilerPanel::OnPluginNameChanged)
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(0.0F, 0.0F, 0.0F, 5.0F)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Masters")))
                .ColorAndOpacity(PanelTextColor)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0F, 0.0F, 0.0F, 6.0F)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(0.0F, 0.0F, 8.0F, 0.0F)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Browse .esm")))
                    .OnClicked(this, &SESMCompilerPanel::BrowseMasters)
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Remove selected")))
                    .OnClicked(this, &SESMCompilerPanel::RemoveSelectedMaster)
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(8.0F, 0.0F, 0.0F, 0.0F)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Move up")))
                    .OnClicked_Lambda([this]() { return MoveSelectedMaster(-1); })
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(8.0F, 0.0F, 0.0F, 0.0F)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Move down")))
                    .OnClicked_Lambda([this]() { return MoveSelectedMaster(1); })
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0F, 0.0F, 0.0F, 14.0F)
            [
                SNew(SBox)
                .HeightOverride(110.0F)
                [
                    SAssignNew(MasterListView, SListView<FMasterItem>)
                    .ListItemsSource(&MasterFiles)
                    .OnGenerateRow(this, &SESMCompilerPanel::GenerateMasterRow)
                    .SelectionMode(ESelectionMode::Single)
                ]
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(0.0F, 0.0F, 0.0F, 5.0F)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("World export")))
                .ColorAndOpacity(PanelTextColor)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0F, 0.0F, 0.0F, 14.0F)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Source: Current Editor World")))
                .ColorAndOpacity(PanelTextColor)
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(0.0F, 0.0F, 0.0F, 7.0F)
            [
                SNew(SProgressBar)
                .Percent_Lambda([this]() { return TOptional<float>(ProgressValue); })
                .FillColorAndOpacity_Lambda([this]() { return bStatusIsError ? ErrorColor : SuccessColor; })
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0F, 0.0F, 0.0F, 12.0F)
            [
                SNew(STextBlock)
                .Text_Lambda([this]() { return FText::FromString(StatusMessage); })
                .ColorAndOpacity_Lambda([this]()
                {
                    return bStatusIsError ? FSlateColor(ErrorColor) : FSlateColor(PanelTextColor);
                })
            ]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Compile to ESM")))
                .IsEnabled_Lambda([this]() { return !bCompiling; })
                .OnClicked(this, &SESMCompilerPanel::CompileToEsm)
            ]
        ]
    ];
}

FReply SESMCompilerPanel::BrowseOutput()
{
    TArray<FString> selectedFiles;
    const FString defaultPath = OutputPath.IsEmpty() ? FPaths::ProjectDir() : FPaths::GetPath(OutputPath);
    if (FDesktopPlatformModule::Get()->SaveFileDialog(
            nullptr,
            TEXT("Choose ESM output"),
            defaultPath,
            PluginName.IsEmpty() ? TEXT("NewWorld.esm") : PluginName + TEXT(".esm"),
            TEXT("Skyrim master (*.esm)|*.esm"),
            EFileDialogFlags::None,
            selectedFiles) && selectedFiles.Num() == 1)
    {
        OutputPath = selectedFiles[0];
        OutputPathTextBox->SetText(FText::FromString(OutputPath));
    }
    return FReply::Handled();
}

FReply SESMCompilerPanel::BrowseMasters()
{
    TArray<FString> selectedFiles;
    if (FDesktopPlatformModule::Get()->OpenFileDialog(
            nullptr,
            TEXT("Select ESM master files"),
            FPaths::ProjectDir(),
            FString(),
            TEXT("Skyrim masters (*.esm)|*.esm"),
            EFileDialogFlags::Multiple,
            selectedFiles))
    {
        for (const FString& file : selectedFiles)
        {
            const bool alreadySelected = MasterFiles.ContainsByPredicate(
                [&file](const FMasterItem& item)
                {
                    return item.IsValid() && item->Equals(file, ESearchCase::IgnoreCase);
                });
            if (!alreadySelected)
            {
                MasterFiles.Add(MakeShared<FString>(file));
            }
        }
        MasterListView->RequestListRefresh();
    }
    return FReply::Handled();
}

FReply SESMCompilerPanel::RemoveSelectedMaster()
{
    const TArray<FMasterItem> selected = MasterListView->GetSelectedItems();
    for (const FMasterItem& item : selected)
    {
        MasterFiles.Remove(item);
    }
    MasterListView->RequestListRefresh();
    return FReply::Handled();
}

FReply SESMCompilerPanel::MoveSelectedMaster(int32 Direction)
{
    const TArray<FMasterItem> selected = MasterListView->GetSelectedItems();
    if (selected.Num() != 1)
    {
        return FReply::Handled();
    }

    const int32 currentIndex = MasterFiles.IndexOfByKey(selected[0]);
    const int32 newIndex = currentIndex + Direction;
    if (currentIndex != INDEX_NONE && MasterFiles.IsValidIndex(newIndex))
    {
        MasterFiles.Swap(currentIndex, newIndex);
        MasterListView->RequestListRefresh();
        MasterListView->SetSelection(selected[0]);
    }
    return FReply::Handled();
}

FReply SESMCompilerPanel::CompileToEsm()
{
    OutputPath = OutputPath.TrimStartAndEnd();
    PluginName = PluginName.TrimStartAndEnd();
    if (OutputPath.IsEmpty() || PluginName.IsEmpty())
    {
        SetStatus(TEXT("Choose an output path and enter a plugin name."), 0.0F, true);
        return FReply::Handled();
    }
    if (GEditor == nullptr)
    {
        SetStatus(TEXT("The Unreal Editor is not available."), 0.0F, true);
        return FReply::Handled();
    }

    UWorld* editorWorld = GEditor->GetEditorWorldContext().World();
    SetStatus(TEXT("Compiling: reading UE5 world..."), StageProgress(EESMCompilerProgressStage::ReadingWorld), false);

    world_ir::World world;
    FString readError;
    if (!FESMExportService::ReadWorld(editorWorld, PluginName, world, readError))
    {
        SetStatus(FString::Printf(TEXT("Compilation failed: %s"), *readError), 0.0F, true);
        return FReply::Handled();
    }

    FESMCompileRequest request;
    request.OutputPath = OutputPath;
    request.PluginName = PluginName;
    for (const FMasterItem& item : MasterFiles)
    {
        if (item.IsValid())
        {
            request.MasterPaths.Add(*item);
        }
    }

    bCompiling = true;
    const TWeakPtr<SESMCompilerPanel> weakPanel =
        StaticCastSharedRef<SESMCompilerPanel>(AsShared());
    Async(EAsyncExecution::ThreadPool,
          [weakPanel, world = MoveTemp(world), request = MoveTemp(request)]() mutable
          {
              const FESMCompilerProgressCallback progress =
                  [weakPanel](EESMCompilerProgressStage stage, const FText& message)
                  {
                      AsyncTask(ENamedThreads::GameThread,
                                [weakPanel, stage, message]()
                                {
                                    if (const TSharedPtr<SESMCompilerPanel> panel = weakPanel.Pin())
                                    {
                                        panel->UpdateProgressFromWorker(static_cast<uint8>(stage), message);
                                    }
                                });
                  };

              const FESMCompileResult result = FESMExportService::CompileWorld(world, request, progress);
              AsyncTask(ENamedThreads::GameThread,
                        [weakPanel, result]()
                        {
                            if (const TSharedPtr<SESMCompilerPanel> panel = weakPanel.Pin())
                            {
                                panel->bCompiling = false;
                                panel->SetStatus(
                                    result.Message,
                                    result.bSucceeded ? 1.0F : panel->ProgressValue,
                                    !result.bSucceeded);
                            }
                        });
          });

    return FReply::Handled();
}

void SESMCompilerPanel::OnOutputPathChanged(const FText& Value)
{
    OutputPath = Value.ToString();
}

void SESMCompilerPanel::OnPluginNameChanged(const FText& Value)
{
    PluginName = Value.ToString();
}

TSharedRef<ITableRow> SESMCompilerPanel::GenerateMasterRow(
    FMasterItem Item,
    const TSharedRef<STableViewBase>& OwnerTable) const
{
    return SNew(STableRow<FMasterItem>, OwnerTable)
    [
        SNew(STextBlock)
        .Text(FText::FromString(Item.IsValid() ? FPaths::GetCleanFilename(*Item) : FString()))
        .ColorAndOpacity(PanelTextColor)
    ];
}

void SESMCompilerPanel::SetStatus(const FString& Message, float Progress, bool bIsError)
{
    StatusMessage = Message;
    ProgressValue = FMath::Clamp(Progress, 0.0F, 1.0F);
    bStatusIsError = bIsError;
}

void SESMCompilerPanel::UpdateProgressFromWorker(uint8 Stage, const FText& Message)
{
    const EESMCompilerProgressStage progressStage = static_cast<EESMCompilerProgressStage>(Stage);
    SetStatus(Message.ToString(), StageProgress(progressStage), false);
}
