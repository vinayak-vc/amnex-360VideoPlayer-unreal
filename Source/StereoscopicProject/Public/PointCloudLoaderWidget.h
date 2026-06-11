#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PointCloudLoaderWidget.generated.h"

UCLASS()
class STEREOSCOPICPROJECT_API UPointCloudLoaderWidget : public UUserWidget {
	GENERATED_BODY()

public:
	// Bind these in the Blueprint widget by matching names exactly
	UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "UI")
	class UEditableTextBox* PathInput;

	UPROPERTY(meta = (BindWidget))
	class UButton* LoadButton;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* StatusText;

	// Camera position controls — widget names must match exactly
	UPROPERTY(meta = (BindWidgetOptional))
	class UEditableTextBox* LeftCamera;

	UPROPERTY(meta = (BindWidgetOptional))
	class UEditableTextBox* RightCamera;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* SetPositionCameraButton;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* StatusText_1;

	// Optional "Browse..." button — opens an OS file dialog to pick a .pak and
	// fills PathInput. Add a UButton named "BrowseButton" in the UMG layout.
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* BrowseButton;

protected:
	virtual void NativeConstruct() override;

protected:
	UFUNCTION(BlueprintCallable, Category="Point Cloud")
	void OnLoadClicked();

private:
	UFUNCTION()
	void OnSetCameraClicked();

	UFUNCTION()
	void OnBrowseClicked();
};
