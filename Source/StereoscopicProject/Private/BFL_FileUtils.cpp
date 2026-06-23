#include "BFL_FileUtils.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

TArray<FString> UBFL_FileUtils::GetFilesInDirectory(const FString& DirectoryPath, const FString& Extension)
{
	TArray<FString> OutFiles;

	// Normalize path separators
	FString NormalizedPath = FPaths::ConvertRelativePathToFull(DirectoryPath);
	FPaths::NormalizeDirectoryName(NormalizedPath);

	// Build search pattern
	FString SearchPattern = NormalizedPath / TEXT("*") + Extension;

	IFileManager::Get().FindFiles(OutFiles, *SearchPattern, true, false);

	// FindFiles returns filenames only — prepend directory to get full paths
	for (FString& File : OutFiles)
	{
		File = NormalizedPath / File;
	}

	return OutFiles;
}

FString UBFL_FileUtils::GetFileNameFromPath(const FString& FilePath)
{
	return FPaths::GetBaseFilename(FilePath);
}

FString UBFL_FileUtils::GetPFConvertPath()
{
#if WITH_EDITOR
	// In Editor, point to the source build directory
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../PointForge/build/Release/pfconvert.exe"));
#else
	// In packaged build, point to the bundled directory
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("PointForge/pfconvert.exe"));
#endif
}