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

    const char input[] = {0x04, 0x10, 0x04, 0x11, 0x04, 0x12, 0x04, 0x13, 0x04, 0x14, 0x00, 0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64};
    const char *pInput = input;
    size_t inlen = sizeof (input);
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);
    char *pOut = output;
    ConversionResult result = ConvertUTF16BEtoUTF8 ((const UTF16**)&pInput, (const UTF16*)(input + inlen), (UTF8**)&pOut, (UTF8*)(output + outlen), strictConversion);

    *pOut = 0;

    XCTAssert(result == conversionOK && !strcmp (output, "АБВГДabcd"), @"Pass");
}

- (void)testConvertUTF16toUTF8 {
    const char input[] = {0x10, 0x04, 0x11, 0x04, 0x12, 0x04, 0x13, 0x04, 0x14, 0x04, 0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64, 0x00};
    const char *pInput = input;
    size_t inlen = sizeof (input);
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);
    char *pOut = output;
    ConversionResult result = ConvertUTF16toUTF8 ((const UTF16**)&pInput, (const UTF16*)(input + inlen), (UTF8**)&pOut, (UTF8*)(output + outlen), strictConversion);

    *pOut = 0;

    XCTAssert(result == conversionOK && !strcmp (output, "АБВГДabcd"), @"Pass");
}

- (void)testConvertUTF8toUTF16 {
    extern ConversionResult
    ConvertUTF8toUTF16BE (const UTF8** sourceStart, const UTF8* sourceEnd, UTF16** targetStart, UTF16* targetEnd, ConversionFlags flags);

    const char input[] = "АБВГДabcd";
    const char *pInput = input;
    size_t inlen = sizeof (input);
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);
    char *pOut = output;
    ConversionResult result = ConvertUTF8toUTF16 ((const UTF8**)&pInput, (const UTF8*)(input + inlen), (UTF16**)&pOut, (UTF16*)(output + outlen), strictConversion);

    *pOut = 0;

    const char utf16be_reference[] = {0x10, 0x04, 0x11, 0x04, 0x12, 0x04, 0x13, 0x04, 0x14, 0x04, 0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64, 0x00};
    XCTAssert(result == conversionOK && !memcmp (output, utf16be_reference, sizeof (utf16be_reference)), @"Pass");
}


- (void)testConvertUTF8toUTF16BE {
    extern ConversionResult
    ConvertUTF8toUTF16BE (const UTF8** sourceStart, const UTF8* sourceEnd, UTF16** targetStart, UTF16* targetEnd, ConversionFlags flags);

    const char input[] = "АБВГДabcd";
    const char *pInput = input;
    size_t inlen = sizeof (input);
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);
    char *pOut = output;
    ConversionResult result = ConvertUTF8toUTF16BE ((const UTF8**)&pInput, (const UTF8*)(input + inlen), (UTF16**)&pOut, (UTF16*)(output + outlen), strictConversion);

    *pOut = 0;

    const char utf16be_reference[] = {0x04, 0x10, 0x04, 0x11, 0x04, 0x12, 0x04, 0x13, 0x04, 0x14, 0x00, 0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64};
    XCTAssert(result == conversionOK && !memcmp (output, utf16be_reference, sizeof (utf16be_reference)), @"Pass");
}

- (void)testConvertUTF8toCP1252 {
    int
    junk_utf8_to_cp1252(const uint8_t *in, int inlen, uint8_t *out, int outlen);

    const char input[] = "€ù‰•abcd";
    size_t inlen = sizeof (input)-1;
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);

    int res = junk_utf8_to_cp1252((const uint8_t *)input, (int)inlen, (uint8_t *)output, (int)outlen);

    const char cp1252_reference[] = { 0x80, 0xf9, 0x89, 0x95, 0x61, 0x62, 0x63, 0x64 };
    XCTAssert(res == sizeof (cp1252_reference) && !memcmp (output, cp1252_reference, sizeof (cp1252_reference)), @"Pass");
}

void
_split_multivalue (char *text, size_t text_size);

- (void)testSplitMultivalue_IgnoresUnspacedSlash {
    char input[] = "Test/Value/With/Shashes";
    _split_multivalue(input, sizeof (input)-1);

    XCTAssert(!strcmp (input, "Test/Value/With/Shashes"), @"Pass");
}

- (void)testSplitMultivalue_SplitsOnSpacedSlash {
    char input[] = "Test / Value / With / Shashes";
    _split_multivalue(input, sizeof (input)-1);

    XCTAssert(!strcmp (input, "Test\0\0\0Value\0\0\0With\0\0\0Shashes"), @"Pass");
}

- (void)testSplitMultivalue_IgnoreSpacedSlashBegin {
    char input[] = "/ Test";
    _split_multivalue(input, sizeof (input)-1);

    XCTAssert(!strcmp (input, "/ Test"), @"Pass");
}

- (void)testSplitMultivalue_IgnoreSpacedSlashEnd {
    char input[] = "Test /";
    _split_multivalue(input, sizeof (input)-1);

    XCTAssert(!strcmp (input, "Test /"), @"Pass");
}

@end
