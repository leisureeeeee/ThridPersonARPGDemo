"""Create the minimal masked lightsaber blade material. Never overwrite assets."""
import unreal


def create_blade_material(package_path='/Game/Weapons/Lightsaber', name='M_LightsaberBlade'):
    if unreal.WidgetService.is_pie_running():
        raise RuntimeError('Stop PIE before creating material assets.')
    path = package_path + '/' + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        raise RuntimeError('Refusing to overwrite existing material: ' + path)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, package_path, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError('Failed to create material.')
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
    material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property('two_sided', True)
    library = unreal.MaterialEditingLibrary
    color = library.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -600, -160)
    color.set_editor_property('parameter_name', 'BladeColor')
    color.set_editor_property('default_value', unreal.LinearColor(0.1, 0.65, 1.0, 1.0))
    intensity = library.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -600, 20)
    intensity.set_editor_property('parameter_name', 'Intensity')
    intensity.set_editor_property('default_value', 20.0)
    enabled = library.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -600, 200)
    enabled.set_editor_property('parameter_name', 'BladeEnabled')
    enabled.set_editor_property('default_value', 0.0)
    emission = library.create_material_expression(material, unreal.MaterialExpressionMultiply, -340, -100)
    gated_emission = library.create_material_expression(material, unreal.MaterialExpressionMultiply, -100, -100)
    connections = [
        library.connect_material_expressions(color, '', emission, 'A'),
        library.connect_material_expressions(intensity, '', emission, 'B'),
        library.connect_material_expressions(emission, '', gated_emission, 'A'),
        library.connect_material_expressions(enabled, '', gated_emission, 'B'),
        library.connect_material_property(gated_emission, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR),
        library.connect_material_property(enabled, '', unreal.MaterialProperty.MP_OPACITY_MASK),
    ]
    if not all(connections):
        raise RuntimeError('Material wiring failed; inspect the new asset before continuing.')
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, False):
        raise RuntimeError('Failed to save material.')
    return path
