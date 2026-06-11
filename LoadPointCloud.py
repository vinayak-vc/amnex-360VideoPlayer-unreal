import unreal
import sys

def load_point_cloud():
    file_path = "C:/UnrealProject/model/Tikal-13.ply"
    
    unreal.log("Attempting to load point cloud from: " + file_path)
    
    if not hasattr(unreal, 'LidarPointCloud'):
        unreal.log_error("LidarPointCloud module not found! Ensure the plugin is enabled.")
        return

    try:
        # The Blueprint library function is available, but might return a tuple due to latent outputs
        result = unreal.LidarPointCloudBlueprintLibrary.create_point_cloud_from_file(file_path, False)
        
        # If it returns a tuple, the point cloud is usually the last element
        if isinstance(result, tuple):
            point_cloud = result[-1]
        else:
            point_cloud = result

    except Exception as e:
        unreal.log_error("Failed to load point cloud using python. " + str(e))
        return

    if not point_cloud:
        unreal.log_error("Returned point cloud is None.")
        return

    unreal.log("Point cloud loaded successfully!")

    # Try to create a new level
    try:
        if hasattr(unreal, 'EditorLevelLibrary'):
            unreal.EditorLevelLibrary.new_level("/Game/PointCloudLevel")
            unreal.log("Created new level /Game/PointCloudLevel")
        else:
            unreal.log_warning("EditorLevelLibrary not found. Spawning in the current level instead.")
    except Exception as e:
        unreal.log_warning("Could not create new level. Spawning in current level. " + str(e))

    # Spawn the LidarPointCloudActor
    try:
        pc_actor_class = unreal.LidarPointCloudActor
        pc_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(pc_actor_class, unreal.Vector(0,0,0), unreal.Rotator(0,0,0))
        
        if pc_actor:
            pc_component = pc_actor.get_point_cloud_component()
            if pc_component:
                pc_component.set_point_cloud(point_cloud)
                unreal.log("Successfully assigned point cloud to actor in the level!")
            else:
                unreal.log_error("LidarPointCloudActor does not have a Point Cloud Component.")
        else:
            unreal.log_error("Failed to spawn LidarPointCloudActor.")
    except Exception as e:
        unreal.log_error("Error spawning actor: " + str(e))

if __name__ == "__main__":
    load_point_cloud()
