/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2024 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#import "MetalBufferLoop.h"

@interface MetalBufferLoop()

// Parameters
@property (nonatomic) id<MTLDevice> device;
@property (nonatomic) NSUInteger bufferCount;
@property (nonatomic) dispatch_semaphore_t semaphore;

// State
@property (nonatomic) NSArray<id<MTLBuffer>> *buffers;
@property (nonatomic) NSUInteger currentBufferSize;
@property (nonatomic) NSUInteger currentBuffer;

@end


@implementation MetalBufferLoop

- (instancetype)initWithMetalDevice:(id<MTLDevice>)device bufferCount:(NSUInteger)bufferCount {
    self = [super init];
    if (self == nil) {
        return nil;
    }

    _device = device;
    _bufferCount = bufferCount;
    _semaphore = dispatch_semaphore_create(bufferCount);

    return self;
}

- (void)ensureBufferSize:(NSUInteger)size {
    if (size == self.currentBufferSize) {
        return;
    }

    self.currentBufferSize = size;
    NSMutableArray<id<MTLBuffer>> *array = [[NSMutableArray alloc] initWithCapacity:self.bufferCount];
    for (NSUInteger i = 0; i < self.bufferCount; i++) {
        [array addObject:[self.device newBufferWithLength:size options:MTLResourceStorageModeShared]];
    }

    self.buffers = array.copy;
}

- (id<MTLBuffer>)nextBufferForSize:(NSUInteger)size {
    dispatch_semaphore_wait(self.semaphore, DISPATCH_TIME_FOREVER);

    [self ensureBufferSize:size];

    id<MTLBuffer> buffer = self.buffers[self.currentBuffer];
    self.currentBuffer += 1;
    if (self.currentBuffer >= self.bufferCount) {
        self.currentBuffer = 0;
    }

    return buffer;
}

- (void)signalCompletion {
    dispatch_semaphore_signal(self.semaphore);
}


@end
