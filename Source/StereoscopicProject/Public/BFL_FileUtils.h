#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BFL_FileUtils.generated.h"

UCLASS()
class STEREOSCOPICPROJECT_API UBFL_FileUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Returns all files in a directory matching the given extension.
	 * @param DirectoryPath  Absolute path to scan (e.g. "H:/VideoRoot/")
	 * @param Extension      File extension filter including dot (e.g. ".mp4"). Empty = all files.
	 * @param OutFiles       Full paths of found files
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FileUtils")
	static TArray<FString> GetFilesInDirectory(const FString& DirectoryPath, const FString& Extension);

	/**
	 * Extract just the filename (without extension) from a full path.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FileUtils")
	static FString GetFileNameFromPath(const FString& FilePath);
};
