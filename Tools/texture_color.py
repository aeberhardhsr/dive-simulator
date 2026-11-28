"""Explicit source transfer functions for the pinned Poly Haven PNG maps."""
import unreal as u


def set_scan_source_encoding(texture, is_color):
    # UE ImageCore treats RGBA16 sources as linear even when the destination
    # texture has SRGB enabled. These diffuse PNGs carry sRGB ICC profiles.
    # Decode at build time; keep original 16-bit pixels and hardware sRGB reads.
    settings = texture.get_editor_property('source_color_settings')
    settings.set_editor_property('encoding_override', u.TextureSourceEncoding.TSE_S_RGB
                                 if is_color else u.TextureSourceEncoding.TSE_LINEAR)
    texture.set_editor_property('source_color_settings', settings)
    texture.set_editor_property('srgb', is_color)
