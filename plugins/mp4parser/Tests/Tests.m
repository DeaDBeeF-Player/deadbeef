//
//  Mp4parserTests.m
//  Mp4parserTests
//
//  Created by Oleksiy Yakovenko on 25/12/2016.
//  Copyright © 2016 Alexey Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "mp4parser.h"

@interface Mp4parserTests : XCTestCase

@end

@implementation Mp4parserTests

- (void)setUp {
    [super setUp];
}

- (void)tearDown {
    [super tearDown];
}

- (void)test_findSample20PosInSimpleStreamWith1000ByteChunks_GivesPosition20000 {
	mp4p_stsc_entry_t entry = {
		1,1,1
	};
	mp4p_stsc_entry_t entries[1] = { entry };

	mp4p_stsc_t stsc = {
		.number_of_entries = 1,
		.entries = entries
	};
	
	mp4p_atom_t stsc_atom = {
		.type = "stsc",
		.data = &stsc
	};

	mp4p_stco_entry_t stco_entries[1] = {
		{.offset = 0}
	};

	mp4p_stco_t stco = {
		.number_of_entries = 1,
		.entries = stco_entries
	};

	mp4p_atom_t stco_atom = {
		.type = "stco",
		.data = &stco,
		.next = &stsc_atom
	};

	mp4p_stsz_t stsz = {
		.sample_size = 1000,
		.number_of_entries = 0,
		.entries = NULL
	};

	mp4p_atom_t stsz_atom = {
		.type = "stsz",
		.data = &stsz,
		.next = &stco_atom
	};
	
	mp4p_atom_t stbl_atom = {
		.type = "stbl",
		.subatoms = &stsz_atom
	};

	// this should find sample 0 in chunk 20
	uint64_t offs = mp4p_sample_offset(&stbl_atom, 20);

	XCTAssert(offs == 20000, @"Got %lld instead of expected 20000", offs);
}

- (void)test_findSample20PosInComplesStreamVariableChunks_GivesExpectedPosition {
	mp4p_stsc_entry_t entries[3] = {
		{ 1, 5, 1 },
		{ 3, 2, 2 },
		{ 10, 3, 3 },
	};

	mp4p_stco_entry_t stco_entries[3] = {
		{.offset = 0},
		{.offset = 10000},
		{.offset = 24000},
	};

	mp4p_stsc_t stsc = {
		.number_of_entries = 3,
		.entries = entries
	};

	mp4p_atom_t stsc_atom = {
		.type = "stsc",
		.data = &stsc
	};

	mp4p_stco_t stco = {
		.number_of_entries = 1,
		.entries = stco_entries
	};

	mp4p_atom_t stco_atom = {
		.type = "stco",
		.data = &stco,
		.next = &stsc_atom
	};

	mp4p_stsz_t stsz = {
		.sample_size = 1000,
		.number_of_entries = 0,
		.entries = NULL
	};

	mp4p_atom_t stsz_atom = {
		.type = "stsz",
		.data = &stsz,
		.next = &stco_atom
	};

	mp4p_atom_t stbl_atom = {
		.type = "stbl",
		.subatoms = &stsz_atom
	};

	// this should find sample 0 in chunk 20
	uint64_t offs = mp4p_sample_offset(&stbl_atom, 20);

	XCTAssert(offs == 20000, @"Got %lld instead of expected 20000", offs);
}

@end
