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

#include <math.h>

#include <glib.h>

#include "gdkglprivate.h"
#include "gdkglshapes.h"

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>

/* 
 * The following code is imported from GLUT.
 */

/* Copyright (c) Mark J. Kilgard, 1994, 1997. */

/**
(c) Copyright 1993, Silicon Graphics, Inc.

ALL RIGHTS RESERVED

Permission to use, copy, modify, and distribute this software
for any purpose and without fee is hereby granted, provided
that the above copyright notice appear in all copies and that
both the copyright notice and this permission notice appear in
supporting documentation, and that the name of Silicon
Graphics, Inc. not be used in advertising or publicity
pertaining to distribution of the software without specific,
written prior permission.

THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU
"AS-IS" AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR
OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  IN NO
EVENT SHALL SILICON GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE
ELSE FOR ANY DIRECT, SPECIAL, INCIDENTAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER,
INCLUDING WITHOUT LIMITATION, LOSS OF PROFIT, LOSS OF USE,
SAVINGS OR REVENUE, OR THE CLAIMS OF THIRD PARTIES, WHETHER OR
NOT SILICON GRAPHICS, INC.  HAS BEEN ADVISED OF THE POSSIBILITY
OF SUCH LOSS, HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
ARISING OUT OF OR IN CONNECTION WITH THE POSSESSION, USE OR
PERFORMANCE OF THIS SOFTWARE.

US Government Users Restricted Rights

Use, duplication, or disclosure by the Government is subject to
restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
(c)(1)(ii) of the Rights in Technical Data and Computer
Software clause at DFARS 252.227-7013 and/or in similar or
successor clauses in the FAR or the DOD or NASA FAR
Supplement.  Unpublished-- rights reserved under the copyright
laws of the United States.  Contractor/manufacturer is Silicon
Graphics, Inc., 2011 N.  Shoreline Blvd., Mountain View, CA
94039-7311.

OpenGL(TM) is a trademark of Silicon Graphics, Inc.
*/

/* 
 * Cube
 */

static void
drawBox(GLfloat size, GLenum type)
{
  static GLfloat n[6][3] =
  {
    {-1.0, 0.0, 0.0},
    {0.0, 1.0, 0.0},
    {1.0, 0.0, 0.0},
    {0.0, -1.0, 0.0},
    {0.0, 0.0, 1.0},
    {0.0, 0.0, -1.0}
  };
  static GLint faces[6][4] =
  {
    {0, 1, 2, 3},
    {3, 2, 6, 7},
    {7, 6, 5, 4},
    {4, 5, 1, 0},
    {5, 6, 2, 1},
    {7, 4, 0, 3}
  };
  GLfloat v[8][3];
  GLint i;

  v[0][0] = v[1][0] = v[2][0] = v[3][0] = -size / 2;
  v[4][0] = v[5][0] = v[6][0] = v[7][0] = size / 2;
  v[0][1] = v[1][1] = v[4][1] = v[5][1] = -size / 2;
  v[2][1] = v[3][1] = v[6][1] = v[7][1] = size / 2;
  v[0][2] = v[3][2] = v[4][2] = v[7][2] = -size / 2;
  v[1][2] = v[2][2] = v[5][2] = v[6][2] = size / 2;

  for (i = 5; i >= 0; i--) {
    glBegin(type);
    glNormal3fv(&n[i][0]);
    glVertex3fv(&v[faces[i][0]][0]);
    glVertex3fv(&v[faces[i][1]][0]);
    glVertex3fv(&v[faces[i][2]][0]);
    glVertex3fv(&v[faces[i][3]][0]);
    glEnd();
  }
}

/**
 * gdk_gl_draw_cube:
 * @solid: TRUE if the cube should be solid.
 * @size: length of cube sides.
 *
 * Renders a cube.
 * The cube is centered at the modeling coordinates origin with sides of
 * length @size.
 *
 **/
void
gdk_gl_draw_cube (gboolean solid,
                  double   size)
{
  if (solid)
    drawBox (size, GL_QUADS);
  else
    drawBox (size, GL_LINE_LOOP);
}

/*
 * Quadrics
 */

static GLUquadricObj *quadObj = NULL;

#define QUAD_OBJ_INIT() { if(!quadObj) initQuadObj(); }

static void
initQuadObj(void)
{
  quadObj = gluNewQuadric();
  if (!quadObj)
    g_error("out of memory.");
}

/**
 * gdk_gl_draw_sphere:
 * @solid: TRUE if the sphere should be solid.
 * @radius: the radius of the sphere.
 * @slices: the number of subdivisions around the Z axis (similar to lines of
 *          longitude).
 * @stacks: the number of subdivisions along the Z axis (similar to lines of
 *          latitude).
 *
 * Renders a sphere centered at the modeling coordinates origin of
 * the specified @radius. The sphere is subdivided around the Z axis into
 * @slices and along the Z axis into @stacks. 
 *
 **/
void
gdk_gl_draw_sphere (gboolean solid,
                    double   radius,
                    int      slices,
                    int      stacks)
{
  QUAD_OBJ_INIT();

  if (solid)
    gluQuadricDrawStyle (quadObj, GLU_FILL);
  else
    gluQuadricDrawStyle (quadObj, GLU_LINE);

  gluQuadricNormals (quadObj, GLU_SMOOTH);

  /* If we ever changed/used the texture or orientation state
     of quadObj, we'd need to change it to the defaults here
     with gluQuadricTexture and/or gluQuadricOrientation. */
  gluSphere (quadObj, radius, slices, stacks);
}

/**
 * gdk_gl_draw_cone:
 * @solid: TRUE if the cone should be solid.
 * @base: the radius of the base of the cone.
 * @height: the height of the cone.
 * @slices: the number of subdivisions around the Z axis.
 * @stacks: the number of subdivisions along the Z axis.
 *
 * Renders a cone oriented along the Z axis.
 * The @base of the cone is placed at Z = 0, and the top at Z = @height.
 * The cone is subdivided around the Z axis into @slices, and along
 * the Z axis into @stacks. 
 *
 **/
void
gdk_gl_draw_cone (gboolean solid,
                  double   base,
                  double   height,
                  int      slices,
                  int      stacks)
{
  QUAD_OBJ_INIT();

  if (solid)
    gluQuadricDrawStyle (quadObj, GLU_FILL);
  else
    gluQuadricDrawStyle (quadObj, GLU_LINE);

  gluQuadricNormals (quadObj, GLU_SMOOTH);

  /* If we ever changed/used the texture or orientation state
     of quadObj, we'd need to change it to the defaults here
     with gluQuadricTexture and/or gluQuadricOrientation. */
  gluCylinder (quadObj, base, 0.0, height, slices, stacks);
}

/* 
 * Torus
 */

static void
doughnut(GLfloat r, GLfloat R, GLint nsides, GLint rings)
{
  int i, j;
  GLfloat theta, phi, theta1;
  GLfloat cosTheta, sinTheta;
  GLfloat cosTheta1, sinTheta1;
  GLfloat ringDelta, sideDelta;

  ringDelta = 2.0 * G_PI / rings;
  sideDelta = 2.0 * G_PI / nsides;

  theta = 0.0;
  cosTheta = 1.0;
  sinTheta = 0.0;
  for (i = rings - 1; i >= 0; i--) {
    theta1 = theta + ringDelta;
    cosTheta1 = cos(theta1);
    sinTheta1 = sin(theta1);
    glBegin(GL_QUAD_STRIP);
    phi = 0.0;
    for (j = nsides; j >= 0; j--) {
      GLfloat cosPhi, sinPhi, dist;

      phi += sideDelta;
      cosPhi = cos(phi);
      sinPhi = sin(phi);
      dist = R + r * cosPhi;

      glNormal3f(cosTheta1 * cosPhi, -sinTheta1 * cosPhi, sinPhi);
      glVertex3f(cosTheta1 * dist, -sinTheta1 * dist, r * sinPhi);
      glNormal3f(cosTheta * cosPhi, -sinTheta * cosPhi, sinPhi);
      glVertex3f(cosTheta * dist, -sinTheta * dist,  r * sinPhi);
    }
    glEnd();
    theta = theta1;
    cosTheta = cosTheta1;
    sinTheta = sinTheta1;
  }
}

/**
 * gdk_gl_draw_torus:
 * @solid: TRUE if the torus should be solid.
 * @inner_radius: inner radius of the torus.
 * @outer_radius: outer radius of the torus.
 * @nsides: number of sides for each radial section.
 * @rings: number of radial divisions for the torus.
 *
 * Renders a torus (doughnut) centered at the modeling coordinates
 * origin whose axis is aligned with the Z axis.
 *
 **/
void
gdk_gl_draw_torus (gboolean solid,
                   double   inner_radius,
                   double   outer_radius,
                   int      nsides,
                   int      rings)
{
  if (solid)
    {
      doughnut (inner_radius, outer_radius, nsides, rings);
    }
  else
    {
      glPushAttrib (GL_POLYGON_BIT);
      glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
      doughnut (inner_radius, outer_radius, nsides, rings);
      glPopAttrib ();
    }
}

#define DIFF3(_a,_b,_c) { \
    (_c)[0] = (_a)[0] - (_b)[0]; \
    (_c)[1] = (_a)[1] - (_b)[1]; \
    (_c)[2] = (_a)[2] - (_b)[2]; \
}

static void
crossprod(GLfloat v1[3], GLfloat v2[3], GLfloat prod[3])
{
  GLfloat p[3];         /* in case prod == v1 or v2 */

  p[0] = v1[1] * v2[2] - v2[1] * v1[2];
  p[1] = v1[2] * v2[0] - v2[2] * v1[0];
  p[2] = v1[0] * v2[1] - v2[0] * v1[1];
  prod[0] = p[0];
  prod[1] = p[1];
  prod[2] = p[2];
}

static void
normalize(GLfloat v[3])
{
  GLfloat d;

  d = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  if (d == 0.0) {
    g_warning("normalize: zero length vector");
    v[0] = d = 1.0;
  }
  d = 1 / d;
  v[0] *= d;
  v[1] *= d;
  v[2] *= d;
}

static void
recorditem(GLfloat * n1, GLfloat * n2, GLfloat * n3,
           GLenum shadeType)
{
  GLfloat q0[3], q1[3];

  DIFF3(n1, n2, q0);
  DIFF3(n2, n3, q1);
  crossprod(q0, q1, q1);
  normalize(q1);

  glBegin(shadeType);
  glNormal3fv(q1);
  glVertex3fv(n1);
  glVertex3fv(n2);
  glVertex3fv(n3);
  glEnd();
}

static void
subdivide(GLfloat * v0, GLfloat * v1, GLfloat * v2,
          GLenum shadeType)
{
  int depth;
  GLfloat w0[3], w1[3], w2[3];
  GLfloat l;
  int i, j, k, n;

  depth = 1;
  for (i = 0; i < depth; i++) {
    for (j = 0; i + j < depth; j++) {
      k = depth - i - j;
      for (n = 0; n < 3; n++) {
        w0[n] = (i * v0[n] + j * v1[n] + k * v2[n]) / depth;
        w1[n] = ((i + 1) * v0[n] + j * v1[n] + (k - 1) * v2[n])
          / depth;
        w2[n] = (i * v0[n] + (j + 1) * v1[n] + (k - 1) * v2[n])
          / depth;
      }
      l = sqrt(w0[0] * w0[0] + w0[1] * w0[1] + w0[2] * w0[2]);
      w0[0] /= l;
      w0[1] /= l;
      w0[2] /= l;
      l = sqrt(w1[0] * w1[0] + w1[1] * w1[1] + w1[2] * w1[2]);
      w1[0] /= l;
      w1[1] /= l;
      w1[2] /= l;
      l = sqrt(w2[0] * w2[0] + w2[1] * w2[1] + w2[2] * w2[2]);
      w2[0] /= l;
      w2[1] /= l;
      w2[2] /= l;
      recorditem(w1, w0, w2, shadeType);
    }
  }
}

static void
drawtriangle(int i, GLfloat data[][3], int ndx[][3],
             GLenum shadeType)
{
  GLfloat *x0, *x1, *x2;

  x0 = data[ndx[i][0]];
  x1 = data[ndx[i][1]];
  x2 = data[ndx[i][2]];
  subdivide(x0, x1, x2, shadeType);
}

/* 
 * Tetrahedron
 */

/* tetrahedron data: */

#define T       1.73205080756887729

static GLfloat tdata[4][3] =
{
  {T, T, T},
  {T, -T, -T},
  {-T, T, -T},
  {-T, -T, T}
};

static int tndex[4][3] =
{
  {0, 1, 3},
  {2, 1, 0},
  {3, 2, 0},
  {1, 2, 3}
};

static void
tetrahedron(GLenum shadeType)
{
  int i;

  for (i = 3; i >= 0; i--)
    drawtriangle(i, tdata, tndex, shadeType);
}

/**
 * gdk_gl_draw_tetrahedron:
 * @solid: TRUE if the tetrahedron should be solid.
 *
 * Renders a tetrahedron centered at the modeling coordinates
 * origin with a radius of the square root of 3.
 *
 **/
void
gdk_gl_draw_tetrahedron (gboolean solid)
{
  if (solid)
    tetrahedron (GL_TRIANGLES);
  else
    tetrahedron (GL_LINE_LOOP);
}

/* 
 * Octahedron
 */

/* octahedron data: The octahedron produced is centered at the
   origin and has radius 1.0 */
static GLfloat odata[6][3] =
{
  {1.0, 0.0, 0.0},
  {-1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0},
  {0.0, -1.0, 0.0},
  {0.0, 0.0, 1.0},
  {0.0, 0.0, -1.0}
};

static int ondex[8][3] =
{
  {0, 4, 2},
  {1, 2, 4},
  {0, 3, 4},
  {1, 4, 3},
  {0, 2, 5},
  {1, 5, 2},
  {0, 5, 3},
  {1, 3, 5}
};

static void
octahedron(GLenum shadeType)
{
  int i;

  for (i = 7; i >= 0; i--) {
    drawtriangle(i, odata, ondex, shadeType);
  }
}

/**
 * gdk_gl_draw_octahedron:
 * @solid: TRUE if the octahedron should be solid.
 *
 * Renders a octahedron centered at the modeling coordinates
 * origin with a radius of 1.0.
 *
 **/
void
gdk_gl_draw_octahedron (gboolean solid)
{
  if (solid)
    octahedron (GL_TRIANGLES);
  else
    octahedron (GL_LINE_LOOP);
}

/* 
 * Icosahedron
 */

/* icosahedron data: These numbers are rigged to make an
   icosahedron of radius 1.0 */

#define X .525731112119133606
#define Z .850650808352039932

static GLfloat idata[12][3] =
{
  {-X, 0, Z},
  {X, 0, Z},
  {-X, 0, -Z},
  {X, 0, -Z},
  {0, Z, X},
  {0, Z, -X},
  {0, -Z, X},
  {0, -Z, -X},
  {Z, X, 0},
  {-Z, X, 0},
  {Z, -X, 0},
  {-Z, -X, 0}
};

static int index[20][3] =
{
  {0, 4, 1},
  {0, 9, 4},
  {9, 5, 4},
  {4, 5, 8},
  {4, 8, 1},
  {8, 10, 1},
  {8, 3, 10},
  {5, 3, 8},
  {5, 2, 3},
  {2, 7, 3},
  {7, 10, 3},
  {7, 6, 10},
  {7, 11, 6},
  {11, 0, 6},
  {0, 1, 6},
  {6, 1, 10},
  {9, 0, 11},
  {9, 11, 2},
  {9, 2, 5},
  {7, 2, 11},
};

static void
icosahedron(GLenum shadeType)
{
  int i;

  for (i = 19; i >= 0; i--) {
    drawtriangle(i, idata, index, shadeType);
  }
}

/**
 * gdk_gl_draw_icosahedron:
 * @solid: TRUE if the icosahedron should be solid.
 *
 * Renders a icosahedron.
 * The icosahedron is centered at the modeling coordinates origin
 * and has a radius of 1.0. 
 *
 **/
void
gdk_gl_draw_icosahedron (gboolean solid)
{
  if (solid)
    icosahedron (GL_TRIANGLES);
  else
    icosahedron (GL_LINE_LOOP);
}

/* 
 * Dodecahedron
 */

static GLfloat dodec[20][3];

static void
initDodecahedron(void)
{
  GLfloat alpha, beta;

  alpha = sqrt(2.0 / (3.0 + sqrt(5.0)));
  beta = 1.0 + sqrt(6.0 / (3.0 + sqrt(5.0)) -
    2.0 + 2.0 * sqrt(2.0 / (3.0 + sqrt(5.0))));
  /* *INDENT-OFF* */
  dodec[0][0] = -alpha; dodec[0][1] = 0; dodec[0][2] = beta;
  dodec[1][0] = alpha; dodec[1][1] = 0; dodec[1][2] = beta;
  dodec[2][0] = -1; dodec[2][1] = -1; dodec[2][2] = -1;
  dodec[3][0] = -1; dodec[3][1] = -1; dodec[3][2] = 1;
  dodec[4][0] = -1; dodec[4][1] = 1; dodec[4][2] = -1;
  dodec[5][0] = -1; dodec[5][1] = 1; dodec[5][2] = 1;
  dodec[6][0] = 1; dodec[6][1] = -1; dodec[6][2] = -1;
  dodec[7][0] = 1; dodec[7][1] = -1; dodec[7][2] = 1;
  dodec[8][0] = 1; dodec[8][1] = 1; dodec[8][2] = -1;
  dodec[9][0] = 1; dodec[9][1] = 1; dodec[9][2] = 1;
  dodec[10][0] = beta; dodec[10][1] = alpha; dodec[10][2] = 0;
  dodec[11][0] = beta; dodec[11][1] = -alpha; dodec[11][2] = 0;
  dodec[12][0] = -beta; dodec[12][1] = alpha; dodec[12][2] = 0;
  dodec[13][0] = -beta; dodec[13][1] = -alpha; dodec[13][2] = 0;
  dodec[14][0] = -alpha; dodec[14][1] = 0; dodec[14][2] = -beta;
  dodec[15][0] = alpha; dodec[15][1] = 0; dodec[15][2] = -beta;
  dodec[16][0] = 0; dodec[16][1] = beta; dodec[16][2] = alpha;
  dodec[17][0] = 0; dodec[17][1] = beta; dodec[17][2] = -alpha;
  dodec[18][0] = 0; dodec[18][1] = -beta; dodec[18][2] = alpha;
  dodec[19][0] = 0; dodec[19][1] = -beta; dodec[19][2] = -alpha;
  /* *INDENT-ON* */

}

static void
pentagon(int a, int b, int c, int d, int e, GLenum shadeType)
{
  GLfloat n0[3], d1[3], d2[3];

  DIFF3(dodec[a], dodec[b], d1);
  DIFF3(dodec[b], dodec[c], d2);
  crossprod(d1, d2, n0);
  normalize(n0);

  glBegin(shadeType);
  glNormal3fv(n0);
  glVertex3fv(&dodec[a][0]);
  glVertex3fv(&dodec[b][0]);
  glVertex3fv(&dodec[c][0]);
  glVertex3fv(&dodec[d][0]);
  glVertex3fv(&dodec[e][0]);
  glEnd();
}

static void
dodecahedron(GLenum type)
{
  static int inited = 0;

  if (inited == 0) {
    inited = 1;
    initDodecahedron();
  }
  pentagon(0, 1, 9, 16, 5, type);
  pentagon(1, 0, 3, 18, 7, type);
  pentagon(1, 7, 11, 10, 9, type);
  pentagon(11, 7, 18, 19, 6, type);
  pentagon(8, 17, 16, 9, 10, type);
  pentagon(2, 14, 15, 6, 19, type);
  pentagon(2, 13, 12, 4, 14, type);
  pentagon(2, 19, 18, 3, 13, type);
  pentagon(3, 0, 5, 12, 13, type);
  pentagon(6, 15, 8, 10, 11, type);
  pentagon(4, 17, 8, 15, 14, type);
  pentagon(4, 12, 5, 16, 17, type);
}

/**
 * gdk_gl_draw_dodecahedron:
 * @solid: TRUE if the dodecahedron should be solid.
 *
 * Renders a dodecahedron centered at the modeling coordinates
 * origin with a radius of the square root of 3.
 *
 **/
void
gdk_gl_draw_dodecahedron (gboolean solid)
{
  if (solid)
    dodecahedron (GL_TRIANGLE_FAN);
  else
    dodecahedron (GL_LINE_LOOP);
}

/* 
 * Teapot
 */

/* Rim, body, lid, and bottom data must be reflected in x and
   y; handle and spout data across the y axis only.  */

static int patchdata[][16] =
{
    /* rim */
  {102, 103, 104, 105, 4, 5, 6, 7, 8, 9, 10, 11,
    12, 13, 14, 15},
    /* body */
  {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27},
  {24, 25, 26, 27, 29, 30, 31, 32, 33, 34, 35, 36,
    37, 38, 39, 40},
    /* lid */
  {96, 96, 96, 96, 97, 98, 99, 100, 101, 101, 101,
    101, 0, 1, 2, 3,},
  {0, 1, 2, 3, 106, 107, 108, 109, 110, 111, 112,
    113, 114, 115, 116, 117},
    /* bottom */
  {118, 118, 118, 118, 124, 122, 119, 121, 123, 126,
    125, 120, 40, 39, 38, 37},
    /* handle */
  {41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
    53, 54, 55, 56},
  {53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
    28, 65, 66, 67},
    /* spout */
  {68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83},
  {80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
    92, 93, 94, 95}
};
/* *INDENT-OFF* */

static float cpdata[][3] =
{
    {0.2, 0, 2.7}, {0.2, -0.112, 2.7}, {0.112, -0.2, 2.7}, {0,
    -0.2, 2.7}, {1.3375, 0, 2.53125}, {1.3375, -0.749, 2.53125},
    {0.749, -1.3375, 2.53125}, {0, -1.3375, 2.53125}, {1.4375,
    0, 2.53125}, {1.4375, -0.805, 2.53125}, {0.805, -1.4375,
    2.53125}, {0, -1.4375, 2.53125}, {1.5, 0, 2.4}, {1.5, -0.84,
    2.4}, {0.84, -1.5, 2.4}, {0, -1.5, 2.4}, {1.75, 0, 1.875},
    {1.75, -0.98, 1.875}, {0.98, -1.75, 1.875}, {0, -1.75,
    1.875}, {2, 0, 1.35}, {2, -1.12, 1.35}, {1.12, -2, 1.35},
    {0, -2, 1.35}, {2, 0, 0.9}, {2, -1.12, 0.9}, {1.12, -2,
    0.9}, {0, -2, 0.9}, {-2, 0, 0.9}, {2, 0, 0.45}, {2, -1.12,
    0.45}, {1.12, -2, 0.45}, {0, -2, 0.45}, {1.5, 0, 0.225},
    {1.5, -0.84, 0.225}, {0.84, -1.5, 0.225}, {0, -1.5, 0.225},
    {1.5, 0, 0.15}, {1.5, -0.84, 0.15}, {0.84, -1.5, 0.15}, {0,
    -1.5, 0.15}, {-1.6, 0, 2.025}, {-1.6, -0.3, 2.025}, {-1.5,
    -0.3, 2.25}, {-1.5, 0, 2.25}, {-2.3, 0, 2.025}, {-2.3, -0.3,
    2.025}, {-2.5, -0.3, 2.25}, {-2.5, 0, 2.25}, {-2.7, 0,
    2.025}, {-2.7, -0.3, 2.025}, {-3, -0.3, 2.25}, {-3, 0,
    2.25}, {-2.7, 0, 1.8}, {-2.7, -0.3, 1.8}, {-3, -0.3, 1.8},
    {-3, 0, 1.8}, {-2.7, 0, 1.575}, {-2.7, -0.3, 1.575}, {-3,
    -0.3, 1.35}, {-3, 0, 1.35}, {-2.5, 0, 1.125}, {-2.5, -0.3,
    1.125}, {-2.65, -0.3, 0.9375}, {-2.65, 0, 0.9375}, {-2,
    -0.3, 0.9}, {-1.9, -0.3, 0.6}, {-1.9, 0, 0.6}, {1.7, 0,
    1.425}, {1.7, -0.66, 1.425}, {1.7, -0.66, 0.6}, {1.7, 0,
    0.6}, {2.6, 0, 1.425}, {2.6, -0.66, 1.425}, {3.1, -0.66,
    0.825}, {3.1, 0, 0.825}, {2.3, 0, 2.1}, {2.3, -0.25, 2.1},
    {2.4, -0.25, 2.025}, {2.4, 0, 2.025}, {2.7, 0, 2.4}, {2.7,
    -0.25, 2.4}, {3.3, -0.25, 2.4}, {3.3, 0, 2.4}, {2.8, 0,
    2.475}, {2.8, -0.25, 2.475}, {3.525, -0.25, 2.49375},
    {3.525, 0, 2.49375}, {2.9, 0, 2.475}, {2.9, -0.15, 2.475},
    {3.45, -0.15, 2.5125}, {3.45, 0, 2.5125}, {2.8, 0, 2.4},
    {2.8, -0.15, 2.4}, {3.2, -0.15, 2.4}, {3.2, 0, 2.4}, {0, 0,
    3.15}, {0.8, 0, 3.15}, {0.8, -0.45, 3.15}, {0.45, -0.8,
    3.15}, {0, -0.8, 3.15}, {0, 0, 2.85}, {1.4, 0, 2.4}, {1.4,
    -0.784, 2.4}, {0.784, -1.4, 2.4}, {0, -1.4, 2.4}, {0.4, 0,
    2.55}, {0.4, -0.224, 2.55}, {0.224, -0.4, 2.55}, {0, -0.4,
    2.55}, {1.3, 0, 2.55}, {1.3, -0.728, 2.55}, {0.728, -1.3,
    2.55}, {0, -1.3, 2.55}, {1.3, 0, 2.4}, {1.3, -0.728, 2.4},
    {0.728, -1.3, 2.4}, {0, -1.3, 2.4}, {0, 0, 0}, {1.425,
    -0.798, 0}, {1.5, 0, 0.075}, {1.425, 0, 0}, {0.798, -1.425,
    0}, {0, -1.5, 0.075}, {0, -1.425, 0}, {1.5, -0.84, 0.075},
    {0.84, -1.5, 0.075}
};

static float tex[2][2][2] =
{
  { {0, 0},
    {1, 0}},
  { {0, 1},
    {1, 1}}
};

/* *INDENT-ON* */

static void
teapot(GLint grid, GLdouble scale, GLenum type)
{
  float p[4][4][3], q[4][4][3], r[4][4][3], s[4][4][3];
  long i, j, k, l;

  glPushAttrib(GL_ENABLE_BIT | GL_EVAL_BIT);
  glEnable(GL_AUTO_NORMAL);
  glEnable(GL_NORMALIZE);
  glEnable(GL_MAP2_VERTEX_3);
  glEnable(GL_MAP2_TEXTURE_COORD_2);
  glPushMatrix();
  glRotatef(270.0, 1.0, 0.0, 0.0);
  glScalef(0.5 * scale, 0.5 * scale, 0.5 * scale);
  glTranslatef(0.0, 0.0, -1.5);
  for (i = 0; i < 10; i++) {
    for (j = 0; j < 4; j++) {
      for (k = 0; k < 4; k++) {
        for (l = 0; l < 3; l++) {
          p[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
          q[j][k][l] = cpdata[patchdata[i][j * 4 + (3 - k)]][l];
          if (l == 1)
            q[j][k][l] *= -1.0;
          if (i < 6) {
            r[j][k][l] =
              cpdata[patchdata[i][j * 4 + (3 - k)]][l];
            if (l == 0)
              r[j][k][l] *= -1.0;
            s[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
            if (l == 0)
              s[j][k][l] *= -1.0;
            if (l == 1)
              s[j][k][l] *= -1.0;
          }
        }
      }
    }
    glMap2f(GL_MAP2_TEXTURE_COORD_2, 0, 1, 2, 2, 0, 1, 4, 2,
      &tex[0][0][0]);
    glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4,
      &p[0][0][0]);
    glMapGrid2f(grid, 0.0, 1.0, grid, 0.0, 1.0);
    glEvalMesh2(type, 0, grid, 0, grid);
    glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4,
      &q[0][0][0]);
    glEvalMesh2(type, 0, grid, 0, grid);
    if (i < 6) {
      glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4,
        &r[0][0][0]);
      glEvalMesh2(type, 0, grid, 0, grid);
      glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4,
        &s[0][0][0]);
      glEvalMesh2(type, 0, grid, 0, grid);
    }
  }
  glPopMatrix();
  glPopAttrib();
}

/**
 * gdk_gl_draw_teapot:
 * @solid: TRUE if the teapot should be solid.
 * @scale: relative size of the teapot.
 *
 * Renders a teapot.
 * Both surface normals and texture coordinates for the teapot are generated.
 * The teapot is generated with OpenGL evaluators. 
 *
 **/
void
gdk_gl_draw_teapot (gboolean solid,
                    double   scale)
{
  if (solid)
    teapot (7, scale, GL_FILL);
  else
    teapot (10, scale, GL_LINE);
}
