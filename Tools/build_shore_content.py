"""Original procedural gravel/soil surface for the shore meshes."""
import unreal as u

def build_shore_material(name="M_Shore", imagery=False):
    lib=u.MaterialEditingLibrary
    path="/Game/Diving/Materials/"+name
    material=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(
        name,"/Game/Diving/Materials",u.Material,u.MaterialFactoryNew())
    if not material: raise RuntimeError("Cannot create shore material")
    for expression in list(lib.get_material_expressions(material)):
        lib.delete_material_expression(material,expression)
    if lib.get_num_material_expressions(material): raise RuntimeError("Uncleared shore graph")
    material.set_editor_property("two_sided",True)
    material.set_editor_property("tangent_space_normal",False)
    position=lib.create_material_expression(material,u.MaterialExpressionWorldPosition)
    normal=lib.create_material_expression(material,u.MaterialExpressionVertexNormalWS)
    color=lib.create_material_expression(material,u.MaterialExpressionVertexColor)
    photo=None
    if imagery:
        photo=lib.create_material_expression(material,u.MaterialExpressionTextureSampleParameter2D)
        photo.set_editor_property("parameter_name","Ortho")
        photo.set_editor_property("texture",u.load_asset("/Game/Diving/Textures/T_Ortho_weesen_strandbad"))
    # Cell size 4 cm, rounded individual pebbles and antialiased distant detail.
    shared=r"""
    float2 q=P.xy/4;
    float2 cell=floor(q), f=frac(q), nearest=0;
    float first=100, second=100, tint=0;
    for(int y=-1;y<=1;y++) for(int x=-1;x<=1;x++) {
        float2 id=cell+float2(x,y);
        // Integer mixing remains irregular at large georeferenced positions;
        // a float sine hash loses precision there and produces visible grids.
        uint2 uid=uint2(int2(id));
        uint hx=uid.x*1664525u+uid.y*1013904223u;
        hx^=hx>>16; hx*=2246822519u; hx^=hx>>13;
        uint hy=hx*3266489917u+374761393u; hy^=hy>>16;
        float2 h=float2(hx&65535u,hy&65535u)/65535.;
        float2 delta=float2(x,y)+h-f;
        float d=dot(delta,delta);
        if(d<first) { second=first; first=d; nearest=delta; tint=h.x; }
        else second=min(second,d);
    }
    float footprint=max(length(ddx(q)),length(ddy(q)));
    float detail=1-smoothstep(.25,1.8,footprint);
    float seam=smoothstep(.012,.055+footprint*.08,second-first);
    float radius=.24+.16*tint;
    float pebble=(1-smoothstep(radius-.035,radius+.035+footprint*.1,sqrt(first)))*seam;
    float coarse=.88+.12*sin(P.x*.004+sin(P.y*.006))*cos(P.y*.003);
    float wet=1-smoothstep(-40,120,P.z);
    float land=smoothstep(0,120,P.z);
    """
    def custom(code,kind,prop):
        node=lib.create_material_expression(material,u.MaterialExpressionCustom)
        node.set_editor_property("code",shared+code)
        node.set_editor_property("output_type",kind)
        inputs=[]
        for name in (("P","N","C","Photo") if imagery else ("P","N","C")):
            item=u.CustomInput(); item.set_editor_property("input_name",name); inputs.append(item)
        node.set_editor_property("inputs",inputs)
        for source,name in ((position,"P"),(normal,"N"),(color,"C")):
            lib.connect_material_expressions(source,"",node,name)
        if imagery: lib.connect_material_expressions(photo,"RGB",node,"Photo")
        lib.connect_material_property(node,"",prop)
    custom("float grain=frac(sin(dot(floor(P.xy*2),float2(27.19,13.37)))*23421.37); float variation=lerp(.7,lerp(.42+.08*grain,.65+.45*tint,pebble),detail); float3 soil=C.rgb*variation*coarse*lerp(1,.55,wet); "+
           ("return lerp(soil,Photo,land);" if imagery else "return soil;"),
           u.CustomMaterialOutputType.CMOT_FLOAT3,u.MaterialProperty.MP_BASE_COLOR)
    custom("float3 slope=float3(nearest*pebble*detail*2,0); "+("slope*=1-land; " if imagery else "")+"return normalize(N-slope+N*dot(slope,N));",
           u.CustomMaterialOutputType.CMOT_FLOAT3,u.MaterialProperty.MP_NORMAL)
    custom("return lerp(.88,.48,wet);",u.CustomMaterialOutputType.CMOT_FLOAT1,u.MaterialProperty.MP_ROUGHNESS)
    lib.recompile_material(material)
    if not u.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError("Could not save shore material")
    u.log("DIVE_SHORE_CONTENT_OK")

if __name__=="__main__":
    if u.EditorAssetLibrary.does_asset_exist("/Game/Diving/Textures/T_Ortho_weesen_strandbad"):
        build_shore_material("M_ShorePhoto",True)
    else:
        build_shore_material()
