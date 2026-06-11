import unreal
import sys

with open("C:/UnrealProject/StereroScopicProject/dump_api.txt", "w") as f:
    if hasattr(unreal, "LidarPointCloudBlueprintLibrary"):
        f.write("LidarPointCloudBlueprintLibrary attributes:\n")
        for attr in dir(unreal.LidarPointCloudBlueprintLibrary):
            f.write(attr + "\n")
    else:
        f.write("No LidarPointCloudBlueprintLibrary found.\n")

    if hasattr(unreal, "LidarPointCloud"):
        f.write("\nLidarPointCloud attributes:\n")
        for attr in dir(unreal.LidarPointCloud):
            f.write(attr + "\n")
    else:
        f.write("No LidarPointCloud found.\n")

sys.exit(0)
