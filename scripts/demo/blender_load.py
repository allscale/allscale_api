# adapt this path as needed
PATH = "D:\\allscale_build_api_msvc2017\\tutorials\\"

import bpy
def set_shadeless():
    for mat in bpy.data.materials:
        mat.use_shadeless = True

def reset_blend():
    for scene in bpy.data.scenes:
        for obj in scene.objects:
            scene.objects.unlink(obj)
    for bpy_data_iter in (
        bpy.data.objects,
        bpy.data.meshes,
        bpy.data.lamps,
        bpy.data.cameras,
    ):
        for id_data in bpy_data_iter:
            bpy_data_iter.remove(id_data)

def load_step(s):
    global PATH
    reset_blend()
    bpy.ops.import_scene.obj(filepath=PATH+'step{:03d}.obj'.format(s))
    set_shadeless()

