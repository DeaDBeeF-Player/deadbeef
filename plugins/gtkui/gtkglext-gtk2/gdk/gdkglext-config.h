/* gdkglext-config.h
 *
 * This is a generated file.  Please modify `configure.in'
 */

#ifndef GDKGLEXT_CONFIG_H
#define GDKGLEXT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GDKGLEXT_WINDOWING_X11

#define GDKGLEXT_MULTIHEAD_SUPPORT

#if !defined(GDKGLEXT_MULTIHEAD_SUPPORT) && defined(GDK_MULTIHEAD_SAFE)
#error "Installed GdkGLExt library doesn't have multihead support."
#endif








#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GDKGLEXT_CONFIG_H */
