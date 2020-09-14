//
//  artwork_png.h
//  artwork
//
//  Created by Alexey Yakovenko on 9/14/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#ifndef artwork_png_h
#define artwork_png_h

#ifdef USE_LIBPNG
int
png_resize (const char *fname, const char *outname, int scaled_size);
#endif

#endif /* artwork_png_h */
