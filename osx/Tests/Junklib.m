//
//  Junklib.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 10/06/15.
//  Copyright (c) 2015 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>
#include "ConvertUTF.h"

@interface Junklib : XCTestCase

@end

@implementation Junklib

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testConvertUTF16BEtoUTF8 {
    extern ConversionResult
    ConvertUTF16BEtoUTF8 (const UTF16** sourceStart, const UTF16* sourceEnd, UTF8** targetStart, UTF8* targetEnd, ConversionFlags flags);

    char input[] = {0x04, 0x10, 0x04, 0x11, 0x04, 0x12, 0x04, 0x13, 0x04, 0x14, 0x00, 0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64};
    char *pInput = input;
    size_t inlen = sizeof (input);
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);
    char *pOut = output;
    ConversionResult result = ConvertUTF16BEtoUTF8 ((const UTF16**)&pInput, (const UTF16*)(input + inlen), (UTF8**)&pOut, (UTF8*)(output + outlen), strictConversion);

    NSString *nsOutput = [NSString stringWithUTF8String:output];
    
    XCTAssert(result == conversionOK && [nsOutput isEqualTo:@"АБВГДabcd"], @"Pass");
}

- (void)testConvertUTF16toUTF8 {
    char input[] = {0x10, 0x04, 0x11, 0x04, 0x12, 0x04, 0x13, 0x04, 0x14, 0x04, 0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64, 0x00};
    char *pInput = input;
    size_t inlen = sizeof (input);
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);
    char *pOut = output;
    ConversionResult result = ConvertUTF16toUTF8 ((const UTF16**)&pInput, (const UTF16*)(input + inlen), (UTF8**)&pOut, (UTF8*)(output + outlen), strictConversion);

    *pOut = 0;
    NSString *nsOutput = [NSString stringWithUTF8String:output];

    XCTAssert(result == conversionOK && [nsOutput isEqualTo:@"АБВГДabcd"], @"Pass");
}

@end
