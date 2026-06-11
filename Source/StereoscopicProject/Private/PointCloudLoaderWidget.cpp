#include "PointCloudLoaderWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "PointCloudLoaderActor.h"
#include "Kismet/GameplayStatics.h"
#include "DisplayClusterRootActor.h"
#include "Components/SceneComponent.h"

// OS file dialog is editor/Development-only (DesktopPlatform isn't linked into
// Shipping builds). Guard so the module still compiles in Shipping.
#if !UE_BUILD_SHIPPING
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"
#endif

void UPointCloudLoaderWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//if(LoadButton) LoadButton->OnClicked.AddDynamic(this, &UPointCloudLoaderWidget::OnLoadClicked);

	if(SetPositionCameraButton) SetPositionCameraButton->OnClicked.AddDynamic(this, &UPointCloudLoaderWidget::OnSetCameraClicked);

	if(BrowseButton) BrowseButton->OnClicked.AddDynamic(this, &UPointCloudLoaderWidget::OnBrowseClicked);

	if(PathInput) PathInput->SetText(FText::FromString(TEXT("C:/UnrealProject/model/Tikal-13.ply")));

	if(StatusText) StatusText->SetText(FText::FromString(TEXT("Enter path and press Load")));

	// Default IPD values
	if(LeftCamera) LeftCamera->SetText(FText::FromString(TEXT("-3.5")));
	if(RightCamera) RightCamera->SetText(FText::FromString(TEXT("3.5")));
	if(StatusText_1) StatusText_1->SetText(FText::FromString(TEXT("Camera: L=-3.5  R=3.5")));
}

void UPointCloudLoaderWidget::OnLoadClicked()
{
	if(!PathInput) return;
	
	FString Path = PathInput->GetText().ToString().TrimStartAndEnd();
	if(Path.IsEmpty())
	{
		if(StatusText) StatusText->SetText(FText::FromString(TEXT("Path cannot be empty.")));
		return;
	}

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APointCloudLoaderActor::StaticClass(), Found);
	
	if(Found.Num() == 0)
	{
		if(StatusText) StatusText->SetText(FText::FromString(TEXT("No PointCloudLoaderActor found in level.")));
		return;
	}

	APointCloudLoaderActor* Loader = Cast<APointCloudLoaderActor>(Found[0]);
	if(Loader)
	{
		if(StatusText) StatusText->SetText(FText::FromString(FString::Printf(TEXT("Loading: %s"), *Path)));
	
		if(Path.EndsWith(TEXT(".pak"), ESearchCase::IgnoreCase))
		{
			// Large-cloud path: mount external pak + auto-derive the cloud asset,
			// cluster-synced across all nDisplay nodes. Empty asset/mount args
			// -> derive asset from pak, use pak's baked mount point.
			Loader->RequestLoadPak(Path, FString(), FString());
		}
		else
		{
			// Small dev clouds (.ply/.las) loaded in-memory on this node only.
			Loader->LoadFromPath(Path);
		}
	}
}

void UPointCloudLoaderWidget::OnSetCameraClicked()
{
	if(!LeftCamera || !RightCamera) return;

	const float LeftY = FCString::Atof(*LeftCamera->GetText().ToString());
	const float RightY = FCString::Atof(*RightCamera->GetText().ToString());

	// Find the DisplayClusterRootActor in the level
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADisplayClusterRootActor::StaticClass(), Found);

	if(Found.Num() == 0)
	{
		if(StatusText_1) StatusText_1->SetText(FText::FromString(TEXT("No nDisplay actor found.")));
		return;
	}

	AActor* RootActor = Found[0];

	// Find camera xform components by name (matches xform IDs in .ndisplay)
	// Component names: Camera_Left1, Camera_Right1 (set in the .ndisplay xforms section)
	bool bFoundLeft = false, bFoundRight = false;

	TArray<USceneComponent*> Components;
	RootActor->GetRootComponent()->GetChildrenComponents(true, Components);

	for(USceneComponent* Comp : Components)
	{
		const FString Name = Comp->GetName();

		if(Name.Contains(TEXT("Camera_Left")))
		{
			FVector Loc = Comp->GetRelativeLocation();
			Loc.Y = LeftY;
			Comp->SetRelativeLocation(Loc);
			bFoundLeft = true;
		}
		else if(Name.Contains(TEXT("Camera_Right")))
		{
			FVector Loc = Comp->GetRelativeLocation();
			Loc.Y = RightY;
			Comp->SetRelativeLocation(Loc);
			bFoundRight = true;
		}
	}

	FString Msg = FString::Printf(
		TEXT("Camera: L=%.2f  R=%.2f  [%s%s]"),
		LeftY, RightY,
		bFoundLeft ? TEXT("L✓") : TEXT("L✗"),
		bFoundRight ? TEXT(" R✓") : TEXT(" R✗")
	);

	if(StatusText_1) StatusText_1->SetText(FText::FromString(Msg));
}

void UPointCloudLoaderWidget::OnBrowseClicked()
{
#if !UE_BUILD_SHIPPING
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if(!DesktopPlatform) return;

	// Parent the dialog to the main window if Slate is up.
	const void* ParentWindowHandle = nullptr;
	if(FSlateApplication::IsInitialized())
	{
		TSharedPtr<SWindow> MainWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
		if(MainWindow.IsValid() && MainWindow->GetNativeWindow().IsValid())
		{
			ParentWindowHandle = MainWindow->GetNativeWindow()->GetOSWindowHandle();
		}
	}

	TArray<FString> OutFiles;
	const bool      bPicked = DesktopPlatform->OpenFileDialog(
		ParentWindowHandle,
		TEXT("Select Point Cloud Pak"),
		TEXT(""), // default path
		TEXT(""), // default file
		TEXT("Point Cloud Pak (*.pak)|*.pak"),
		EFileDialogFlags::None,
		OutFiles);

	if(bPicked && OutFiles.Num() > 0 && PathInput)
	{
		PathInput->SetText(FText::FromString(OutFiles[0]));
		if(StatusText) StatusText->SetText(FText::FromString(TEXT("Pak selected. Press Load.")));
	}
#else
	// Shipping: no OS dialog. Type/paste the path into PathInput manually.
	if(StatusText) StatusText->SetText(FText::FromString(TEXT("Browse unavailable in Shipping — type the path.")));
#endif
}
