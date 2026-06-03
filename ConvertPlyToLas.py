import sys
try:
    import numpy as np
    import laspy
    import open3d as o3d
except ImportError:
    print("Error: Missing required libraries.")
    print("Please install them by running: pip install open3d laspy")
    sys.exit(1)

def convert_ply_to_las(ply_path, las_path):
    print(f"Reading PLY file: {ply_path}...")
    # Read the PLY file using Open3D
    pcd = o3d.io.read_point_cloud(ply_path)
    points = np.asarray(pcd.points)
    
    if len(points) == 0:
        print("Error: Point cloud is empty or could not be read.")
        return
        
    print(f"Loaded {len(points)} points. Creating LAS file...")
    
    # Create a new LAS file (Format 2 supports color)
    header = laspy.LasHeader(point_format=2, version="1.2")
    header.offsets = np.min(points, axis=0)
    
    # Set the scale (precision). 0.001 = millimeter precision
    header.scales = np.array([0.001, 0.001, 0.001])
    
    las = laspy.LasData(header)
    
    las.x = points[:, 0]
    las.y = points[:, 1]
    las.z = points[:, 2]
    
    # If the PLY file has colors, transfer them to the LAS file
    if pcd.has_colors():
        print("Color data found, transferring colors...")
        # Open3D colors are float [0, 1], LAS expects 16-bit integers [0, 65535]
        colors = np.asarray(pcd.colors) * 65535.0
        las.red = colors[:, 0].astype(np.uint16)
        las.green = colors[:, 1].astype(np.uint16)
        las.blue = colors[:, 2].astype(np.uint16)
    else:
        print("No color data found in PLY.")
        
    print(f"Writing to {las_path}...")
    las.write(las_path)
    print(f"Successfully converted and saved to: {las_path}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python ConvertPlyToLas.py <input.ply> <output.las>")
        print(r"Example: python ConvertPlyToLas.py C:\UnrealProject\model\Tikal-13.ply C:\UnrealProject\model\Tikal-13.las")
    else:
        convert_ply_to_las(sys.argv[1], sys.argv[2])
