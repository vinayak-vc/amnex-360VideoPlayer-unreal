#include "PointCloudLoaderWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "DisplayClusterRootActor.h"
#include "Components/SceneComponent.h"
#include "PFConvert.h"
#include "PFConvertPanel.h"
#include "PFPointCloudActor.h"

#include "Framework/Application/SlateApplication.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsHWrapper.h"
#include <commdlg.h>
#include "Windows/HideWindowsPlatformTypes.h"
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
	if (!PathInput) return;

	FString Path = PathInput->GetText().ToString().TrimStartAndEnd();
	if (Path.IsEmpty())
	{
		if (StatusText) StatusText->SetText(FText::FromString(TEXT("Path cannot be empty.")));
		return;
	}

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APFPointCloudActor::StaticClass(), Found);
	if (Found.Num() == 0)
	{
		if (StatusText) StatusText->SetText(FText::FromString(TEXT("No PFPointCloudActor in level.")));
		return;
	}

	APFPointCloudActor* PFActor = Cast<APFPointCloudActor>(Found[0]);
	if (!PFActor) return;

	const FString CacheDir = FPFConvert::GetCacheDirFor(Path);
	const bool bConverted = FPFConvert::IsConverted(CacheDir);
	UE_LOG(LogTemp, Warning, TEXT("[LoadPanel] Path=%s | CacheDir=%s | IsConverted=%d"), *Path, *CacheDir, bConverted ? 1 : 0);

	if (!bConverted)
	{
		if (StatusText) StatusText->SetText(FText::FromString(
			TEXT("Model not converted — press Convert in the panel.")));
		PFActor->ShowConvertPanel();
		if (PFActor->ConvertPanel)
		{
			PFActor->ConvertPanel->SetStatusText(TEXT("Model not converted — press Convert in the panel."));
			PFActor->ConvertPanel->SetSourcePath(Path);
		}
		return;
	}

	if (StatusText) StatusText->SetText(FText::FromString(FString::Printf(TEXT("Loading: %s"), *Path)));
	PFActor->LoadPointCloudFile(Path);
	//RemoveFromParent();
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
#if PLATFORM_WINDOWS
	// Windows native file dialog — works in Editor and packaged builds alike.
	HWND ParentHwnd = nullptr;
	if(FSlateApplication::IsInitialized())
	{
		TSharedPtr<SWindow> TopWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
		if(TopWindow.IsValid() && TopWindow->GetNativeWindow().IsValid())
		{
			ParentHwnd = static_cast<HWND>(TopWindow->GetNativeWindow()->GetOSWindowHandle());
		}
	}

	wchar_t szFile[MAX_PATH] = { 0 };
	OPENFILENAMEW ofn       = {};
	ofn.lStructSize         = sizeof(ofn);
	ofn.hwndOwner           = ParentHwnd;
	ofn.lpstrFile           = szFile;
	ofn.nMaxFile            = MAX_PATH;
	ofn.lpstrFilter         = L"Point Cloud Files\0*.ply;*.las;*.laz;*.e57\0All Files\0*.*\0";
	ofn.Flags               = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if(GetOpenFileNameW(&ofn) && PathInput)
	{
		PathInput->SetText(FText::FromString(FString(szFile)));
		if(StatusText) StatusText->SetText(FText::FromString(TEXT("File selected. Press Load.")));
	}
#else
	if(StatusText) StatusText->SetText(FText::FromString(TEXT("Browse unavailable on this platform — type the path.")));
#endif
}

