#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PointCloudLoaderWidget.generated.h"

UCLASS()
class STEREOSCOPICPROJECT_API UPointCloudLoaderWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Bind these in the Blueprint widget by matching names exactly
    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* PathInput;

    UPROPERTY(meta = (BindWidget))
    class UButton* LoadButton;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* StatusText;

protected:
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void OnLoadClicked();
};
