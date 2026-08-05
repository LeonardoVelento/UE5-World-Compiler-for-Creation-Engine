#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class SListViewBase;
template <typename ItemType>
class SListView;
class ITableRow;
class STableViewBase;

class SESMCompilerPanel final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SESMCompilerPanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    using FMasterItem = TSharedPtr<FString>;

    FReply BrowseOutput();
    FReply BrowseMasters();
    FReply RemoveSelectedMaster();
    FReply MoveSelectedMaster(int32 Direction);
    FReply CompileToEsm();

    void OnOutputPathChanged(const FText& Value);
    void OnPluginNameChanged(const FText& Value);
    TSharedRef<ITableRow> GenerateMasterRow(FMasterItem Item,
                                             const TSharedRef<STableViewBase>& OwnerTable) const;

    void SetStatus(const FString& Message, float Progress, bool bIsError);
    void UpdateProgressFromWorker(uint8 Stage, const FText& Message);

    FString OutputPath;
    FString PluginName;
    TArray<FMasterItem> MasterFiles;
    TSharedPtr<SEditableTextBox> OutputPathTextBox;
    TSharedPtr<SEditableTextBox> PluginNameTextBox;
    TSharedPtr<SListView<FMasterItem>> MasterListView;

    FString StatusMessage = TEXT("Ready.");
    float ProgressValue = 0.0F;
    bool bCompiling = false;
    bool bStatusIsError = false;
};
