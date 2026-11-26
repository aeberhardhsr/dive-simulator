// Expand the pinned glTF compression without changing geometry or materials.
// Build-only meshoptimizer and Basis Universal are fetched by prepare_beech_asset.py.
import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { createRequire } from 'node:module';
import { deflateSync } from 'node:zlib';
import { createHash } from 'node:crypto';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const require = createRequire(import.meta.url);
const decoderDir = path.join(root, 'Tools/.downloads/tree-decoders');
const pinned = JSON.parse(await fs.readFile(path.join(root,'Documentation/VEGETATION_BEECH_SOURCE.json'),'utf8'));
for (const item of pinned.build_only_decoders) {
    const data = await fs.readFile(path.join(decoderDir,item.file));
    if (createHash('sha256').update(data).digest('hex') !== item.sha256) throw Error('Decoder checksum mismatch');
}
const { MeshoptDecoder } = await import('./.downloads/tree-decoders/meshopt_decoder.mjs');
await MeshoptDecoder.ready;
const basis = await require(path.join(decoderDir, 'basis_transcoder.cjs'))({wasmBinary:await fs.readFile(path.join(decoderDir,'basis_transcoder.wasm'))});
basis.initializeBasis();

function crc32(data) {
    let crc = 0xffffffff;
    for (const value of data) {
        crc ^= value;
        for (let k=0;k<8;k++) crc = (crc>>>1) ^ ((crc&1)?0xedb88320:0);
    }
    return (crc^0xffffffff)>>>0;
}
function png(width,height,rgba) {
    function chunk(type,data) {
        const body=Buffer.concat([Buffer.from(type),data]), header=Buffer.alloc(4), footer=Buffer.alloc(4);
        header.writeUInt32BE(data.length);footer.writeUInt32BE(crc32(body));return Buffer.concat([header,body,footer]);
    }
    const header=Buffer.alloc(13);header.writeUInt32BE(width);header.writeUInt32BE(height,4);header[8]=8;header[9]=6;
    const scan=Buffer.alloc((width*4+1)*height);
    for(let y=0;y<height;y++) Buffer.from(rgba.buffer,rgba.byteOffset+y*width*4,width*4).copy(scan,y*(width*4+1)+1);
    return Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]),chunk('IHDR',header),chunk('IDAT',deflateSync(scan)),chunk('IEND',Buffer.alloc(0))]);
}

const folder=path.join(root,'Art/Vegetation/european_beech');
for (const source of pinned.files) {
    const level=source.lod, input=await fs.readFile(path.join(folder,`lod${level}.glb`));
    if(createHash('sha256').update(input).digest('hex')!==source.sha256) throw Error('GLB checksum mismatch');
    if(input.readUInt32LE(0)!==0x46546c67||input.readUInt32LE(4)!==2) throw Error('Not glTF 2');
    const jsonLength=input.readUInt32LE(12), model=JSON.parse(input.subarray(20,20+jsonLength));
    const bin=input.subarray(28+jsonLength);
    const expanded=[];
    for(const view of model.bufferViews) {
        const ext=view.extensions?.EXT_meshopt_compression;
        let bytes;
        if(ext) {
            if(ext.buffer!==0) throw Error('Unexpected compression buffer');
            bytes=Buffer.alloc(ext.count*ext.byteStride);
            MeshoptDecoder.decodeGltfBuffer(bytes,ext.count,ext.byteStride,bin.subarray(ext.byteOffset??0,(ext.byteOffset??0)+ext.byteLength),ext.mode,ext.filter);
            delete view.extensions;
        } else {
            if(view.buffer!==0) throw Error('Unexpected source buffer');
            bytes=Buffer.from(bin.subarray(view.byteOffset??0,(view.byteOffset??0)+view.byteLength));
        }
        expanded.push(bytes);
    }
    for(const texture of model.textures) {
        if(texture.extensions?.KHR_texture_basisu) { texture.source=texture.extensions.KHR_texture_basisu.source;delete texture.extensions; }
    }
    for(const [index,image] of model.images.entries()) {
        if(image.mimeType!=='image/ktx2') throw Error('Unexpected image format');
        const file=new basis.KTX2File(expanded[image.bufferView]);
        try {
            const w=file.getWidth(),h=file.getHeight();
            if(!w||!h||file.getLayers()>1||file.getFaces()>1||!file.startTranscoding()) throw Error('Invalid KTX2');
            const rgba=new Uint8Array(file.getImageTranscodedSizeInBytes(0,0,0,13));
            if(rgba.length!==w*h*4||!file.transcodeImage(rgba,0,0,0,13,0,-1,-1)) throw Error('RGBA transcoding failed');
            const name=`lod${level}_texture${index}.png`;
            await fs.writeFile(path.join(folder,name),png(w,h,rgba));
            delete image.bufferView;image.mimeType='image/png';image.uri=name;
        } finally { file.close();file.delete(); }
    }
    let offset=0;const parts=[];
    for(const [i,view] of model.bufferViews.entries()) {
        const bytes=expanded[i];view.buffer=0;view.byteOffset=offset;view.byteLength=bytes.length;
        parts.push(bytes);offset+=bytes.length;
        if(offset%4) { const pad=Buffer.alloc(4-offset%4);parts.push(pad);offset+=pad.length; }
    }
    model.buffers=[{byteLength:offset,uri:`lod${level}.bin`}];
    for(const key of ['extensionsRequired','extensionsUsed']) model[key]=model[key].filter(x=>!['EXT_meshopt_compression','KHR_texture_basisu'].includes(x));
    await fs.writeFile(path.join(folder,`lod${level}.bin`),Buffer.concat(parts));
    await fs.writeFile(path.join(folder,`lod${level}.gltf`),JSON.stringify(model));
    console.log('BEECH_DECODE_PASS',level,source.triangles);
}
