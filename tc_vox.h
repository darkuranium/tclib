/*
 * tc_vox.h: MagicaVoxel *.vox file loader.
 *
 * DEPENDS:
 * VERSION: 0.3.0 (2025-11-04)
 * LICENSE: CC0 & Boost (dual-licensed)
 * AUTHOR: Tim Čas
 * URL: https://github.com/darkuranium/tclib
 *
 * VERSION HISTORY:
 * 0.3.0    fix rotation handling to match the behaviour of MagicaVoxel (and presumably the intended behaviour of the file format)
 * 0.2.1    properly handle "simple" .vox files, i.e. those without nodes but only model(s)
 *          fix an issue with transforms not being properly reset when we finish iterating a node
 * 0.2.0    added rOBJ & IMAP chunk handling
 *          fixed parsing of MATL chunks in some files (it appears that some files use a material ID of 0, and others use 256 for the same thing)
 *          reworked a number of datastructures, transform-related logic is now based on tcvox_transform_t
 *          added iterator, transform, and bounds APIs, to ease the loading of large (multi-model) files
 * 0.1.0    initial public release
 *
 * TODOS:
 * - Program & usage samples
 * - Better handling of file versions other than 150 & 200
 * - Make the iterator API support animations
 * - Parse out common rOBJ types
 */

#ifndef TC_VOX_H_
#define TC_VOX_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifndef TC__STATIC_SIZE
#if __STDC_VERSION__ >= 199901
#define TC__STATIC_SIZE(N)  static N
#else
#define TC__STATIC_SIZE(N)  N
#endif
#endif

typedef enum tcvox_material_type
{
    TCVOX_MATERIAL_TYPE_UNKNOWN,
    TCVOX_MATERIAL_TYPE_DIFFUSE,// <default> (_diffuse?)
    TCVOX_MATERIAL_TYPE_METAL,  // _metal
    TCVOX_MATERIAL_TYPE_GLASS,  // _glass
    TCVOX_MATERIAL_TYPE_EMIT,   // _emit
    TCVOX_MATERIAL_TYPE_BLEND,  // _blend;  combines all of the above
    TCVOX_MATERIAL_TYPE_CLOUD,  // _media
} tcvox_material_type_t;

typedef enum tcvox_media_type
{
    TCVOX_MEDIA_TYPE_ABSORB,
    TCVOX_MEDIA_TYPE_SCATTER,
    TCVOX_MEDIA_TYPE_EMISSIVE,
    TCVOX_MEDIA_TYPE_SUBSURFACE,
} tcvox_media_type_t;

typedef struct tcvox_attr
{
    char* key;
    char* value;
} tcvox_attr_t;

typedef union tcvox_ivec3
{
    struct { int32_t x, y, z; };
    int32_t xyz[3];
} tcvox_ivec3_t;

typedef struct tcvox_rotation_axis
{
    uint8_t index : 2;
    uint8_t sign : 1;
    uint8_t : 5;
} tcvox_rotation_axis_t;

typedef union tcvox_rotation
{
    struct { tcvox_rotation_axis_t x, y, z; };
    tcvox_rotation_axis_t xyz[3];
} tcvox_rotation_t;

typedef struct tcvox_transform
{
    tcvox_rotation_t r;
    tcvox_ivec3_t t;
} tcvox_transform_t;

typedef struct tcvox_material
{
    tcvox_material_type_t type; // _type
    // roughness, index_of_refraction, and density seem to always be provided
    // (probably an artefact of the materials preserving hidden values)
    // METAL & GLASS:
    float roughness;            // _rough
    float index_of_refraction;  // 1 + _ior
    // METAL:
    float metalness;            // _metal
    float specular;             // _sp
    // GLASS:
    float alpha;                // 1 - _alpha [_trans?]
    // GLASS & CLOUD:
    tcvox_media_type_t media_type;  // 0=ABSORB, 1=SCATTER, 2=
    float scatter_phase;        // _g; used if media_type==SCATTER
    float density;              // _d   ; UI 0-100 is mapped to 0.0-0.1
    // EMIT:
    float emissiveness;         // _emit * pow(10, 1 + _flux) + _ldr

    struct
    {
        // EMIT:
        float emit, flux, ldr;
    } raw;
} tcvox_material_t;

typedef struct tcvox_palette
{
    uint32_t abgr[256];
    tcvox_material_t materials[256];
    char* names[256];
} tcvox_palette_t;

typedef struct tcvox_frame
{
    uint32_t nattrs;
    tcvox_attr_t* attrs;

    // extracted from `attrs`
    tcvox_transform_t transform;    // via r & t
    uint32_t index;     // frame index
} tcvox_frame_t;

typedef struct tcvox_voxel
{
    uint8_t xyz[3];
    uint8_t index;
} tcvox_voxel_t;

typedef struct tcvox_model
{
    uint32_t id;
    tcvox_ivec3_t size;

    uint32_t nvoxels;
    tcvox_voxel_t* voxels;
} tcvox_model_t;

typedef struct tcvox_layer
{
    uint32_t id;

    uint32_t nattrs;
    tcvox_attr_t* attrs;

    // extracted from `attrs`
    char* name;
    bool is_hidden;
    uint32_t color_abgr;
} tcvox_layer_t;

typedef enum tcvox_camera_mode
{
    TCVOX_CAMERA_MODE_UNKNOWN,
    TCVOX_CAMERA_MODE_PERSPECTIVE,  // _pers
} tcvox_camera_mode_t;

typedef struct tcvox_camera
{
    uint32_t id;

    uint32_t nattrs;
    tcvox_attr_t* attrs;

    // extracted from `attrs`
    tcvox_camera_mode_t mode;
    float focus[3];
    float angle[3];
    float radius;
    float frustum;
    float fov;
} tcvox_camera_t;

typedef struct tcvox_group_node
{
    uint32_t id;

    uint32_t nattrs;
    tcvox_attr_t* attrs;

    uint32_t nchildren;
    struct tcvox_transform_node** children;
} tcvox_group_node_t;

typedef struct tcvox_shape_node_model
{
    tcvox_model_t* model;

    uint32_t nattrs;
    tcvox_attr_t* attrs;

    // extracted from `attrs`
    uint32_t starting_frame;
} tcvox_shape_node_model_t;

typedef struct tcvox_shape_node
{
    uint32_t id;

    uint32_t nattrs;
    tcvox_attr_t* attrs;

    uint32_t nmodels;
    tcvox_shape_node_model_t* models;
} tcvox_shape_node_t;

typedef struct tcvox_transform_node
{
    uint32_t id;

    uint32_t nattrs;
    tcvox_attr_t* attrs;

    // extracted from `attrs`
    char* name;
    bool is_hidden;

    // only one of these two will be populated
    struct
    {
        tcvox_group_node_t* group;
        tcvox_shape_node_t* shape;
    } child;

    tcvox_layer_t* layer;

    uint32_t nframes;
    tcvox_frame_t* frames;
} tcvox_transform_node_t;

typedef enum tcvox_node_ref_type
{
    TCVOX_NODE_REF_TYPE_NONE = 0,
    TCVOX_NODE_REF_TYPE_GROUP = 1,
    TCVOX_NODE_REF_TYPE_TRANSFORM = 2,
    TCVOX_NODE_REF_TYPE_SHAPE = 3,
} tcvox_node_ref_type_t;

typedef struct tcvox_node_ref
{
    uint32_t index  : 30;
    uint32_t type   : 2;
} tcvox_node_ref_t;

typedef struct tcvox_object
{
    uint32_t nattrs;
    tcvox_attr_t* attrs;

    // extracted from `attrs`
    char* type;
} tcvox_object_t;

typedef struct tcvox_scene
{
    const char* error;

    tcvox_palette_t palette;

    uint32_t nmodels;
    tcvox_model_t* models;

    uint32_t nlayers;
    tcvox_layer_t* layers;

    uint32_t ncameras;
    tcvox_camera_t* cameras;

    uint32_t nobjects;
    tcvox_object_t* objects;

    /*
        `true` if file had no nodes whatsoever.
        Note that if it had at least 1 model, we'll create a single shape (`index[0]` & `shapes[0]`) containing all the models.
        This keeps the user access the same regardless of file layout.
     */
    bool no_nodes_in_file;
    struct
    {
        // root node sits at index[0]
        uint32_t nindex;
        tcvox_node_ref_t* index;

        uint32_t ngroups;
        tcvox_group_node_t* groups;

        uint32_t nshapes;
        tcvox_shape_node_t* shapes;

        uint32_t ntransforms;
        tcvox_transform_node_t* transforms;
    } nodes;
} tcvox_scene_t;

typedef struct tcvox_iter
{
    tcvox_scene_t* scene;
    tcvox_shape_node_t* shape;
    tcvox_transform_t transform;

    bool include_hidden : 1;
    bool done : 1;

// private data follows
    struct
    {
        uint32_t mem, len;
        struct tcvox_iter_stack_entry* ptr;
    } _stack;
    tcvox_shape_node_t* _pending_shape;
} tcvox_iter_t;

typedef enum tcvox_compute_bounds_flags
{
    TCVOX_BOUNDSFLG_INCLUDE_HIDDEN      = 1 << 0,
    TCVOX_BOUNDSFLG_POINT_CLOUD_MODE    = 1 << 1,
    TCVOX_BOUNDSFLG_IGNORE_EMPTY        = 1 << 2,
} tcvox_compute_bounds_flags_t;

// Provided publicly in case it's useful.
extern const tcvox_material_t tcvox_default_material_params;    // Default parameters if omitted.
extern const tcvox_material_t tcvox_default_material;           // Default material in palette.
extern const tcvox_palette_t tcvox_default_palette;

extern const tcvox_transform_t tcvox_transform_identity;

tcvox_scene_t* tcvox_load_memory(tcvox_scene_t* scene, const void* ptr, size_t length);
tcvox_scene_t* tcvox_load_fname(tcvox_scene_t* scene, const char* fname);
void tcvox_unload(const tcvox_scene_t* scene);

// Combine a transform of `parent` followed by `child`.
tcvox_transform_t tcvox_transform_combine(tcvox_transform_t parent, tcvox_transform_t child);
// Apply a transform in `frame` to `vector`.
tcvox_ivec3_t tcvox_transform_apply(tcvox_transform_t transform, tcvox_ivec3_t vector, bool apply_translation);
// Same as `tcvox_transform_apply`, but it works on voxel / point cloud coordinates. TODO: This needs a better name.
tcvox_ivec3_t tcvox_transform_apply_voxel_vec(tcvox_transform_t transform, tcvox_ivec3_t vector, bool apply_translation);
tcvox_ivec3_t tcvox_transform_apply_voxel(tcvox_transform_t transform, tcvox_voxel_t voxel, bool apply_translation);
// The returned matrix is row-major, i.e. mat3[row][col].
void tcvox_rotation_to_mat3(tcvox_rotation_t rotation, int8_t mat3[TC__STATIC_SIZE(3)][3]);
tcvox_rotation_t tcvox_rotation_combine(tcvox_rotation_t first, tcvox_rotation_t second);

tcvox_iter_t tcvox_scene_iter_shapes(tcvox_scene_t* scene, bool include_hidden);
bool tcvox_iter_next(tcvox_iter_t* it);
void tcvox_iter_finish(tcvox_iter_t* it);   // Only required to be called if we want to terminate the iterator before it ends.

// Helper functions, exposed because they might be useful for manual bounds calculation.
void tcvox_bounds_init(tcvox_ivec3_t bounds[TC__STATIC_SIZE(2)]);
void tcvox_bounds_update(tcvox_ivec3_t bounds[TC__STATIC_SIZE(2)], tcvox_ivec3_t v);
// Compute a bounding box of the entire scene (min/max in each axis). Note that this does *not* inspect voxel data, but merely uses the outer boxes of each model/node.
bool tcvox_scene_compute_bounds(tcvox_scene_t* scene, tcvox_ivec3_t bounds[TC__STATIC_SIZE(2)], tcvox_compute_bounds_flags_t flags);

#endif /* TC_VOX_H_ */


#ifdef TC_VOX_IMPLEMENTATION
#undef TC_VOX_IMPLEMENTATION

// Useful resources:
// http://paulbourke.net/dataformats/vox/
// https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef __cplusplus
    #if defined(_MSC_VER) || defined(__GNUC__)
        #define restrict __restrict
    #else
        #define restrict
    #endif
//#define TC__RESTRICT(T)     T restrict
#endif

#ifdef __cplusplus
#define TC__VOID_CAST(T,x)  static_cast<T>(x)
#else
#define TC__VOID_CAST(T,x)  (x)
#endif

#define TCVOX_TAG_(A,B,C,D) ((uint32_t)(A)<<0 | (uint32_t)(B)<<8 | (uint32_t)(C)<<16 | (uint32_t)(D)<<24)

struct tcvox_chunk_
{
    uint32_t tag;
    uint32_t nbytes_data;
    uint32_t nbytes_children;

    // derived
    uint32_t offset_data;
    uint32_t offset_children;
    uint32_t offset_end;
};

#define TCVOX_DEFAULT_MATERIAL_ \
    {   \
        .type = TCVOX_MATERIAL_TYPE_DIFFUSE,    \
        .roughness = 0.1f,  \
        .index_of_refraction = 0.3f,    \
        .density = 0.05f,   \
    }

const tcvox_material_t tcvox_default_material_params = {
    .type = TCVOX_MATERIAL_TYPE_DIFFUSE,
    // METAL & GLASS:
    .roughness = 0.0f,
    .index_of_refraction = 1.0f,
    // METAL:
    .metalness = 0.0f,
    .specular = 1.0f,
    // GLASS:
    .alpha = 1.0f,
    // GLASS & CLOUD:
    .media_type = TCVOX_MEDIA_TYPE_ABSORB,
    .scatter_phase = 0.0f,
    .density = 0.0f,
    // EMIT:
    .emissiveness = 0.0f,

    .raw = {
        // EMIT:
        .emit = 0.0f, .flux = 1.0f, .ldr = 0.0f,
    },
};
const tcvox_material_t tcvox_default_material = TCVOX_DEFAULT_MATERIAL_;
const tcvox_palette_t tcvox_default_palette = {{
    0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff,
    0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
    0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff,
    0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
    0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc,
    0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
    0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc,
    0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
    0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc,
    0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
    0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999,
    0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
    0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099,
    0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
    0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66,
    0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666,
    0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366,
    0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
    0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33,
    0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
    0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633,
    0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033,
    0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00,
    0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
    0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600,
    0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
    0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000,
    0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
    0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700,
    0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
    0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd,
    0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111
}, {
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
    TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_, TCVOX_DEFAULT_MATERIAL_,
}};

const tcvox_transform_t tcvox_transform_identity = {
    .r = {{
        {0, 0},
        {1, 0},
        {2, 0},
    }},
    .t = {{0, 0, 0}},
};

#define TCVOX_CHECK_(cond, msg) do { if(!(cond)) { error = (msg); goto error; } } while(0)
#define TCVOX_ALLOC_CHECK_(ptr, num, name)      \
    do {                                        \
        (ptr) = calloc((num), sizeof(*(ptr)));   \
        TCVOX_CHECK_((ptr), "Cannot allocate " name ": Out of memory"); \
    } while(0)
#define TCVOX_ALLOC_CHECK_NOINIT_(ptr, num, name)   \
    do {                                            \
        (ptr) = malloc((num) * sizeof(*(ptr)));     \
        TCVOX_CHECK_((ptr), "Cannot allocate " name ": Out of memory"); \
    } while(0)

static const char* tcvox_make_attr_(tcvox_attr_t* restrict attr, const char* key, size_t klen, const char* value, size_t vlen)
{
    const char* error = NULL;

    TCVOX_ALLOC_CHECK_(attr->key, klen + 1 + vlen + 1, "attribute");
    attr->value = attr->key + (klen + 1);

    memcpy(attr->key, key, klen);
    attr->key[klen] = 0;

    memcpy(attr->value, value, vlen);
    attr->value[vlen] = 0;

error:
    return error;
}
#define TCVOX_MAKE_ATTR_(attr)  do { if((error = tcvox_make_attr_(attr, key, klen, val, vlen))) goto error; } while(0)

static const char* tcvox_read_bytes_(void* restrict dst, size_t nbytes, const void* restrict ptr, size_t length, size_t offset)
{
    if(!(offset + nbytes <= length))
        return "Out of bounds read (truncated file?)";

    const uint8_t* restrict uptr = TC__VOID_CAST(const uint8_t* restrict,ptr);
    memcpy(dst, uptr + offset, nbytes);

    return NULL;
}
#define TCVOX_READ_BYTES_(dst, nbytes, offset)   do { if((error = tcvox_read_bytes_(dst, nbytes, ptr, length, offset))) goto error; } while(0)

static const char* tcvox_read_bytes_nocpy_(const uint8_t** restrict dst, size_t nbytes, const void* restrict ptr, size_t length, size_t offset)
{
    if(!(offset + nbytes <= length))
        return "Out of bounds read (truncated file?)";

    const uint8_t* restrict cptr = TC__VOID_CAST(const uint8_t* restrict,ptr);
    *dst = &cptr[offset];

    return NULL;
}
#define TCVOX_READ_BYTES_NOCPY_(dst, nbytes, offset)   do { if((error = tcvox_read_bytes_nocpy_(dst, nbytes, ptr, length, offset))) goto error; } while(0)

static const char* tcvox_read_32le_(uint32_t* restrict dst, const void* restrict ptr, size_t length, size_t offset)
{
    if(!(offset + sizeof(*dst) <= length))
        return "Out of bounds read (truncated file?)";

    const uint8_t* restrict uptr = TC__VOID_CAST(const uint8_t* restrict,ptr);
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    memcpy(dst, uptr + offset, sizeof(*dst));
#else   // unknown or BE
    *dst =    (uint32_t)uptr[offset+0] << 0
            | (uint32_t)uptr[offset+1] << 8
            | (uint32_t)uptr[offset+2] << 16
            | (uint32_t)uptr[offset+3] << 24;
#endif

    return NULL;
}
#define TCVOX_READ_32LE_(dst, offset)   do { if((error = tcvox_read_32le_(dst, ptr, length, offset))) goto error; } while(0)

static const char* tcvox_read_string_nocpy_(const char** restrict dst, uint32_t* restrict dstlen, const void* restrict ptr, size_t length, size_t offset)
{
    const char* error;

    TCVOX_READ_32LE_(dstlen, offset);
    if(!(offset + sizeof(*dstlen) + *dstlen <= length))
        return "Out of bounds read (truncated file?)";

    const char* restrict cptr = TC__VOID_CAST(const char* restrict,ptr);
    *dst = &cptr[offset + 1 * sizeof(uint32_t)];

error:
    return error;
}
#define TCVOX_READ_STRING_NOCPY_(dst, dstlen, offset)   do { if((error = tcvox_read_string_nocpy_(dst, dstlen, ptr, length, offset))) goto error; } while(0)

static const char* tcvox_read_chunk_header_(struct tcvox_chunk_* restrict chunk, const void* restrict ptr, size_t length, size_t offset)
{
    const char* error;

    TCVOX_READ_32LE_(&chunk->tag, offset + 0 * sizeof(uint32_t));
    TCVOX_READ_32LE_(&chunk->nbytes_data, offset + 1 * sizeof(uint32_t));
    TCVOX_READ_32LE_(&chunk->nbytes_children, offset + 2 * sizeof(uint32_t));

    chunk->offset_data = offset + 3 * sizeof(uint32_t);
    chunk->offset_children = chunk->offset_data + chunk->nbytes_data;
    chunk->offset_end = chunk->offset_children + chunk->nbytes_children;

error:
    return error;
}
#define TCVOX_READ_CHUNK_HEADER_(dst, offset)   do { if((error = tcvox_read_chunk_header_(dst, ptr, length, offset))) goto error; } while(0)

#define TCVOX_IS_KEY_(S)    (klen == sizeof(#S) - 1U && !memcmp(key, #S, sizeof(#S) - 1U))
#define TCVOX_IS_VAL_(S)    (vlen == sizeof(#S) - 1U && !memcmp(val, #S, sizeof(#S) - 1U))

// Main function.
static const char* tcvox_load_memory_(tcvox_scene_t* scene, const void* ptr, size_t length)
{
    const char* error;

    uint32_t tag;
    TCVOX_READ_32LE_(&tag, 0 * sizeof(uint32_t));
    TCVOX_CHECK_(tag == TCVOX_TAG_('V','O','X',' '), "Invalid VOX file (magic number mismatch)");

    // TODO: We probably want to relax this at some point.
    uint32_t version;
    TCVOX_READ_32LE_(&version, 1 * sizeof(uint32_t));
    TCVOX_CHECK_(version == 150 || version == 200, "Unknown VOX version");

    struct tcvox_chunk_ c_MAIN;
    TCVOX_READ_CHUNK_HEADER_(&c_MAIN, 2 * sizeof(uint32_t));
    TCVOX_CHECK_(c_MAIN.tag == TCVOX_TAG_('M','A','I','N'), "Wrong initial file chunk");
    TCVOX_CHECK_(c_MAIN.nbytes_data == 0, "Expected initial chunk to have no data");

    // First, iterate over chunks, gather various counts.
    uint32_t pack_nmodels = UINT32_MAX;
    for(uint32_t offset = c_MAIN.offset_children; offset < length;)
    {
        struct tcvox_chunk_ c;
        TCVOX_READ_CHUNK_HEADER_(&c, offset);
        switch(c.tag)
        {
        case TCVOX_TAG_('P','A','C','K'):
            TCVOX_CHECK_(c.nbytes_data == 1 * sizeof(uint32_t), "Incorrect PACK chunk length");
            TCVOX_READ_32LE_(&pack_nmodels, c.offset_data + 0 * sizeof(uint32_t));
            break;
        case TCVOX_TAG_('S','I','Z','E'):
            ++scene->nmodels;
            break;
        case TCVOX_TAG_('n','T','R','N'):
            ++scene->nodes.ntransforms;
            break;
        case TCVOX_TAG_('n','G','R','P'):
            ++scene->nodes.ngroups;
            break;
        case TCVOX_TAG_('n','S','H','P'):
            ++scene->nodes.nshapes;
            break;
        case TCVOX_TAG_('L','A','Y','R'):
            ++scene->nlayers;
            break;
        case TCVOX_TAG_('r','C','A','M'):
            ++scene->ncameras;
            break;
        case TCVOX_TAG_('r','O','B','J'):
            ++scene->nobjects;
            break;
        }
        offset = c.offset_end;
    }

    if(pack_nmodels != UINT32_MAX)
    {
        TCVOX_CHECK_(scene->nmodels == pack_nmodels, "Invalid number of specified models");
        //scene->nmodels = pack_nmodels;
    }

    TCVOX_ALLOC_CHECK_(scene->models, scene->nmodels, "models");
    TCVOX_ALLOC_CHECK_(scene->layers, scene->nlayers, "layers");
    TCVOX_ALLOC_CHECK_(scene->cameras, scene->ncameras, "cameras");
    TCVOX_ALLOC_CHECK_(scene->objects, scene->nobjects, "objects");

    scene->nodes.nindex = scene->nodes.ngroups + scene->nodes.nshapes + scene->nodes.ntransforms;
    TCVOX_ALLOC_CHECK_(scene->nodes.index, scene->nodes.nindex, "nodes index");
    TCVOX_ALLOC_CHECK_(scene->nodes.groups, scene->nodes.ngroups, "group nodes");
    TCVOX_ALLOC_CHECK_(scene->nodes.shapes, scene->nodes.nshapes, "shape nodes");
    TCVOX_ALLOC_CHECK_(scene->nodes.transforms, scene->nodes.ntransforms, "transform nodes");

    tcvox_model_t* model;
    tcvox_object_t* object;
    tcvox_group_node_t* group;
    tcvox_shape_node_t* shape;
    tcvox_transform_node_t* transform;

    scene->palette = tcvox_default_palette;

    // Populate node ID lookup table (`scene->nodes.index`).
    group = &scene->nodes.groups[0];
    shape = &scene->nodes.shapes[0];
    transform = &scene->nodes.transforms[0];
    for(uint32_t offset = c_MAIN.offset_children; offset < length;)
    {
        struct tcvox_chunk_ c;
        TCVOX_READ_CHUNK_HEADER_(&c, offset);

        switch(c.tag)
        {
        case TCVOX_TAG_('n','T','R','N'): {
            // We count above, so this should always pass
            assert(transform - scene->nodes.transforms < scene->nodes.ntransforms);

            uint32_t id;
            TCVOX_READ_32LE_(&id, c.offset_data + 0 * sizeof(uint32_t));

            TCVOX_CHECK_(id < scene->nodes.nindex, "nTRN ID out of bounds");
            TCVOX_CHECK_(!scene->nodes.index[id].type, "Duplicate node ID");
            scene->nodes.index[id].index = transform - scene->nodes.transforms;
            scene->nodes.index[id].type = TCVOX_NODE_REF_TYPE_TRANSFORM;

            ++transform;
            } break;

        case TCVOX_TAG_('n','G','R','P'): {
            // We count above, so this should always pass
            assert(group - scene->nodes.groups < scene->nodes.ngroups);

            uint32_t id;
            TCVOX_READ_32LE_(&id, c.offset_data + 0 * sizeof(uint32_t));

            TCVOX_CHECK_(id < scene->nodes.nindex, "nGRP ID out of bounds");
            TCVOX_CHECK_(!scene->nodes.index[id].type, "Duplicate node ID");
            scene->nodes.index[id].index = group - scene->nodes.groups;
            scene->nodes.index[id].type = TCVOX_NODE_REF_TYPE_GROUP;

            ++group;
            } break;

        case TCVOX_TAG_('n','S','H','P'): {
            // We count above, so this should always pass
            assert(shape - scene->nodes.shapes < scene->nodes.nshapes);

            uint32_t id;
            TCVOX_READ_32LE_(&id, c.offset_data + 0 * sizeof(uint32_t));

            TCVOX_CHECK_(id < scene->nodes.nindex, "nSHP ID out of bounds");
            TCVOX_CHECK_(!scene->nodes.index[id].type, "Duplicate node ID");
            scene->nodes.index[id].index = shape - scene->nodes.shapes;
            scene->nodes.index[id].type = TCVOX_NODE_REF_TYPE_SHAPE;

            ++shape;
            } break;

        default: break;
        }

        offset = c.offset_end;
    }

    // Handle IMAP chunks properly, by keeping track of a remapping table. The default table is a simple identity mapping.
    static const uint8_t imap_identity[256] = {
          0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
         16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
         32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
         48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
         64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
         80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
         96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
        128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
        144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
        160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
        176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
        192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
        208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
        224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
        240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
    };
    const uint8_t* imap = imap_identity;

    // Perform final read.
    model = &scene->models[0];
    object = &scene->objects[0];
    group = &scene->nodes.groups[0];
    shape = &scene->nodes.shapes[0];
    transform = &scene->nodes.transforms[0];
    for(uint32_t offset = c_MAIN.offset_children; offset < length;)
    {
        struct tcvox_chunk_ c;
        TCVOX_READ_CHUNK_HEADER_(&c, offset);

        switch(c.tag)
        {
        case TCVOX_TAG_('P','A','C','K'):
            // nothing to do
            break;
        case TCVOX_TAG_('S','I','Z','E'):
            // We count nmodels above, so this should always pass
            assert(model - scene->models < scene->nmodels);

            TCVOX_CHECK_(c.nbytes_data == 3 * sizeof(uint32_t), "Invalid SIZE chunk length");
            for(size_t i = 0; i < 3; i++)
            {
                uint32_t s = 0;
                TCVOX_READ_32LE_(&s, c.offset_data + i * sizeof(uint32_t));
                model->size.xyz[i] = s;
                TCVOX_CHECK_(model->size.xyz[i], "Zero-sized model");   // TODO: Should we simply ignore such models, instead of erroring on them?
            }

            model->id = model - scene->models;
            break;

        case TCVOX_TAG_('X','Y','Z','I'):
            TCVOX_CHECK_(model - scene->models < scene->nmodels
                      && !model->voxels, "Too many XYZI blocks in file");

            TCVOX_CHECK_(model->size.x && model->size.y && model->size.z, "XYZI chunk must follow SIZE chunk");
            TCVOX_READ_32LE_(&model->nvoxels, c.offset_data + 0 * sizeof(uint32_t));
            TCVOX_CHECK_(c.nbytes_data >= sizeof(uint32_t) + model->nvoxels * sizeof(uint32_t), "Invalid XYZI chunk size (truncated chunk?)");

            model->voxels = malloc(model->nvoxels * sizeof(*model->voxels));
            TCVOX_READ_BYTES_(model->voxels, model->nvoxels * 4, c.offset_data + 1 * sizeof(uint32_t));

            ++model;
            break;

        case TCVOX_TAG_('n','T','R','N'): {
            // We count above, so this should always pass
            assert(transform - scene->nodes.transforms < scene->nodes.ntransforms);
            TCVOX_READ_32LE_(&transform->id, c.offset_data + 0 * sizeof(uint32_t));
            assert(scene->nodes.index[transform->id].type == TCVOX_NODE_REF_TYPE_TRANSFORM);

            TCVOX_READ_32LE_(&transform->nattrs, c.offset_data + 1 * sizeof(uint32_t));
            uint32_t ioffset = c.offset_data + 2 * sizeof(uint32_t);

            TCVOX_ALLOC_CHECK_(transform->attrs, transform->nattrs, "transform attributes");
            for(size_t a = 0; a < transform->nattrs; a++)
            {
                const char* key; uint32_t klen;
                const char* val; uint32_t vlen;
                TCVOX_READ_STRING_NOCPY_(&key, &klen, ioffset); ioffset += sizeof(uint32_t) + klen;
                TCVOX_READ_STRING_NOCPY_(&val, &vlen, ioffset); ioffset += sizeof(uint32_t) + vlen;
                TCVOX_MAKE_ATTR_(&transform->attrs[a]);

                if(TCVOX_IS_KEY_(_name))
                    transform->name = transform->attrs[a].value;
                else if(TCVOX_IS_KEY_(_hidden))
                    transform->is_hidden = strtoul(transform->attrs[a].value, NULL, 10);
            }

            uint32_t child_id;
            TCVOX_READ_32LE_(&child_id, ioffset); ioffset += sizeof(uint32_t);
            TCVOX_CHECK_(child_id < scene->nodes.nindex, "Child ID out of bounds");
            switch(scene->nodes.index[child_id].type)
            {
            case TCVOX_NODE_REF_TYPE_GROUP: transform->child.group = &scene->nodes.groups[scene->nodes.index[child_id].index]; break;
            case TCVOX_NODE_REF_TYPE_SHAPE: transform->child.shape = &scene->nodes.shapes[scene->nodes.index[child_id].index]; break;
            default: TCVOX_CHECK_(false, "Invalid child type for transform");
            }

            uint32_t reserved_id;
            TCVOX_READ_32LE_(&reserved_id, ioffset); ioffset += sizeof(uint32_t);
            TCVOX_CHECK_(reserved_id == UINT32_MAX, "Reserved ID must be `-1` / `UINT32_MAX`.");

            uint32_t layer_id;
            TCVOX_READ_32LE_(&layer_id, ioffset); ioffset += sizeof(uint32_t);
            if(layer_id == UINT32_MAX)
                transform->layer = NULL;
            else
            {
                TCVOX_CHECK_(layer_id < scene->nlayers, "Layer ID out of bounds");
                transform->layer = &scene->layers[layer_id];
            }

            TCVOX_READ_32LE_(&transform->nframes, ioffset); ioffset += sizeof(uint32_t);
            TCVOX_CHECK_(transform->nframes > 0, "Number of frames in nTRN must be at least 1");
            TCVOX_ALLOC_CHECK_(transform->frames, transform->nframes, "transform frames");

            for(size_t f = 0; f < transform->nframes; f++)
            {
                tcvox_frame_t* frame = &transform->frames[f];

                TCVOX_READ_32LE_(&frame->nattrs, ioffset); ioffset += sizeof(uint32_t);
                TCVOX_ALLOC_CHECK_(frame->attrs, frame->nattrs, "frame attributes");

                // initialize values to defaults
                for(size_t i = 0; i < 3; i++)
                    frame->transform.r.xyz[i].index = i;
                //frame->r.xyz[i].sign is already okay  (0)
                //frame->t is already okay              (0 0 0)
                //frame->index is already okay          (0)

                for(size_t a = 0; a < frame->nattrs; a++)
                {
                    const char* key; uint32_t klen;
                    const char* val; uint32_t vlen;
                    TCVOX_READ_STRING_NOCPY_(&key, &klen, ioffset); ioffset += sizeof(uint32_t) + klen;
                    TCVOX_READ_STRING_NOCPY_(&val, &vlen, ioffset); ioffset += sizeof(uint32_t) + vlen;
                    TCVOX_MAKE_ATTR_(&frame->attrs[a]);

                    if(TCVOX_IS_KEY_(_r))
                    {
                        uint8_t r = strtoul(frame->attrs[a].value, NULL, 10);
                        frame->transform.r.x.index = (r >> 0) & 3;
                        frame->transform.r.y.index = (r >> 2) & 3;
                        // this computes the remaining index:
                        // ~(0 | 1) == ~(0b00 | 0b01) == 0b10 == 2
                        // ~(0 | 2) == ~(0b00 | 0b10) == 0b01 == 1
                        // ~(1 | 2) == ~(0b01 | 0b10) == 0b00 == 0
                        frame->transform.r.z.index = ~(frame->transform.r.x.index | frame->transform.r.y.index);
                        frame->transform.r.x.sign = (r >> 4) & 1;
                        frame->transform.r.y.sign = (r >> 5) & 1;
                        frame->transform.r.z.sign = (r >> 6) & 1;
                    }
                    else if(TCVOX_IS_KEY_(_t))
                    {
                        char* end = frame->attrs[a].value;
                        for(size_t i = 0; i < 3; i++)
                            frame->transform.t.xyz[i] = strtol(end, &end, 10);
                    }
                    else if(TCVOX_IS_KEY_(_f))
                        frame->index = strtoul(frame->attrs[a].value, NULL, 10);
                }
            }

            ++transform;
            } break;

        case TCVOX_TAG_('n','G','R','P'): {
            // We count above, so this should always pass
            assert(group - scene->nodes.groups < scene->nodes.ngroups);
            TCVOX_READ_32LE_(&group->id, c.offset_data + 0 * sizeof(uint32_t));
            assert(scene->nodes.index[group->id].type == TCVOX_NODE_REF_TYPE_GROUP);

            TCVOX_READ_32LE_(&group->nattrs, c.offset_data + 1 * sizeof(uint32_t));
            uint32_t ioffset = c.offset_data + 2 * sizeof(uint32_t);

            TCVOX_ALLOC_CHECK_(group->attrs, group->nattrs, "group attributes");
            for(size_t a = 0; a < group->nattrs; a++)
            {
                const char* key; uint32_t klen;
                const char* val; uint32_t vlen;
                TCVOX_READ_STRING_NOCPY_(&key, &klen, ioffset); ioffset += sizeof(uint32_t) + klen;
                TCVOX_READ_STRING_NOCPY_(&val, &vlen, ioffset); ioffset += sizeof(uint32_t) + vlen;
                TCVOX_MAKE_ATTR_(&group->attrs[a]);
            }

            TCVOX_READ_32LE_(&group->nchildren, ioffset); ioffset += sizeof(uint32_t);
            TCVOX_ALLOC_CHECK_(group->children, group->nchildren, "group children");
            for(size_t c = 0; c < group->nchildren; c++)
            {
                uint32_t child_id;
                TCVOX_READ_32LE_(&child_id, ioffset); ioffset += sizeof(uint32_t);
                TCVOX_CHECK_(child_id < scene->nodes.nindex, "Child ID out of bounds");
                TCVOX_CHECK_(scene->nodes.index[child_id].type == TCVOX_NODE_REF_TYPE_TRANSFORM, "Invalid child type for group");
                group->children[c] = &scene->nodes.transforms[scene->nodes.index[child_id].index];
            }

            ++group;
            } break;

        case TCVOX_TAG_('n','S','H','P'): {
            // We count above, so this should always pass
            assert(shape - scene->nodes.shapes < scene->nodes.nshapes);
            TCVOX_READ_32LE_(&shape->id, c.offset_data + 0 * sizeof(uint32_t));
            assert(scene->nodes.index[shape->id].type == TCVOX_NODE_REF_TYPE_SHAPE);

            TCVOX_READ_32LE_(&shape->nattrs, c.offset_data + 1 * sizeof(uint32_t));
            uint32_t ioffset = c.offset_data + 2 * sizeof(uint32_t);

            TCVOX_ALLOC_CHECK_(shape->attrs, shape->nattrs, "shape attributes");
            for(size_t a = 0; a < shape->nattrs; a++)
            {
                const char* key; uint32_t klen;
                const char* val; uint32_t vlen;
                TCVOX_READ_STRING_NOCPY_(&key, &klen, ioffset); ioffset += sizeof(uint32_t) + klen;
                TCVOX_READ_STRING_NOCPY_(&val, &vlen, ioffset); ioffset += sizeof(uint32_t) + vlen;
                TCVOX_MAKE_ATTR_(&shape->attrs[a]);
            }

            TCVOX_READ_32LE_(&shape->nmodels, ioffset); ioffset += sizeof(uint32_t);
            TCVOX_CHECK_(shape->nmodels > 0, "A shape must have at least 1 model");
            TCVOX_ALLOC_CHECK_(shape->models, shape->nmodels, "shape models");
            for(size_t m = 0; m < shape->nmodels; m++)
            {
                tcvox_shape_node_model_t* smodel = &shape->models[m];

                uint32_t model_id;
                TCVOX_READ_32LE_(&model_id, ioffset); ioffset += sizeof(uint32_t);
                TCVOX_CHECK_(model_id < scene->nmodels, "Shape model ID out of bounds");
                smodel->model = &scene->models[model_id];

                TCVOX_READ_32LE_(&smodel->nattrs, ioffset); ioffset += sizeof(uint32_t);
                TCVOX_ALLOC_CHECK_(smodel->attrs, smodel->nattrs, "shape model attributes");
                for(size_t a = 0; a < smodel->nattrs; a++)
                {
                    const char* key; uint32_t klen;
                    const char* val; uint32_t vlen;
                    TCVOX_READ_STRING_NOCPY_(&key, &klen, ioffset); ioffset += sizeof(uint32_t) + klen;
                    TCVOX_READ_STRING_NOCPY_(&val, &vlen, ioffset); ioffset += sizeof(uint32_t) + vlen;
                    TCVOX_MAKE_ATTR_(&smodel->attrs[a]);

                    if(TCVOX_IS_KEY_(_f))
                        smodel->starting_frame = strtoul(shape->attrs[a].value, NULL, 10);
                }
            }

            ++shape;
            } break;

        case TCVOX_TAG_('L','A','Y','R'): {
            uint32_t id;
            TCVOX_READ_32LE_(&id, c.offset_data + 0 * sizeof(uint32_t));
            TCVOX_CHECK_(id < scene->nlayers, "Layer ID out of bounds");

            tcvox_layer_t* layer = &scene->layers[id];
            layer->id = id;

            TCVOX_READ_32LE_(&layer->nattrs, c.offset_data + 1 * sizeof(uint32_t));
            uint32_t ioffset = c.offset_data + 2 * sizeof(uint32_t);

            TCVOX_ALLOC_CHECK_(layer->attrs, layer->nattrs, "layer attributes");
            for(size_t a = 0; a < layer->nattrs; a++)
            {
                const char* key; uint32_t klen;
                const char* val; uint32_t vlen;
                TCVOX_READ_STRING_NOCPY_(&key, &klen, ioffset); ioffset += sizeof(uint32_t) + klen;
                TCVOX_READ_STRING_NOCPY_(&val, &vlen, ioffset); ioffset += sizeof(uint32_t) + vlen;
                TCVOX_MAKE_ATTR_(&layer->attrs[a]);

                if(TCVOX_IS_KEY_(_name))
                    layer->name = layer->attrs[a].value;
                else if(TCVOX_IS_KEY_(_hidden))
                    layer->is_hidden = strtoul(layer->attrs[a].value, NULL, 10);
                else if(TCVOX_IS_KEY_(_color))
                {
                    uint8_t rgb[3];
                    char* end = layer->attrs[a].value;
                    rgb[0] = strtoul(end, &end, 10);
                    rgb[1] = strtoul(end, &end, 10);
                    rgb[2] = strtoul(end, &end, 10);
                    layer->color_abgr = (uint32_t)rgb[0]
                                      | (uint32_t)rgb[1] << 8
                                      | (uint32_t)rgb[2] << 16
                                      | (uint32_t)0xFF << 24
                                      ;
                }
            }

            uint32_t reserved_id;
            TCVOX_READ_32LE_(&reserved_id, ioffset); ioffset += sizeof(uint32_t);
            TCVOX_CHECK_(reserved_id == UINT32_MAX, "Layer reserved ID must be `-1` / `UINT32_MAX`");
            } break;

        case TCVOX_TAG_('r','O','B','J'): {
            // We count above, so this should always pass
            assert(object - scene->objects < scene->nobjects);

            TCVOX_READ_32LE_(&object->nattrs, c.offset_data + 0 * sizeof(uint32_t));
            uint32_t ioffset = c.offset_data + 1 * sizeof(uint32_t);

            TCVOX_ALLOC_CHECK_(object->attrs, object->nattrs, "object attributes");
            for(size_t a = 0; a < object->nattrs; a++)
            {
                const char* key; uint32_t klen;
                const char* val; uint32_t vlen;
                TCVOX_READ_STRING_NOCPY_(&key, &klen, ioffset); ioffset += sizeof(uint32_t) + klen;
                TCVOX_READ_STRING_NOCPY_(&val, &vlen, ioffset); ioffset += sizeof(uint32_t) + vlen;
                TCVOX_MAKE_ATTR_(&object->attrs[a]);

                if(TCVOX_IS_KEY_(_type))
                    object->type = object->attrs[a].value;
            }

            ++object;
            } break;

        case TCVOX_TAG_('r','C','A','M'): {
            uint32_t id;
            TCVOX_READ_32LE_(&id, c.offset_data + 0 * sizeof(uint32_t));
            TCVOX_CHECK_(id < scene->ncameras, "Camera ID out of bounds");

            tcvox_camera_t* camera = &scene->cameras[id];
            camera->id = id;

            TCVOX_READ_32LE_(&camera->nattrs, c.offset_data + 1 * sizeof(uint32_t));
            uint32_t ioffset = c.offset_data + 2 * sizeof(uint32_t);

            TCVOX_ALLOC_CHECK_(camera->attrs, camera->nattrs, "camera attributes");
            for(size_t a = 0; a < camera->nattrs; a++)
            {
                const char* key; uint32_t klen;
                const char* val; uint32_t vlen;
                TCVOX_READ_STRING_NOCPY_(&key, &klen, ioffset); ioffset += sizeof(uint32_t) + klen;
                TCVOX_READ_STRING_NOCPY_(&val, &vlen, ioffset); ioffset += sizeof(uint32_t) + vlen;
                TCVOX_MAKE_ATTR_(&camera->attrs[a]);

                if(TCVOX_IS_KEY_(_mode))
                {
                    if(TCVOX_IS_VAL_(_pers))
                        camera->mode = TCVOX_CAMERA_MODE_PERSPECTIVE;
                    else
                        camera->mode = TCVOX_CAMERA_MODE_UNKNOWN;
                }
                else if(TCVOX_IS_KEY_(_focus))
                {
                    char* end = camera->attrs[a].value;
                    for(size_t i = 0; i < 3; i++)
                        camera->focus[i] = strtod(end, &end);
                }
                else if(TCVOX_IS_KEY_(angle))
                {
                    char* end = camera->attrs[a].value;
                    for(size_t i = 0; i < 3; i++)
                        camera->angle[i] = strtod(end, &end);
                }
                else if(TCVOX_IS_KEY_(_radius))
                    camera->radius = strtod(camera->attrs[a].value, NULL);
                else if(TCVOX_IS_KEY_(_frustum))
                    camera->frustum = strtod(camera->attrs[a].value, NULL);
                else if(TCVOX_IS_KEY_(_fov))
                    camera->fov = strtod(camera->attrs[a].value, NULL);
            }

            } break;

        case TCVOX_TAG_('R','G','B','A'):
            TCVOX_CHECK_(c.nbytes_data == 4 * 256, "Invalid RGBA chunk length");
            //scene->palette.abgr[0] = 0x00000000;  // set in `default_palette` already
            // colors are offset by 1, hence `i + 1` & `i < 255`.
            for(size_t i = 0; i < 255; i++)
                TCVOX_READ_32LE_(&scene->palette.abgr[i + 1], c.offset_data + i * sizeof(uint32_t));
            break;

        case TCVOX_TAG_('I','M','A','P'):
            TCVOX_CHECK_(c.nbytes_data == 256, "Invalid IMAP chunk length");
            // No need to copy as we don't keep this information in the output --- simply keep a pointer to the file data at the relevant offset.
            TCVOX_READ_BYTES_NOCPY_(&imap, 256, c.offset_data);
            break;

        case TCVOX_TAG_('M','A','T','L'): {
            uint32_t id, npairs;
            TCVOX_READ_32LE_(&id, c.offset_data + 0 * sizeof(uint32_t));
            TCVOX_CHECK_(0 <= id && id <= 256, "Invalid material index in MATL chunk");
            id &= 255;  // material 256 becomes 0 (some files use 256, some use 0)

            tcvox_material_t* mtl = &scene->palette.materials[id];
            *mtl = tcvox_default_material_params;

            TCVOX_READ_32LE_(&npairs, c.offset_data + 1 * sizeof(uint32_t));
            uint32_t ioffset = c.offset_data + 2 * sizeof(uint32_t);
            for(size_t p = 0; p < npairs; p++)
            {
                const char* key; uint32_t klen;
                const char* val; uint32_t vlen;
                TCVOX_READ_STRING_NOCPY_(&key, &klen, ioffset); ioffset += sizeof(uint32_t) + klen;
                TCVOX_READ_STRING_NOCPY_(&val, &vlen, ioffset); ioffset += sizeof(uint32_t) + vlen;

                if(TCVOX_IS_KEY_(_type))
                {
                    if(TCVOX_IS_VAL_(_diffuse))
                        mtl->type = TCVOX_MATERIAL_TYPE_DIFFUSE;
                    else if(TCVOX_IS_VAL_(_metal))
                        mtl->type = TCVOX_MATERIAL_TYPE_METAL;
                    else if(TCVOX_IS_VAL_(_galss))
                        mtl->type = TCVOX_MATERIAL_TYPE_GLASS;
                    else if(TCVOX_IS_VAL_(_emit))
                        mtl->type = TCVOX_MATERIAL_TYPE_EMIT;
                    else if(TCVOX_IS_VAL_(_blend))
                        mtl->type = TCVOX_MATERIAL_TYPE_BLEND;
                    else if(TCVOX_IS_VAL_(_media))
                        mtl->type = TCVOX_MATERIAL_TYPE_CLOUD;
                    else
                        //TCVOX_CHECK_(false, "Unknown material type");
                        mtl->type = TCVOX_MATERIAL_TYPE_UNKNOWN;
                }
                // METAL & GLASS:
                else if(TCVOX_IS_KEY_(_rough))
                    mtl->roughness = strtof(val, NULL);
                else if(TCVOX_IS_KEY_(_ior))
                    mtl->index_of_refraction = 1.0f + strtof(val, NULL);
                // METAL:
                else if(TCVOX_IS_KEY_(_metal))
                    mtl->metalness = strtof(val, NULL);
                else if(TCVOX_IS_KEY_(_sp))
                    mtl->specular = strtof(val, NULL);
                // GLASS:
                else if(TCVOX_IS_KEY_(_alpha))
                    mtl->alpha = strtof(val, NULL);
                else if(TCVOX_IS_KEY_(_trans))  // aliases _alpha
                    mtl->alpha = strtof(val, NULL);
                // GLASS & CLOUD:
                else if(TCVOX_IS_KEY_(_media))
                {
                    if(TCVOX_IS_VAL_(1))
                        mtl->media_type = TCVOX_MEDIA_TYPE_SCATTER;
                    if(TCVOX_IS_VAL_(2))
                        mtl->media_type = TCVOX_MEDIA_TYPE_EMISSIVE;
                    if(TCVOX_IS_VAL_(3))
                        mtl->media_type = TCVOX_MEDIA_TYPE_SUBSURFACE;
                }
                else if(TCVOX_IS_KEY_(_g))
                    mtl->scatter_phase = strtof(val, NULL);
                else if(TCVOX_IS_KEY_(_d))
                    mtl->density = strtof(val, NULL);
                // TYPE_EMIT:
                else if(TCVOX_IS_KEY_(_emit))
                    mtl->raw.emit = strtof(val, NULL);
                else if(TCVOX_IS_KEY_(_flux))
                    mtl->raw.flux = 1.0f + strtof(val, NULL);
                else if(TCVOX_IS_KEY_(_ldr))
                    mtl->raw.ldr = strtof(val, NULL);
            }

            mtl->emissiveness = mtl->raw.emit * powf(10.0f, mtl->raw.flux) + mtl->raw.ldr;
            } break;

        case TCVOX_TAG_('N','O','T','E'): {
            uint32_t nnames;
            TCVOX_READ_32LE_(&nnames, c.offset_data + 0 * sizeof(uint32_t));
            TCVOX_CHECK_(nnames < 256, "Palette names out of bounds");
            uint32_t ioffset;

            // first, sum up the lengths
            size_t total = 0;

            const char* names[256];
            uint32_t nlens[256];

            ioffset = c.offset_data + 1 * sizeof(uint32_t);
            for(size_t n = 0; n < nnames; n++)
            {
                TCVOX_READ_STRING_NOCPY_(&names[n], &nlens[n], ioffset); ioffset += sizeof(nlens[n]) + nlens[n];
                total += nlens[n] + 1;
            }

            if(total)
            {
                char* ptr;
                TCVOX_ALLOC_CHECK_NOINIT_(ptr, total, "palette names");

                for(size_t n = 0; n < nnames; n++)
                {
                    memcpy(ptr, names[n], nlens[n]);
                    ptr[nlens[n]] = 0;
                    scene->palette.names[n] = ptr;
                    ptr += nlens[n] + 1;
                }
            }
            } break;

        default:
            break;
        }

        offset = c.offset_end;
    }

    // special case: some .vox files only have model (SIZE/XYZI) information, without nodes; in that case, create some dummy nodes
    scene->no_nodes_in_file = !scene->nodes.nindex;
    if(scene->no_nodes_in_file && scene->nmodels)
    {
        assert(!scene->nodes.nshapes);
        scene->nodes.nindex = 1;
        scene->nodes.nshapes = 1;
        TCVOX_ALLOC_CHECK_NOINIT_(scene->nodes.index, scene->nodes.nindex, "index for single model");
        TCVOX_ALLOC_CHECK_NOINIT_(scene->nodes.shapes, scene->nodes.nshapes, "shape for single model");

        scene->nodes.index[0] = (tcvox_node_ref_t){ 0, TCVOX_NODE_REF_TYPE_SHAPE };
        scene->nodes.shapes[0] = (tcvox_shape_node_t){
            .nmodels = scene->nmodels,
        };
        TCVOX_ALLOC_CHECK_NOINIT_(scene->nodes.shapes[0].models, scene->nodes.shapes[0].nmodels, "shape models");

        for(size_t m = 0; m < scene->nmodels; m++)
            scene->nodes.shapes[0].models[m] = (tcvox_shape_node_model_t){
                .model = &scene->models[m],
            };
    }

    assert(!error);
    return NULL;
error:
    assert(error);
    tcvox_unload(scene);
    return error;
}

tcvox_scene_t* tcvox_load_memory(tcvox_scene_t* scene, const void* ptr, size_t length)
{
    if(!scene) return NULL;
    *scene = (tcvox_scene_t){ NULL };
    scene->error = tcvox_load_memory_(scene, ptr, length);
    return !scene->error ? scene : NULL;
}

tcvox_scene_t* tcvox_load_fname(tcvox_scene_t* scene, const char* fname)
{
    if(!scene) return NULL;

    void* data = NULL;
    FILE* file = fopen(fname, "rb");
    if(!file)
    {
        scene->error = "Unable to open file";
        goto error;
    }

    fseek(file, 0, SEEK_END);
    long int len = ftell(file);
    if(len < 0)
    {
        scene->error = "Unable to determine filesize";
        goto error;
    }
    rewind(file);

    data = malloc(len);
    if(!data)
    {
        scene->error = "Out of memory";
        goto error;
    }

    if(fread(data, 1, len, file) != len)
    {
        scene->error = "Unable to read file";
        goto error;
    }

    scene = tcvox_load_memory(scene, data, len);

    free(data);
    return scene;
error:
    if(data) free(data);
    if(file) fclose(file);
    return NULL;
}

static void tcvox_free_attrs_(tcvox_attr_t* attrs, uint32_t nattrs)
{
    if(!attrs) return;

    while(nattrs--)
        free(attrs[nattrs].key);    // value is assigned from key
    free(attrs);
}

void tcvox_unload(const tcvox_scene_t* scene)
{
    if(!scene) return;

    if(scene->models)
    {
        for(size_t i = 0; i < scene->nmodels; i++)
            free(scene->models[i].voxels);
        free(scene->models);
    }
    if(scene->layers)
    {
        for(size_t i = 0; i < scene->nlayers; i++)
            tcvox_free_attrs_(scene->layers[i].attrs, scene->layers[i].nattrs);
        free(scene->layers);
    }
    if(scene->cameras)
    {
        for(size_t i = 0; i < scene->ncameras; i++)
            tcvox_free_attrs_(scene->cameras[i].attrs, scene->cameras[i].nattrs);
        free(scene->cameras);
    }

    free(scene->nodes.index);
    if(scene->nodes.groups)
    {
        for(size_t i = 0; i < scene->nodes.ngroups; i++)
        {
            tcvox_free_attrs_(scene->nodes.groups[i].attrs, scene->nodes.groups[i].nattrs);
            free(scene->nodes.groups[i].children);
        }
        free(scene->nodes.groups);
    }
    if(scene->nodes.shapes)
    {
        for(size_t i = 0; i < scene->nodes.nshapes; i++)
        {
            tcvox_free_attrs_(scene->nodes.shapes[i].attrs, scene->nodes.shapes[i].nattrs);
            if(scene->nodes.shapes[i].models)
            {
                for(size_t m = 0; m < scene->nodes.shapes[i].nmodels; m++)
                    tcvox_free_attrs_(scene->nodes.shapes[i].models[m].attrs, scene->nodes.shapes[i].models[m].nattrs);
                free(scene->nodes.shapes[i].models);
            }
        }
        free(scene->nodes.shapes);
    }
    if(scene->nodes.transforms)
    {
        for(size_t i = 0; i < scene->nodes.ntransforms; i++)
        {
            tcvox_free_attrs_(scene->nodes.transforms[i].attrs, scene->nodes.transforms[i].nattrs);
            if(scene->nodes.transforms[i].frames)
            {
                for(size_t f = 0; f < scene->nodes.transforms[i].nframes; f++)
                    tcvox_free_attrs_(scene->nodes.transforms[i].frames[f].attrs, scene->nodes.transforms[i].frames[f].nattrs);
                free(scene->nodes.transforms[i].frames);
            }
        }
        free(scene->nodes.transforms);
    }
}

tcvox_transform_t tcvox_transform_combine(tcvox_transform_t parent, tcvox_transform_t child)
{
    return (tcvox_transform_t){
        .r = tcvox_rotation_combine(parent.r, child.r),
        .t = tcvox_transform_apply(parent, child.t, true),
    };
}

tcvox_ivec3_t tcvox_transform_apply(tcvox_transform_t transform, tcvox_ivec3_t vector, bool apply_translation)
{
    tcvox_ivec3_t result;

    assert((1 << transform.r.xyz[0].index | 1 << transform.r.xyz[1].index | 1 << transform.r.xyz[2].index) == 0x7 && "Invalid rotation indices (uninitialized rotation?)");
    for(size_t i = 0; i < 3; i++)
    {
        int32_t coord = vector.xyz[transform.r.xyz[i].index];
        if(transform.r.xyz[i].sign) coord = -coord;
        result.xyz[i] = coord;
    }

    if(apply_translation)
        for(size_t i = 0; i < 3; i++)
            result.xyz[i] += transform.t.xyz[i];

    return result;
}

tcvox_ivec3_t tcvox_transform_apply_voxel_vec(tcvox_transform_t transform, tcvox_ivec3_t vector, bool apply_translation)
{
    // This strange logic emulates MagicaVoxel's coordinate handling (determined via blackbox testing).
    // Essentially, we convert to 32p1 fixed-point and subtract `0.5` in each direction.
    // Author note: Adding would make more sense (so that each object's bbox start is at [0,0,0]), but this is the logic the format uses.
    for(size_t i = 0; i < 3; i++)
    {
        transform.t.xyz[i] = transform.t.xyz[i] * 2 - 1;
        vector.xyz[i] = vector.xyz[i] * 2 - 1;
    }
    tcvox_ivec3_t result = tcvox_transform_apply(transform, vector, apply_translation);
    for(size_t i = 0; i < 3; i++)
        result.xyz[i] = result.xyz[i] / 2;
    return result;
}
tcvox_ivec3_t tcvox_transform_apply_voxel(tcvox_transform_t transform, tcvox_voxel_t voxel, bool apply_translation)
{
    return tcvox_transform_apply_voxel_vec(transform, (tcvox_ivec3_t){{ voxel.xyz[0], voxel.xyz[1], voxel.xyz[2] }}, apply_translation);
}

void tcvox_rotation_to_mat3(tcvox_rotation_t rotation, int8_t mat3[TC__STATIC_SIZE(3)][3])
{
    assert((1 << rotation.xyz[0].index | 1 << rotation.xyz[1].index | 1 << rotation.xyz[2].index) == 0x7 && "Invalid rotation indices (uninitialized rotation?)");

    memset(mat3, 0, 3 * 3 * sizeof(int8_t));
    for(size_t i = 0; i < 3; i++)
        mat3[i][rotation.xyz[i].index] = rotation.xyz[i].sign ? -1 : +1;
}

tcvox_rotation_t tcvox_rotation_combine(tcvox_rotation_t first, tcvox_rotation_t second)
{
    assert((1 << first.xyz[0].index | 1 << first.xyz[1].index | 1 << first.xyz[2].index) == 0x7 && "Invalid rotation indices (uninitialized rotation?)");
    assert((1 << second.xyz[0].index | 1 << second.xyz[1].index | 1 << second.xyz[2].index) == 0x7 && "Invalid rotation indices (uninitialized rotation?)");

    tcvox_rotation_t result;
    for(size_t i = 0; i < 3; i++)
    {
        tcvox_rotation_axis_t axis = second.xyz[first.xyz[i].index];
        axis.sign ^= first.xyz[i].sign;
        result.xyz[i] = axis;
    }

    return result;
}


struct tcvox_iter_stack_entry
{
    tcvox_transform_t transform;
    tcvox_group_node_t* group;
    uint32_t next_child;
};
static bool tcvox_iter_stack_push_(tcvox_iter_t* it, const struct tcvox_iter_stack_entry* entry)
{
    if(it->_stack.len + 1 >= it->_stack.mem)
    {
        it->_stack.mem = it->_stack.mem ? it->_stack.mem << 1 : 4;
        struct tcvox_iter_stack_entry* nptr = realloc(it->_stack.ptr, it->_stack.mem * sizeof(*it->_stack.ptr));
        if(!nptr)   // out of memory
        {
            tcvox_iter_finish(it);
            return false;
        }
        it->_stack.ptr = nptr;
    }
    it->_stack.ptr[it->_stack.len++] = *entry;
    return true;
}
static bool tcvox_iter_stack_push_group_(tcvox_iter_t* it, tcvox_group_node_t* group)
{
    struct tcvox_iter_stack_entry entry = {
        .transform = it->transform,
        .group = group,
        .next_child = 0,
    };
    return tcvox_iter_stack_push_(it, &entry);
}
static bool tcvox_iter_stack_push_transform_(tcvox_iter_t* it, tcvox_transform_node_t* transform)
{
    if(!it->include_hidden
    && (transform->is_hidden || (transform->layer && transform->layer->is_hidden)))
        return true;

    it->transform = tcvox_transform_combine(it->transform, transform->frames[0].transform);
    if(transform->child.group)
        return tcvox_iter_stack_push_group_(it, transform->child.group);
    else if(transform->child.shape)
    {
        assert(!it->_pending_shape);
        it->_pending_shape = transform->child.shape;
        return true;
    }
    else
        return true;    // TODO: return false here or assert?
}
static void tcvox_iter_stack_pop_(tcvox_iter_t* it)
{
    assert(it->_stack.len);
    --it->_stack.len;
}

tcvox_iter_t tcvox_scene_iter_shapes(tcvox_scene_t* scene, bool include_hidden)
{
    tcvox_iter_t it = {
        .scene = scene,
        .transform = tcvox_transform_identity,
        .include_hidden = include_hidden,
    };
    if(!scene->nodes.nshapes)   // zero shapes => we're done immediately
        return it;

    tcvox_node_ref_t ref = scene->nodes.index[0];
    switch(ref.type)
    {
        case TCVOX_NODE_REF_TYPE_GROUP:
            tcvox_iter_stack_push_group_(&it, &scene->nodes.groups[ref.index]);
            break;
        case TCVOX_NODE_REF_TYPE_TRANSFORM:
            tcvox_iter_stack_push_transform_(&it, &scene->nodes.transforms[ref.index]);
            break;
        case TCVOX_NODE_REF_TYPE_SHAPE:
            it._pending_shape = &scene->nodes.shapes[ref.index];
            break;
    }
    return it;
}

bool tcvox_iter_next(tcvox_iter_t* it)
{
    assert(!it->done && "Called tcvox_iter_next after the iterator finished");

    for(;;)
    {
        if(it->_pending_shape)
        {
            it->shape = it->_pending_shape;
            it->_pending_shape = NULL;
            return true;
        }

        if(!it->_stack.len)
            break;

        struct tcvox_iter_stack_entry* entry = &it->_stack.ptr[it->_stack.len - 1];
        it->transform = entry->transform;

        if(entry->next_child >= entry->group->nchildren)
        {
            tcvox_iter_stack_pop_(it);
            continue;
        }
        tcvox_iter_stack_push_transform_(it, entry->group->children[entry->next_child++]);
    }

    tcvox_iter_finish(it);
    return false;
}

void tcvox_iter_finish(tcvox_iter_t* it)
{
    if(it->done)
        return;
    it->done = true;

    it->shape = NULL;
    if(it->_stack.ptr) free(it->_stack.ptr);
}

void tcvox_bounds_init(tcvox_ivec3_t bounds[TC__STATIC_SIZE(2)])
{
    for(size_t i = 0; i < 3; i++)
    {
        bounds[0].xyz[i] = INT32_MAX;
        bounds[1].xyz[i] = INT32_MIN;
    }
}

void tcvox_bounds_update(tcvox_ivec3_t bounds[TC__STATIC_SIZE(2)], tcvox_ivec3_t v)
{
    for(size_t i = 0; i < 3; i++)
    {
        if(v.xyz[i] < bounds[0].xyz[i]) bounds[0].xyz[i] = v.xyz[i];
        if(bounds[1].xyz[i] < v.xyz[i]) bounds[1].xyz[i] = v.xyz[i];
    }
}

bool tcvox_scene_compute_bounds(tcvox_scene_t* scene, tcvox_ivec3_t bounds[TC__STATIC_SIZE(2)], tcvox_compute_bounds_flags_t flags)
{
    tcvox_bounds_init(bounds);

    tcvox_iter_t it = tcvox_scene_iter_shapes(scene, !!(flags & TCVOX_BOUNDSFLG_INCLUDE_HIDDEN));
    if(it.done) // out-of-memory
        return false;
    while(tcvox_iter_next(&it))
    {
        if((flags & TCVOX_BOUNDSFLG_IGNORE_EMPTY) && !it.shape->nmodels)
            continue;

        tcvox_ivec3_t v = {{0,0,0}};
        v = (flags & TCVOX_BOUNDSFLG_POINT_CLOUD_MODE)
          ? tcvox_transform_apply_voxel_vec(it.transform, v, true)
          : tcvox_transform_apply(it.transform, v, true);
        tcvox_bounds_update(bounds, v);

        for(size_t m = 0; m < it.shape->nmodels; m++)
        {
            v = it.shape->models[m].model->size;
            v = (flags & TCVOX_BOUNDSFLG_POINT_CLOUD_MODE)
              ? tcvox_transform_apply_voxel_vec(it.transform, (tcvox_ivec3_t){{ v.x - 1, v.y - 1, v.z - 1 }}, true)
              : tcvox_transform_apply(it.transform, v, true);
            tcvox_bounds_update(bounds, v);
        }
    }
    return true;
}


#endif /* TC_VOX_IMPLEMENTATION */
