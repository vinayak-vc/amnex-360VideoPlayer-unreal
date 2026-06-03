#include "PointCloudLoaderWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "PointCloudLoaderActor.h"
#include "Kismet/GameplayStatics.h"

void UPointCloudLoaderWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (LoadButton)
        LoadButton->OnClicked.AddDynamic(this, &UPointCloudLoaderWidget::OnLoadClicked);

    if (PathInput)
        PathInput->SetText(FText::FromString(TEXT("C:/UnrealProject/model/Tikal-13.ply")));

    if (StatusText)
        StatusText->SetText(FText::FromString(TEXT("Enter path and press Load")));
}

void UPointCloudLoaderWidget::OnLoadClicked()
{
    if (!PathInput) return;

    FString Path = PathInput->GetText().ToString().TrimStartAndEnd();
    if (Path.IsEmpty())
    {
        if (StatusText)
            StatusText->SetText(FText::FromString(TEXT("Path cannot be empty.")));
        return;
    }

    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APointCloudLoaderActor::StaticClass(), Found);

    if (Found.Num() == 0)
    {
        if (StatusText)
            StatusText->SetText(FText::FromString(TEXT("No PointCloudLoaderActor found in level.")));
        return;
    }

    APointCloudLoaderActor* Loader = Cast<APointCloudLoaderActor>(Found[0]);
    if (Loader)
    {
        if (StatusText)
            StatusText->SetText(FText::FromString(FString::Printf(TEXT("Loading: %s"), *Path)));
        Loader->LoadFromPath(Path);
    }
}
