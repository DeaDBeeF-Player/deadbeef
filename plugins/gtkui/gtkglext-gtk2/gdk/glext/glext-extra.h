/* GdkGLExt - OpenGL Extension to GDK
 * Copyright (C) 2002-2004  Naofumi Yasufuku
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA.
 */

/*
 * Additional OpenGL extensions.
 */

#ifndef __glext_extra_h_
#define __glext_extra_h_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && !defined(APIENTRY) && !defined(__CYGWIN__)
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef GLAPI
#define GLAPI extern
#endif

/*
 * 3DFX
 */

/*
 * APPLE
 */

#ifndef GL_APPLE_texture_range
#define GL_TEXTURE_RANGE_LENGTH_APPLE      0x85B7
#define GL_TEXTURE_RANGE_POINTER_APPLE     0x85B8
#define GL_TEXTURE_STORAGE_HINT_APPLE      0x85BC
#endif
#if !defined(GL_APPLE_vertex_array_range) && !defined(GL_APPLE_texture_range)
#define GL_STORAGE_PRIVATE_APPLE           0x85BD
#define GL_STORAGE_CACHED_APPLE            0x85BE
#define GL_STORAGE_SHARED_APPLE            0x85BF
#endif

#ifndef GL_APPLE_float_pixels
#define GL_COLOR_FLOAT_APPLE               0x8A0F
#define GL_RGBA_FLOAT32_APPLE              0x8814
#define GL_RGB_FLOAT32_APPLE               0x8815
#define GL_ALPHA_FLOAT32_APPLE             0x8816
#define GL_INTENSITY_FLOAT32_APPLE         0x8817
#define GL_LUMINANCE_FLOAT32_APPLE         0x8818
#define GL_LUMINANCE_ALPHA_FLOAT32_APPLE   0x8819
#define GL_RGBA_FLOAT16_APPLE              0x881A
#define GL_RGB_FLOAT16_APPLE               0x881B
#define GL_ALPHA_FLOAT16_APPLE             0x881C
#define GL_INTENSITY_FLOAT16_APPLE         0x881D
#define GL_LUMINANCE_FLOAT16_APPLE         0x881E
#define GL_LUMINANCE_ALPHA_FLOAT16_APPLE   0x881F
#endif

#ifndef GL_APPLE_vertex_program_evaluators
#define GL_VERTEX_ATTRIB_MAP1_ARB                        0x8A00
#define GL_VERTEX_ATTRIB_MAP2_ARB                        0x8A01
#define GL_VERTEX_ATTRIB_MAP1_SIZE_ARB                   0x8A02
#define GL_VERTEX_ATTRIB_MAP1_COEFF_ARB                  0x8A03
#define GL_VERTEX_ATTRIB_MAP1_ORDER_ARB                  0x8A04
#define GL_VERTEX_ATTRIB_MAP1_DOMAIN_ARB                 0x8A05
#define GL_VERTEX_ATTRIB_MAP2_SIZE_ARB                   0x8A06
#define GL_VERTEX_ATTRIB_MAP2_COEFF_ARB                  0x8A07
#define GL_VERTEX_ATTRIB_MAP2_ORDER_ARB                  0x8A08
#define GL_VERTEX_ATTRIB_MAP2_DOMAIN_ARB                 0x8A09
#endif

#ifndef GL_APPLE_packed_pixels
#define GL_APPLE_packed_pixels 1
#endif

#ifndef GL_APPLE_texture_range
#define GL_APPLE_texture_range 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glTextureRangeAPPLE (GLenum, GLsizei, const GLvoid *);
GLAPI void APIENTRY glGetTexParameterPointervAPPLE (GLenum, GLenum, GLvoid **);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLTEXTURERANGEAPPLEPROC) (GLenum target, GLsizei length, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLGETTEXPARAMETERPOINTERVAPPLEPROC) (GLenum target, GLenum pname, GLvoid **params);
#endif

#ifndef GL_APPLE_float_pixels
#define GL_APPLE_float_pixels 1
#endif

#ifndef GL_APPLE_vertex_program_evaluators
#define GL_APPLE_vertex_program_evaluators 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glEnableVertexAttribAPPLE (GLuint index, GLenum pname);
GLAPI void APIENTRY glDisableVertexAttribAPPLE (GLuint index, GLenum pname);
GLAPI GLboolean APIENTRY glIsVertexAttribEnabledAPPLE (GLuint index, GLenum pname);
GLAPI void APIENTRY glMapVertexAttrib1dAPPLE (GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
GLAPI void APIENTRY glMapVertexAttrib1fAPPLE (GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
GLAPI void APIENTRY glMapVertexAttrib2dAPPLE (GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
GLAPI void APIENTRY glMapVertexAttrib2fAPPLE (GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLENABLEVERTEXATTRIBAPPLEPROC) (GLuint index, GLenum pname);
typedef void (APIENTRY * PFNGLDISABLEVERTEXATTRIBAPPLEPROC) (GLuint index, GLenum pname);
typedef GLboolean (APIENTRY * PFNGLISVERTEXATTRIBENABLEDAPPLEPROC) (GLuint index, GLenum pname);
typedef void (APIENTRY * PFNGLMAPVERTEXATTRIB1DAPPLEPROC) (GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
typedef void (APIENTRY * PFNGLMAPVERTEXATTRIB1FAPPLEPROC) (GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
typedef void (APIENTRY * PFNGLMAPVERTEXATTRIB2DAPPLEPROC) (GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
typedef void (APIENTRY * PFNGLMAPVERTEXATTRIB2FAPPLEPROC) (GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
#endif

/*
 * ARB
 */

/*
 * ATI
 */

#ifndef GL_ATI_blend_equation_separate
#define GL_ALPHA_BLEND_EQUATION_ATI       0x883D
#endif

#ifndef GL_ATI_blend_weighted_minmax
#define GL_MIN_WEIGHTED_ATI               0x877D
#define GL_MAX_WEIGHTED_ATI               0x877E
#endif

#ifndef GL_ATI_point_cull_mode
#define GL_POINT_CULL_MODE_ATI            0x60B3
#define GL_POINT_CULL_CENTER_ATI          0x60B4
#define GL_POINT_CULL_CLIP_ATI            0x60B5
#endif

#ifndef GL_ATI_blend_equation_separate
#define GL_ATI_blend_equation_separate 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glBlendEquationSeparateATI (GLenum, GLenum);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLBLENDEQUATIONSEPARATEATIPROC) (GLenum equationRGB, GLenum equationAlpha);
#endif

#ifndef GL_ATI_blend_weighted_minmax
#define GL_ATI_blend_weighted_minmax 1
#endif

#ifndef GL_ATI_point_cull_mode
#define GL_ATI_point_cull_mode 1
#endif

/*
 * ATIX
 */

#ifndef GL_ATIX_pn_triangles
#define GL_PN_TRIANGLES_ATIX                            0x6090
#define GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATIX      0x6091
#define GL_PN_TRIANGLES_POINT_MODE_ATIX                 0x6092
#define GL_PN_TRIANGLES_NORMAL_MODE_ATIX                0x6093
#define GL_PN_TRIANGLES_TESSELATION_LEVEL_ATIX          0x6094
#define GL_PN_TRIANGLES_POINT_MODE_LINEAR_ATIX          0x6095
#define GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATIX           0x6096
#define GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATIX         0x6097
#define GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATIX      0x6098
#endif

#ifndef GL_ATIX_texture_env_combine3
#define GL_MODULATE_ADD_ATIX              0x8744
#define GL_MODULATE_SIGNED_ADD_ATIX       0x8745
#define GL_MODULATE_SUBTRACT_ATIX         0x8746
#endif

#ifndef GL_ATIX_texture_env_route
#define GL_SECONDARY_COLOR_ATIX           0x8747
#define GL_TEXTURE_OUTPUT_RGB_ATIX        0x8748
#define GL_TEXTURE_OUTPUT_ALPHA_ATIX      0x8749
#endif

#ifndef GL_ATIX_vertex_shader_output_point_size
#define GL_OUTPUT_POINT_SIZE_ATIX         0x610E
#endif

#ifndef GL_ATIX_point_sprites
#define GL_ATIX_point_sprites 1
#endif

#ifndef GL_ATIX_pn_triangles
#define GL_ATIX_pn_triangles 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glPNTrianglesiATIX (GLenum, GLint);
GLAPI void APIENTRY glPNTrianglesfATIX (GLenum, GLfloat);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLPNTRIANGLESIATIXPROC) (GLenum pname, GLint param);
typedef void (APIENTRY * PFNGLPNTRIANGLESFATIXPROC) (GLenum pname, GLfloat param);
#endif

#ifndef GL_ATIX_texture_env_combine3
#define GL_ATIX_texture_env_combine3 1
#endif

#ifndef GL_ATIX_texture_env_route
#define GL_ATIX_texture_env_route 1
#endif

#ifndef GL_ATIX_vertex_shader_output_point_size
#define GL_ATIX_vertex_shader_output_point_size 1
#endif

/*
 * Autodesk
 */

#ifndef GL_Autodesk_facet_normal
#define GL_FACET_NORMAL                   0x85D0 
#endif

#ifndef GL_Autodesk_facet_normal
#define GL_Autodesk_facet_normal 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glFacetNormal3b (GLbyte, GLbyte, GLbyte);
GLAPI void APIENTRY glFacetNormal3d (GLdouble, GLdouble, GLdouble);
GLAPI void APIENTRY glFacetNormal3f (GLfloat, GLfloat, GLfloat);
GLAPI void APIENTRY glFacetNormal3i (GLint, GLint, GLint);
GLAPI void APIENTRY glFacetNormal3s (GLshort, GLshort, GLshort);
GLAPI void APIENTRY glFacetNormal3bv (const GLbyte *);
GLAPI void APIENTRY glFacetNormal3dv (const GLdouble *);
GLAPI void APIENTRY glFacetNormal3fv (const GLfloat *);
GLAPI void APIENTRY glFacetNormal3iv (const GLint *);
GLAPI void APIENTRY glFacetNormal3sv (const GLshort *);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLFACETNORMAL3BPROC) (GLbyte nx, GLbyte ny, GLbyte nz);
typedef void (APIENTRY * PFNGLFACETNORMAL3DPROC) (GLdouble nx, GLdouble ny, GLdouble nz);
typedef void (APIENTRY * PFNGLFACETNORMAL3FPROC) (GLfloat nx, GLfloat ny, GLfloat nz);
typedef void (APIENTRY * PFNGLFACETNORMAL3IPROC) (GLint nx, GLint ny, GLint nz);
typedef void (APIENTRY * PFNGLFACETNORMAL3SPROC) (GLshort nx, GLshort ny, GLshort nz);
typedef void (APIENTRY * PFNGLFACETNORMAL3BVPROC) (const GLbyte *v);
typedef void (APIENTRY * PFNGLFACETNORMAL3DVPROC) (const GLdouble *v);
typedef void (APIENTRY * PFNGLFACETNORMAL3FVPROC) (const GLfloat *v);
typedef void (APIENTRY * PFNGLFACETNORMAL3IVPROC) (const GLint *v);
typedef void (APIENTRY * PFNGLFACETNORMAL3SVPROC) (const GLshort *v);
#endif

#ifndef GL_Autodesk_valid_back_buffer_hint
#define GL_Autodesk_valid_back_buffer_hint 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glWindowBackBufferHint (void);
GLAPI GLboolean APIENTRY glValidBackBufferHint (GLint, GLint, GLsizei, GLsizei);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLWINDOWBACKBUFFERHINTPROC) (void);
typedef GLboolean (APIENTRY * PFNGLVALIDBACKBUFFERHINTPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
#endif

/*
 * EXT
 */

#ifndef GL_EXT_depth_bounds_test
#define GL_DEPTH_BOUNDS_TEST_EXT          0x8890
#define GL_DEPTH_BOUNDS_EXT               0x8891
#endif

#ifndef GL_EXT_fragment_lighting
#define GL_FRAGMENT_LIGHTING_EXT                               0x8400
#define GL_FRAGMENT_COLOR_MATERIAL_EXT                         0x8401
#define GL_FRAGMENT_COLOR_MATERIAL_FACE_EXT                    0x8402
#define GL_FRAGMENT_COLOR_MATERIAL_PARAMETER_EXT               0x8403
#define GL_MAX_FRAGMENT_LIGHTS_EXT                             0x8404
#define GL_MAX_ACTIVE_LIGHTS_EXT                               0x8405
#define GL_CURRENT_RASTER_NORMAL_EXT                           0x8406
#define GL_LIGHT_ENV_MODE_EXT                                  0x8407
#define GL_FRAGMENT_LIGHT_MODEL_LOCAL_VIEWER_EXT               0x8408
#define GL_FRAGMENT_LIGHT_MODEL_TWO_SIDE_EXT                   0x8409
#define GL_FRAGMENT_LIGHT_MODEL_AMBIENT_EXT                    0x840A
#define GL_FRAGMENT_LIGHT_MODEL_NORMAL_INTERPOLATION_EXT       0x840B
#define GL_FRAGMENT_LIGHT0_EXT                                 0x840C
#define GL_FRAGMENT_LIGHT1_EXT                                 0x840D
#define GL_FRAGMENT_LIGHT2_EXT                                 0x840E
#define GL_FRAGMENT_LIGHT3_EXT                                 0x840F
#define GL_FRAGMENT_LIGHT4_EXT                                 0x8410
#define GL_FRAGMENT_LIGHT5_EXT                                 0x8411
#define GL_FRAGMENT_LIGHT6_EXT                                 0x8412
#define GL_FRAGMENT_LIGHT7_EXT                                 0x8413
#endif

#ifndef GL_EXT_multitexture
#define GL_SELECTED_TEXTURE_EXT           0x83C0
#define GL_SELECTED_TEXTURE_COORD_SET_EXT 0x83C1
#define GL_SELECTED_TEXTURE_TRANSFORM_EXT 0x83C2
#define GL_MAX_TEXTURES_EXT               0x83C3
#define GL_MAX_TEXTURE_COORD_SETS_EXT     0x83C4
#define GL_TEXTURE_ENV_COORD_SET_EXT      0x83C5
#define GL_TEXTURE0_EXT                   0x83C6
#define GL_TEXTURE1_EXT                   0x83C7
#define GL_TEXTURE2_EXT                   0x83C8
#define GL_TEXTURE3_EXT                   0x83C9
#define GL_TEXTURE4_EXT                   0x83CA
#define GL_TEXTURE5_EXT                   0x83CB
#define GL_TEXTURE6_EXT                   0x83CC
#define GL_TEXTURE7_EXT                   0x83CD
#define GL_TEXTURE8_EXT                   0x83CE
#define GL_TEXTURE9_EXT                   0x83CF
#define GL_TEXTURE10_EXT                  0x83D0
#define GL_TEXTURE11_EXT                  0x83D1
#define GL_TEXTURE12_EXT                  0x83D2
#define GL_TEXTURE13_EXT                  0x83D3
#define GL_TEXTURE14_EXT                  0x83D4
#define GL_TEXTURE15_EXT                  0x83D5
#define GL_TEXTURE16_EXT                  0x83D6
#define GL_TEXTURE17_EXT                  0x83D7
#define GL_TEXTURE18_EXT                  0x83D8
#define GL_TEXTURE19_EXT                  0x83D9
#define GL_TEXTURE20_EXT                  0x83DA
#define GL_TEXTURE21_EXT                  0x83DB
#define GL_TEXTURE22_EXT                  0x83DC
#define GL_TEXTURE23_EXT                  0x83DD
#define GL_TEXTURE24_EXT                  0x83DE
#define GL_TEXTURE25_EXT                  0x83DF
#define GL_TEXTURE26_EXT                  0x83E0
#define GL_TEXTURE27_EXT                  0x83E1
#define GL_TEXTURE28_EXT                  0x83E2
#define GL_TEXTURE29_EXT                  0x83E3
#define GL_TEXTURE30_EXT                  0x83E4
#define GL_TEXTURE31_EXT                  0x83E5
#endif

/* unknown */
#ifndef GL_EXT_scene_marker
/*
#define GL_SCENE_REQUIRED_EXT             0
*/
#endif

#ifndef GL_EXT_texgen_reflection
#define GL_NORMAL_MAP_EXT                 0x8511
#define GL_REFLECTION_MAP_EXT             0x8512
#endif

#ifndef GL_EXT_texture_edge_clamp
#define GL_CLAMP_TO_EDGE_EXT              0x812F
#endif

#ifndef GL_EXT_texture_rectangle
#define GL_TEXTURE_RECTANGLE_EXT          0x84F5
#define GL_TEXTURE_BINDING_RECTANGLE_EXT  0x84F6
#define GL_PROXY_TEXTURE_RECTANGLE_EXT    0x84F7
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT 0x84F8
#endif

#ifndef GL_EXT_depth_bounds_test
#define GL_EXT_depth_bounds_test 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glDepthBoundsEXT (GLclampd, GLclampd);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLDEPTHBOUNDSEXTPROC) (GLclampd zmin, GLclampd zmax);
#endif

#ifndef GL_EXT_fragment_lighting
#define GL_EXT_fragment_lighting 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glFragmentLightModelfEXT (GLenum, GLfloat);
GLAPI void APIENTRY glFragmentLightModelfvEXT (GLenum, GLfloat *);
GLAPI void APIENTRY glFragmentLightModeliEXT (GLenum, GLint);
GLAPI void APIENTRY glFragmentLightModelivEXT (GLenum, GLint *);
GLAPI void APIENTRY glFragmentLightfEXT (GLenum, GLenum, GLfloat);
GLAPI void APIENTRY glFragmentLightfvEXT (GLenum, GLenum, GLfloat *);
GLAPI void APIENTRY glFragmentLightiEXT (GLenum, GLenum, GLint);
GLAPI void APIENTRY glFragmentLightivEXT (GLenum, GLenum, GLint *);
GLAPI void APIENTRY glGetFragmentLightfvEXT (GLenum, GLenum, GLfloat *);
GLAPI void APIENTRY glGetFragmentLightivEXT (GLenum, GLenum, GLint *);
GLAPI void APIENTRY glFragmentMaterialfEXT (GLenum, GLenum, const GLfloat);
GLAPI void APIENTRY glFragmentMaterialfvEXT (GLenum, GLenum, const GLfloat *);
GLAPI void APIENTRY glFragmentMaterialiEXT (GLenum, GLenum, const GLint);
GLAPI void APIENTRY glFragmentMaterialivEXT (GLenum, GLenum, const GLint *);
GLAPI void APIENTRY glFragmentColorMaterialEXT (GLenum, GLenum);
GLAPI void APIENTRY glGetFragmentMaterialfvEXT (GLenum, GLenum, const GLfloat *);
GLAPI void APIENTRY glGetFragmentMaterialivEXT (GLenum, GLenum, const GLint *);
GLAPI void APIENTRY glLightEnviEXT (GLenum, GLint);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTMODELFEXTPROC) (GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTMODELFVEXTPROC) (GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTMODELIEXTPROC) (GLenum pname, GLint param);
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTMODELIVEXTPROC) (GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTFEXTPROC) (GLenum light, GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTFVEXTPROC) (GLenum light, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTIEXTPROC) (GLenum light, GLenum pname, GLint param);
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTIVEXTPROC) (GLenum light, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETFRAGMENTLIGHTFVEXTPROC) (GLenum light, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETFRAGMENTLIGHTIVEXTPROC) (GLenum light, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLFRAGMENTMATERIALFEXTPROC) (GLenum face, GLenum pname, const GLfloat param);
typedef void (APIENTRY * PFNGLFRAGMENTMATERIALFVEXTPROC) (GLenum face, GLenum pname, const GLfloat *params);
typedef void (APIENTRY * PFNGLFRAGMENTMATERIALIEXTPROC) (GLenum face, GLenum pname, const GLint param);
typedef void (APIENTRY * PFNGLFRAGMENTMATERIALIVEXTPROC) (GLenum face, GLenum pname, const GLint *params);
typedef void (APIENTRY * PFNGLFRAGMENTCOLORMATERIALEXTPROC) (GLenum face, GLenum mode);
typedef void (APIENTRY * PFNGLGETFRAGMENTMATERIALFVEXTPROC) (GLenum face, GLenum pname, const GLfloat *params);
typedef void (APIENTRY * PFNGLGETFRAGMENTMATERIALIVEXTPROC) (GLenum face, GLenum pname, const GLint *params);
typedef void (APIENTRY * PFNGLLIGHTENVIEXTPROC) (GLenum pname, GLint param);
#endif

#ifndef GL_EXT_multitexture
#define GL_EXT_multitexture 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glMultiTexCoord1dEXT (GLenum, GLdouble);
GLAPI void APIENTRY glMultiTexCoord1dvEXT (GLenum, const GLdouble *);
GLAPI void APIENTRY glMultiTexCoord1fEXT (GLenum, GLfloat);
GLAPI void APIENTRY glMultiTexCoord1fvEXT (GLenum, const GLfloat *);
GLAPI void APIENTRY glMultiTexCoord1iEXT (GLenum, GLint);
GLAPI void APIENTRY glMultiTexCoord1ivEXT (GLenum, const GLint *);
GLAPI void APIENTRY glMultiTexCoord1sEXT (GLenum, GLshort);
GLAPI void APIENTRY glMultiTexCoord1svEXT (GLenum, const GLshort *);
GLAPI void APIENTRY glMultiTexCoord2dEXT (GLenum, GLdouble, GLdouble);
GLAPI void APIENTRY glMultiTexCoord2dvEXT (GLenum, const GLdouble *);
GLAPI void APIENTRY glMultiTexCoord2fEXT (GLenum, GLfloat, GLfloat);
GLAPI void APIENTRY glMultiTexCoord2fvEXT (GLenum, const GLfloat *);
GLAPI void APIENTRY glMultiTexCoord2iEXT (GLenum, GLint, GLint);
GLAPI void APIENTRY glMultiTexCoord2ivEXT (GLenum, const GLint *);
GLAPI void APIENTRY glMultiTexCoord2sEXT (GLenum, GLshort, GLshort);
GLAPI void APIENTRY glMultiTexCoord2svEXT (GLenum, const GLshort *);
GLAPI void APIENTRY glMultiTexCoord3dEXT (GLenum, GLdouble, GLdouble, GLdouble);
GLAPI void APIENTRY glMultiTexCoord3dvEXT (GLenum, const GLdouble *);
GLAPI void APIENTRY glMultiTexCoord3fEXT (GLenum, GLfloat, GLfloat, GLfloat);
GLAPI void APIENTRY glMultiTexCoord3fvEXT (GLenum, const GLfloat *);
GLAPI void APIENTRY glMultiTexCoord3iEXT (GLenum, GLint, GLint, GLint);
GLAPI void APIENTRY glMultiTexCoord3ivEXT (GLenum, const GLint *);
GLAPI void APIENTRY glMultiTexCoord3sEXT (GLenum, GLshort, GLshort, GLshort);
GLAPI void APIENTRY glMultiTexCoord3svEXT (GLenum, const GLshort *);
GLAPI void APIENTRY glMultiTexCoord4dEXT (GLenum, GLdouble, GLdouble, GLdouble, GLdouble);
GLAPI void APIENTRY glMultiTexCoord4dvEXT (GLenum, const GLdouble *);
GLAPI void APIENTRY glMultiTexCoord4fEXT (GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
GLAPI void APIENTRY glMultiTexCoord4fvEXT (GLenum, const GLfloat *);
GLAPI void APIENTRY glMultiTexCoord4iEXT (GLenum, GLint, GLint, GLint, GLint);
GLAPI void APIENTRY glMultiTexCoord4ivEXT (GLenum, const GLint *);
GLAPI void APIENTRY glMultiTexCoord4sEXT (GLenum, GLshort, GLshort, GLshort, GLshort);
GLAPI void APIENTRY glMultiTexCoord4svEXT (GLenum, const GLshort *);
GLAPI void APIENTRY glInterleavedTextureCoordSetsEXT (GLint);
GLAPI void APIENTRY glSelectTextureEXT (GLenum);
GLAPI void APIENTRY glSelectTextureCoordSetEXT (GLenum);
GLAPI void APIENTRY glSelectTextureTransformEXT (GLenum);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLMULTITEXCOORD1DEXTPROC) (GLenum target, GLdouble s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1DVEXTPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1FEXTPROC) (GLenum target, GLfloat s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1FVEXTPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1IEXTPROC) (GLenum target, GLint s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1IVEXTPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1SEXTPROC) (GLenum target, GLshort s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1SVEXTPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2DEXTPROC) (GLenum target, GLdouble s, GLdouble t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2DVEXTPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2FEXTPROC) (GLenum target, GLfloat s, GLfloat t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2FVEXTPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2IEXTPROC) (GLenum target, GLint s, GLint t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2IVEXTPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2SEXTPROC) (GLenum target, GLshort s, GLshort t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2SVEXTPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3DEXTPROC) (GLenum target, GLdouble s, GLdouble t, GLdouble r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3DVEXTPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3FEXTPROC) (GLenum target, GLfloat s, GLfloat t, GLfloat r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3FVEXTPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3IEXTPROC) (GLenum target, GLint s, GLint t, GLint r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3IVEXTPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3SEXTPROC) (GLenum target, GLshort s, GLshort t, GLshort r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3SVEXTPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4DEXTPROC) (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4DVEXTPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4FEXTPROC) (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4FVEXTPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4IEXTPROC) (GLenum target, GLint s, GLint t, GLint r, GLint q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4IVEXTPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4SEXTPROC) (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4SVEXTPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLINTERLEAVEDTEXTURECOORDSETSEXTPROC) (GLint factor);
typedef void (APIENTRY * PFNGLSELECTTEXTUREEXTPROC) (GLenum target);
typedef void (APIENTRY * PFNGLSELECTTEXTURECOORDSETEXTPROC) (GLenum target);
typedef void (APIENTRY * PFNGLSELECTTEXTURETRANSFORMEXTPROC) (GLenum target);
#endif

/* unknown */
#ifndef GL_EXT_scene_marker
/* #define GL_EXT_scene_marker 1 */
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glBeginSceneEXT (void);
GLAPI void APIENTRY glEndSceneEXT (void);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLBEGINSCENEEXTPROC) (void);
typedef void (APIENTRY * PFNGLENDSCENEEXTPROC) (void);
#endif

#ifndef GL_EXT_texgen_reflection
#define GL_EXT_texgen_reflection 1
#endif

#ifndef GL_EXT_texture_edge_clamp
#define GL_EXT_texture_edge_clamp 1
#endif

#ifndef GL_EXT_texture_rectangle
#define GL_EXT_texture_rectangle 1
#endif

/*
 * HP
 */

/*
 * IBM
 */

#ifndef GL_IBM_static_data
#define GL_ALL_STATIC_DATA_IBM            103060
#define GL_STATIC_VERTEX_ARRAY_IBM        103061
#endif

#ifndef GL_IBM_static_data
#define GL_IBM_static_data 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glFlushStaticDataIBM (GLenum);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLFLUSHSTATICDATAIBMPROC) (GLenum target);
#endif

/*
 * INGR
 */

/*
 * INTEL
 */

/*
 * KTX
 */

#ifndef GL_KTX_buffer_region
#define GL_KTX_FRONT_REGION               0x0
#define GL_KTX_BACK_REGION                0x1
#define GL_KTX_Z_REGION                   0x2
#define GL_KTX_STENCIL_REGION             0x3
#endif

#ifndef GL_KTX_buffer_region
#define GL_KTX_buffer_region 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI GLuint APIENTRY glBufferRegionEnabled (void);
GLAPI GLuint APIENTRY glNewBufferRegion (GLenum);
GLAPI void APIENTRY glDeleteBufferRegion (GLenum);
GLAPI void APIENTRY glReadBufferRegion (GLuint, GLint, GLint, GLsizei, GLsizei);
GLAPI void APIENTRY glDrawBufferRegion (GLuint, GLint, GLint, GLsizei, GLsizei, GLint, GLint);
#endif /* GL_GLEXT_PROTOTYPES */
typedef GLuint (APIENTRY * PFNGLBUFFERREGIONENABLEDPROC) (void);
typedef GLuint (APIENTRY * PFNGLNEWBUFFERREGIONPROC) (GLenum region);
typedef void (APIENTRY * PFNGLDELETEBUFFERREGIONPROC) (GLenum region);
typedef void (APIENTRY * PFNGLREADBUFFERREGIONPROC) (GLuint region, GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRY * PFNGLDRAWBUFFERREGIONPROC) (GLuint region, GLint x, GLint y, GLsizei width, GLsizei height, GLint xDest, GLint yDest);
#endif

/*
 * MESA
 */

/*
 * NV
 */

#ifndef GL_NV_element_array
#define GL_ELEMENT_ARRAY_TYPE_NV          0x8769
#define GL_ELEMENT_ARRAY_POINTER_NV       0x876A
#endif

#ifndef GL_NV_stencil_two_side
#define GL_STENCIL_TEST_TWO_SIDE_NV       0x8910
#define GL_ACTIVE_STENCIL_FACE_NV         0x8911
#endif

#ifndef GL_NV_element_array
#define GL_NV_element_array 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glElementPointerNV (GLenum, const GLvoid *);
GLAPI void APIENTRY glDrawElementArrayNV (GLenum, GLint, GLsizei);
GLAPI void APIENTRY glDrawRangeElementArrayNV (GLenum, GLuint, GLuint, GLint, GLsizei);
GLAPI void APIENTRY glMultiDrawElementArrayNV (GLenum, const GLint *, const GLsizei *, GLsizei);
GLAPI void APIENTRY glMultiDrawRangeElementArrayNV (GLenum, GLuint, GLuint, const GLint *, const GLsizei *, GLsizei);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLELEMENTPOINTERNVPROC) (GLenum type, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLDRAWELEMENTARRAYNVPROC) (GLenum mode, GLint first, GLsizei count);
typedef void (APIENTRY * PFNGLDRAWRANGEELEMENTARRAYNVPROC) (GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count);
typedef void (APIENTRY * PFNGLMULTIDRAWELEMENTARRAYNVPROC) (GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount);
typedef void (APIENTRY * PFNGLMULTIDRAWRANGEELEMENTARRAYNVPROC) (GLenum mode, GLuint start, GLuint end, const GLint *first, const GLsizei *count, GLsizei primcount);
#endif

#ifndef GL_NV_stencil_two_side
#define GL_NV_stencil_two_side 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glActiveStencilFaceNV (GLenum);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLACTIVESTENCILFACENVPROC) (GLenum face);
#endif

/*
 * OML
 */

/*
 * PGI
 */

/*
 * REND
 */

/*
 * S3
 */

/*
 * SGI
 */

#ifndef GL_SGI_texture_edge_clamp
#define GL_CLAMP_TO_EDGE_SGI              0x812F
#endif

#ifndef GL_SGI_texture_edge_clamp
#define GL_SGI_texture_edge_clamp 1
#endif

/*
 * SGIS
 */

#ifndef GL_SGIS_color_range
#define EXTENDED_RANGE_SGIS             0x85A5
#define MIN_RED_SGIS                    0x85A6
#define MAX_RED_SGIS                    0x85A7
#define MIN_GREEN_SGIS                  0x85A8
#define MAX_GREEN_SGIS                  0x85A9
#define MIN_BLUE_SGIS                   0x85AA
#define MAX_BLUE_SGIS                   0x85AB
#define MIN_ALPHA_SGIS                  0x85AC
#define MAX_ALPHA_SGIS                  0x85AD
#endif

#ifndef GL_SGIS_multitexture
#define GL_SELECTED_TEXTURE_SGIS	   0x835C
#define GL_SELECTED_TEXTURE_COORD_SET_SGIS 0x835D
#define GL_MAX_TEXTURES_SGIS		   0x835E
#define GL_TEXTURE0_SGIS                   0x835F
#define GL_TEXTURE1_SGIS                   0x8360
#define GL_TEXTURE2_SGIS                   0x8361
#define GL_TEXTURE3_SGIS                   0x8362
#define GL_TEXTURE4_SGIS                   0x8363
#define GL_TEXTURE5_SGIS                   0x8364
#define GL_TEXTURE6_SGIS                   0x8365
#define GL_TEXTURE7_SGIS                   0x8366
#define GL_TEXTURE8_SGIS                   0x8367
#define GL_TEXTURE9_SGIS                   0x8368
#define GL_TEXTURE10_SGIS                  0x8369
#define GL_TEXTURE11_SGIS                  0x836A
#define GL_TEXTURE12_SGIS                  0x836B
#define GL_TEXTURE13_SGIS                  0x836C
#define GL_TEXTURE14_SGIS                  0x836D
#define GL_TEXTURE15_SGIS                  0x836E
#define GL_TEXTURE16_SGIS                  0x836F
#define GL_TEXTURE17_SGIS                  0x8370
#define GL_TEXTURE18_SGIS                  0x8371
#define GL_TEXTURE19_SGIS                  0x8372
#define GL_TEXTURE20_SGIS                  0x8373
#define GL_TEXTURE21_SGIS                  0x8374
#define GL_TEXTURE22_SGIS                  0x8375
#define GL_TEXTURE23_SGIS                  0x8376
#define GL_TEXTURE24_SGIS                  0x8377
#define GL_TEXTURE25_SGIS                  0x8378
#define GL_TEXTURE26_SGIS                  0x8379
#define GL_TEXTURE27_SGIS                  0x837A
#define GL_TEXTURE28_SGIS                  0x837B
#define GL_TEXTURE29_SGIS                  0x837C
#define GL_TEXTURE30_SGIS                  0x837D
#define GL_TEXTURE31_SGIS                  0x837E
#define GL_TEXTURE_COORD_SET_SOURCE_SGIS   0x8363
#endif

#ifndef GL_SGIS_color_range
#define GL_SGIS_color_range 1
#endif

#ifndef GL_SGIS_multitexture
#define GL_SGIS_multitexture 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glMultiTexCoord1dSGIS (GLenum, GLdouble);
GLAPI void APIENTRY glMultiTexCoord1dvSGIS (GLenum, const GLdouble *);
GLAPI void APIENTRY glMultiTexCoord1fSGIS (GLenum, GLfloat);
GLAPI void APIENTRY glMultiTexCoord1fvSGIS (GLenum, const GLfloat *);
GLAPI void APIENTRY glMultiTexCoord1iSGIS (GLenum, GLint);
GLAPI void APIENTRY glMultiTexCoord1ivSGIS (GLenum, const GLint *);
GLAPI void APIENTRY glMultiTexCoord1sSGIS (GLenum, GLshort);
GLAPI void APIENTRY glMultiTexCoord1svSGIS (GLenum, const GLshort *);
GLAPI void APIENTRY glMultiTexCoord2dSGIS (GLenum, GLdouble, GLdouble);
GLAPI void APIENTRY glMultiTexCoord2dvSGIS (GLenum, const GLdouble *);
GLAPI void APIENTRY glMultiTexCoord2fSGIS (GLenum, GLfloat, GLfloat);
GLAPI void APIENTRY glMultiTexCoord2fvSGIS (GLenum, const GLfloat *);
GLAPI void APIENTRY glMultiTexCoord2iSGIS (GLenum, GLint, GLint);
GLAPI void APIENTRY glMultiTexCoord2ivSGIS (GLenum, const GLint *);
GLAPI void APIENTRY glMultiTexCoord2sSGIS (GLenum, GLshort, GLshort);
GLAPI void APIENTRY glMultiTexCoord2svSGIS (GLenum, const GLshort *);
GLAPI void APIENTRY glMultiTexCoord3dSGIS (GLenum, GLdouble, GLdouble, GLdouble);
GLAPI void APIENTRY glMultiTexCoord3dvSGIS (GLenum, const GLdouble *);
GLAPI void APIENTRY glMultiTexCoord3fSGIS (GLenum, GLfloat, GLfloat, GLfloat);
GLAPI void APIENTRY glMultiTexCoord3fvSGIS (GLenum, const GLfloat *);
GLAPI void APIENTRY glMultiTexCoord3iSGIS (GLenum, GLint, GLint, GLint);
GLAPI void APIENTRY glMultiTexCoord3ivSGIS (GLenum, const GLint *);
GLAPI void APIENTRY glMultiTexCoord3sSGIS (GLenum, GLshort, GLshort, GLshort);
GLAPI void APIENTRY glMultiTexCoord3svSGIS (GLenum, const GLshort *);
GLAPI void APIENTRY glMultiTexCoord4dSGIS (GLenum, GLdouble, GLdouble, GLdouble, GLdouble);
GLAPI void APIENTRY glMultiTexCoord4dvSGIS (GLenum, const GLdouble *);
GLAPI void APIENTRY glMultiTexCoord4fSGIS (GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
GLAPI void APIENTRY glMultiTexCoord4fvSGIS (GLenum, const GLfloat *);
GLAPI void APIENTRY glMultiTexCoord4iSGIS (GLenum, GLint, GLint, GLint, GLint);
GLAPI void APIENTRY glMultiTexCoord4ivSGIS (GLenum, const GLint *);
GLAPI void APIENTRY glMultiTexCoord4sSGIS (GLenum, GLshort, GLshort, GLshort, GLshort);
GLAPI void APIENTRY glMultiTexCoord4svSGIS (GLenum, const GLshort *);
GLAPI void APIENTRY glMultiTexCoordPointerSGIS (GLenum, GLint, GLenum, GLsizei, const void *);
GLAPI void APIENTRY glSelectTextureSGIS (GLenum);
GLAPI void APIENTRY glSelectTextureCoordSetSGIS (GLenum);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLMULTITEXCOORD1DSGISPROC) (GLenum target, GLdouble s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1DVSGISPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1FSGISPROC) (GLenum target, GLfloat s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1FVSGISPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1ISGISPROC) (GLenum target, GLint s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1IVSGISPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1SSGISPROC) (GLenum target, GLshort s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1SVSGISPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2DSGISPROC) (GLenum target, GLdouble s, GLdouble t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2DVSGISPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2FSGISPROC) (GLenum target, GLfloat s, GLfloat t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2FVSGISPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2ISGISPROC) (GLenum target, GLint s, GLint t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2IVSGISPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2SSGISPROC) (GLenum target, GLshort s, GLshort t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2SVSGISPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3DSGISPROC) (GLenum target, GLdouble s, GLdouble t, GLdouble r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3DVSGISPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3FSGISPROC) (GLenum target, GLfloat s, GLfloat t, GLfloat r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3FVSGISPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3ISGISPROC) (GLenum target, GLint s, GLint t, GLint r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3IVSGISPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3SSGISPROC) (GLenum target, GLshort s, GLshort t, GLshort r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3SVSGISPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4DSGISPROC) (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4DVSGISPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4FSGISPROC) (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4FVSGISPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4ISGISPROC) (GLenum target, GLint s, GLint t, GLint r, GLint q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4IVSGISPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4SSGISPROC) (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4SVSGISPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORDPOINTERSGISPROC) (GLenum target, GLint size, GLenum type, GLsizei stride, const void *pointer);
typedef void (APIENTRY * PFNGLSELECTTEXTURESGISPROC) (GLenum target);
typedef void (APIENTRY * PFNGLSELECTTEXTURECOORDSETSGISPROC) (GLenum target);
#endif

/*
 * SGIX
 */

/* unknown */
#ifndef GL_SGIX_fog_texture
/*
#define GL_FRAGMENT_FOG_SGIX              0
#define GL_TEXTURE_FOG_SGIX               0
#define GL_FOG_PATCHY_FACTOR_SGIX         0
*/
#endif

/* unknown */
#ifndef GL_SGIX_pixel_texture_bits
/*
#define GL_COLOR_TO_TEXTURE_COORD_SGIX    0
#define GL_COLOR_BIT_PATTERN_SGIX         0
#define GL_COLOR_VALUE_SGIX               0
*/
#endif

#ifndef GL_SGIX_texture_range
#define GL_RGB_SIGNED_SGIX                         0x85E0
#define GL_RGBA_SIGNED_SGIX                        0x85E1
#define GL_ALPHA_SIGNED_SGIX                       0x85E2
#define GL_LUMINANCE_SIGNED_SGIX                   0x85E3
#define GL_INTENSITY_SIGNED_SGIX                   0x85E4
#define GL_LUMINANCE_ALPHA_SIGNED_SGIX             0x85E5
#define GL_RGB16_SIGNED_SGIX                       0x85E6
#define GL_RGBA16_SIGNED_SGIX                      0x85E7
#define GL_ALPHA16_SIGNED_SGIX                     0x85E8
#define GL_LUMINANCE16_SIGNED_SGIX                 0x85E9
#define GL_INTENSITY16_SIGNED_SGIX                 0x85EA
#define GL_LUMINANCE16_ALPHA16_SIGNED_SGIX         0x85EB
#define GL_RGB_EXTENDED_RANGE_SGIX                 0x85EC
#define GL_RGBA_EXTENDED_RANGE_SGIX                0x85ED
#define GL_ALPHA_EXTENDED_RANGE_SGIX               0x85EE
#define GL_LUMINANCE_EXTENDED_RANGE_SGIX           0x85EF
#define GL_INTENSITY_EXTENDED_RANGE_SGIX           0x85F0
#define GL_LUMINANCE_ALPHA_EXTENDED_RANGE_SGIX     0x85F1
#define GL_RGB16_EXTENDED_RANGE_SGIX               0x85F2
#define GL_RGBA16_EXTENDED_RANGE_SGIX              0x85F3
#define GL_ALPHA16_EXTENDED_RANGE_SGIX             0x85F4
#define GL_LUMINANCE16_EXTENDED_RANGE_SGIX         0x85F5
#define GL_INTENSITY16_EXTENDED_RANGE_SGIX         0x85F6
#define GL_LUMINANCE16_ALPHA16_EXTENDED_RANGE_SGIX 0x85F7
#define GL_MIN_LUMINANCE_SGIS                      0x85F8
#define GL_MAX_LUMINANCE_SGIS                      0x85F9
#define GL_MIN_INTENSITY_SGIS                      0x85FA
#define GL_MAX_INTENSITY_SGIS                      0x85FB
#endif

#ifndef GL_SGIX_vertex_preclip_hint
/* defined in glext.h
#define GL_VERTEX_PRECLIP_HINT_SGIX       0x83EF
*/
#endif

/* unknown */
#ifndef GL_SGIX_fog_texture
/* #define GL_SGIX_fog_texture 1 */
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glTextureFogSGIX (GLenum pname);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLTEXTUREFOGSGIXPROC) (GLenum pname);
#endif

/* unknown */
#ifndef GL_SGIX_pixel_texture_bits
/* #define GL_SGIX_pixel_texture_bits 1 */
#endif

#ifndef GL_SGIX_texture_range
#define GL_SGIX_texture_range 1
#endif

#ifndef GL_SGIX_vertex_preclip_hint
#define GL_SGIX_vertex_preclip_hint 1
#endif

/*
 * SUN
 */

#ifndef GL_SUN_multi_draw_arrays
#define GL_SUN_multi_draw_arrays 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glMultiDrawArraysSUN (GLenum, GLint *, GLsizei *, GLsizei);
GLAPI void APIENTRY glMultiDrawElementsSUN (GLenum, const GLsizei *, GLenum, const GLvoid* *, GLsizei);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLMULTIDRAWARRAYSSUNPROC) (GLenum mode, GLint *first, GLsizei *count, GLsizei primcount);
typedef void (APIENTRY * PFNGLMULTIDRAWELEMENTSSUNPROC) (GLenum mode, const GLsizei *count, GLenum type, const GLvoid* *indices, GLsizei primcount);
#endif

/*
 * SUNX
 */

/*
 * WIN
 */

#ifndef GL_WIN_swap_hint
#define GL_WIN_swap_hint 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glAddSwapHintRectWIN (GLint, GLint, GLsizei, GLsizei);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLADDSWAPHINTRECTWINPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __glext_extra_h_ */
