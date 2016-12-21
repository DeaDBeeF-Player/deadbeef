
/* -----------------------------------------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2015 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V.
  All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software that implements
the MPEG Advanced Audio Coding ("AAC") encoding and decoding scheme for digital audio.
This FDK AAC Codec software is intended to be used on a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient general perceptual
audio codecs. AAC-ELD is considered the best-performing full-bandwidth communications codec by
independent studies and is widely deployed. AAC has been standardized by ISO and IEC as part
of the MPEG specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including those of Fraunhofer)
may be obtained through Via Licensing (www.vialicensing.com) or through the respective patent owners
individually for the purpose of encoding or decoding bit streams in products that are compliant with
the ISO/IEC MPEG audio standards. Please note that most manufacturers of Android devices already license
these patent claims through Via Licensing or directly from the patent owners, and therefore FDK AAC Codec
software may already be covered under those patent licenses when it is used for those licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions with enhanced sound quality,
are also available from Fraunhofer. Users are encouraged to check the Fraunhofer website for additional
applications information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification, are permitted without
payment of copyright license fees provided that you satisfy the following conditions:

You must retain the complete text of this software license in redistributions of the FDK AAC Codec or
your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation and/or other materials
provided with redistributions of the FDK AAC Codec or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived from this library without
prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute the FDK AAC Codec
software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating that you changed the software
and the date of any change. For modified versions of the FDK AAC Codec, the term
"Fraunhofer FDK AAC Codec Library for Android" must be replaced by the term
"Third-Party Modified Version of the Fraunhofer FDK AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without limitation the patents of Fraunhofer,
ARE GRANTED BY THIS SOFTWARE LICENSE. Fraunhofer provides no warranty of patent non-infringement with
respect to this software.

You may use this FDK AAC Codec software or modifications thereto only for purposes that are authorized
by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright holders and contributors
"AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied warranties
of merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary, or consequential damages,
including but not limited to procurement of substitute goods or services; loss of use, data, or profits,
or business interruption, however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------------------------------------- */

/***************************  Fraunhofer IIS FDK Tools  ***********************

   Author(s):   Oliver Moser
   Description: ROM tables used by FDK tools

******************************************************************************/

#include "FDK_tools_rom.h"











RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_STP SineTable480[] =
{
  STCP(0x7fffffff, 0x00000000), STCP(0x7fffd315, 0x006b3b9b), STCP(0x7fff4c54, 0x00d676eb), STCP(0x7ffe6bbf, 0x0141b1a5),
  STCP(0x7ffd3154, 0x01aceb7c), STCP(0x7ffb9d15, 0x02182427), STCP(0x7ff9af04, 0x02835b5a), STCP(0x7ff76721, 0x02ee90c8),
  STCP(0x7ff4c56f, 0x0359c428), STCP(0x7ff1c9ef, 0x03c4f52f), STCP(0x7fee74a2, 0x0430238f), STCP(0x7feac58d, 0x049b4f00),
  STCP(0x7fe6bcb0, 0x05067734), STCP(0x7fe25a0f, 0x05719be2), STCP(0x7fdd9dad, 0x05dcbcbe), STCP(0x7fd8878e, 0x0647d97c),
  STCP(0x7fd317b4, 0x06b2f1d2), STCP(0x7fcd4e24, 0x071e0575), STCP(0x7fc72ae2, 0x07891418), STCP(0x7fc0adf2, 0x07f41d72),
  STCP(0x7fb9d759, 0x085f2137), STCP(0x7fb2a71b, 0x08ca1f1b), STCP(0x7fab1d3d, 0x093516d4), STCP(0x7fa339c5, 0x09a00817),
  STCP(0x7f9afcb9, 0x0a0af299), STCP(0x7f92661d, 0x0a75d60e), STCP(0x7f8975f9, 0x0ae0b22c), STCP(0x7f802c52, 0x0b4b86a8),
  STCP(0x7f76892f, 0x0bb65336), STCP(0x7f6c8c96, 0x0c21178c), STCP(0x7f62368f, 0x0c8bd35e), STCP(0x7f578721, 0x0cf68662),
  STCP(0x7f4c7e54, 0x0d61304e), STCP(0x7f411c2f, 0x0dcbd0d5), STCP(0x7f3560b9, 0x0e3667ad), STCP(0x7f294bfd, 0x0ea0f48c),
  STCP(0x7f1cde01, 0x0f0b7727), STCP(0x7f1016ce, 0x0f75ef33), STCP(0x7f02f66f, 0x0fe05c64), STCP(0x7ef57cea, 0x104abe71),
  STCP(0x7ee7aa4c, 0x10b5150f), STCP(0x7ed97e9c, 0x111f5ff4), STCP(0x7ecaf9e5, 0x11899ed3), STCP(0x7ebc1c31, 0x11f3d164),
  STCP(0x7eace58a, 0x125df75b), STCP(0x7e9d55fc, 0x12c8106f), STCP(0x7e8d6d91, 0x13321c53), STCP(0x7e7d2c54, 0x139c1abf),
  STCP(0x7e6c9251, 0x14060b68), STCP(0x7e5b9f93, 0x146fee03), STCP(0x7e4a5426, 0x14d9c245), STCP(0x7e38b017, 0x154387e6),
  STCP(0x7e26b371, 0x15ad3e9a), STCP(0x7e145e42, 0x1616e618), STCP(0x7e01b096, 0x16807e15), STCP(0x7deeaa7a, 0x16ea0646),
  STCP(0x7ddb4bfc, 0x17537e63), STCP(0x7dc79529, 0x17bce621), STCP(0x7db3860f, 0x18263d36), STCP(0x7d9f1ebd, 0x188f8357),
  STCP(0x7d8a5f40, 0x18f8b83c), STCP(0x7d7547a7, 0x1961db9b), STCP(0x7d5fd801, 0x19caed29), STCP(0x7d4a105d, 0x1a33ec9c),
  STCP(0x7d33f0ca, 0x1a9cd9ac), STCP(0x7d1d7958, 0x1b05b40f), STCP(0x7d06aa16, 0x1b6e7b7a), STCP(0x7cef8315, 0x1bd72fa4),
  STCP(0x7cd80464, 0x1c3fd045), STCP(0x7cc02e15, 0x1ca85d12), STCP(0x7ca80038, 0x1d10d5c2), STCP(0x7c8f7ade, 0x1d793a0b),
  STCP(0x7c769e18, 0x1de189a6), STCP(0x7c5d69f7, 0x1e49c447), STCP(0x7c43de8e, 0x1eb1e9a7), STCP(0x7c29fbee, 0x1f19f97b),
  STCP(0x7c0fc22a, 0x1f81f37c), STCP(0x7bf53153, 0x1fe9d75f), STCP(0x7bda497d, 0x2051a4dd), STCP(0x7bbf0aba, 0x20b95bac),
  STCP(0x7ba3751d, 0x2120fb83), STCP(0x7b8788ba, 0x2188841a), STCP(0x7b6b45a5, 0x21eff528), STCP(0x7b4eabf1, 0x22574e65),
  STCP(0x7b31bbb2, 0x22be8f87), STCP(0x7b1474fd, 0x2325b847), STCP(0x7af6d7e6, 0x238cc85d), STCP(0x7ad8e482, 0x23f3bf7e),
  STCP(0x7aba9ae6, 0x245a9d65), STCP(0x7a9bfb27, 0x24c161c7), STCP(0x7a7d055b, 0x25280c5e), STCP(0x7a5db997, 0x258e9ce0),
  STCP(0x7a3e17f2, 0x25f51307), STCP(0x7a1e2082, 0x265b6e8a), STCP(0x79fdd35c, 0x26c1af22), STCP(0x79dd3098, 0x2727d486),
  STCP(0x79bc384d, 0x278dde6e), STCP(0x799aea92, 0x27f3cc94), STCP(0x7979477d, 0x28599eb0), STCP(0x79574f28, 0x28bf547b),
  STCP(0x793501a9, 0x2924edac), STCP(0x79125f19, 0x298a69fc), STCP(0x78ef678f, 0x29efc925), STCP(0x78cc1b26, 0x2a550adf),
  STCP(0x78a879f4, 0x2aba2ee4), STCP(0x78848414, 0x2b1f34eb), STCP(0x7860399e, 0x2b841caf), STCP(0x783b9aad, 0x2be8e5e8),
  STCP(0x7816a759, 0x2c4d9050), STCP(0x77f15fbc, 0x2cb21ba0), STCP(0x77cbc3f2, 0x2d168792), STCP(0x77a5d413, 0x2d7ad3de),
  STCP(0x777f903c, 0x2ddf0040), STCP(0x7758f886, 0x2e430c6f), STCP(0x77320d0d, 0x2ea6f827), STCP(0x770acdec, 0x2f0ac320),
  STCP(0x76e33b3f, 0x2f6e6d16), STCP(0x76bb5521, 0x2fd1f5c1), STCP(0x76931bae, 0x30355cdd), STCP(0x766a8f04, 0x3098a223),
  STCP(0x7641af3d, 0x30fbc54d), STCP(0x76187c77, 0x315ec617), STCP(0x75eef6ce, 0x31c1a43b), STCP(0x75c51e61, 0x32245f72),
  STCP(0x759af34c, 0x3286f779), STCP(0x757075ac, 0x32e96c09), STCP(0x7545a5a0, 0x334bbcde), STCP(0x751a8346, 0x33ade9b3),
  STCP(0x74ef0ebc, 0x340ff242), STCP(0x74c34820, 0x3471d647), STCP(0x74972f92, 0x34d3957e), STCP(0x746ac52f, 0x35352fa1),
  STCP(0x743e0918, 0x3596a46c), STCP(0x7410fb6b, 0x35f7f39c), STCP(0x73e39c49, 0x36591cea), STCP(0x73b5ebd1, 0x36ba2014),
  STCP(0x7387ea23, 0x371afcd5), STCP(0x73599760, 0x377bb2e9), STCP(0x732af3a7, 0x37dc420c), STCP(0x72fbff1b, 0x383ca9fb),
  STCP(0x72ccb9db, 0x389cea72), STCP(0x729d2409, 0x38fd032d), STCP(0x726d3dc6, 0x395cf3e9), STCP(0x723d0734, 0x39bcbc63),
  STCP(0x720c8075, 0x3a1c5c57), STCP(0x71dba9ab, 0x3a7bd382), STCP(0x71aa82f7, 0x3adb21a1), STCP(0x71790c7e, 0x3b3a4672),
  STCP(0x71474660, 0x3b9941b1), STCP(0x711530c2, 0x3bf8131c), STCP(0x70e2cbc6, 0x3c56ba70), STCP(0x70b01790, 0x3cb5376b),
  STCP(0x707d1443, 0x3d1389cb), STCP(0x7049c203, 0x3d71b14d), STCP(0x701620f5, 0x3dcfadb0), STCP(0x6fe2313c, 0x3e2d7eb1),
  STCP(0x6fadf2fc, 0x3e8b240e), STCP(0x6f79665b, 0x3ee89d86), STCP(0x6f448b7e, 0x3f45ead8), STCP(0x6f0f6289, 0x3fa30bc1),
  STCP(0x6ed9eba1, 0x40000000), STCP(0x6ea426ed, 0x405cc754), STCP(0x6e6e1492, 0x40b9617d), STCP(0x6e37b4b6, 0x4115ce38),
  STCP(0x6e010780, 0x41720d46), STCP(0x6dca0d14, 0x41ce1e65), STCP(0x6d92c59b, 0x422a0154), STCP(0x6d5b313b, 0x4285b5d4),
  STCP(0x6d23501b, 0x42e13ba4), STCP(0x6ceb2261, 0x433c9283), STCP(0x6cb2a837, 0x4397ba32), STCP(0x6c79e1c2, 0x43f2b271),
  STCP(0x6c40cf2c, 0x444d7aff), STCP(0x6c07709b, 0x44a8139e), STCP(0x6bcdc639, 0x45027c0c), STCP(0x6b93d02e, 0x455cb40c),
  STCP(0x6b598ea3, 0x45b6bb5e), STCP(0x6b1f01c0, 0x461091c2), STCP(0x6ae429ae, 0x466a36f9), STCP(0x6aa90697, 0x46c3aac5),
  STCP(0x6a6d98a4, 0x471cece7), STCP(0x6a31e000, 0x4775fd1f), STCP(0x69f5dcd3, 0x47cedb31), STCP(0x69b98f48, 0x482786dc),
  STCP(0x697cf78a, 0x487fffe4), STCP(0x694015c3, 0x48d84609), STCP(0x6902ea1d, 0x4930590f), STCP(0x68c574c4, 0x498838b6),
  STCP(0x6887b5e2, 0x49dfe4c2), STCP(0x6849ada3, 0x4a375cf5), STCP(0x680b5c33, 0x4a8ea111), STCP(0x67ccc1be, 0x4ae5b0da),
  STCP(0x678dde6e, 0x4b3c8c12), STCP(0x674eb271, 0x4b93327c), STCP(0x670f3df3, 0x4be9a3db), STCP(0x66cf8120, 0x4c3fdff4),
  STCP(0x668f7c25, 0x4c95e688), STCP(0x664f2f2e, 0x4cebb75c), STCP(0x660e9a6a, 0x4d415234), STCP(0x65cdbe05, 0x4d96b6d3),
  STCP(0x658c9a2d, 0x4debe4fe), STCP(0x654b2f10, 0x4e40dc79), STCP(0x65097cdb, 0x4e959d08), STCP(0x64c783bd, 0x4eea2670),
  STCP(0x648543e4, 0x4f3e7875), STCP(0x6442bd7e, 0x4f9292dc), STCP(0x63fff0ba, 0x4fe6756a), STCP(0x63bcddc7, 0x503a1fe5),
  STCP(0x637984d4, 0x508d9211), STCP(0x6335e611, 0x50e0cbb4), STCP(0x62f201ac, 0x5133cc94), STCP(0x62add7d6, 0x51869476),
  STCP(0x626968be, 0x51d92321), STCP(0x6224b495, 0x522b7859), STCP(0x61dfbb8a, 0x527d93e6), STCP(0x619a7dce, 0x52cf758f),
  STCP(0x6154fb91, 0x53211d18), STCP(0x610f3505, 0x53728a4a), STCP(0x60c92a5a, 0x53c3bcea), STCP(0x6082dbc1, 0x5414b4c1),
  STCP(0x603c496c, 0x54657194), STCP(0x5ff5738d, 0x54b5f32c), STCP(0x5fae5a55, 0x55063951), STCP(0x5f66fdf5, 0x555643c8),
  STCP(0x5f1f5ea1, 0x55a6125c), STCP(0x5ed77c8a, 0x55f5a4d2), STCP(0x5e8f57e2, 0x5644faf4), STCP(0x5e46f0dd, 0x5694148b),
  STCP(0x5dfe47ad, 0x56e2f15d), STCP(0x5db55c86, 0x57319135), STCP(0x5d6c2f99, 0x577ff3da), STCP(0x5d22c11c, 0x57ce1917),
  STCP(0x5cd91140, 0x581c00b3), STCP(0x5c8f203b, 0x5869aa79), STCP(0x5c44ee40, 0x58b71632), STCP(0x5bfa7b82, 0x590443a7),
  STCP(0x5bafc837, 0x595132a2), STCP(0x5b64d492, 0x599de2ee), STCP(0x5b19a0c8, 0x59ea5454), STCP(0x5ace2d0f, 0x5a36869f),
  STCP(0x5a82799a, 0x5a82799a)
};


RAM_ALIGN
LNK_SECTION_CONSTDATA
LNK_SECTION_CONSTDATA_L1
const FIXP_STP SineTable512[] =
{
  STCP(0x7fffffff, 0x00000000), STCP(0x7fffd886, 0x006487e3), STCP(0x7fff6216, 0x00c90f88), STCP(0x7ffe9cb2, 0x012d96b1),
  STCP(0x7ffd885a, 0x01921d20), STCP(0x7ffc250f, 0x01f6a297), STCP(0x7ffa72d1, 0x025b26d7), STCP(0x7ff871a2, 0x02bfa9a4),
  STCP(0x7ff62182, 0x03242abf), STCP(0x7ff38274, 0x0388a9ea), STCP(0x7ff09478, 0x03ed26e6), STCP(0x7fed5791, 0x0451a177),
  STCP(0x7fe9cbc0, 0x04b6195d), STCP(0x7fe5f108, 0x051a8e5c), STCP(0x7fe1c76b, 0x057f0035), STCP(0x7fdd4eec, 0x05e36ea9),
  STCP(0x7fd8878e, 0x0647d97c), STCP(0x7fd37153, 0x06ac406f), STCP(0x7fce0c3e, 0x0710a345), STCP(0x7fc85854, 0x077501be),
  STCP(0x7fc25596, 0x07d95b9e), STCP(0x7fbc040a, 0x083db0a7), STCP(0x7fb563b3, 0x08a2009a), STCP(0x7fae7495, 0x09064b3a),
  STCP(0x7fa736b4, 0x096a9049), STCP(0x7f9faa15, 0x09cecf89), STCP(0x7f97cebd, 0x0a3308bd), STCP(0x7f8fa4b0, 0x0a973ba5),
  STCP(0x7f872bf3, 0x0afb6805), STCP(0x7f7e648c, 0x0b5f8d9f), STCP(0x7f754e80, 0x0bc3ac35), STCP(0x7f6be9d4, 0x0c27c389),
  STCP(0x7f62368f, 0x0c8bd35e), STCP(0x7f5834b7, 0x0cefdb76), STCP(0x7f4de451, 0x0d53db92), STCP(0x7f434563, 0x0db7d376),
  STCP(0x7f3857f6, 0x0e1bc2e4), STCP(0x7f2d1c0e, 0x0e7fa99e), STCP(0x7f2191b4, 0x0ee38766), STCP(0x7f15b8ee, 0x0f475bff),
  STCP(0x7f0991c4, 0x0fab272b), STCP(0x7efd1c3c, 0x100ee8ad), STCP(0x7ef05860, 0x1072a048), STCP(0x7ee34636, 0x10d64dbd),
  STCP(0x7ed5e5c6, 0x1139f0cf), STCP(0x7ec8371a, 0x119d8941), STCP(0x7eba3a39, 0x120116d5), STCP(0x7eabef2c, 0x1264994e),
  STCP(0x7e9d55fc, 0x12c8106f), STCP(0x7e8e6eb2, 0x132b7bf9), STCP(0x7e7f3957, 0x138edbb1), STCP(0x7e6fb5f4, 0x13f22f58),
  STCP(0x7e5fe493, 0x145576b1), STCP(0x7e4fc53e, 0x14b8b17f), STCP(0x7e3f57ff, 0x151bdf86), STCP(0x7e2e9cdf, 0x157f0086),
  STCP(0x7e1d93ea, 0x15e21445), STCP(0x7e0c3d29, 0x16451a83), STCP(0x7dfa98a8, 0x16a81305), STCP(0x7de8a670, 0x170afd8d),
  STCP(0x7dd6668f, 0x176dd9de), STCP(0x7dc3d90d, 0x17d0a7bc), STCP(0x7db0fdf8, 0x183366e9), STCP(0x7d9dd55a, 0x18961728),
  STCP(0x7d8a5f40, 0x18f8b83c), STCP(0x7d769bb5, 0x195b49ea), STCP(0x7d628ac6, 0x19bdcbf3), STCP(0x7d4e2c7f, 0x1a203e1b),
  STCP(0x7d3980ec, 0x1a82a026), STCP(0x7d24881b, 0x1ae4f1d6), STCP(0x7d0f4218, 0x1b4732ef), STCP(0x7cf9aef0, 0x1ba96335),
  STCP(0x7ce3ceb2, 0x1c0b826a), STCP(0x7ccda169, 0x1c6d9053), STCP(0x7cb72724, 0x1ccf8cb3), STCP(0x7ca05ff1, 0x1d31774d),
  STCP(0x7c894bde, 0x1d934fe5), STCP(0x7c71eaf9, 0x1df5163f), STCP(0x7c5a3d50, 0x1e56ca1e), STCP(0x7c4242f2, 0x1eb86b46),
  STCP(0x7c29fbee, 0x1f19f97b), STCP(0x7c116853, 0x1f7b7481), STCP(0x7bf88830, 0x1fdcdc1b), STCP(0x7bdf5b94, 0x203e300d),
  STCP(0x7bc5e290, 0x209f701c), STCP(0x7bac1d31, 0x21009c0c), STCP(0x7b920b89, 0x2161b3a0), STCP(0x7b77ada8, 0x21c2b69c),
  STCP(0x7b5d039e, 0x2223a4c5), STCP(0x7b420d7a, 0x22847de0), STCP(0x7b26cb4f, 0x22e541af), STCP(0x7b0b3d2c, 0x2345eff8),
  STCP(0x7aef6323, 0x23a6887f), STCP(0x7ad33d45, 0x24070b08), STCP(0x7ab6cba4, 0x24677758), STCP(0x7a9a0e50, 0x24c7cd33),
  STCP(0x7a7d055b, 0x25280c5e), STCP(0x7a5fb0d8, 0x2588349d), STCP(0x7a4210d8, 0x25e845b6), STCP(0x7a24256f, 0x26483f6c),
  STCP(0x7a05eead, 0x26a82186), STCP(0x79e76ca7, 0x2707ebc7), STCP(0x79c89f6e, 0x27679df4), STCP(0x79a98715, 0x27c737d3),
  STCP(0x798a23b1, 0x2826b928), STCP(0x796a7554, 0x288621b9), STCP(0x794a7c12, 0x28e5714b), STCP(0x792a37fe, 0x2944a7a2),
  STCP(0x7909a92d, 0x29a3c485), STCP(0x78e8cfb2, 0x2a02c7b8), STCP(0x78c7aba2, 0x2a61b101), STCP(0x78a63d11, 0x2ac08026),
  STCP(0x78848414, 0x2b1f34eb), STCP(0x786280bf, 0x2b7dcf17), STCP(0x78403329, 0x2bdc4e6f), STCP(0x781d9b65, 0x2c3ab2b9),
  STCP(0x77fab989, 0x2c98fbba), STCP(0x77d78daa, 0x2cf72939), STCP(0x77b417df, 0x2d553afc), STCP(0x7790583e, 0x2db330c7),
  STCP(0x776c4edb, 0x2e110a62), STCP(0x7747fbce, 0x2e6ec792), STCP(0x77235f2d, 0x2ecc681e), STCP(0x76fe790e, 0x2f29ebcc),
  STCP(0x76d94989, 0x2f875262), STCP(0x76b3d0b4, 0x2fe49ba7), STCP(0x768e0ea6, 0x3041c761), STCP(0x76680376, 0x309ed556),
  STCP(0x7641af3d, 0x30fbc54d), STCP(0x761b1211, 0x3158970e), STCP(0x75f42c0b, 0x31b54a5e), STCP(0x75ccfd42, 0x3211df04),
  STCP(0x75a585cf, 0x326e54c7), STCP(0x757dc5ca, 0x32caab6f), STCP(0x7555bd4c, 0x3326e2c3), STCP(0x752d6c6c, 0x3382fa88),
  STCP(0x7504d345, 0x33def287), STCP(0x74dbf1ef, 0x343aca87), STCP(0x74b2c884, 0x34968250), STCP(0x7489571c, 0x34f219a8),
  STCP(0x745f9dd1, 0x354d9057), STCP(0x74359cbd, 0x35a8e625), STCP(0x740b53fb, 0x36041ad9), STCP(0x73e0c3a3, 0x365f2e3b),
  STCP(0x73b5ebd1, 0x36ba2014), STCP(0x738acc9e, 0x3714f02a), STCP(0x735f6626, 0x376f9e46), STCP(0x7333b883, 0x37ca2a30),
  STCP(0x7307c3d0, 0x382493b0), STCP(0x72db8828, 0x387eda8e), STCP(0x72af05a7, 0x38d8fe93), STCP(0x72823c67, 0x3932ff87),
  STCP(0x72552c85, 0x398cdd32), STCP(0x7227d61c, 0x39e6975e), STCP(0x71fa3949, 0x3a402dd2), STCP(0x71cc5626, 0x3a99a057),
  STCP(0x719e2cd2, 0x3af2eeb7), STCP(0x716fbd68, 0x3b4c18ba), STCP(0x71410805, 0x3ba51e29), STCP(0x71120cc5, 0x3bfdfecd),
  STCP(0x70e2cbc6, 0x3c56ba70), STCP(0x70b34525, 0x3caf50da), STCP(0x708378ff, 0x3d07c1d6), STCP(0x70536771, 0x3d600d2c),
  STCP(0x7023109a, 0x3db832a6), STCP(0x6ff27497, 0x3e10320d), STCP(0x6fc19385, 0x3e680b2c), STCP(0x6f906d84, 0x3ebfbdcd),
  STCP(0x6f5f02b2, 0x3f1749b8), STCP(0x6f2d532c, 0x3f6eaeb8), STCP(0x6efb5f12, 0x3fc5ec98), STCP(0x6ec92683, 0x401d0321),
  STCP(0x6e96a99d, 0x4073f21d), STCP(0x6e63e87f, 0x40cab958), STCP(0x6e30e34a, 0x4121589b), STCP(0x6dfd9a1c, 0x4177cfb1),
  STCP(0x6dca0d14, 0x41ce1e65), STCP(0x6d963c54, 0x42244481), STCP(0x6d6227fa, 0x427a41d0), STCP(0x6d2dd027, 0x42d0161e),
  STCP(0x6cf934fc, 0x4325c135), STCP(0x6cc45698, 0x437b42e1), STCP(0x6c8f351c, 0x43d09aed), STCP(0x6c59d0a9, 0x4425c923),
  STCP(0x6c242960, 0x447acd50), STCP(0x6bee3f62, 0x44cfa740), STCP(0x6bb812d1, 0x452456bd), STCP(0x6b81a3cd, 0x4578db93),
  STCP(0x6b4af279, 0x45cd358f), STCP(0x6b13fef5, 0x4621647d), STCP(0x6adcc964, 0x46756828), STCP(0x6aa551e9, 0x46c9405c),
  STCP(0x6a6d98a4, 0x471cece7), STCP(0x6a359db9, 0x47706d93), STCP(0x69fd614a, 0x47c3c22f), STCP(0x69c4e37a, 0x4816ea86),
  STCP(0x698c246c, 0x4869e665), STCP(0x69532442, 0x48bcb599), STCP(0x6919e320, 0x490f57ee), STCP(0x68e06129, 0x4961cd33),
  STCP(0x68a69e81, 0x49b41533), STCP(0x686c9b4b, 0x4a062fbd), STCP(0x683257ab, 0x4a581c9e), STCP(0x67f7d3c5, 0x4aa9dba2),
  STCP(0x67bd0fbd, 0x4afb6c98), STCP(0x67820bb7, 0x4b4ccf4d), STCP(0x6746c7d8, 0x4b9e0390), STCP(0x670b4444, 0x4bef092d),
  STCP(0x66cf8120, 0x4c3fdff4), STCP(0x66937e91, 0x4c9087b1), STCP(0x66573cbb, 0x4ce10034), STCP(0x661abbc5, 0x4d31494b),
  STCP(0x65ddfbd3, 0x4d8162c4), STCP(0x65a0fd0b, 0x4dd14c6e), STCP(0x6563bf92, 0x4e210617), STCP(0x6526438f, 0x4e708f8f),
  STCP(0x64e88926, 0x4ebfe8a5), STCP(0x64aa907f, 0x4f0f1126), STCP(0x646c59bf, 0x4f5e08e3), STCP(0x642de50d, 0x4faccfab),
  STCP(0x63ef3290, 0x4ffb654d), STCP(0x63b0426d, 0x5049c999), STCP(0x637114cc, 0x5097fc5e), STCP(0x6331a9d4, 0x50e5fd6d),
  STCP(0x62f201ac, 0x5133cc94), STCP(0x62b21c7b, 0x518169a5), STCP(0x6271fa69, 0x51ced46e), STCP(0x62319b9d, 0x521c0cc2),
  STCP(0x61f1003f, 0x5269126e), STCP(0x61b02876, 0x52b5e546), STCP(0x616f146c, 0x53028518), STCP(0x612dc447, 0x534ef1b5),
  STCP(0x60ec3830, 0x539b2af0), STCP(0x60aa7050, 0x53e73097), STCP(0x60686ccf, 0x5433027d), STCP(0x60262dd6, 0x547ea073),
  STCP(0x5fe3b38d, 0x54ca0a4b), STCP(0x5fa0fe1f, 0x55153fd4), STCP(0x5f5e0db3, 0x556040e2), STCP(0x5f1ae274, 0x55ab0d46),
  STCP(0x5ed77c8a, 0x55f5a4d2), STCP(0x5e93dc1f, 0x56400758), STCP(0x5e50015d, 0x568a34a9), STCP(0x5e0bec6e, 0x56d42c99),
  STCP(0x5dc79d7c, 0x571deefa), STCP(0x5d8314b1, 0x57677b9d), STCP(0x5d3e5237, 0x57b0d256), STCP(0x5cf95638, 0x57f9f2f8),
  STCP(0x5cb420e0, 0x5842dd54), STCP(0x5c6eb258, 0x588b9140), STCP(0x5c290acc, 0x58d40e8c), STCP(0x5be32a67, 0x591c550e),
  STCP(0x5b9d1154, 0x59646498), STCP(0x5b56bfbd, 0x59ac3cfd), STCP(0x5b1035cf, 0x59f3de12), STCP(0x5ac973b5, 0x5a3b47ab),
  STCP(0x5a82799a, 0x5a82799a)
};





















/* generate_fft_tables.m 4 15 */
RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_STB RotVectorReal60[] =
{
  STC(0x7f4c7e54), STC(0x7d33f0ca), STC(0x79bc384d),
  STC(0x7d33f0ca), STC(0x74ef0ebc), STC(0x678dde6e),
  STC(0x79bc384d), STC(0x678dde6e), STC(0x4b3c8c12),
  STC(0x74ef0ebc), STC(0x55a6125c), STC(0x278dde6e),
  STC(0x6ed9eba1), STC(0x40000000), STC(0x00000000),
  STC(0x678dde6e), STC(0x278dde6e), STC(0xd8722191),
  STC(0x5f1f5ea1), STC(0x0d61304e), STC(0xb4c373ed),
  STC(0x55a6125c), STC(0xf29ecfb1), STC(0x98722191),
  STC(0x4b3c8c12), STC(0xd8722191), STC(0x8643c7b2),
  STC(0x40000000), STC(0xbfffffff), STC(0x80000000),
  STC(0x340ff242), STC(0xaa59eda3), STC(0x8643c7b2),
  STC(0x278dde6e), STC(0x98722191), STC(0x98722191),
  STC(0x1a9cd9ac), STC(0x8b10f143), STC(0xb4c373ed),
  STC(0x0d61304e), STC(0x82cc0f35), STC(0xd8722191),
};
RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_STB RotVectorImag60[] =
{
  STC(0x0d61304e), STC(0x1a9cd9ac), STC(0x278dde6e),
  STC(0x1a9cd9ac), STC(0x340ff242), STC(0x4b3c8c12),
  STC(0x278dde6e), STC(0x4b3c8c12), STC(0x678dde6e),
  STC(0x340ff242), STC(0x5f1f5ea1), STC(0x79bc384d),
  STC(0x40000000), STC(0x6ed9eba1), STC(0x7fffffff),
  STC(0x4b3c8c12), STC(0x79bc384d), STC(0x79bc384d),
  STC(0x55a6125c), STC(0x7f4c7e54), STC(0x678dde6e),
  STC(0x5f1f5ea1), STC(0x7f4c7e54), STC(0x4b3c8c12),
  STC(0x678dde6e), STC(0x79bc384d), STC(0x278dde6e),
  STC(0x6ed9eba1), STC(0x6ed9eba1), STC(0x00000000),
  STC(0x74ef0ebc), STC(0x5f1f5ea1), STC(0xd8722191),
  STC(0x79bc384d), STC(0x4b3c8c12), STC(0xb4c373ed),
  STC(0x7d33f0ca), STC(0x340ff242), STC(0x98722191),
  STC(0x7f4c7e54), STC(0x1a9cd9ac), STC(0x8643c7b2),
};



RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_STB RotVectorReal240[] =
{
  STC(0x7ff4c56f), STC(0x7fd317b4), STC(0x7f9afcb9), STC(0x7f4c7e54), STC(0x7ee7aa4c), STC(0x7e6c9251), STC(0x7ddb4bfc), STC(0x7d33f0ca), STC(0x7c769e18), STC(0x7ba3751d), STC(0x7aba9ae6), STC(0x79bc384d), STC(0x78a879f4), STC(0x777f903c), STC(0x7641af3d),
  STC(0x7fd317b4), STC(0x7f4c7e54), STC(0x7e6c9251), STC(0x7d33f0ca), STC(0x7ba3751d), STC(0x79bc384d), STC(0x777f903c), STC(0x74ef0ebc), STC(0x720c8075), STC(0x6ed9eba1), STC(0x6b598ea3), STC(0x678dde6e), STC(0x637984d4), STC(0x5f1f5ea1), STC(0x5a82799a),
  STC(0x7f9afcb9), STC(0x7e6c9251), STC(0x7c769e18), STC(0x79bc384d), STC(0x7641af3d), STC(0x720c8075), STC(0x6d23501b), STC(0x678dde6e), STC(0x6154fb91), STC(0x5a82799a), STC(0x53211d18), STC(0x4b3c8c12), STC(0x42e13ba4), STC(0x3a1c5c57), STC(0x30fbc54d),
  STC(0x7f4c7e54), STC(0x7d33f0ca), STC(0x79bc384d), STC(0x74ef0ebc), STC(0x6ed9eba1), STC(0x678dde6e), STC(0x5f1f5ea1), STC(0x55a6125c), STC(0x4b3c8c12), STC(0x40000000), STC(0x340ff242), STC(0x278dde6e), STC(0x1a9cd9ac), STC(0x0d61304e), STC(0x00000000),
  STC(0x7ee7aa4c), STC(0x7ba3751d), STC(0x7641af3d), STC(0x6ed9eba1), STC(0x658c9a2d), STC(0x5a82799a), STC(0x4debe4fe), STC(0x40000000), STC(0x30fbc54d), STC(0x2120fb83), STC(0x10b5150f), STC(0x00000000), STC(0xef4aeaf0), STC(0xdedf047c), STC(0xcf043ab2),
  STC(0x7e6c9251), STC(0x79bc384d), STC(0x720c8075), STC(0x678dde6e), STC(0x5a82799a), STC(0x4b3c8c12), STC(0x3a1c5c57), STC(0x278dde6e), STC(0x14060b68), STC(0x00000000), STC(0xebf9f497), STC(0xd8722191), STC(0xc5e3a3a8), STC(0xb4c373ed), STC(0xa57d8665),
  STC(0x7ddb4bfc), STC(0x777f903c), STC(0x6d23501b), STC(0x5f1f5ea1), STC(0x4debe4fe), STC(0x3a1c5c57), STC(0x245a9d65), STC(0x0d61304e), STC(0xf5f50d66), STC(0xdedf047c), STC(0xc8e5032a), STC(0xb4c373ed), STC(0xa326eebf), STC(0x94a6715c), STC(0x89be50c2),
  STC(0x7d33f0ca), STC(0x74ef0ebc), STC(0x678dde6e), STC(0x55a6125c), STC(0x40000000), STC(0x278dde6e), STC(0x0d61304e), STC(0xf29ecfb1), STC(0xd8722191), STC(0xbfffffff), STC(0xaa59eda3), STC(0x98722191), STC(0x8b10f143), STC(0x82cc0f35), STC(0x80000000),
  STC(0x7c769e18), STC(0x720c8075), STC(0x6154fb91), STC(0x4b3c8c12), STC(0x30fbc54d), STC(0x14060b68), STC(0xf5f50d66), STC(0xd8722191), STC(0xbd1ec45b), STC(0xa57d8665), STC(0x92dcafe4), STC(0x8643c7b2), STC(0x80650346), STC(0x81936dae), STC(0x89be50c2),
  STC(0x7ba3751d), STC(0x6ed9eba1), STC(0x5a82799a), STC(0x40000000), STC(0x2120fb83), STC(0x00000000), STC(0xdedf047c), STC(0xbfffffff), STC(0xa57d8665), STC(0x9126145e), STC(0x845c8ae2), STC(0x80000000), STC(0x845c8ae2), STC(0x9126145e), STC(0xa57d8665),
  STC(0x7aba9ae6), STC(0x6b598ea3), STC(0x53211d18), STC(0x340ff242), STC(0x10b5150f), STC(0xebf9f497), STC(0xc8e5032a), STC(0xaa59eda3), STC(0x92dcafe4), STC(0x845c8ae2), STC(0x800b3a90), STC(0x8643c7b2), STC(0x96830875), STC(0xaf726dee), STC(0xcf043ab2),
  STC(0x79bc384d), STC(0x678dde6e), STC(0x4b3c8c12), STC(0x278dde6e), STC(0x00000000), STC(0xd8722191), STC(0xb4c373ed), STC(0x98722191), STC(0x8643c7b2), STC(0x80000000), STC(0x8643c7b2), STC(0x98722191), STC(0xb4c373ed), STC(0xd8722191), STC(0xffffffff),
  STC(0x78a879f4), STC(0x637984d4), STC(0x42e13ba4), STC(0x1a9cd9ac), STC(0xef4aeaf0), STC(0xc5e3a3a8), STC(0xa326eebf), STC(0x8b10f143), STC(0x80650346), STC(0x845c8ae2), STC(0x96830875), STC(0xb4c373ed), STC(0xdba5629a), STC(0x06b2f1d2), STC(0x30fbc54d),
  STC(0x777f903c), STC(0x5f1f5ea1), STC(0x3a1c5c57), STC(0x0d61304e), STC(0xdedf047c), STC(0xb4c373ed), STC(0x94a6715c), STC(0x82cc0f35), STC(0x81936dae), STC(0x9126145e), STC(0xaf726dee), STC(0xd8722191), STC(0x06b2f1d2), STC(0x340ff242), STC(0x5a82799a),
};
RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_STB RotVectorImag240[] =
{
  STC(0x0359c428), STC(0x06b2f1d2), STC(0x0a0af299), STC(0x0d61304e), STC(0x10b5150f), STC(0x14060b68), STC(0x17537e63), STC(0x1a9cd9ac), STC(0x1de189a6), STC(0x2120fb83), STC(0x245a9d65), STC(0x278dde6e), STC(0x2aba2ee4), STC(0x2ddf0040), STC(0x30fbc54d),
  STC(0x06b2f1d2), STC(0x0d61304e), STC(0x14060b68), STC(0x1a9cd9ac), STC(0x2120fb83), STC(0x278dde6e), STC(0x2ddf0040), STC(0x340ff242), STC(0x3a1c5c57), STC(0x40000000), STC(0x45b6bb5e), STC(0x4b3c8c12), STC(0x508d9211), STC(0x55a6125c), STC(0x5a82799a),
  STC(0x0a0af299), STC(0x14060b68), STC(0x1de189a6), STC(0x278dde6e), STC(0x30fbc54d), STC(0x3a1c5c57), STC(0x42e13ba4), STC(0x4b3c8c12), STC(0x53211d18), STC(0x5a82799a), STC(0x6154fb91), STC(0x678dde6e), STC(0x6d23501b), STC(0x720c8075), STC(0x7641af3d),
  STC(0x0d61304e), STC(0x1a9cd9ac), STC(0x278dde6e), STC(0x340ff242), STC(0x40000000), STC(0x4b3c8c12), STC(0x55a6125c), STC(0x5f1f5ea1), STC(0x678dde6e), STC(0x6ed9eba1), STC(0x74ef0ebc), STC(0x79bc384d), STC(0x7d33f0ca), STC(0x7f4c7e54), STC(0x7fffffff),
  STC(0x10b5150f), STC(0x2120fb83), STC(0x30fbc54d), STC(0x40000000), STC(0x4debe4fe), STC(0x5a82799a), STC(0x658c9a2d), STC(0x6ed9eba1), STC(0x7641af3d), STC(0x7ba3751d), STC(0x7ee7aa4c), STC(0x7fffffff), STC(0x7ee7aa4c), STC(0x7ba3751d), STC(0x7641af3d),
  STC(0x14060b68), STC(0x278dde6e), STC(0x3a1c5c57), STC(0x4b3c8c12), STC(0x5a82799a), STC(0x678dde6e), STC(0x720c8075), STC(0x79bc384d), STC(0x7e6c9251), STC(0x7fffffff), STC(0x7e6c9251), STC(0x79bc384d), STC(0x720c8075), STC(0x678dde6e), STC(0x5a82799a),
  STC(0x17537e63), STC(0x2ddf0040), STC(0x42e13ba4), STC(0x55a6125c), STC(0x658c9a2d), STC(0x720c8075), STC(0x7aba9ae6), STC(0x7f4c7e54), STC(0x7f9afcb9), STC(0x7ba3751d), STC(0x7387ea23), STC(0x678dde6e), STC(0x581c00b3), STC(0x45b6bb5e), STC(0x30fbc54d),
  STC(0x1a9cd9ac), STC(0x340ff242), STC(0x4b3c8c12), STC(0x5f1f5ea1), STC(0x6ed9eba1), STC(0x79bc384d), STC(0x7f4c7e54), STC(0x7f4c7e54), STC(0x79bc384d), STC(0x6ed9eba1), STC(0x5f1f5ea1), STC(0x4b3c8c12), STC(0x340ff242), STC(0x1a9cd9ac), STC(0x00000000),
  STC(0x1de189a6), STC(0x3a1c5c57), STC(0x53211d18), STC(0x678dde6e), STC(0x7641af3d), STC(0x7e6c9251), STC(0x7f9afcb9), STC(0x79bc384d), STC(0x6d23501b), STC(0x5a82799a), STC(0x42e13ba4), STC(0x278dde6e), STC(0x0a0af299), STC(0xebf9f497), STC(0xcf043ab2),
  STC(0x2120fb83), STC(0x40000000), STC(0x5a82799a), STC(0x6ed9eba1), STC(0x7ba3751d), STC(0x7fffffff), STC(0x7ba3751d), STC(0x6ed9eba1), STC(0x5a82799a), STC(0x40000000), STC(0x2120fb83), STC(0x00000000), STC(0xdedf047c), STC(0xbfffffff), STC(0xa57d8665),
  STC(0x245a9d65), STC(0x45b6bb5e), STC(0x6154fb91), STC(0x74ef0ebc), STC(0x7ee7aa4c), STC(0x7e6c9251), STC(0x7387ea23), STC(0x5f1f5ea1), STC(0x42e13ba4), STC(0x2120fb83), STC(0xfca63bd7), STC(0xd8722191), STC(0xb780001b), STC(0x9c867b2b), STC(0x89be50c2),
  STC(0x278dde6e), STC(0x4b3c8c12), STC(0x678dde6e), STC(0x79bc384d), STC(0x7fffffff), STC(0x79bc384d), STC(0x678dde6e), STC(0x4b3c8c12), STC(0x278dde6e), STC(0x00000000), STC(0xd8722191), STC(0xb4c373ed), STC(0x98722191), STC(0x8643c7b2), STC(0x80000000),
  STC(0x2aba2ee4), STC(0x508d9211), STC(0x6d23501b), STC(0x7d33f0ca), STC(0x7ee7aa4c), STC(0x720c8075), STC(0x581c00b3), STC(0x340ff242), STC(0x0a0af299), STC(0xdedf047c), STC(0xb780001b), STC(0x98722191), STC(0x85456519), STC(0x802ce84b), STC(0x89be50c2),
  STC(0x2ddf0040), STC(0x55a6125c), STC(0x720c8075), STC(0x7f4c7e54), STC(0x7ba3751d), STC(0x678dde6e), STC(0x45b6bb5e), STC(0x1a9cd9ac), STC(0xebf9f497), STC(0xbfffffff), STC(0x9c867b2b), STC(0x8643c7b2), STC(0x802ce84b), STC(0x8b10f143), STC(0xa57d8665),
};

RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_STB RotVectorReal480[] =
{
  STC(0x7ffd3154), STC(0x7ff4c56f), STC(0x7fe6bcb0), STC(0x7fd317b4), STC(0x7fb9d759), STC(0x7f9afcb9), STC(0x7f76892f), STC(0x7f4c7e54), STC(0x7f1cde01), STC(0x7ee7aa4c), STC(0x7eace58a), STC(0x7e6c9251), STC(0x7e26b371), STC(0x7ddb4bfc), STC(0x7d8a5f40), STC(0x7d33f0ca), STC(0x7cd80464), STC(0x7c769e18), STC(0x7c0fc22a), STC(0x7ba3751d), STC(0x7b31bbb2), STC(0x7aba9ae6), STC(0x7a3e17f2), STC(0x79bc384d), STC(0x793501a9), STC(0x78a879f4), STC(0x7816a759), STC(0x777f903c), STC(0x76e33b3f), STC(0x7641af3d), STC(0x759af34c),
  STC(0x7ff4c56f), STC(0x7fd317b4), STC(0x7f9afcb9), STC(0x7f4c7e54), STC(0x7ee7aa4c), STC(0x7e6c9251), STC(0x7ddb4bfc), STC(0x7d33f0ca), STC(0x7c769e18), STC(0x7ba3751d), STC(0x7aba9ae6), STC(0x79bc384d), STC(0x78a879f4), STC(0x777f903c), STC(0x7641af3d), STC(0x74ef0ebc), STC(0x7387ea23), STC(0x720c8075), STC(0x707d1443), STC(0x6ed9eba1), STC(0x6d23501b), STC(0x6b598ea3), STC(0x697cf78a), STC(0x678dde6e), STC(0x658c9a2d), STC(0x637984d4), STC(0x6154fb91), STC(0x5f1f5ea1), STC(0x5cd91140), STC(0x5a82799a), STC(0x581c00b3),
  STC(0x7fe6bcb0), STC(0x7f9afcb9), STC(0x7f1cde01), STC(0x7e6c9251), STC(0x7d8a5f40), STC(0x7c769e18), STC(0x7b31bbb2), STC(0x79bc384d), STC(0x7816a759), STC(0x7641af3d), STC(0x743e0918), STC(0x720c8075), STC(0x6fadf2fc), STC(0x6d23501b), STC(0x6a6d98a4), STC(0x678dde6e), STC(0x648543e4), STC(0x6154fb91), STC(0x5dfe47ad), STC(0x5a82799a), STC(0x56e2f15d), STC(0x53211d18), STC(0x4f3e7875), STC(0x4b3c8c12), STC(0x471cece7), STC(0x42e13ba4), STC(0x3e8b240e), STC(0x3a1c5c57), STC(0x3596a46c), STC(0x30fbc54d), STC(0x2c4d9050),
  STC(0x7fd317b4), STC(0x7f4c7e54), STC(0x7e6c9251), STC(0x7d33f0ca), STC(0x7ba3751d), STC(0x79bc384d), STC(0x777f903c), STC(0x74ef0ebc), STC(0x720c8075), STC(0x6ed9eba1), STC(0x6b598ea3), STC(0x678dde6e), STC(0x637984d4), STC(0x5f1f5ea1), STC(0x5a82799a), STC(0x55a6125c), STC(0x508d9211), STC(0x4b3c8c12), STC(0x45b6bb5e), STC(0x40000000), STC(0x3a1c5c57), STC(0x340ff242), STC(0x2ddf0040), STC(0x278dde6e), STC(0x2120fb83), STC(0x1a9cd9ac), STC(0x14060b68), STC(0x0d61304e), STC(0x06b2f1d2), STC(0x00000000), STC(0xf94d0e2d),
  STC(0x7fb9d759), STC(0x7ee7aa4c), STC(0x7d8a5f40), STC(0x7ba3751d), STC(0x793501a9), STC(0x7641af3d), STC(0x72ccb9db), STC(0x6ed9eba1), STC(0x6a6d98a4), STC(0x658c9a2d), STC(0x603c496c), STC(0x5a82799a), STC(0x54657194), STC(0x4debe4fe), STC(0x471cece7), STC(0x40000000), STC(0x389cea72), STC(0x30fbc54d), STC(0x2924edac), STC(0x2120fb83), STC(0x18f8b83c), STC(0x10b5150f), STC(0x085f2137), STC(0x00000000), STC(0xf7a0dec8), STC(0xef4aeaf0), STC(0xe70747c3), STC(0xdedf047c), STC(0xd6db1253), STC(0xcf043ab2), STC(0xc763158d),
  STC(0x7f9afcb9), STC(0x7e6c9251), STC(0x7c769e18), STC(0x79bc384d), STC(0x7641af3d), STC(0x720c8075), STC(0x6d23501b), STC(0x678dde6e), STC(0x6154fb91), STC(0x5a82799a), STC(0x53211d18), STC(0x4b3c8c12), STC(0x42e13ba4), STC(0x3a1c5c57), STC(0x30fbc54d), STC(0x278dde6e), STC(0x1de189a6), STC(0x14060b68), STC(0x0a0af299), STC(0x00000000), STC(0xf5f50d66), STC(0xebf9f497), STC(0xe21e7659), STC(0xd8722191), STC(0xcf043ab2), STC(0xc5e3a3a8), STC(0xbd1ec45b), STC(0xb4c373ed), STC(0xacdee2e7), STC(0xa57d8665), STC(0x9eab046e),
  STC(0x7f76892f), STC(0x7ddb4bfc), STC(0x7b31bbb2), STC(0x777f903c), STC(0x72ccb9db), STC(0x6d23501b), STC(0x668f7c25), STC(0x5f1f5ea1), STC(0x56e2f15d), STC(0x4debe4fe), STC(0x444d7aff), STC(0x3a1c5c57), STC(0x2f6e6d16), STC(0x245a9d65), STC(0x18f8b83c), STC(0x0d61304e), STC(0x01aceb7c), STC(0xf5f50d66), STC(0xea52c165), STC(0xdedf047c), STC(0xd3b26faf), STC(0xc8e5032a), STC(0xbe8df2b9), STC(0xb4c373ed), STC(0xab9a8e6b), STC(0xa326eebf), STC(0x9b7abc1b), STC(0x94a6715c), STC(0x8eb8b99f), STC(0x89be50c2), STC(0x85c1e80d),
  STC(0x7f4c7e54), STC(0x7d33f0ca), STC(0x79bc384d), STC(0x74ef0ebc), STC(0x6ed9eba1), STC(0x678dde6e), STC(0x5f1f5ea1), STC(0x55a6125c), STC(0x4b3c8c12), STC(0x40000000), STC(0x340ff242), STC(0x278dde6e), STC(0x1a9cd9ac), STC(0x0d61304e), STC(0x00000000), STC(0xf29ecfb1), STC(0xe5632653), STC(0xd8722191), STC(0xcbf00dbd), STC(0xbfffffff), STC(0xb4c373ed), STC(0xaa59eda3), STC(0xa0e0a15e), STC(0x98722191), STC(0x9126145e), STC(0x8b10f143), STC(0x8643c7b2), STC(0x82cc0f35), STC(0x80b381ab), STC(0x80000000), STC(0x80b381ab),
  STC(0x7f1cde01), STC(0x7c769e18), STC(0x7816a759), STC(0x720c8075), STC(0x6a6d98a4), STC(0x6154fb91), STC(0x56e2f15d), STC(0x4b3c8c12), STC(0x3e8b240e), STC(0x30fbc54d), STC(0x22be8f87), STC(0x14060b68), STC(0x05067734), STC(0xf5f50d66), STC(0xe70747c3), STC(0xd8722191), STC(0xca695b93), STC(0xbd1ec45b), STC(0xb0c1878a), STC(0xa57d8665), STC(0x9b7abc1b), STC(0x92dcafe4), STC(0x8bc1f6e7), STC(0x8643c7b2), STC(0x8275a0bf), STC(0x80650346), STC(0x8019434f), STC(0x81936dae), STC(0x84ce444d), STC(0x89be50c2), STC(0x90520d03),
  STC(0x7ee7aa4c), STC(0x7ba3751d), STC(0x7641af3d), STC(0x6ed9eba1), STC(0x658c9a2d), STC(0x5a82799a), STC(0x4debe4fe), STC(0x40000000), STC(0x30fbc54d), STC(0x2120fb83), STC(0x10b5150f), STC(0x00000000), STC(0xef4aeaf0), STC(0xdedf047c), STC(0xcf043ab2), STC(0xbfffffff), STC(0xb2141b01), STC(0xa57d8665), STC(0x9a7365d2), STC(0x9126145e), STC(0x89be50c2), STC(0x845c8ae2), STC(0x811855b3), STC(0x80000000), STC(0x811855b3), STC(0x845c8ae2), STC(0x89be50c2), STC(0x9126145e), STC(0x9a7365d2), STC(0xa57d8665), STC(0xb2141b01),
  STC(0x7eace58a), STC(0x7aba9ae6), STC(0x743e0918), STC(0x6b598ea3), STC(0x603c496c), STC(0x53211d18), STC(0x444d7aff), STC(0x340ff242), STC(0x22be8f87), STC(0x10b5150f), STC(0xfe531483), STC(0xebf9f497), STC(0xda0aecf8), STC(0xc8e5032a), STC(0xb8e31318), STC(0xaa59eda3), STC(0x9d969741), STC(0x92dcafe4), STC(0x8a650cb3), STC(0x845c8ae2), STC(0x80e321fe), STC(0x800b3a90), STC(0x81d94c8e), STC(0x8643c7b2), STC(0x8d334624), STC(0x96830875), STC(0xa201b852), STC(0xaf726dee), STC(0xbe8df2b9), STC(0xcf043ab2), STC(0xe07e0c83),
  STC(0x7e6c9251), STC(0x79bc384d), STC(0x720c8075), STC(0x678dde6e), STC(0x5a82799a), STC(0x4b3c8c12), STC(0x3a1c5c57), STC(0x278dde6e), STC(0x14060b68), STC(0x00000000), STC(0xebf9f497), STC(0xd8722191), STC(0xc5e3a3a8), STC(0xb4c373ed), STC(0xa57d8665), STC(0x98722191), STC(0x8df37f8a), STC(0x8643c7b2), STC(0x81936dae), STC(0x80000000), STC(0x81936dae), STC(0x8643c7b2), STC(0x8df37f8a), STC(0x98722191), STC(0xa57d8665), STC(0xb4c373ed), STC(0xc5e3a3a8), STC(0xd8722191), STC(0xebf9f497), STC(0xffffffff), STC(0x14060b68),
  STC(0x7e26b371), STC(0x78a879f4), STC(0x6fadf2fc), STC(0x637984d4), STC(0x54657194), STC(0x42e13ba4), STC(0x2f6e6d16), STC(0x1a9cd9ac), STC(0x05067734), STC(0xef4aeaf0), STC(0xda0aecf8), STC(0xc5e3a3a8), STC(0xb36a1977), STC(0xa326eebf), STC(0x9592675b), STC(0x8b10f143), STC(0x83f03dd5), STC(0x80650346), STC(0x808976d0), STC(0x845c8ae2), STC(0x8bc1f6e7), STC(0x96830875), STC(0xa45037c8), STC(0xb4c373ed), STC(0xc763158d), STC(0xdba5629a), STC(0xf0f488d8), STC(0x06b2f1d2), STC(0x1c3fd045), STC(0x30fbc54d), STC(0x444d7aff),
  STC(0x7ddb4bfc), STC(0x777f903c), STC(0x6d23501b), STC(0x5f1f5ea1), STC(0x4debe4fe), STC(0x3a1c5c57), STC(0x245a9d65), STC(0x0d61304e), STC(0xf5f50d66), STC(0xdedf047c), STC(0xc8e5032a), STC(0xb4c373ed), STC(0xa326eebf), STC(0x94a6715c), STC(0x89be50c2), STC(0x82cc0f35), STC(0x800b3a90), STC(0x81936dae), STC(0x8757860b), STC(0x9126145e), STC(0x9eab046e), STC(0xaf726dee), STC(0xc2ec7634), STC(0xd8722191), STC(0xef4aeaf0), STC(0x06b2f1d2), STC(0x1de189a6), STC(0x340ff242), STC(0x487fffe4), STC(0x5a82799a), STC(0x697cf78a),
};
RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_STB RotVectorImag480[] =
{
  STC(0x01aceb7c), STC(0x0359c428), STC(0x05067734), STC(0x06b2f1d2), STC(0x085f2137), STC(0x0a0af299), STC(0x0bb65336), STC(0x0d61304e), STC(0x0f0b7727), STC(0x10b5150f), STC(0x125df75b), STC(0x14060b68), STC(0x15ad3e9a), STC(0x17537e63), STC(0x18f8b83c), STC(0x1a9cd9ac), STC(0x1c3fd045), STC(0x1de189a6), STC(0x1f81f37c), STC(0x2120fb83), STC(0x22be8f87), STC(0x245a9d65), STC(0x25f51307), STC(0x278dde6e), STC(0x2924edac), STC(0x2aba2ee4), STC(0x2c4d9050), STC(0x2ddf0040), STC(0x2f6e6d16), STC(0x30fbc54d), STC(0x3286f779),
  STC(0x0359c428), STC(0x06b2f1d2), STC(0x0a0af299), STC(0x0d61304e), STC(0x10b5150f), STC(0x14060b68), STC(0x17537e63), STC(0x1a9cd9ac), STC(0x1de189a6), STC(0x2120fb83), STC(0x245a9d65), STC(0x278dde6e), STC(0x2aba2ee4), STC(0x2ddf0040), STC(0x30fbc54d), STC(0x340ff242), STC(0x371afcd5), STC(0x3a1c5c57), STC(0x3d1389cb), STC(0x40000000), STC(0x42e13ba4), STC(0x45b6bb5e), STC(0x487fffe4), STC(0x4b3c8c12), STC(0x4debe4fe), STC(0x508d9211), STC(0x53211d18), STC(0x55a6125c), STC(0x581c00b3), STC(0x5a82799a), STC(0x5cd91140),
  STC(0x05067734), STC(0x0a0af299), STC(0x0f0b7727), STC(0x14060b68), STC(0x18f8b83c), STC(0x1de189a6), STC(0x22be8f87), STC(0x278dde6e), STC(0x2c4d9050), STC(0x30fbc54d), STC(0x3596a46c), STC(0x3a1c5c57), STC(0x3e8b240e), STC(0x42e13ba4), STC(0x471cece7), STC(0x4b3c8c12), STC(0x4f3e7875), STC(0x53211d18), STC(0x56e2f15d), STC(0x5a82799a), STC(0x5dfe47ad), STC(0x6154fb91), STC(0x648543e4), STC(0x678dde6e), STC(0x6a6d98a4), STC(0x6d23501b), STC(0x6fadf2fc), STC(0x720c8075), STC(0x743e0918), STC(0x7641af3d), STC(0x7816a759),
  STC(0x06b2f1d2), STC(0x0d61304e), STC(0x14060b68), STC(0x1a9cd9ac), STC(0x2120fb83), STC(0x278dde6e), STC(0x2ddf0040), STC(0x340ff242), STC(0x3a1c5c57), STC(0x40000000), STC(0x45b6bb5e), STC(0x4b3c8c12), STC(0x508d9211), STC(0x55a6125c), STC(0x5a82799a), STC(0x5f1f5ea1), STC(0x637984d4), STC(0x678dde6e), STC(0x6b598ea3), STC(0x6ed9eba1), STC(0x720c8075), STC(0x74ef0ebc), STC(0x777f903c), STC(0x79bc384d), STC(0x7ba3751d), STC(0x7d33f0ca), STC(0x7e6c9251), STC(0x7f4c7e54), STC(0x7fd317b4), STC(0x7fffffff), STC(0x7fd317b4),
  STC(0x085f2137), STC(0x10b5150f), STC(0x18f8b83c), STC(0x2120fb83), STC(0x2924edac), STC(0x30fbc54d), STC(0x389cea72), STC(0x40000000), STC(0x471cece7), STC(0x4debe4fe), STC(0x54657194), STC(0x5a82799a), STC(0x603c496c), STC(0x658c9a2d), STC(0x6a6d98a4), STC(0x6ed9eba1), STC(0x72ccb9db), STC(0x7641af3d), STC(0x793501a9), STC(0x7ba3751d), STC(0x7d8a5f40), STC(0x7ee7aa4c), STC(0x7fb9d759), STC(0x7fffffff), STC(0x7fb9d759), STC(0x7ee7aa4c), STC(0x7d8a5f40), STC(0x7ba3751d), STC(0x793501a9), STC(0x7641af3d), STC(0x72ccb9db),
  STC(0x0a0af299), STC(0x14060b68), STC(0x1de189a6), STC(0x278dde6e), STC(0x30fbc54d), STC(0x3a1c5c57), STC(0x42e13ba4), STC(0x4b3c8c12), STC(0x53211d18), STC(0x5a82799a), STC(0x6154fb91), STC(0x678dde6e), STC(0x6d23501b), STC(0x720c8075), STC(0x7641af3d), STC(0x79bc384d), STC(0x7c769e18), STC(0x7e6c9251), STC(0x7f9afcb9), STC(0x7fffffff), STC(0x7f9afcb9), STC(0x7e6c9251), STC(0x7c769e18), STC(0x79bc384d), STC(0x7641af3d), STC(0x720c8075), STC(0x6d23501b), STC(0x678dde6e), STC(0x6154fb91), STC(0x5a82799a), STC(0x53211d18),
  STC(0x0bb65336), STC(0x17537e63), STC(0x22be8f87), STC(0x2ddf0040), STC(0x389cea72), STC(0x42e13ba4), STC(0x4c95e688), STC(0x55a6125c), STC(0x5dfe47ad), STC(0x658c9a2d), STC(0x6c40cf2c), STC(0x720c8075), STC(0x76e33b3f), STC(0x7aba9ae6), STC(0x7d8a5f40), STC(0x7f4c7e54), STC(0x7ffd3154), STC(0x7f9afcb9), STC(0x7e26b371), STC(0x7ba3751d), STC(0x7816a759), STC(0x7387ea23), STC(0x6e010780), STC(0x678dde6e), STC(0x603c496c), STC(0x581c00b3), STC(0x4f3e7875), STC(0x45b6bb5e), STC(0x3b9941b1), STC(0x30fbc54d), STC(0x25f51307),
  STC(0x0d61304e), STC(0x1a9cd9ac), STC(0x278dde6e), STC(0x340ff242), STC(0x40000000), STC(0x4b3c8c12), STC(0x55a6125c), STC(0x5f1f5ea1), STC(0x678dde6e), STC(0x6ed9eba1), STC(0x74ef0ebc), STC(0x79bc384d), STC(0x7d33f0ca), STC(0x7f4c7e54), STC(0x7fffffff), STC(0x7f4c7e54), STC(0x7d33f0ca), STC(0x79bc384d), STC(0x74ef0ebc), STC(0x6ed9eba1), STC(0x678dde6e), STC(0x5f1f5ea1), STC(0x55a6125c), STC(0x4b3c8c12), STC(0x40000000), STC(0x340ff242), STC(0x278dde6e), STC(0x1a9cd9ac), STC(0x0d61304e), STC(0x00000000), STC(0xf29ecfb1),
  STC(0x0f0b7727), STC(0x1de189a6), STC(0x2c4d9050), STC(0x3a1c5c57), STC(0x471cece7), STC(0x53211d18), STC(0x5dfe47ad), STC(0x678dde6e), STC(0x6fadf2fc), STC(0x7641af3d), STC(0x7b31bbb2), STC(0x7e6c9251), STC(0x7fe6bcb0), STC(0x7f9afcb9), STC(0x7d8a5f40), STC(0x79bc384d), STC(0x743e0918), STC(0x6d23501b), STC(0x648543e4), STC(0x5a82799a), STC(0x4f3e7875), STC(0x42e13ba4), STC(0x3596a46c), STC(0x278dde6e), STC(0x18f8b83c), STC(0x0a0af299), STC(0xfaf988cb), STC(0xebf9f497), STC(0xdd417078), STC(0xcf043ab2), STC(0xc174dbf1),
  STC(0x10b5150f), STC(0x2120fb83), STC(0x30fbc54d), STC(0x40000000), STC(0x4debe4fe), STC(0x5a82799a), STC(0x658c9a2d), STC(0x6ed9eba1), STC(0x7641af3d), STC(0x7ba3751d), STC(0x7ee7aa4c), STC(0x7fffffff), STC(0x7ee7aa4c), STC(0x7ba3751d), STC(0x7641af3d), STC(0x6ed9eba1), STC(0x658c9a2d), STC(0x5a82799a), STC(0x4debe4fe), STC(0x40000000), STC(0x30fbc54d), STC(0x2120fb83), STC(0x10b5150f), STC(0x00000000), STC(0xef4aeaf0), STC(0xdedf047c), STC(0xcf043ab2), STC(0xbfffffff), STC(0xb2141b01), STC(0xa57d8665), STC(0x9a7365d2),
  STC(0x125df75b), STC(0x245a9d65), STC(0x3596a46c), STC(0x45b6bb5e), STC(0x54657194), STC(0x6154fb91), STC(0x6c40cf2c), STC(0x74ef0ebc), STC(0x7b31bbb2), STC(0x7ee7aa4c), STC(0x7ffd3154), STC(0x7e6c9251), STC(0x7a3e17f2), STC(0x7387ea23), STC(0x6a6d98a4), STC(0x5f1f5ea1), STC(0x51d92321), STC(0x42e13ba4), STC(0x3286f779), STC(0x2120fb83), STC(0x0f0b7727), STC(0xfca63bd7), STC(0xea52c165), STC(0xd8722191), STC(0xc763158d), STC(0xb780001b), STC(0xa91d0ea2), STC(0x9c867b2b), STC(0x91fef87f), STC(0x89be50c2), STC(0x83f03dd5),
  STC(0x14060b68), STC(0x278dde6e), STC(0x3a1c5c57), STC(0x4b3c8c12), STC(0x5a82799a), STC(0x678dde6e), STC(0x720c8075), STC(0x79bc384d), STC(0x7e6c9251), STC(0x7fffffff), STC(0x7e6c9251), STC(0x79bc384d), STC(0x720c8075), STC(0x678dde6e), STC(0x5a82799a), STC(0x4b3c8c12), STC(0x3a1c5c57), STC(0x278dde6e), STC(0x14060b68), STC(0x00000000), STC(0xebf9f497), STC(0xd8722191), STC(0xc5e3a3a8), STC(0xb4c373ed), STC(0xa57d8665), STC(0x98722191), STC(0x8df37f8a), STC(0x8643c7b2), STC(0x81936dae), STC(0x80000000), STC(0x81936dae),
  STC(0x15ad3e9a), STC(0x2aba2ee4), STC(0x3e8b240e), STC(0x508d9211), STC(0x603c496c), STC(0x6d23501b), STC(0x76e33b3f), STC(0x7d33f0ca), STC(0x7fe6bcb0), STC(0x7ee7aa4c), STC(0x7a3e17f2), STC(0x720c8075), STC(0x668f7c25), STC(0x581c00b3), STC(0x471cece7), STC(0x340ff242), STC(0x1f81f37c), STC(0x0a0af299), STC(0xf449acc9), STC(0xdedf047c), STC(0xca695b93), STC(0xb780001b), STC(0xa6aecd5d), STC(0x98722191), STC(0x8d334624), STC(0x85456519), STC(0x80e321fe), STC(0x802ce84b), STC(0x8327fb9b), STC(0x89be50c2), STC(0x93bf30d3),
  STC(0x17537e63), STC(0x2ddf0040), STC(0x42e13ba4), STC(0x55a6125c), STC(0x658c9a2d), STC(0x720c8075), STC(0x7aba9ae6), STC(0x7f4c7e54), STC(0x7f9afcb9), STC(0x7ba3751d), STC(0x7387ea23), STC(0x678dde6e), STC(0x581c00b3), STC(0x45b6bb5e), STC(0x30fbc54d), STC(0x1a9cd9ac), STC(0x0359c428), STC(0xebf9f497), STC(0xd545d11b), STC(0xbfffffff), STC(0xacdee2e7), STC(0x9c867b2b), STC(0x8f82ebbc), STC(0x8643c7b2), STC(0x811855b3), STC(0x802ce84b), STC(0x838961e7), STC(0x8b10f143), STC(0x96830875), STC(0xa57d8665), STC(0xb780001b),
};


RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_WTP SineWindow1024[] =
{
  WTCP(0x7ffffd88, 0x001921fb), WTCP(0x7fffe9cb, 0x004b65ee), WTCP(0x7fffc251, 0x007da9d4), WTCP(0x7fff8719, 0x00afeda8),
  WTCP(0x7fff3824, 0x00e23160), WTCP(0x7ffed572, 0x011474f6), WTCP(0x7ffe5f03, 0x0146b860), WTCP(0x7ffdd4d7, 0x0178fb99),
  WTCP(0x7ffd36ee, 0x01ab3e97), WTCP(0x7ffc8549, 0x01dd8154), WTCP(0x7ffbbfe6, 0x020fc3c6), WTCP(0x7ffae6c7, 0x024205e8),
  WTCP(0x7ff9f9ec, 0x027447b0), WTCP(0x7ff8f954, 0x02a68917), WTCP(0x7ff7e500, 0x02d8ca16), WTCP(0x7ff6bcf0, 0x030b0aa4),
  WTCP(0x7ff58125, 0x033d4abb), WTCP(0x7ff4319d, 0x036f8a51), WTCP(0x7ff2ce5b, 0x03a1c960), WTCP(0x7ff1575d, 0x03d407df),
  WTCP(0x7fefcca4, 0x040645c7), WTCP(0x7fee2e30, 0x04388310), WTCP(0x7fec7c02, 0x046abfb3), WTCP(0x7feab61a, 0x049cfba7),
  WTCP(0x7fe8dc78, 0x04cf36e5), WTCP(0x7fe6ef1c, 0x05017165), WTCP(0x7fe4ee06, 0x0533ab20), WTCP(0x7fe2d938, 0x0565e40d),
  WTCP(0x7fe0b0b1, 0x05981c26), WTCP(0x7fde7471, 0x05ca5361), WTCP(0x7fdc247a, 0x05fc89b8), WTCP(0x7fd9c0ca, 0x062ebf22),
  WTCP(0x7fd74964, 0x0660f398), WTCP(0x7fd4be46, 0x06932713), WTCP(0x7fd21f72, 0x06c5598a), WTCP(0x7fcf6ce8, 0x06f78af6),
  WTCP(0x7fcca6a7, 0x0729bb4e), WTCP(0x7fc9ccb2, 0x075bea8c), WTCP(0x7fc6df08, 0x078e18a7), WTCP(0x7fc3dda9, 0x07c04598),
  WTCP(0x7fc0c896, 0x07f27157), WTCP(0x7fbd9fd0, 0x08249bdd), WTCP(0x7fba6357, 0x0856c520), WTCP(0x7fb7132b, 0x0888ed1b),
  WTCP(0x7fb3af4e, 0x08bb13c5), WTCP(0x7fb037bf, 0x08ed3916), WTCP(0x7facac7f, 0x091f5d06), WTCP(0x7fa90d8e, 0x09517f8f),
  WTCP(0x7fa55aee, 0x0983a0a7), WTCP(0x7fa1949e, 0x09b5c048), WTCP(0x7f9dbaa0, 0x09e7de6a), WTCP(0x7f99ccf4, 0x0a19fb04),
  WTCP(0x7f95cb9a, 0x0a4c1610), WTCP(0x7f91b694, 0x0a7e2f85), WTCP(0x7f8d8de1, 0x0ab0475c), WTCP(0x7f895182, 0x0ae25d8d),
  WTCP(0x7f850179, 0x0b147211), WTCP(0x7f809dc5, 0x0b4684df), WTCP(0x7f7c2668, 0x0b7895f0), WTCP(0x7f779b62, 0x0baaa53b),
  WTCP(0x7f72fcb4, 0x0bdcb2bb), WTCP(0x7f6e4a5e, 0x0c0ebe66), WTCP(0x7f698461, 0x0c40c835), WTCP(0x7f64aabf, 0x0c72d020),
  WTCP(0x7f5fbd77, 0x0ca4d620), WTCP(0x7f5abc8a, 0x0cd6da2d), WTCP(0x7f55a7fa, 0x0d08dc3f), WTCP(0x7f507fc7, 0x0d3adc4e),
  WTCP(0x7f4b43f2, 0x0d6cda53), WTCP(0x7f45f47b, 0x0d9ed646), WTCP(0x7f409164, 0x0dd0d01f), WTCP(0x7f3b1aad, 0x0e02c7d7),
  WTCP(0x7f359057, 0x0e34bd66), WTCP(0x7f2ff263, 0x0e66b0c3), WTCP(0x7f2a40d2, 0x0e98a1e9), WTCP(0x7f247ba5, 0x0eca90ce),
  WTCP(0x7f1ea2dc, 0x0efc7d6b), WTCP(0x7f18b679, 0x0f2e67b8), WTCP(0x7f12b67c, 0x0f604faf), WTCP(0x7f0ca2e7, 0x0f923546),
  WTCP(0x7f067bba, 0x0fc41876), WTCP(0x7f0040f6, 0x0ff5f938), WTCP(0x7ef9f29d, 0x1027d784), WTCP(0x7ef390ae, 0x1059b352),
  WTCP(0x7eed1b2c, 0x108b8c9b), WTCP(0x7ee69217, 0x10bd6356), WTCP(0x7edff570, 0x10ef377d), WTCP(0x7ed94538, 0x11210907),
  WTCP(0x7ed28171, 0x1152d7ed), WTCP(0x7ecbaa1a, 0x1184a427), WTCP(0x7ec4bf36, 0x11b66dad), WTCP(0x7ebdc0c6, 0x11e83478),
  WTCP(0x7eb6aeca, 0x1219f880), WTCP(0x7eaf8943, 0x124bb9be), WTCP(0x7ea85033, 0x127d7829), WTCP(0x7ea1039b, 0x12af33ba),
  WTCP(0x7e99a37c, 0x12e0ec6a), WTCP(0x7e922fd6, 0x1312a230), WTCP(0x7e8aa8ac, 0x13445505), WTCP(0x7e830dff, 0x137604e2),
  WTCP(0x7e7b5fce, 0x13a7b1bf), WTCP(0x7e739e1d, 0x13d95b93), WTCP(0x7e6bc8eb, 0x140b0258), WTCP(0x7e63e03b, 0x143ca605),
  WTCP(0x7e5be40c, 0x146e4694), WTCP(0x7e53d462, 0x149fe3fc), WTCP(0x7e4bb13c, 0x14d17e36), WTCP(0x7e437a9c, 0x1503153a),
  WTCP(0x7e3b3083, 0x1534a901), WTCP(0x7e32d2f4, 0x15663982), WTCP(0x7e2a61ed, 0x1597c6b7), WTCP(0x7e21dd73, 0x15c95097),
  WTCP(0x7e194584, 0x15fad71b), WTCP(0x7e109a24, 0x162c5a3b), WTCP(0x7e07db52, 0x165dd9f0), WTCP(0x7dff0911, 0x168f5632),
  WTCP(0x7df62362, 0x16c0cef9), WTCP(0x7ded2a47, 0x16f2443e), WTCP(0x7de41dc0, 0x1723b5f9), WTCP(0x7ddafdce, 0x17552422),
  WTCP(0x7dd1ca75, 0x17868eb3), WTCP(0x7dc883b4, 0x17b7f5a3), WTCP(0x7dbf298d, 0x17e958ea), WTCP(0x7db5bc02, 0x181ab881),
  WTCP(0x7dac3b15, 0x184c1461), WTCP(0x7da2a6c6, 0x187d6c82), WTCP(0x7d98ff17, 0x18aec0db), WTCP(0x7d8f4409, 0x18e01167),
  WTCP(0x7d85759f, 0x19115e1c), WTCP(0x7d7b93da, 0x1942a6f3), WTCP(0x7d719eba, 0x1973ebe6), WTCP(0x7d679642, 0x19a52ceb),
  WTCP(0x7d5d7a74, 0x19d669fc), WTCP(0x7d534b50, 0x1a07a311), WTCP(0x7d4908d9, 0x1a38d823), WTCP(0x7d3eb30f, 0x1a6a0929),
  WTCP(0x7d3449f5, 0x1a9b361d), WTCP(0x7d29cd8c, 0x1acc5ef6), WTCP(0x7d1f3dd6, 0x1afd83ad), WTCP(0x7d149ad5, 0x1b2ea43a),
  WTCP(0x7d09e489, 0x1b5fc097), WTCP(0x7cff1af5, 0x1b90d8bb), WTCP(0x7cf43e1a, 0x1bc1ec9e), WTCP(0x7ce94dfb, 0x1bf2fc3a),
  WTCP(0x7cde4a98, 0x1c240786), WTCP(0x7cd333f3, 0x1c550e7c), WTCP(0x7cc80a0f, 0x1c861113), WTCP(0x7cbcccec, 0x1cb70f43),
  WTCP(0x7cb17c8d, 0x1ce80906), WTCP(0x7ca618f3, 0x1d18fe54), WTCP(0x7c9aa221, 0x1d49ef26), WTCP(0x7c8f1817, 0x1d7adb73),
  WTCP(0x7c837ad8, 0x1dabc334), WTCP(0x7c77ca65, 0x1ddca662), WTCP(0x7c6c06c0, 0x1e0d84f5), WTCP(0x7c602fec, 0x1e3e5ee5),
  WTCP(0x7c5445e9, 0x1e6f342c), WTCP(0x7c4848ba, 0x1ea004c1), WTCP(0x7c3c3860, 0x1ed0d09d), WTCP(0x7c3014de, 0x1f0197b8),
  WTCP(0x7c23de35, 0x1f325a0b), WTCP(0x7c179467, 0x1f63178f), WTCP(0x7c0b3777, 0x1f93d03c), WTCP(0x7bfec765, 0x1fc4840a),
  WTCP(0x7bf24434, 0x1ff532f2), WTCP(0x7be5ade6, 0x2025dcec), WTCP(0x7bd9047c, 0x205681f1), WTCP(0x7bcc47fa, 0x208721f9),
  WTCP(0x7bbf7860, 0x20b7bcfe), WTCP(0x7bb295b0, 0x20e852f6), WTCP(0x7ba59fee, 0x2118e3dc), WTCP(0x7b989719, 0x21496fa7),
  WTCP(0x7b8b7b36, 0x2179f64f), WTCP(0x7b7e4c45, 0x21aa77cf), WTCP(0x7b710a49, 0x21daf41d), WTCP(0x7b63b543, 0x220b6b32),
  WTCP(0x7b564d36, 0x223bdd08), WTCP(0x7b48d225, 0x226c4996), WTCP(0x7b3b4410, 0x229cb0d5), WTCP(0x7b2da2fa, 0x22cd12bd),
  WTCP(0x7b1feee5, 0x22fd6f48), WTCP(0x7b1227d3, 0x232dc66d), WTCP(0x7b044dc7, 0x235e1826), WTCP(0x7af660c2, 0x238e646a),
  WTCP(0x7ae860c7, 0x23beab33), WTCP(0x7ada4dd8, 0x23eeec78), WTCP(0x7acc27f7, 0x241f2833), WTCP(0x7abdef25, 0x244f5e5c),
  WTCP(0x7aafa367, 0x247f8eec), WTCP(0x7aa144bc, 0x24afb9da), WTCP(0x7a92d329, 0x24dfdf20), WTCP(0x7a844eae, 0x250ffeb7),
  WTCP(0x7a75b74f, 0x25401896), WTCP(0x7a670d0d, 0x25702cb7), WTCP(0x7a584feb, 0x25a03b11), WTCP(0x7a497feb, 0x25d0439f),
  WTCP(0x7a3a9d0f, 0x26004657), WTCP(0x7a2ba75a, 0x26304333), WTCP(0x7a1c9ece, 0x26603a2c), WTCP(0x7a0d836d, 0x26902b39),
  WTCP(0x79fe5539, 0x26c01655), WTCP(0x79ef1436, 0x26effb76), WTCP(0x79dfc064, 0x271fda96), WTCP(0x79d059c8, 0x274fb3ae),
  WTCP(0x79c0e062, 0x277f86b5), WTCP(0x79b15435, 0x27af53a6), WTCP(0x79a1b545, 0x27df1a77), WTCP(0x79920392, 0x280edb23),
  WTCP(0x79823f20, 0x283e95a1), WTCP(0x797267f2, 0x286e49ea), WTCP(0x79627e08, 0x289df7f8), WTCP(0x79528167, 0x28cd9fc1),
  WTCP(0x79427210, 0x28fd4140), WTCP(0x79325006, 0x292cdc6d), WTCP(0x79221b4b, 0x295c7140), WTCP(0x7911d3e2, 0x298bffb2),
  WTCP(0x790179cd, 0x29bb87bc), WTCP(0x78f10d0f, 0x29eb0957), WTCP(0x78e08dab, 0x2a1a847b), WTCP(0x78cffba3, 0x2a49f920),
  WTCP(0x78bf56f9, 0x2a796740), WTCP(0x78ae9fb0, 0x2aa8ced3), WTCP(0x789dd5cb, 0x2ad82fd2), WTCP(0x788cf94c, 0x2b078a36),
  WTCP(0x787c0a36, 0x2b36ddf7), WTCP(0x786b088c, 0x2b662b0e), WTCP(0x7859f44f, 0x2b957173), WTCP(0x7848cd83, 0x2bc4b120),
  WTCP(0x7837942b, 0x2bf3ea0d), WTCP(0x78264849, 0x2c231c33), WTCP(0x7814e9df, 0x2c52478a), WTCP(0x780378f1, 0x2c816c0c),
  WTCP(0x77f1f581, 0x2cb089b1), WTCP(0x77e05f91, 0x2cdfa071), WTCP(0x77ceb725, 0x2d0eb046), WTCP(0x77bcfc3f, 0x2d3db928),
  WTCP(0x77ab2ee2, 0x2d6cbb10), WTCP(0x77994f11, 0x2d9bb5f6), WTCP(0x77875cce, 0x2dcaa9d5), WTCP(0x7775581d, 0x2df996a3),
  WTCP(0x776340ff, 0x2e287c5a), WTCP(0x77511778, 0x2e575af3), WTCP(0x773edb8b, 0x2e863267), WTCP(0x772c8d3a, 0x2eb502ae),
  WTCP(0x771a2c88, 0x2ee3cbc1), WTCP(0x7707b979, 0x2f128d99), WTCP(0x76f5340e, 0x2f41482e), WTCP(0x76e29c4b, 0x2f6ffb7a),
  WTCP(0x76cff232, 0x2f9ea775), WTCP(0x76bd35c7, 0x2fcd4c19), WTCP(0x76aa670d, 0x2ffbe95d), WTCP(0x76978605, 0x302a7f3a),
  WTCP(0x768492b4, 0x30590dab), WTCP(0x76718d1c, 0x308794a6), WTCP(0x765e7540, 0x30b61426), WTCP(0x764b4b23, 0x30e48c22),
  WTCP(0x76380ec8, 0x3112fc95), WTCP(0x7624c031, 0x31416576), WTCP(0x76115f63, 0x316fc6be), WTCP(0x75fdec60, 0x319e2067),
  WTCP(0x75ea672a, 0x31cc7269), WTCP(0x75d6cfc5, 0x31fabcbd), WTCP(0x75c32634, 0x3228ff5c), WTCP(0x75af6a7b, 0x32573a3f),
  WTCP(0x759b9c9b, 0x32856d5e), WTCP(0x7587bc98, 0x32b398b3), WTCP(0x7573ca75, 0x32e1bc36), WTCP(0x755fc635, 0x330fd7e1),
  WTCP(0x754bafdc, 0x333debab), WTCP(0x7537876c, 0x336bf78f), WTCP(0x75234ce8, 0x3399fb85), WTCP(0x750f0054, 0x33c7f785),
  WTCP(0x74faa1b3, 0x33f5eb89), WTCP(0x74e63108, 0x3423d78a), WTCP(0x74d1ae55, 0x3451bb81), WTCP(0x74bd199f, 0x347f9766),
  WTCP(0x74a872e8, 0x34ad6b32), WTCP(0x7493ba34, 0x34db36df), WTCP(0x747eef85, 0x3508fa66), WTCP(0x746a12df, 0x3536b5be),
  WTCP(0x74552446, 0x356468e2), WTCP(0x744023bc, 0x359213c9), WTCP(0x742b1144, 0x35bfb66e), WTCP(0x7415ece2, 0x35ed50c9),
  WTCP(0x7400b69a, 0x361ae2d3), WTCP(0x73eb6e6e, 0x36486c86), WTCP(0x73d61461, 0x3675edd9), WTCP(0x73c0a878, 0x36a366c6),
  WTCP(0x73ab2ab4, 0x36d0d746), WTCP(0x73959b1b, 0x36fe3f52), WTCP(0x737ff9ae, 0x372b9ee3), WTCP(0x736a4671, 0x3758f5f2),
  WTCP(0x73548168, 0x37864477), WTCP(0x733eaa96, 0x37b38a6d), WTCP(0x7328c1ff, 0x37e0c7cc), WTCP(0x7312c7a5, 0x380dfc8d),
  WTCP(0x72fcbb8c, 0x383b28a9), WTCP(0x72e69db7, 0x38684c19), WTCP(0x72d06e2b, 0x389566d6), WTCP(0x72ba2cea, 0x38c278d9),
  WTCP(0x72a3d9f7, 0x38ef821c), WTCP(0x728d7557, 0x391c8297), WTCP(0x7276ff0d, 0x39497a43), WTCP(0x7260771b, 0x39766919),
  WTCP(0x7249dd86, 0x39a34f13), WTCP(0x72333251, 0x39d02c2a), WTCP(0x721c7580, 0x39fd0056), WTCP(0x7205a716, 0x3a29cb91),
  WTCP(0x71eec716, 0x3a568dd4), WTCP(0x71d7d585, 0x3a834717), WTCP(0x71c0d265, 0x3aaff755), WTCP(0x71a9bdba, 0x3adc9e86),
  WTCP(0x71929789, 0x3b093ca3), WTCP(0x717b5fd3, 0x3b35d1a5), WTCP(0x7164169d, 0x3b625d86), WTCP(0x714cbbeb, 0x3b8ee03e),
  WTCP(0x71354fc0, 0x3bbb59c7), WTCP(0x711dd220, 0x3be7ca1a), WTCP(0x7106430e, 0x3c143130), WTCP(0x70eea28e, 0x3c408f03),
  WTCP(0x70d6f0a4, 0x3c6ce38a), WTCP(0x70bf2d53, 0x3c992ec0), WTCP(0x70a7589f, 0x3cc5709e), WTCP(0x708f728b, 0x3cf1a91c),
  WTCP(0x70777b1c, 0x3d1dd835), WTCP(0x705f7255, 0x3d49fde1), WTCP(0x70475839, 0x3d761a19), WTCP(0x702f2ccd, 0x3da22cd7),
  WTCP(0x7016f014, 0x3dce3614), WTCP(0x6ffea212, 0x3dfa35c8), WTCP(0x6fe642ca, 0x3e262bee), WTCP(0x6fcdd241, 0x3e52187f),
  WTCP(0x6fb5507a, 0x3e7dfb73), WTCP(0x6f9cbd79, 0x3ea9d4c3), WTCP(0x6f841942, 0x3ed5a46b), WTCP(0x6f6b63d8, 0x3f016a61),
  WTCP(0x6f529d40, 0x3f2d26a0), WTCP(0x6f39c57d, 0x3f58d921), WTCP(0x6f20dc92, 0x3f8481dd), WTCP(0x6f07e285, 0x3fb020ce),
  WTCP(0x6eeed758, 0x3fdbb5ec), WTCP(0x6ed5bb10, 0x40074132), WTCP(0x6ebc8db0, 0x4032c297), WTCP(0x6ea34f3d, 0x405e3a16),
  WTCP(0x6e89ffb9, 0x4089a7a8), WTCP(0x6e709f2a, 0x40b50b46), WTCP(0x6e572d93, 0x40e064ea), WTCP(0x6e3daaf8, 0x410bb48c),
  WTCP(0x6e24175c, 0x4136fa27), WTCP(0x6e0a72c5, 0x416235b2), WTCP(0x6df0bd35, 0x418d6729), WTCP(0x6dd6f6b1, 0x41b88e84),
  WTCP(0x6dbd1f3c, 0x41e3abbc), WTCP(0x6da336dc, 0x420ebecb), WTCP(0x6d893d93, 0x4239c7aa), WTCP(0x6d6f3365, 0x4264c653),
  WTCP(0x6d551858, 0x428fbabe), WTCP(0x6d3aec6e, 0x42baa4e6), WTCP(0x6d20afac, 0x42e584c3), WTCP(0x6d066215, 0x43105a50),
  WTCP(0x6cec03af, 0x433b2585), WTCP(0x6cd1947c, 0x4365e65b), WTCP(0x6cb71482, 0x43909ccd), WTCP(0x6c9c83c3, 0x43bb48d4),
  WTCP(0x6c81e245, 0x43e5ea68), WTCP(0x6c67300b, 0x44108184), WTCP(0x6c4c6d1a, 0x443b0e21), WTCP(0x6c319975, 0x44659039),
  WTCP(0x6c16b521, 0x449007c4), WTCP(0x6bfbc021, 0x44ba74bd), WTCP(0x6be0ba7b, 0x44e4d71c), WTCP(0x6bc5a431, 0x450f2edb),
  WTCP(0x6baa7d49, 0x45397bf4), WTCP(0x6b8f45c7, 0x4563be60), WTCP(0x6b73fdae, 0x458df619), WTCP(0x6b58a503, 0x45b82318),
  WTCP(0x6b3d3bcb, 0x45e24556), WTCP(0x6b21c208, 0x460c5cce), WTCP(0x6b0637c1, 0x46366978), WTCP(0x6aea9cf8, 0x46606b4e),
  WTCP(0x6acef1b2, 0x468a624a), WTCP(0x6ab335f4, 0x46b44e65), WTCP(0x6a9769c1, 0x46de2f99), WTCP(0x6a7b8d1e, 0x470805df),
  WTCP(0x6a5fa010, 0x4731d131), WTCP(0x6a43a29a, 0x475b9188), WTCP(0x6a2794c1, 0x478546de), WTCP(0x6a0b7689, 0x47aef12c),
  WTCP(0x69ef47f6, 0x47d8906d), WTCP(0x69d3090e, 0x48022499), WTCP(0x69b6b9d3, 0x482badab), WTCP(0x699a5a4c, 0x48552b9b),
  WTCP(0x697dea7b, 0x487e9e64), WTCP(0x69616a65, 0x48a805ff), WTCP(0x6944da10, 0x48d16265), WTCP(0x6928397e, 0x48fab391),
  WTCP(0x690b88b5, 0x4923f97b), WTCP(0x68eec7b9, 0x494d341e), WTCP(0x68d1f68f, 0x49766373), WTCP(0x68b5153a, 0x499f8774),
  WTCP(0x689823bf, 0x49c8a01b), WTCP(0x687b2224, 0x49f1ad61), WTCP(0x685e106c, 0x4a1aaf3f), WTCP(0x6840ee9b, 0x4a43a5b0),
  WTCP(0x6823bcb7, 0x4a6c90ad), WTCP(0x68067ac3, 0x4a957030), WTCP(0x67e928c5, 0x4abe4433), WTCP(0x67cbc6c0, 0x4ae70caf),
  WTCP(0x67ae54ba, 0x4b0fc99d), WTCP(0x6790d2b6, 0x4b387af9), WTCP(0x677340ba, 0x4b6120bb), WTCP(0x67559eca, 0x4b89badd),
  WTCP(0x6737ecea, 0x4bb24958), WTCP(0x671a2b20, 0x4bdacc28), WTCP(0x66fc596f, 0x4c034345), WTCP(0x66de77dc, 0x4c2baea9),
  WTCP(0x66c0866d, 0x4c540e4e), WTCP(0x66a28524, 0x4c7c622d), WTCP(0x66847408, 0x4ca4aa41), WTCP(0x6666531d, 0x4ccce684),
  WTCP(0x66482267, 0x4cf516ee), WTCP(0x6629e1ec, 0x4d1d3b7a), WTCP(0x660b91af, 0x4d455422), WTCP(0x65ed31b5, 0x4d6d60df),
  WTCP(0x65cec204, 0x4d9561ac), WTCP(0x65b0429f, 0x4dbd5682), WTCP(0x6591b38c, 0x4de53f5a), WTCP(0x657314cf, 0x4e0d1c30),
  WTCP(0x6554666d, 0x4e34ecfc), WTCP(0x6535a86b, 0x4e5cb1b9), WTCP(0x6516dacd, 0x4e846a60), WTCP(0x64f7fd98, 0x4eac16eb),
  WTCP(0x64d910d1, 0x4ed3b755), WTCP(0x64ba147d, 0x4efb4b96), WTCP(0x649b08a0, 0x4f22d3aa), WTCP(0x647bed3f, 0x4f4a4f89),
  WTCP(0x645cc260, 0x4f71bf2e), WTCP(0x643d8806, 0x4f992293), WTCP(0x641e3e38, 0x4fc079b1), WTCP(0x63fee4f8, 0x4fe7c483),
  WTCP(0x63df7c4d, 0x500f0302), WTCP(0x63c0043b, 0x50363529), WTCP(0x63a07cc7, 0x505d5af1), WTCP(0x6380e5f6, 0x50847454),
  WTCP(0x63613fcd, 0x50ab814d), WTCP(0x63418a50, 0x50d281d5), WTCP(0x6321c585, 0x50f975e6), WTCP(0x6301f171, 0x51205d7b),
  WTCP(0x62e20e17, 0x5147388c), WTCP(0x62c21b7e, 0x516e0715), WTCP(0x62a219aa, 0x5194c910), WTCP(0x628208a1, 0x51bb7e75),
  WTCP(0x6261e866, 0x51e22740), WTCP(0x6241b8ff, 0x5208c36a), WTCP(0x62217a72, 0x522f52ee), WTCP(0x62012cc2, 0x5255d5c5),
  WTCP(0x61e0cff5, 0x527c4bea), WTCP(0x61c06410, 0x52a2b556), WTCP(0x619fe918, 0x52c91204), WTCP(0x617f5f12, 0x52ef61ee),
  WTCP(0x615ec603, 0x5315a50e), WTCP(0x613e1df0, 0x533bdb5d), WTCP(0x611d66de, 0x536204d7), WTCP(0x60fca0d2, 0x53882175),
  WTCP(0x60dbcbd1, 0x53ae3131), WTCP(0x60bae7e1, 0x53d43406), WTCP(0x6099f505, 0x53fa29ed), WTCP(0x6078f344, 0x542012e1),
  WTCP(0x6057e2a2, 0x5445eedb), WTCP(0x6036c325, 0x546bbdd7), WTCP(0x601594d1, 0x54917fce), WTCP(0x5ff457ad, 0x54b734ba),
  WTCP(0x5fd30bbc, 0x54dcdc96), WTCP(0x5fb1b104, 0x5502775c), WTCP(0x5f90478a, 0x55280505), WTCP(0x5f6ecf53, 0x554d858d),
  WTCP(0x5f4d4865, 0x5572f8ed), WTCP(0x5f2bb2c5, 0x55985f20), WTCP(0x5f0a0e77, 0x55bdb81f), WTCP(0x5ee85b82, 0x55e303e6),
  WTCP(0x5ec699e9, 0x5608426e), WTCP(0x5ea4c9b3, 0x562d73b2), WTCP(0x5e82eae5, 0x565297ab), WTCP(0x5e60fd84, 0x5677ae54),
  WTCP(0x5e3f0194, 0x569cb7a8), WTCP(0x5e1cf71c, 0x56c1b3a1), WTCP(0x5dfade20, 0x56e6a239), WTCP(0x5dd8b6a7, 0x570b8369),
  WTCP(0x5db680b4, 0x5730572e), WTCP(0x5d943c4e, 0x57551d80), WTCP(0x5d71e979, 0x5779d65b), WTCP(0x5d4f883b, 0x579e81b8),
  WTCP(0x5d2d189a, 0x57c31f92), WTCP(0x5d0a9a9a, 0x57e7afe4), WTCP(0x5ce80e41, 0x580c32a7), WTCP(0x5cc57394, 0x5830a7d6),
  WTCP(0x5ca2ca99, 0x58550f6c), WTCP(0x5c801354, 0x58796962), WTCP(0x5c5d4dcc, 0x589db5b3), WTCP(0x5c3a7a05, 0x58c1f45b),
  WTCP(0x5c179806, 0x58e62552), WTCP(0x5bf4a7d2, 0x590a4893), WTCP(0x5bd1a971, 0x592e5e19), WTCP(0x5bae9ce7, 0x595265df),
  WTCP(0x5b8b8239, 0x59765fde), WTCP(0x5b68596d, 0x599a4c12), WTCP(0x5b452288, 0x59be2a74), WTCP(0x5b21dd90, 0x59e1faff),
  WTCP(0x5afe8a8b, 0x5a05bdae), WTCP(0x5adb297d, 0x5a29727b), WTCP(0x5ab7ba6c, 0x5a4d1960), WTCP(0x5a943d5e, 0x5a70b258),
};

RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_WTP KBDWindow1024[] =
{
  WTCP(0x7fffffa4, 0x0009962f), WTCP(0x7fffff39, 0x000e16fb), WTCP(0x7ffffebf, 0x0011ea65), WTCP(0x7ffffe34, 0x0015750e),
  WTCP(0x7ffffd96, 0x0018dc74), WTCP(0x7ffffce5, 0x001c332e), WTCP(0x7ffffc1f, 0x001f83f5), WTCP(0x7ffffb43, 0x0022d59a),
  WTCP(0x7ffffa4f, 0x00262cc2), WTCP(0x7ffff942, 0x00298cc4), WTCP(0x7ffff81a, 0x002cf81f), WTCP(0x7ffff6d6, 0x003070c4),
  WTCP(0x7ffff573, 0x0033f840), WTCP(0x7ffff3f1, 0x00378fd9), WTCP(0x7ffff24d, 0x003b38a1), WTCP(0x7ffff085, 0x003ef381),
  WTCP(0x7fffee98, 0x0042c147), WTCP(0x7fffec83, 0x0046a2a8), WTCP(0x7fffea44, 0x004a9847), WTCP(0x7fffe7d8, 0x004ea2b7),
  WTCP(0x7fffe53f, 0x0052c283), WTCP(0x7fffe274, 0x0056f829), WTCP(0x7fffdf76, 0x005b4422), WTCP(0x7fffdc43, 0x005fa6dd),
  WTCP(0x7fffd8d6, 0x006420c8), WTCP(0x7fffd52f, 0x0068b249), WTCP(0x7fffd149, 0x006d5bc4), WTCP(0x7fffcd22, 0x00721d9a),
  WTCP(0x7fffc8b6, 0x0076f828), WTCP(0x7fffc404, 0x007bebca), WTCP(0x7fffbf06, 0x0080f8d9), WTCP(0x7fffb9bb, 0x00861fae),
  WTCP(0x7fffb41e, 0x008b609e), WTCP(0x7fffae2c, 0x0090bbff), WTCP(0x7fffa7e1, 0x00963224), WTCP(0x7fffa13a, 0x009bc362),
  WTCP(0x7fff9a32, 0x00a17009), WTCP(0x7fff92c5, 0x00a7386c), WTCP(0x7fff8af0, 0x00ad1cdc), WTCP(0x7fff82ad, 0x00b31da8),
  WTCP(0x7fff79f9, 0x00b93b21), WTCP(0x7fff70cf, 0x00bf7596), WTCP(0x7fff672a, 0x00c5cd57), WTCP(0x7fff5d05, 0x00cc42b1),
  WTCP(0x7fff525c, 0x00d2d5f3), WTCP(0x7fff4729, 0x00d9876c), WTCP(0x7fff3b66, 0x00e05769), WTCP(0x7fff2f10, 0x00e74638),
  WTCP(0x7fff221f, 0x00ee5426), WTCP(0x7fff148e, 0x00f58182), WTCP(0x7fff0658, 0x00fcce97), WTCP(0x7ffef776, 0x01043bb3),
  WTCP(0x7ffee7e2, 0x010bc923), WTCP(0x7ffed795, 0x01137733), WTCP(0x7ffec68a, 0x011b4631), WTCP(0x7ffeb4ba, 0x01233669),
  WTCP(0x7ffea21d, 0x012b4827), WTCP(0x7ffe8eac, 0x01337bb8), WTCP(0x7ffe7a61, 0x013bd167), WTCP(0x7ffe6533, 0x01444982),
  WTCP(0x7ffe4f1c, 0x014ce454), WTCP(0x7ffe3813, 0x0155a229), WTCP(0x7ffe2011, 0x015e834d), WTCP(0x7ffe070d, 0x0167880c),
  WTCP(0x7ffdecff, 0x0170b0b2), WTCP(0x7ffdd1df, 0x0179fd8b), WTCP(0x7ffdb5a2, 0x01836ee1), WTCP(0x7ffd9842, 0x018d0500),
  WTCP(0x7ffd79b3, 0x0196c035), WTCP(0x7ffd59ee, 0x01a0a0ca), WTCP(0x7ffd38e8, 0x01aaa70a), WTCP(0x7ffd1697, 0x01b4d341),
  WTCP(0x7ffcf2f2, 0x01bf25b9), WTCP(0x7ffccdee, 0x01c99ebd), WTCP(0x7ffca780, 0x01d43e99), WTCP(0x7ffc7f9e, 0x01df0597),
  WTCP(0x7ffc563d, 0x01e9f401), WTCP(0x7ffc2b51, 0x01f50a22), WTCP(0x7ffbfecf, 0x02004844), WTCP(0x7ffbd0ab, 0x020baeb1),
  WTCP(0x7ffba0da, 0x02173db4), WTCP(0x7ffb6f4f, 0x0222f596), WTCP(0x7ffb3bfd, 0x022ed6a1), WTCP(0x7ffb06d8, 0x023ae11f),
  WTCP(0x7ffacfd3, 0x02471558), WTCP(0x7ffa96e0, 0x02537397), WTCP(0x7ffa5bf2, 0x025ffc25), WTCP(0x7ffa1efc, 0x026caf4a),
  WTCP(0x7ff9dfee, 0x02798d4f), WTCP(0x7ff99ebb, 0x0286967c), WTCP(0x7ff95b55, 0x0293cb1b), WTCP(0x7ff915ab, 0x02a12b72),
  WTCP(0x7ff8cdaf, 0x02aeb7cb), WTCP(0x7ff88351, 0x02bc706d), WTCP(0x7ff83682, 0x02ca559f), WTCP(0x7ff7e731, 0x02d867a9),
  WTCP(0x7ff7954e, 0x02e6a6d2), WTCP(0x7ff740c8, 0x02f51361), WTCP(0x7ff6e98e, 0x0303ad9c), WTCP(0x7ff68f8f, 0x031275ca),
  WTCP(0x7ff632ba, 0x03216c30), WTCP(0x7ff5d2fb, 0x03309116), WTCP(0x7ff57042, 0x033fe4bf), WTCP(0x7ff50a7a, 0x034f6773),
  WTCP(0x7ff4a192, 0x035f1975), WTCP(0x7ff43576, 0x036efb0a), WTCP(0x7ff3c612, 0x037f0c78), WTCP(0x7ff35353, 0x038f4e02),
  WTCP(0x7ff2dd24, 0x039fbfeb), WTCP(0x7ff26370, 0x03b06279), WTCP(0x7ff1e623, 0x03c135ed), WTCP(0x7ff16527, 0x03d23a8b),
  WTCP(0x7ff0e067, 0x03e37095), WTCP(0x7ff057cc, 0x03f4d84e), WTCP(0x7fefcb40, 0x040671f7), WTCP(0x7fef3aad, 0x04183dd3),
  WTCP(0x7feea5fa, 0x042a3c22), WTCP(0x7fee0d11, 0x043c6d25), WTCP(0x7fed6fda, 0x044ed11d), WTCP(0x7fecce3d, 0x04616849),
  WTCP(0x7fec2821, 0x047432eb), WTCP(0x7feb7d6c, 0x04873140), WTCP(0x7feace07, 0x049a6388), WTCP(0x7fea19d6, 0x04adca01),
  WTCP(0x7fe960c0, 0x04c164ea), WTCP(0x7fe8a2aa, 0x04d53481), WTCP(0x7fe7df79, 0x04e93902), WTCP(0x7fe71712, 0x04fd72aa),
  WTCP(0x7fe6495a, 0x0511e1b6), WTCP(0x7fe57634, 0x05268663), WTCP(0x7fe49d83, 0x053b60eb), WTCP(0x7fe3bf2b, 0x05507189),
  WTCP(0x7fe2db0f, 0x0565b879), WTCP(0x7fe1f110, 0x057b35f4), WTCP(0x7fe10111, 0x0590ea35), WTCP(0x7fe00af3, 0x05a6d574),
  WTCP(0x7fdf0e97, 0x05bcf7ea), WTCP(0x7fde0bdd, 0x05d351cf), WTCP(0x7fdd02a6, 0x05e9e35c), WTCP(0x7fdbf2d2, 0x0600acc8),
  WTCP(0x7fdadc40, 0x0617ae48), WTCP(0x7fd9becf, 0x062ee814), WTCP(0x7fd89a5e, 0x06465a62), WTCP(0x7fd76eca, 0x065e0565),
  WTCP(0x7fd63bf1, 0x0675e954), WTCP(0x7fd501b0, 0x068e0662), WTCP(0x7fd3bfe4, 0x06a65cc3), WTCP(0x7fd2766a, 0x06beecaa),
  WTCP(0x7fd1251e, 0x06d7b648), WTCP(0x7fcfcbda, 0x06f0b9d1), WTCP(0x7fce6a7a, 0x0709f775), WTCP(0x7fcd00d8, 0x07236f65),
  WTCP(0x7fcb8ecf, 0x073d21d2), WTCP(0x7fca1439, 0x07570eea), WTCP(0x7fc890ed, 0x077136dd), WTCP(0x7fc704c7, 0x078b99da),
  WTCP(0x7fc56f9d, 0x07a6380d), WTCP(0x7fc3d147, 0x07c111a4), WTCP(0x7fc2299e, 0x07dc26cc), WTCP(0x7fc07878, 0x07f777b1),
  WTCP(0x7fbebdac, 0x0813047d), WTCP(0x7fbcf90f, 0x082ecd5b), WTCP(0x7fbb2a78, 0x084ad276), WTCP(0x7fb951bc, 0x086713f7),
  WTCP(0x7fb76eaf, 0x08839206), WTCP(0x7fb58126, 0x08a04ccb), WTCP(0x7fb388f4, 0x08bd446e), WTCP(0x7fb185ee, 0x08da7915),
  WTCP(0x7faf77e5, 0x08f7eae7), WTCP(0x7fad5ead, 0x09159a09), WTCP(0x7fab3a17, 0x0933869f), WTCP(0x7fa909f6, 0x0951b0cd),
  WTCP(0x7fa6ce1a, 0x097018b7), WTCP(0x7fa48653, 0x098ebe7f), WTCP(0x7fa23273, 0x09ada248), WTCP(0x7f9fd249, 0x09ccc431),
  WTCP(0x7f9d65a4, 0x09ec245b), WTCP(0x7f9aec53, 0x0a0bc2e7), WTCP(0x7f986625, 0x0a2b9ff3), WTCP(0x7f95d2e7, 0x0a4bbb9e),
  WTCP(0x7f933267, 0x0a6c1604), WTCP(0x7f908472, 0x0a8caf43), WTCP(0x7f8dc8d5, 0x0aad8776), WTCP(0x7f8aff5c, 0x0ace9eb9),
  WTCP(0x7f8827d3, 0x0aeff526), WTCP(0x7f854204, 0x0b118ad8), WTCP(0x7f824dbb, 0x0b335fe6), WTCP(0x7f7f4ac3, 0x0b557469),
  WTCP(0x7f7c38e4, 0x0b77c879), WTCP(0x7f7917e9, 0x0b9a5c2b), WTCP(0x7f75e79b, 0x0bbd2f97), WTCP(0x7f72a7c3, 0x0be042d0),
  WTCP(0x7f6f5828, 0x0c0395ec), WTCP(0x7f6bf892, 0x0c2728fd), WTCP(0x7f6888c9, 0x0c4afc16), WTCP(0x7f650894, 0x0c6f0f4a),
  WTCP(0x7f6177b9, 0x0c9362a8), WTCP(0x7f5dd5ff, 0x0cb7f642), WTCP(0x7f5a232a, 0x0cdcca26), WTCP(0x7f565f00, 0x0d01de63),
  WTCP(0x7f528947, 0x0d273307), WTCP(0x7f4ea1c2, 0x0d4cc81f), WTCP(0x7f4aa835, 0x0d729db7), WTCP(0x7f469c65, 0x0d98b3da),
  WTCP(0x7f427e13, 0x0dbf0a92), WTCP(0x7f3e4d04, 0x0de5a1e9), WTCP(0x7f3a08f9, 0x0e0c79e7), WTCP(0x7f35b1b4, 0x0e339295),
  WTCP(0x7f3146f8, 0x0e5aebfa), WTCP(0x7f2cc884, 0x0e82861a), WTCP(0x7f28361b, 0x0eaa60fd), WTCP(0x7f238f7c, 0x0ed27ca5),
  WTCP(0x7f1ed467, 0x0efad917), WTCP(0x7f1a049d, 0x0f237656), WTCP(0x7f151fdc, 0x0f4c5462), WTCP(0x7f1025e3, 0x0f75733d),
  WTCP(0x7f0b1672, 0x0f9ed2e6), WTCP(0x7f05f146, 0x0fc8735e), WTCP(0x7f00b61d, 0x0ff254a1), WTCP(0x7efb64b4, 0x101c76ae),
  WTCP(0x7ef5fcca, 0x1046d981), WTCP(0x7ef07e19, 0x10717d15), WTCP(0x7eeae860, 0x109c6165), WTCP(0x7ee53b5b, 0x10c7866a),
  WTCP(0x7edf76c4, 0x10f2ec1e), WTCP(0x7ed99a58, 0x111e9279), WTCP(0x7ed3a5d1, 0x114a7971), WTCP(0x7ecd98eb, 0x1176a0fc),
  WTCP(0x7ec77360, 0x11a30910), WTCP(0x7ec134eb, 0x11cfb1a1), WTCP(0x7ebadd44, 0x11fc9aa2), WTCP(0x7eb46c27, 0x1229c406),
  WTCP(0x7eade14c, 0x12572dbf), WTCP(0x7ea73c6c, 0x1284d7bc), WTCP(0x7ea07d41, 0x12b2c1ed), WTCP(0x7e99a382, 0x12e0ec42),
  WTCP(0x7e92aee7, 0x130f56a8), WTCP(0x7e8b9f2a, 0x133e010b), WTCP(0x7e847402, 0x136ceb59), WTCP(0x7e7d2d25, 0x139c157b),
  WTCP(0x7e75ca4c, 0x13cb7f5d), WTCP(0x7e6e4b2d, 0x13fb28e6), WTCP(0x7e66af7f, 0x142b1200), WTCP(0x7e5ef6f8, 0x145b3a92),
  WTCP(0x7e572150, 0x148ba281), WTCP(0x7e4f2e3b, 0x14bc49b4), WTCP(0x7e471d70, 0x14ed300f), WTCP(0x7e3eeea5, 0x151e5575),
  WTCP(0x7e36a18e, 0x154fb9c9), WTCP(0x7e2e35e2, 0x15815ced), WTCP(0x7e25ab56, 0x15b33ec1), WTCP(0x7e1d019e, 0x15e55f25),
  WTCP(0x7e14386e, 0x1617bdf9), WTCP(0x7e0b4f7d, 0x164a5b19), WTCP(0x7e02467e, 0x167d3662), WTCP(0x7df91d25, 0x16b04fb2),
  WTCP(0x7defd327, 0x16e3a6e2), WTCP(0x7de66837, 0x17173bce), WTCP(0x7ddcdc0a, 0x174b0e4d), WTCP(0x7dd32e53, 0x177f1e39),
  WTCP(0x7dc95ec6, 0x17b36b69), WTCP(0x7dbf6d17, 0x17e7f5b3), WTCP(0x7db558f9, 0x181cbcec), WTCP(0x7dab221f, 0x1851c0e9),
  WTCP(0x7da0c83c, 0x1887017d), WTCP(0x7d964b05, 0x18bc7e7c), WTCP(0x7d8baa2b, 0x18f237b6), WTCP(0x7d80e563, 0x19282cfd),
  WTCP(0x7d75fc5e, 0x195e5e20), WTCP(0x7d6aeed0, 0x1994caee), WTCP(0x7d5fbc6d, 0x19cb7335), WTCP(0x7d5464e6, 0x1a0256c2),
  WTCP(0x7d48e7ef, 0x1a397561), WTCP(0x7d3d453b, 0x1a70cede), WTCP(0x7d317c7c, 0x1aa86301), WTCP(0x7d258d65, 0x1ae03195),
  WTCP(0x7d1977aa, 0x1b183a63), WTCP(0x7d0d3afc, 0x1b507d30), WTCP(0x7d00d710, 0x1b88f9c5), WTCP(0x7cf44b97, 0x1bc1afe6),
  WTCP(0x7ce79846, 0x1bfa9f58), WTCP(0x7cdabcce, 0x1c33c7e0), WTCP(0x7ccdb8e4, 0x1c6d293f), WTCP(0x7cc08c39, 0x1ca6c337),
  WTCP(0x7cb33682, 0x1ce0958a), WTCP(0x7ca5b772, 0x1d1a9ff8), WTCP(0x7c980ebd, 0x1d54e240), WTCP(0x7c8a3c14, 0x1d8f5c21),
  WTCP(0x7c7c3f2e, 0x1dca0d56), WTCP(0x7c6e17bc, 0x1e04f59f), WTCP(0x7c5fc573, 0x1e4014b4), WTCP(0x7c514807, 0x1e7b6a53),
  WTCP(0x7c429f2c, 0x1eb6f633), WTCP(0x7c33ca96, 0x1ef2b80f), WTCP(0x7c24c9fa, 0x1f2eaf9e), WTCP(0x7c159d0d, 0x1f6adc98),
  WTCP(0x7c064383, 0x1fa73eb2), WTCP(0x7bf6bd11, 0x1fe3d5a3), WTCP(0x7be7096c, 0x2020a11e), WTCP(0x7bd7284a, 0x205da0d8),
  WTCP(0x7bc71960, 0x209ad483), WTCP(0x7bb6dc65, 0x20d83bd1), WTCP(0x7ba6710d, 0x2115d674), WTCP(0x7b95d710, 0x2153a41b),
  WTCP(0x7b850e24, 0x2191a476), WTCP(0x7b7415ff, 0x21cfd734), WTCP(0x7b62ee59, 0x220e3c02), WTCP(0x7b5196e9, 0x224cd28d),
  WTCP(0x7b400f67, 0x228b9a82), WTCP(0x7b2e578a, 0x22ca938a), WTCP(0x7b1c6f0b, 0x2309bd52), WTCP(0x7b0a55a1, 0x23491783),
  WTCP(0x7af80b07, 0x2388a1c4), WTCP(0x7ae58ef5, 0x23c85bbf), WTCP(0x7ad2e124, 0x2408451a), WTCP(0x7ac0014e, 0x24485d7c),
  WTCP(0x7aacef2e, 0x2488a48a), WTCP(0x7a99aa7e, 0x24c919e9), WTCP(0x7a8632f8, 0x2509bd3d), WTCP(0x7a728858, 0x254a8e29),
  WTCP(0x7a5eaa5a, 0x258b8c50), WTCP(0x7a4a98b9, 0x25ccb753), WTCP(0x7a365333, 0x260e0ed3), WTCP(0x7a21d983, 0x264f9271),
  WTCP(0x7a0d2b68, 0x269141cb), WTCP(0x79f8489e, 0x26d31c80), WTCP(0x79e330e4, 0x2715222f), WTCP(0x79cde3f8, 0x27575273),
  WTCP(0x79b8619a, 0x2799acea), WTCP(0x79a2a989, 0x27dc3130), WTCP(0x798cbb85, 0x281ededf), WTCP(0x7976974e, 0x2861b591),
  WTCP(0x79603ca5, 0x28a4b4e0), WTCP(0x7949ab4c, 0x28e7dc65), WTCP(0x7932e304, 0x292b2bb8), WTCP(0x791be390, 0x296ea270),
  WTCP(0x7904acb3, 0x29b24024), WTCP(0x78ed3e30, 0x29f6046b), WTCP(0x78d597cc, 0x2a39eed8), WTCP(0x78bdb94a, 0x2a7dff02),
  WTCP(0x78a5a270, 0x2ac2347c), WTCP(0x788d5304, 0x2b068eda), WTCP(0x7874cacb, 0x2b4b0dae), WTCP(0x785c098d, 0x2b8fb08a),
  WTCP(0x78430f11, 0x2bd47700), WTCP(0x7829db1f, 0x2c1960a1), WTCP(0x78106d7f, 0x2c5e6cfd), WTCP(0x77f6c5fb, 0x2ca39ba3),
  WTCP(0x77dce45c, 0x2ce8ec23), WTCP(0x77c2c86e, 0x2d2e5e0b), WTCP(0x77a871fa, 0x2d73f0e8), WTCP(0x778de0cd, 0x2db9a449),
  WTCP(0x777314b2, 0x2dff77b8), WTCP(0x77580d78, 0x2e456ac4), WTCP(0x773ccaeb, 0x2e8b7cf6), WTCP(0x77214cdb, 0x2ed1addb),
  WTCP(0x77059315, 0x2f17fcfb), WTCP(0x76e99d69, 0x2f5e69e2), WTCP(0x76cd6ba9, 0x2fa4f419), WTCP(0x76b0fda4, 0x2feb9b27),
  WTCP(0x7694532e, 0x30325e96), WTCP(0x76776c17, 0x30793dee), WTCP(0x765a4834, 0x30c038b5), WTCP(0x763ce759, 0x31074e72),
  WTCP(0x761f4959, 0x314e7eab), WTCP(0x76016e0b, 0x3195c8e6), WTCP(0x75e35545, 0x31dd2ca9), WTCP(0x75c4fedc, 0x3224a979),
  WTCP(0x75a66aab, 0x326c3ed8), WTCP(0x75879887, 0x32b3ec4d), WTCP(0x7568884b, 0x32fbb159), WTCP(0x754939d1, 0x33438d81),
  WTCP(0x7529acf4, 0x338b8045), WTCP(0x7509e18e, 0x33d3892a), WTCP(0x74e9d77d, 0x341ba7b1), WTCP(0x74c98e9e, 0x3463db5a),
  WTCP(0x74a906cd, 0x34ac23a7), WTCP(0x74883fec, 0x34f48019), WTCP(0x746739d8, 0x353cf02f), WTCP(0x7445f472, 0x3585736a),
  WTCP(0x74246f9c, 0x35ce0949), WTCP(0x7402ab37, 0x3616b14c), WTCP(0x73e0a727, 0x365f6af0), WTCP(0x73be6350, 0x36a835b5),
  WTCP(0x739bdf95, 0x36f11118), WTCP(0x73791bdd, 0x3739fc98), WTCP(0x7356180e, 0x3782f7b2), WTCP(0x7332d410, 0x37cc01e3),
  WTCP(0x730f4fc9, 0x38151aa8), WTCP(0x72eb8b24, 0x385e417e), WTCP(0x72c7860a, 0x38a775e1), WTCP(0x72a34066, 0x38f0b74d),
  WTCP(0x727eba24, 0x393a053e), WTCP(0x7259f331, 0x39835f30), WTCP(0x7234eb79, 0x39ccc49e), WTCP(0x720fa2eb, 0x3a163503),
  WTCP(0x71ea1977, 0x3a5fafda), WTCP(0x71c44f0c, 0x3aa9349e), WTCP(0x719e439d, 0x3af2c2ca), WTCP(0x7177f71a, 0x3b3c59d7),
  WTCP(0x71516978, 0x3b85f940), WTCP(0x712a9aaa, 0x3bcfa07e), WTCP(0x71038aa4, 0x3c194f0d), WTCP(0x70dc395e, 0x3c630464),
  WTCP(0x70b4a6cd, 0x3cacbfff), WTCP(0x708cd2e9, 0x3cf68155), WTCP(0x7064bdab, 0x3d4047e1), WTCP(0x703c670d, 0x3d8a131c),
  WTCP(0x7013cf0a, 0x3dd3e27e), WTCP(0x6feaf59c, 0x3e1db580), WTCP(0x6fc1dac1, 0x3e678b9b), WTCP(0x6f987e76, 0x3eb16449),
  WTCP(0x6f6ee0b9, 0x3efb3f01), WTCP(0x6f45018b, 0x3f451b3d), WTCP(0x6f1ae0eb, 0x3f8ef874), WTCP(0x6ef07edb, 0x3fd8d620),
  WTCP(0x6ec5db5d, 0x4022b3b9), WTCP(0x6e9af675, 0x406c90b7), WTCP(0x6e6fd027, 0x40b66c93), WTCP(0x6e446879, 0x410046c5),
  WTCP(0x6e18bf71, 0x414a1ec6), WTCP(0x6decd517, 0x4193f40d), WTCP(0x6dc0a972, 0x41ddc615), WTCP(0x6d943c8d, 0x42279455),
  WTCP(0x6d678e71, 0x42715e45), WTCP(0x6d3a9f2a, 0x42bb235f), WTCP(0x6d0d6ec5, 0x4304e31a), WTCP(0x6cdffd4f, 0x434e9cf1),
  WTCP(0x6cb24ad6, 0x4398505b), WTCP(0x6c84576b, 0x43e1fcd1), WTCP(0x6c56231c, 0x442ba1cd), WTCP(0x6c27adfd, 0x44753ec7),
  WTCP(0x6bf8f81e, 0x44bed33a), WTCP(0x6bca0195, 0x45085e9d), WTCP(0x6b9aca75, 0x4551e06b), WTCP(0x6b6b52d5, 0x459b581e),
  WTCP(0x6b3b9ac9, 0x45e4c52f), WTCP(0x6b0ba26b, 0x462e2717), WTCP(0x6adb69d3, 0x46777d52), WTCP(0x6aaaf11b, 0x46c0c75a),
  WTCP(0x6a7a385c, 0x470a04a9), WTCP(0x6a493fb3, 0x475334b9), WTCP(0x6a18073d, 0x479c5707), WTCP(0x69e68f17, 0x47e56b0c),
  WTCP(0x69b4d761, 0x482e7045), WTCP(0x6982e039, 0x4877662c), WTCP(0x6950a9c0, 0x48c04c3f), WTCP(0x691e341a, 0x490921f8),
  WTCP(0x68eb7f67, 0x4951e6d5), WTCP(0x68b88bcd, 0x499a9a51), WTCP(0x68855970, 0x49e33beb), WTCP(0x6851e875, 0x4a2bcb1f),
  WTCP(0x681e3905, 0x4a74476b), WTCP(0x67ea4b47, 0x4abcb04c), WTCP(0x67b61f63, 0x4b050541), WTCP(0x6781b585, 0x4b4d45c9),
  WTCP(0x674d0dd6, 0x4b957162), WTCP(0x67182883, 0x4bdd878c), WTCP(0x66e305b8, 0x4c2587c6), WTCP(0x66ada5a5, 0x4c6d7190),
  WTCP(0x66780878, 0x4cb5446a), WTCP(0x66422e60, 0x4cfcffd5), WTCP(0x660c1790, 0x4d44a353), WTCP(0x65d5c439, 0x4d8c2e64),
  WTCP(0x659f348e, 0x4dd3a08c), WTCP(0x656868c3, 0x4e1af94b), WTCP(0x6531610d, 0x4e623825), WTCP(0x64fa1da3, 0x4ea95c9d),
  WTCP(0x64c29ebb, 0x4ef06637), WTCP(0x648ae48d, 0x4f375477), WTCP(0x6452ef53, 0x4f7e26e1), WTCP(0x641abf46, 0x4fc4dcfb),
  WTCP(0x63e254a2, 0x500b7649), WTCP(0x63a9afa2, 0x5051f253), WTCP(0x6370d083, 0x5098509f), WTCP(0x6337b784, 0x50de90b3),
  WTCP(0x62fe64e3, 0x5124b218), WTCP(0x62c4d8e0, 0x516ab455), WTCP(0x628b13bc, 0x51b096f3), WTCP(0x625115b8, 0x51f6597b),
  WTCP(0x6216df18, 0x523bfb78), WTCP(0x61dc701f, 0x52817c72), WTCP(0x61a1c912, 0x52c6dbf5), WTCP(0x6166ea36, 0x530c198d),
  WTCP(0x612bd3d2, 0x535134c5), WTCP(0x60f0862d, 0x53962d2a), WTCP(0x60b50190, 0x53db024a), WTCP(0x60794644, 0x541fb3b1),
  WTCP(0x603d5494, 0x546440ef), WTCP(0x60012cca, 0x54a8a992), WTCP(0x5fc4cf33, 0x54eced2b), WTCP(0x5f883c1c, 0x55310b48),
  WTCP(0x5f4b73d2, 0x5575037c), WTCP(0x5f0e76a5, 0x55b8d558), WTCP(0x5ed144e5, 0x55fc806f), WTCP(0x5e93dee1, 0x56400452),
  WTCP(0x5e5644ec, 0x56836096), WTCP(0x5e187757, 0x56c694cf), WTCP(0x5dda7677, 0x5709a092), WTCP(0x5d9c429f, 0x574c8374),
  WTCP(0x5d5ddc24, 0x578f3d0d), WTCP(0x5d1f435d, 0x57d1ccf2), WTCP(0x5ce078a0, 0x581432bd), WTCP(0x5ca17c45, 0x58566e04),
  WTCP(0x5c624ea4, 0x58987e63), WTCP(0x5c22f016, 0x58da6372), WTCP(0x5be360f6, 0x591c1ccc), WTCP(0x5ba3a19f, 0x595daa0d),
  WTCP(0x5b63b26c, 0x599f0ad1), WTCP(0x5b2393ba, 0x59e03eb6), WTCP(0x5ae345e7, 0x5a214558), WTCP(0x5aa2c951, 0x5a621e56),
};

RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_WTP SineWindow128[] =
{
  WTCP(0x7fff6216, 0x00c90f88), WTCP(0x7ffa72d1, 0x025b26d7), WTCP(0x7ff09478, 0x03ed26e6), WTCP(0x7fe1c76b, 0x057f0035),
  WTCP(0x7fce0c3e, 0x0710a345), WTCP(0x7fb563b3, 0x08a2009a), WTCP(0x7f97cebd, 0x0a3308bd), WTCP(0x7f754e80, 0x0bc3ac35),
  WTCP(0x7f4de451, 0x0d53db92), WTCP(0x7f2191b4, 0x0ee38766), WTCP(0x7ef05860, 0x1072a048), WTCP(0x7eba3a39, 0x120116d5),
  WTCP(0x7e7f3957, 0x138edbb1), WTCP(0x7e3f57ff, 0x151bdf86), WTCP(0x7dfa98a8, 0x16a81305), WTCP(0x7db0fdf8, 0x183366e9),
  WTCP(0x7d628ac6, 0x19bdcbf3), WTCP(0x7d0f4218, 0x1b4732ef), WTCP(0x7cb72724, 0x1ccf8cb3), WTCP(0x7c5a3d50, 0x1e56ca1e),
  WTCP(0x7bf88830, 0x1fdcdc1b), WTCP(0x7b920b89, 0x2161b3a0), WTCP(0x7b26cb4f, 0x22e541af), WTCP(0x7ab6cba4, 0x24677758),
  WTCP(0x7a4210d8, 0x25e845b6), WTCP(0x79c89f6e, 0x27679df4), WTCP(0x794a7c12, 0x28e5714b), WTCP(0x78c7aba2, 0x2a61b101),
  WTCP(0x78403329, 0x2bdc4e6f), WTCP(0x77b417df, 0x2d553afc), WTCP(0x77235f2d, 0x2ecc681e), WTCP(0x768e0ea6, 0x3041c761),
  WTCP(0x75f42c0b, 0x31b54a5e), WTCP(0x7555bd4c, 0x3326e2c3), WTCP(0x74b2c884, 0x34968250), WTCP(0x740b53fb, 0x36041ad9),
  WTCP(0x735f6626, 0x376f9e46), WTCP(0x72af05a7, 0x38d8fe93), WTCP(0x71fa3949, 0x3a402dd2), WTCP(0x71410805, 0x3ba51e29),
  WTCP(0x708378ff, 0x3d07c1d6), WTCP(0x6fc19385, 0x3e680b2c), WTCP(0x6efb5f12, 0x3fc5ec98), WTCP(0x6e30e34a, 0x4121589b),
  WTCP(0x6d6227fa, 0x427a41d0), WTCP(0x6c8f351c, 0x43d09aed), WTCP(0x6bb812d1, 0x452456bd), WTCP(0x6adcc964, 0x46756828),
  WTCP(0x69fd614a, 0x47c3c22f), WTCP(0x6919e320, 0x490f57ee), WTCP(0x683257ab, 0x4a581c9e), WTCP(0x6746c7d8, 0x4b9e0390),
  WTCP(0x66573cbb, 0x4ce10034), WTCP(0x6563bf92, 0x4e210617), WTCP(0x646c59bf, 0x4f5e08e3), WTCP(0x637114cc, 0x5097fc5e),
  WTCP(0x6271fa69, 0x51ced46e), WTCP(0x616f146c, 0x53028518), WTCP(0x60686ccf, 0x5433027d), WTCP(0x5f5e0db3, 0x556040e2),
  WTCP(0x5e50015d, 0x568a34a9), WTCP(0x5d3e5237, 0x57b0d256), WTCP(0x5c290acc, 0x58d40e8c), WTCP(0x5b1035cf, 0x59f3de12),
};

RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_WTP KBDWindow128[] =
{
  WTCP(0x7ffffffe, 0x00016f63), WTCP(0x7ffffff1, 0x0003e382), WTCP(0x7fffffc7, 0x00078f64), WTCP(0x7fffff5d, 0x000cc323),
  WTCP(0x7ffffe76, 0x0013d9ed), WTCP(0x7ffffcaa, 0x001d3a9d), WTCP(0x7ffff953, 0x0029581f), WTCP(0x7ffff372, 0x0038b1bd),
  WTCP(0x7fffe98b, 0x004bd34d), WTCP(0x7fffd975, 0x00635538), WTCP(0x7fffc024, 0x007fdc64), WTCP(0x7fff995b, 0x00a219f1),
  WTCP(0x7fff5f5b, 0x00cacad0), WTCP(0x7fff0a75, 0x00fab72d), WTCP(0x7ffe9091, 0x0132b1af), WTCP(0x7ffde49e, 0x01739689),
  WTCP(0x7ffcf5ef, 0x01be4a63), WTCP(0x7ffbaf84, 0x0213b910), WTCP(0x7ff9f73a, 0x0274d41e), WTCP(0x7ff7acf1, 0x02e2913a),
  WTCP(0x7ff4a99a, 0x035de86c), WTCP(0x7ff0be3d, 0x03e7d233), WTCP(0x7febb2f1, 0x0481457c), WTCP(0x7fe545d4, 0x052b357c),
  WTCP(0x7fdd2a02, 0x05e68f77), WTCP(0x7fd30695, 0x06b4386f), WTCP(0x7fc675b4, 0x07950acb), WTCP(0x7fb703be, 0x0889d3ef),
  WTCP(0x7fa42e89, 0x099351e0), WTCP(0x7f8d64d8, 0x0ab230e0), WTCP(0x7f7205f8, 0x0be70923), WTCP(0x7f516195, 0x0d325c93),
  WTCP(0x7f2ab7d0, 0x0e9494ae), WTCP(0x7efd3997, 0x100e0085), WTCP(0x7ec8094a, 0x119ed2ef), WTCP(0x7e8a3ba7, 0x134720d8),
  WTCP(0x7e42d906, 0x1506dfdc), WTCP(0x7df0dee4, 0x16dde50b), WTCP(0x7d9341b4, 0x18cbe3f7), WTCP(0x7d28ef02, 0x1ad06e07),
  WTCP(0x7cb0cfcc, 0x1ceaf215), WTCP(0x7c29cb20, 0x1f1abc4f), WTCP(0x7b92c8eb, 0x215ef677), WTCP(0x7aeab4ec, 0x23b6a867),
  WTCP(0x7a3081d0, 0x2620b8ec), WTCP(0x79632c5a, 0x289beef5), WTCP(0x7881be95, 0x2b26f30b), WTCP(0x778b5304, 0x2dc0511f),
  WTCP(0x767f17c0, 0x30667aa2), WTCP(0x755c5178, 0x3317c8dd), WTCP(0x74225e50, 0x35d27f98), WTCP(0x72d0b887, 0x3894cff3),
  WTCP(0x7166f8e7, 0x3b5cdb7b), WTCP(0x6fe4d8e8, 0x3e28b770), WTCP(0x6e4a3491, 0x40f6702a), WTCP(0x6c970bfc, 0x43c40caa),
  WTCP(0x6acb8483, 0x468f9231), WTCP(0x68e7e994, 0x495707f5), WTCP(0x66ecad1c, 0x4c187ac7), WTCP(0x64da6797, 0x4ed200c5),
  WTCP(0x62b1d7b7, 0x5181bcea), WTCP(0x6073e1ae, 0x5425e28e), WTCP(0x5e218e16, 0x56bcb8c2), WTCP(0x5bbc0875, 0x59449d76),
};








RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_WTP SineWindow960[] =
{
  WTCP(0x7ffffd31, 0x001aceea), WTCP(0x7fffe6bc, 0x00506cb9), WTCP(0x7fffb9d1, 0x00860a79), WTCP(0x7fff7671, 0x00bba822),
  WTCP(0x7fff1c9b, 0x00f145ab), WTCP(0x7ffeac50, 0x0126e309), WTCP(0x7ffe2590, 0x015c8033), WTCP(0x7ffd885a, 0x01921d20),
  WTCP(0x7ffcd4b0, 0x01c7b9c6), WTCP(0x7ffc0a91, 0x01fd561d), WTCP(0x7ffb29fd, 0x0232f21a), WTCP(0x7ffa32f4, 0x02688db4),
  WTCP(0x7ff92577, 0x029e28e2), WTCP(0x7ff80186, 0x02d3c39b), WTCP(0x7ff6c720, 0x03095dd5), WTCP(0x7ff57647, 0x033ef786),
  WTCP(0x7ff40efa, 0x037490a5), WTCP(0x7ff2913a, 0x03aa292a), WTCP(0x7ff0fd07, 0x03dfc109), WTCP(0x7fef5260, 0x0415583b),
  WTCP(0x7fed9148, 0x044aeeb5), WTCP(0x7febb9bd, 0x0480846e), WTCP(0x7fe9cbc0, 0x04b6195d), WTCP(0x7fe7c752, 0x04ebad79),
  WTCP(0x7fe5ac72, 0x052140b7), WTCP(0x7fe37b22, 0x0556d30f), WTCP(0x7fe13361, 0x058c6478), WTCP(0x7fded530, 0x05c1f4e7),
  WTCP(0x7fdc608f, 0x05f78453), WTCP(0x7fd9d57f, 0x062d12b4), WTCP(0x7fd73401, 0x06629ffe), WTCP(0x7fd47c14, 0x06982c2b),
  WTCP(0x7fd1adb9, 0x06cdb72f), WTCP(0x7fcec8f1, 0x07034101), WTCP(0x7fcbcdbc, 0x0738c998), WTCP(0x7fc8bc1b, 0x076e50eb),
  WTCP(0x7fc5940e, 0x07a3d6f0), WTCP(0x7fc25596, 0x07d95b9e), WTCP(0x7fbf00b3, 0x080edeec), WTCP(0x7fbb9567, 0x084460cf),
  WTCP(0x7fb813b0, 0x0879e140), WTCP(0x7fb47b91, 0x08af6033), WTCP(0x7fb0cd0a, 0x08e4dda0), WTCP(0x7fad081b, 0x091a597e),
  WTCP(0x7fa92cc5, 0x094fd3c3), WTCP(0x7fa53b09, 0x09854c66), WTCP(0x7fa132e8, 0x09bac35d), WTCP(0x7f9d1461, 0x09f0389f),
  WTCP(0x7f98df77, 0x0a25ac23), WTCP(0x7f949429, 0x0a5b1dde), WTCP(0x7f903279, 0x0a908dc9), WTCP(0x7f8bba66, 0x0ac5fbd9),
  WTCP(0x7f872bf3, 0x0afb6805), WTCP(0x7f82871f, 0x0b30d244), WTCP(0x7f7dcbec, 0x0b663a8c), WTCP(0x7f78fa5b, 0x0b9ba0d5),
  WTCP(0x7f74126b, 0x0bd10513), WTCP(0x7f6f141f, 0x0c066740), WTCP(0x7f69ff76, 0x0c3bc74f), WTCP(0x7f64d473, 0x0c71253a),
  WTCP(0x7f5f9315, 0x0ca680f5), WTCP(0x7f5a3b5e, 0x0cdbda79), WTCP(0x7f54cd4f, 0x0d1131ba), WTCP(0x7f4f48e8, 0x0d4686b1),
  WTCP(0x7f49ae2a, 0x0d7bd954), WTCP(0x7f43fd18, 0x0db12999), WTCP(0x7f3e35b0, 0x0de67776), WTCP(0x7f3857f6, 0x0e1bc2e4),
  WTCP(0x7f3263e9, 0x0e510bd8), WTCP(0x7f2c598a, 0x0e865248), WTCP(0x7f2638db, 0x0ebb962c), WTCP(0x7f2001dd, 0x0ef0d77b),
  WTCP(0x7f19b491, 0x0f26162a), WTCP(0x7f1350f8, 0x0f5b5231), WTCP(0x7f0cd712, 0x0f908b86), WTCP(0x7f0646e2, 0x0fc5c220),
  WTCP(0x7effa069, 0x0ffaf5f6), WTCP(0x7ef8e3a6, 0x103026fe), WTCP(0x7ef2109d, 0x1065552e), WTCP(0x7eeb274d, 0x109a807e),
  WTCP(0x7ee427b9, 0x10cfa8e5), WTCP(0x7edd11e1, 0x1104ce58), WTCP(0x7ed5e5c6, 0x1139f0cf), WTCP(0x7ecea36b, 0x116f1040),
  WTCP(0x7ec74acf, 0x11a42ca2), WTCP(0x7ebfdbf5, 0x11d945eb), WTCP(0x7eb856de, 0x120e5c13), WTCP(0x7eb0bb8a, 0x12436f10),
  WTCP(0x7ea909fc, 0x12787ed8), WTCP(0x7ea14235, 0x12ad8b63), WTCP(0x7e996436, 0x12e294a7), WTCP(0x7e917000, 0x13179a9b),
  WTCP(0x7e896595, 0x134c9d34), WTCP(0x7e8144f6, 0x13819c6c), WTCP(0x7e790e25, 0x13b69836), WTCP(0x7e70c124, 0x13eb908c),
  WTCP(0x7e685df2, 0x14208563), WTCP(0x7e5fe493, 0x145576b1), WTCP(0x7e575508, 0x148a646e), WTCP(0x7e4eaf51, 0x14bf4e91),
  WTCP(0x7e45f371, 0x14f43510), WTCP(0x7e3d2169, 0x152917e1), WTCP(0x7e34393b, 0x155df6fc), WTCP(0x7e2b3ae8, 0x1592d257),
  WTCP(0x7e222672, 0x15c7a9ea), WTCP(0x7e18fbda, 0x15fc7daa), WTCP(0x7e0fbb22, 0x16314d8e), WTCP(0x7e06644c, 0x1666198d),
  WTCP(0x7dfcf759, 0x169ae19f), WTCP(0x7df3744b, 0x16cfa5b9), WTCP(0x7de9db23, 0x170465d2), WTCP(0x7de02be4, 0x173921e2),
  WTCP(0x7dd6668f, 0x176dd9de), WTCP(0x7dcc8b25, 0x17a28dbe), WTCP(0x7dc299a9, 0x17d73d79), WTCP(0x7db8921c, 0x180be904),
  WTCP(0x7dae747f, 0x18409058), WTCP(0x7da440d6, 0x1875336a), WTCP(0x7d99f721, 0x18a9d231), WTCP(0x7d8f9762, 0x18de6ca5),
  WTCP(0x7d85219c, 0x191302bc), WTCP(0x7d7a95cf, 0x1947946c), WTCP(0x7d6ff3fe, 0x197c21ad), WTCP(0x7d653c2b, 0x19b0aa75),
  WTCP(0x7d5a6e57, 0x19e52ebb), WTCP(0x7d4f8a85, 0x1a19ae76), WTCP(0x7d4490b6, 0x1a4e299d), WTCP(0x7d3980ec, 0x1a82a026),
  WTCP(0x7d2e5b2a, 0x1ab71208), WTCP(0x7d231f70, 0x1aeb7f3a), WTCP(0x7d17cdc2, 0x1b1fe7b3), WTCP(0x7d0c6621, 0x1b544b6a),
  WTCP(0x7d00e88f, 0x1b88aa55), WTCP(0x7cf5550e, 0x1bbd046c), WTCP(0x7ce9aba1, 0x1bf159a4), WTCP(0x7cddec48, 0x1c25a9f6),
  WTCP(0x7cd21707, 0x1c59f557), WTCP(0x7cc62bdf, 0x1c8e3bbe), WTCP(0x7cba2ad3, 0x1cc27d23), WTCP(0x7cae13e4, 0x1cf6b97c),
  WTCP(0x7ca1e715, 0x1d2af0c1), WTCP(0x7c95a467, 0x1d5f22e7), WTCP(0x7c894bde, 0x1d934fe5), WTCP(0x7c7cdd7b, 0x1dc777b3),
  WTCP(0x7c705940, 0x1dfb9a48), WTCP(0x7c63bf2f, 0x1e2fb79a), WTCP(0x7c570f4b, 0x1e63cfa0), WTCP(0x7c4a4996, 0x1e97e251),
  WTCP(0x7c3d6e13, 0x1ecbefa4), WTCP(0x7c307cc2, 0x1efff78f), WTCP(0x7c2375a8, 0x1f33fa0a), WTCP(0x7c1658c5, 0x1f67f70b),
  WTCP(0x7c09261d, 0x1f9bee8a), WTCP(0x7bfbddb1, 0x1fcfe07d), WTCP(0x7bee7f85, 0x2003ccdb), WTCP(0x7be10b99, 0x2037b39b),
  WTCP(0x7bd381f1, 0x206b94b4), WTCP(0x7bc5e290, 0x209f701c), WTCP(0x7bb82d76, 0x20d345cc), WTCP(0x7baa62a8, 0x210715b8),
  WTCP(0x7b9c8226, 0x213adfda), WTCP(0x7b8e8bf5, 0x216ea426), WTCP(0x7b808015, 0x21a26295), WTCP(0x7b725e8a, 0x21d61b1e),
  WTCP(0x7b642756, 0x2209cdb6), WTCP(0x7b55da7c, 0x223d7a55), WTCP(0x7b4777fe, 0x227120f3), WTCP(0x7b38ffde, 0x22a4c185),
  WTCP(0x7b2a721f, 0x22d85c04), WTCP(0x7b1bcec4, 0x230bf065), WTCP(0x7b0d15d0, 0x233f7ea0), WTCP(0x7afe4744, 0x237306ab),
  WTCP(0x7aef6323, 0x23a6887f), WTCP(0x7ae06971, 0x23da0411), WTCP(0x7ad15a2f, 0x240d7958), WTCP(0x7ac23561, 0x2440e84d),
  WTCP(0x7ab2fb09, 0x247450e4), WTCP(0x7aa3ab29, 0x24a7b317), WTCP(0x7a9445c5, 0x24db0edb), WTCP(0x7a84cade, 0x250e6427),
  WTCP(0x7a753a79, 0x2541b2f3), WTCP(0x7a659496, 0x2574fb36), WTCP(0x7a55d93a, 0x25a83ce6), WTCP(0x7a460867, 0x25db77fa),
  WTCP(0x7a362220, 0x260eac6a), WTCP(0x7a262668, 0x2641da2d), WTCP(0x7a161540, 0x26750139), WTCP(0x7a05eead, 0x26a82186),
  WTCP(0x79f5b2b1, 0x26db3b0a), WTCP(0x79e5614f, 0x270e4dbd), WTCP(0x79d4fa89, 0x27415996), WTCP(0x79c47e63, 0x27745e8c),
  WTCP(0x79b3ece0, 0x27a75c95), WTCP(0x79a34602, 0x27da53a9), WTCP(0x799289cc, 0x280d43bf), WTCP(0x7981b841, 0x28402cce),
  WTCP(0x7970d165, 0x28730ecd), WTCP(0x795fd53a, 0x28a5e9b4), WTCP(0x794ec3c3, 0x28d8bd78), WTCP(0x793d9d03, 0x290b8a12),
  WTCP(0x792c60fe, 0x293e4f78), WTCP(0x791b0fb5, 0x29710da1), WTCP(0x7909a92d, 0x29a3c485), WTCP(0x78f82d68, 0x29d6741b),
  WTCP(0x78e69c69, 0x2a091c59), WTCP(0x78d4f634, 0x2a3bbd37), WTCP(0x78c33acb, 0x2a6e56ac), WTCP(0x78b16a32, 0x2aa0e8b0),
  WTCP(0x789f846b, 0x2ad37338), WTCP(0x788d897b, 0x2b05f63d), WTCP(0x787b7963, 0x2b3871b5), WTCP(0x78695428, 0x2b6ae598),
  WTCP(0x785719cc, 0x2b9d51dd), WTCP(0x7844ca53, 0x2bcfb67b), WTCP(0x783265c0, 0x2c021369), WTCP(0x781fec15, 0x2c34689e),
  WTCP(0x780d5d57, 0x2c66b611), WTCP(0x77fab989, 0x2c98fbba), WTCP(0x77e800ad, 0x2ccb3990), WTCP(0x77d532c7, 0x2cfd6f8a),
  WTCP(0x77c24fdb, 0x2d2f9d9f), WTCP(0x77af57eb, 0x2d61c3c7), WTCP(0x779c4afc, 0x2d93e1f8), WTCP(0x77892910, 0x2dc5f829),
  WTCP(0x7775f22a, 0x2df80653), WTCP(0x7762a64f, 0x2e2a0c6c), WTCP(0x774f4581, 0x2e5c0a6b), WTCP(0x773bcfc4, 0x2e8e0048),
  WTCP(0x7728451c, 0x2ebfedfa), WTCP(0x7714a58b, 0x2ef1d377), WTCP(0x7700f115, 0x2f23b0b9), WTCP(0x76ed27be, 0x2f5585b5),
  WTCP(0x76d94989, 0x2f875262), WTCP(0x76c55679, 0x2fb916b9), WTCP(0x76b14e93, 0x2fead2b0), WTCP(0x769d31d9, 0x301c863f),
  WTCP(0x76890050, 0x304e315d), WTCP(0x7674b9fa, 0x307fd401), WTCP(0x76605edb, 0x30b16e23), WTCP(0x764beef8, 0x30e2ffb9),
  WTCP(0x76376a52, 0x311488bc), WTCP(0x7622d0ef, 0x31460922), WTCP(0x760e22d1, 0x317780e2), WTCP(0x75f95ffc, 0x31a8eff5),
  WTCP(0x75e48874, 0x31da5651), WTCP(0x75cf9c3d, 0x320bb3ee), WTCP(0x75ba9b5a, 0x323d08c3), WTCP(0x75a585cf, 0x326e54c7),
  WTCP(0x75905ba0, 0x329f97f3), WTCP(0x757b1ccf, 0x32d0d23c), WTCP(0x7565c962, 0x3302039b), WTCP(0x7550615c, 0x33332c06),
  WTCP(0x753ae4c0, 0x33644b76), WTCP(0x75255392, 0x339561e1), WTCP(0x750fadd7, 0x33c66f40), WTCP(0x74f9f391, 0x33f77388),
  WTCP(0x74e424c5, 0x34286eb3), WTCP(0x74ce4177, 0x345960b7), WTCP(0x74b849aa, 0x348a498b), WTCP(0x74a23d62, 0x34bb2927),
  WTCP(0x748c1ca4, 0x34ebff83), WTCP(0x7475e772, 0x351ccc96), WTCP(0x745f9dd1, 0x354d9057), WTCP(0x74493fc5, 0x357e4abe),
  WTCP(0x7432cd51, 0x35aefbc2), WTCP(0x741c467b, 0x35dfa35a), WTCP(0x7405ab45, 0x3610417f), WTCP(0x73eefbb3, 0x3640d627),
  WTCP(0x73d837ca, 0x3671614b), WTCP(0x73c15f8d, 0x36a1e2e0), WTCP(0x73aa7301, 0x36d25ae0), WTCP(0x7393722a, 0x3702c942),
  WTCP(0x737c5d0b, 0x37332dfd), WTCP(0x736533a9, 0x37638908), WTCP(0x734df607, 0x3793da5b), WTCP(0x7336a42b, 0x37c421ee),
  WTCP(0x731f3e17, 0x37f45fb7), WTCP(0x7307c3d0, 0x382493b0), WTCP(0x72f0355a, 0x3854bdcf), WTCP(0x72d892ba, 0x3884de0b),
  WTCP(0x72c0dbf3, 0x38b4f45d), WTCP(0x72a91109, 0x38e500bc), WTCP(0x72913201, 0x3915031f), WTCP(0x72793edf, 0x3944fb7e),
  WTCP(0x726137a8, 0x3974e9d0), WTCP(0x72491c5e, 0x39a4ce0e), WTCP(0x7230ed07, 0x39d4a82f), WTCP(0x7218a9a7, 0x3a04782a),
  WTCP(0x72005242, 0x3a343df7), WTCP(0x71e7e6dc, 0x3a63f98d), WTCP(0x71cf677a, 0x3a93aae5), WTCP(0x71b6d420, 0x3ac351f6),
  WTCP(0x719e2cd2, 0x3af2eeb7), WTCP(0x71857195, 0x3b228120), WTCP(0x716ca26c, 0x3b52092a), WTCP(0x7153bf5d, 0x3b8186ca),
  WTCP(0x713ac86b, 0x3bb0f9fa), WTCP(0x7121bd9c, 0x3be062b0), WTCP(0x71089ef2, 0x3c0fc0e6), WTCP(0x70ef6c74, 0x3c3f1491),
  WTCP(0x70d62625, 0x3c6e5daa), WTCP(0x70bccc09, 0x3c9d9c28), WTCP(0x70a35e25, 0x3cccd004), WTCP(0x7089dc7e, 0x3cfbf935),
  WTCP(0x70704718, 0x3d2b17b3), WTCP(0x70569df8, 0x3d5a2b75), WTCP(0x703ce122, 0x3d893474), WTCP(0x7023109a, 0x3db832a6),
  WTCP(0x70092c65, 0x3de72604), WTCP(0x6fef3488, 0x3e160e85), WTCP(0x6fd52907, 0x3e44ec22), WTCP(0x6fbb09e7, 0x3e73bed2),
  WTCP(0x6fa0d72c, 0x3ea2868c), WTCP(0x6f8690db, 0x3ed14349), WTCP(0x6f6c36f8, 0x3efff501), WTCP(0x6f51c989, 0x3f2e9bab),
  WTCP(0x6f374891, 0x3f5d373e), WTCP(0x6f1cb416, 0x3f8bc7b4), WTCP(0x6f020c1c, 0x3fba4d03), WTCP(0x6ee750a8, 0x3fe8c724),
  WTCP(0x6ecc81be, 0x4017360e), WTCP(0x6eb19f64, 0x404599b9), WTCP(0x6e96a99d, 0x4073f21d), WTCP(0x6e7ba06f, 0x40a23f32),
  WTCP(0x6e6083de, 0x40d080f0), WTCP(0x6e4553ef, 0x40feb74f), WTCP(0x6e2a10a8, 0x412ce246), WTCP(0x6e0eba0c, 0x415b01ce),
  WTCP(0x6df35020, 0x418915de), WTCP(0x6dd7d2ea, 0x41b71e6f), WTCP(0x6dbc426e, 0x41e51b77), WTCP(0x6da09eb1, 0x42130cf0),
  WTCP(0x6d84e7b7, 0x4240f2d1), WTCP(0x6d691d87, 0x426ecd12), WTCP(0x6d4d4023, 0x429c9bab), WTCP(0x6d314f93, 0x42ca5e94),
  WTCP(0x6d154bd9, 0x42f815c5), WTCP(0x6cf934fc, 0x4325c135), WTCP(0x6cdd0b00, 0x435360de), WTCP(0x6cc0cdea, 0x4380f4b7),
  WTCP(0x6ca47dbf, 0x43ae7cb7), WTCP(0x6c881a84, 0x43dbf8d7), WTCP(0x6c6ba43e, 0x44096910), WTCP(0x6c4f1af2, 0x4436cd58),
  WTCP(0x6c327ea6, 0x446425a8), WTCP(0x6c15cf5d, 0x449171f8), WTCP(0x6bf90d1d, 0x44beb240), WTCP(0x6bdc37eb, 0x44ebe679),
  WTCP(0x6bbf4fcd, 0x45190e99), WTCP(0x6ba254c7, 0x45462a9a), WTCP(0x6b8546de, 0x45733a73), WTCP(0x6b682617, 0x45a03e1d),
  WTCP(0x6b4af279, 0x45cd358f), WTCP(0x6b2dac06, 0x45fa20c2), WTCP(0x6b1052c6, 0x4626ffae), WTCP(0x6af2e6bc, 0x4653d24b),
  WTCP(0x6ad567ef, 0x46809891), WTCP(0x6ab7d663, 0x46ad5278), WTCP(0x6a9a321d, 0x46d9fff8), WTCP(0x6a7c7b23, 0x4706a10a),
  WTCP(0x6a5eb17a, 0x473335a5), WTCP(0x6a40d527, 0x475fbdc3), WTCP(0x6a22e630, 0x478c395a), WTCP(0x6a04e499, 0x47b8a864),
  WTCP(0x69e6d067, 0x47e50ad8), WTCP(0x69c8a9a1, 0x481160ae), WTCP(0x69aa704c, 0x483da9e0), WTCP(0x698c246c, 0x4869e665),
  WTCP(0x696dc607, 0x48961635), WTCP(0x694f5523, 0x48c23949), WTCP(0x6930d1c4, 0x48ee4f98), WTCP(0x69123bf1, 0x491a591c),
  WTCP(0x68f393ae, 0x494655cc), WTCP(0x68d4d900, 0x497245a1), WTCP(0x68b60bee, 0x499e2892), WTCP(0x68972c7d, 0x49c9fe99),
  WTCP(0x68783ab1, 0x49f5c7ae), WTCP(0x68593691, 0x4a2183c8), WTCP(0x683a2022, 0x4a4d32e1), WTCP(0x681af76a, 0x4a78d4f0),
  WTCP(0x67fbbc6d, 0x4aa469ee), WTCP(0x67dc6f31, 0x4acff1d3), WTCP(0x67bd0fbd, 0x4afb6c98), WTCP(0x679d9e14, 0x4b26da35),
  WTCP(0x677e1a3e, 0x4b523aa2), WTCP(0x675e843e, 0x4b7d8dd8), WTCP(0x673edc1c, 0x4ba8d3cf), WTCP(0x671f21dc, 0x4bd40c80),
  WTCP(0x66ff5584, 0x4bff37e2), WTCP(0x66df771a, 0x4c2a55ef), WTCP(0x66bf86a3, 0x4c55669f), WTCP(0x669f8425, 0x4c8069ea),
  WTCP(0x667f6fa5, 0x4cab5fc9), WTCP(0x665f4929, 0x4cd64834), WTCP(0x663f10b7, 0x4d012324), WTCP(0x661ec654, 0x4d2bf091),
  WTCP(0x65fe6a06, 0x4d56b073), WTCP(0x65ddfbd3, 0x4d8162c4), WTCP(0x65bd7bc0, 0x4dac077b), WTCP(0x659ce9d4, 0x4dd69e92),
  WTCP(0x657c4613, 0x4e012800), WTCP(0x655b9083, 0x4e2ba3be), WTCP(0x653ac92b, 0x4e5611c5), WTCP(0x6519f010, 0x4e80720e),
  WTCP(0x64f90538, 0x4eaac490), WTCP(0x64d808a8, 0x4ed50945), WTCP(0x64b6fa66, 0x4eff4025), WTCP(0x6495da79, 0x4f296928),
  WTCP(0x6474a8e5, 0x4f538448), WTCP(0x645365b2, 0x4f7d917c), WTCP(0x643210e4, 0x4fa790be), WTCP(0x6410aa81, 0x4fd18206),
  WTCP(0x63ef3290, 0x4ffb654d), WTCP(0x63cda916, 0x50253a8b), WTCP(0x63ac0e19, 0x504f01ba), WTCP(0x638a619e, 0x5078bad1),
  WTCP(0x6368a3ad, 0x50a265c9), WTCP(0x6346d44b, 0x50cc029c), WTCP(0x6324f37d, 0x50f59141), WTCP(0x6303014a, 0x511f11b2),
  WTCP(0x62e0fdb8, 0x514883e7), WTCP(0x62bee8cc, 0x5171e7d9), WTCP(0x629cc28c, 0x519b3d80), WTCP(0x627a8b00, 0x51c484d6),
  WTCP(0x6258422c, 0x51edbdd4), WTCP(0x6235e816, 0x5216e871), WTCP(0x62137cc5, 0x524004a7), WTCP(0x61f1003f, 0x5269126e),
  WTCP(0x61ce7289, 0x529211c0), WTCP(0x61abd3ab, 0x52bb0295), WTCP(0x618923a9, 0x52e3e4e6), WTCP(0x61666289, 0x530cb8ac),
  WTCP(0x61439053, 0x53357ddf), WTCP(0x6120ad0d, 0x535e3479), WTCP(0x60fdb8bb, 0x5386dc72), WTCP(0x60dab365, 0x53af75c3),
  WTCP(0x60b79d10, 0x53d80065), WTCP(0x609475c3, 0x54007c51), WTCP(0x60713d84, 0x5428e980), WTCP(0x604df459, 0x545147eb),
  WTCP(0x602a9a48, 0x5479978a), WTCP(0x60072f57, 0x54a1d857), WTCP(0x5fe3b38d, 0x54ca0a4b), WTCP(0x5fc026f0, 0x54f22d5d),
  WTCP(0x5f9c8987, 0x551a4189), WTCP(0x5f78db56, 0x554246c6), WTCP(0x5f551c65, 0x556a3d0d), WTCP(0x5f314cba, 0x55922457),
  WTCP(0x5f0d6c5b, 0x55b9fc9e), WTCP(0x5ee97b4f, 0x55e1c5da), WTCP(0x5ec5799b, 0x56098005), WTCP(0x5ea16747, 0x56312b17),
  WTCP(0x5e7d4458, 0x5658c709), WTCP(0x5e5910d4, 0x568053d5), WTCP(0x5e34ccc3, 0x56a7d174), WTCP(0x5e10782b, 0x56cf3fde),
  WTCP(0x5dec1311, 0x56f69f0d), WTCP(0x5dc79d7c, 0x571deefa), WTCP(0x5da31773, 0x57452f9d), WTCP(0x5d7e80fc, 0x576c60f1),
  WTCP(0x5d59da1e, 0x579382ee), WTCP(0x5d3522de, 0x57ba958d), WTCP(0x5d105b44, 0x57e198c7), WTCP(0x5ceb8355, 0x58088c96),
  WTCP(0x5cc69b19, 0x582f70f3), WTCP(0x5ca1a295, 0x585645d7), WTCP(0x5c7c99d1, 0x587d0b3b), WTCP(0x5c5780d3, 0x58a3c118),
  WTCP(0x5c3257a0, 0x58ca6767), WTCP(0x5c0d1e41, 0x58f0fe23), WTCP(0x5be7d4ba, 0x59178543), WTCP(0x5bc27b14, 0x593dfcc2),
  WTCP(0x5b9d1154, 0x59646498), WTCP(0x5b779780, 0x598abcbe), WTCP(0x5b520da1, 0x59b1052f), WTCP(0x5b2c73bb, 0x59d73de3),
  WTCP(0x5b06c9d6, 0x59fd66d4), WTCP(0x5ae10ff9, 0x5a237ffa), WTCP(0x5abb4629, 0x5a498950), WTCP(0x5a956c6e, 0x5a6f82ce),
};

RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_WTP KBDWindow960[] =
{
  WTCP(0x7fffff9e, 0x0009e6ac), WTCP(0x7fffff2b, 0x000e96d5), WTCP(0x7ffffea6, 0x0012987e), WTCP(0x7ffffe0e, 0x001652b6),
  WTCP(0x7ffffd60, 0x0019ebce), WTCP(0x7ffffc9c, 0x001d76bf), WTCP(0x7ffffbbf, 0x0020fe79), WTCP(0x7ffffac9, 0x002489ef),
  WTCP(0x7ffff9b7, 0x00281de2), WTCP(0x7ffff887, 0x002bbdbb), WTCP(0x7ffff737, 0x002f6c0d), WTCP(0x7ffff5c6, 0x00332ad8),
  WTCP(0x7ffff431, 0x0036fbb9), WTCP(0x7ffff276, 0x003ae004), WTCP(0x7ffff092, 0x003ed8d8), WTCP(0x7fffee84, 0x0042e72f),
  WTCP(0x7fffec48, 0x00470be3), WTCP(0x7fffe9dd, 0x004b47b8), WTCP(0x7fffe73f, 0x004f9b5f), WTCP(0x7fffe46b, 0x0054077a),
  WTCP(0x7fffe15f, 0x00588ca1), WTCP(0x7fffde17, 0x005d2b61), WTCP(0x7fffda91, 0x0061e442), WTCP(0x7fffd6c9, 0x0066b7c2),
  WTCP(0x7fffd2bb, 0x006ba65c), WTCP(0x7fffce65, 0x0070b087), WTCP(0x7fffc9c2, 0x0075d6b5), WTCP(0x7fffc4cf, 0x007b1955),
  WTCP(0x7fffbf87, 0x008078d5), WTCP(0x7fffb9e7, 0x0085f5a0), WTCP(0x7fffb3ea, 0x008b901d), WTCP(0x7fffad8c, 0x009148b4),
  WTCP(0x7fffa6c9, 0x00971fcb), WTCP(0x7fff9f9c, 0x009d15c7), WTCP(0x7fff9800, 0x00a32b0b), WTCP(0x7fff8ff0, 0x00a95ff9),
  WTCP(0x7fff8767, 0x00afb4f4), WTCP(0x7fff7e5f, 0x00b62a5c), WTCP(0x7fff74d4, 0x00bcc093), WTCP(0x7fff6ac0, 0x00c377f8),
  WTCP(0x7fff601c, 0x00ca50eb), WTCP(0x7fff54e3, 0x00d14bcb), WTCP(0x7fff490e, 0x00d868f7), WTCP(0x7fff3c98, 0x00dfa8ce),
  WTCP(0x7fff2f79, 0x00e70bad), WTCP(0x7fff21ac, 0x00ee91f3), WTCP(0x7fff1328, 0x00f63bfe), WTCP(0x7fff03e7, 0x00fe0a2c),
  WTCP(0x7ffef3e1, 0x0105fcd9), WTCP(0x7ffee310, 0x010e1462), WTCP(0x7ffed16a, 0x01165126), WTCP(0x7ffebee9, 0x011eb381),
  WTCP(0x7ffeab83, 0x01273bd0), WTCP(0x7ffe9731, 0x012fea6f), WTCP(0x7ffe81ea, 0x0138bfbc), WTCP(0x7ffe6ba4, 0x0141bc12),
  WTCP(0x7ffe5457, 0x014adfce), WTCP(0x7ffe3bfa, 0x01542b4d), WTCP(0x7ffe2282, 0x015d9ee9), WTCP(0x7ffe07e6, 0x01673b01),
  WTCP(0x7ffdec1b, 0x0170ffee), WTCP(0x7ffdcf17, 0x017aee0e), WTCP(0x7ffdb0d0, 0x018505bc), WTCP(0x7ffd913b, 0x018f4754),
  WTCP(0x7ffd704b, 0x0199b330), WTCP(0x7ffd4df7, 0x01a449ad), WTCP(0x7ffd2a31, 0x01af0b25), WTCP(0x7ffd04ef, 0x01b9f7f4),
  WTCP(0x7ffcde23, 0x01c51074), WTCP(0x7ffcb5c1, 0x01d05501), WTCP(0x7ffc8bbc, 0x01dbc5f5), WTCP(0x7ffc6006, 0x01e763ab),
  WTCP(0x7ffc3293, 0x01f32e7d), WTCP(0x7ffc0354, 0x01ff26c5), WTCP(0x7ffbd23b, 0x020b4cde), WTCP(0x7ffb9f3a, 0x0217a120),
  WTCP(0x7ffb6a41, 0x022423e6), WTCP(0x7ffb3342, 0x0230d58a), WTCP(0x7ffafa2d, 0x023db664), WTCP(0x7ffabef2, 0x024ac6ce),
  WTCP(0x7ffa8180, 0x02580720), WTCP(0x7ffa41c9, 0x026577b3), WTCP(0x7ff9ffb9, 0x027318e0), WTCP(0x7ff9bb41, 0x0280eaff),
  WTCP(0x7ff9744e, 0x028eee68), WTCP(0x7ff92acf, 0x029d2371), WTCP(0x7ff8deb1, 0x02ab8a74), WTCP(0x7ff88fe2, 0x02ba23c7),
  WTCP(0x7ff83e4d, 0x02c8efc0), WTCP(0x7ff7e9e1, 0x02d7eeb7), WTCP(0x7ff79288, 0x02e72101), WTCP(0x7ff7382f, 0x02f686f5),
  WTCP(0x7ff6dac1, 0x030620e9), WTCP(0x7ff67a29, 0x0315ef31), WTCP(0x7ff61651, 0x0325f224), WTCP(0x7ff5af23, 0x03362a14),
  WTCP(0x7ff5448a, 0x03469758), WTCP(0x7ff4d66d, 0x03573a42), WTCP(0x7ff464b7, 0x03681327), WTCP(0x7ff3ef4f, 0x0379225a),
  WTCP(0x7ff3761d, 0x038a682e), WTCP(0x7ff2f90a, 0x039be4f4), WTCP(0x7ff277fb, 0x03ad9900), WTCP(0x7ff1f2d8, 0x03bf84a3),
  WTCP(0x7ff16986, 0x03d1a82e), WTCP(0x7ff0dbec, 0x03e403f3), WTCP(0x7ff049ef, 0x03f69840), WTCP(0x7fefb373, 0x04096568),
  WTCP(0x7fef185d, 0x041c6bb8), WTCP(0x7fee7890, 0x042fab81), WTCP(0x7fedd3f1, 0x04432510), WTCP(0x7fed2a61, 0x0456d8b4),
  WTCP(0x7fec7bc4, 0x046ac6ba), WTCP(0x7febc7fb, 0x047eef70), WTCP(0x7feb0ee8, 0x04935322), WTCP(0x7fea506b, 0x04a7f21d),
  WTCP(0x7fe98c65, 0x04bcccab), WTCP(0x7fe8c2b7, 0x04d1e318), WTCP(0x7fe7f33e, 0x04e735af), WTCP(0x7fe71ddb, 0x04fcc4ba),
  WTCP(0x7fe6426c, 0x05129081), WTCP(0x7fe560ce, 0x0528994d), WTCP(0x7fe478df, 0x053edf68), WTCP(0x7fe38a7c, 0x05556318),
  WTCP(0x7fe29581, 0x056c24a5), WTCP(0x7fe199ca, 0x05832455), WTCP(0x7fe09733, 0x059a626e), WTCP(0x7fdf8d95, 0x05b1df35),
  WTCP(0x7fde7ccb, 0x05c99aef), WTCP(0x7fdd64af, 0x05e195e0), WTCP(0x7fdc451a, 0x05f9d04b), WTCP(0x7fdb1de4, 0x06124a73),
  WTCP(0x7fd9eee5, 0x062b0499), WTCP(0x7fd8b7f5, 0x0643ff00), WTCP(0x7fd778ec, 0x065d39e7), WTCP(0x7fd6319e, 0x0676b58f),
  WTCP(0x7fd4e1e2, 0x06907237), WTCP(0x7fd3898d, 0x06aa701d), WTCP(0x7fd22873, 0x06c4af80), WTCP(0x7fd0be6a, 0x06df309c),
  WTCP(0x7fcf4b44, 0x06f9f3ad), WTCP(0x7fcdced4, 0x0714f8f0), WTCP(0x7fcc48ed, 0x0730409f), WTCP(0x7fcab960, 0x074bcaf5),
  WTCP(0x7fc91fff, 0x0767982a), WTCP(0x7fc77c9a, 0x0783a877), WTCP(0x7fc5cf02, 0x079ffc14), WTCP(0x7fc41705, 0x07bc9338),
  WTCP(0x7fc25474, 0x07d96e19), WTCP(0x7fc0871b, 0x07f68ced), WTCP(0x7fbeaeca, 0x0813efe7), WTCP(0x7fbccb4c, 0x0831973d),
  WTCP(0x7fbadc70, 0x084f8320), WTCP(0x7fb8e200, 0x086db3c3), WTCP(0x7fb6dbc8, 0x088c2957), WTCP(0x7fb4c993, 0x08aae40c),
  WTCP(0x7fb2ab2b, 0x08c9e412), WTCP(0x7fb0805a, 0x08e92997), WTCP(0x7fae48e9, 0x0908b4c9), WTCP(0x7fac04a0, 0x092885d6),
  WTCP(0x7fa9b347, 0x09489ce8), WTCP(0x7fa754a6, 0x0968fa2c), WTCP(0x7fa4e884, 0x09899dcb), WTCP(0x7fa26ea6, 0x09aa87ee),
  WTCP(0x7f9fe6d1, 0x09cbb8be), WTCP(0x7f9d50cc, 0x09ed3062), WTCP(0x7f9aac5a, 0x0a0eef00), WTCP(0x7f97f93f, 0x0a30f4bf),
  WTCP(0x7f95373e, 0x0a5341c2), WTCP(0x7f92661b, 0x0a75d62e), WTCP(0x7f8f8596, 0x0a98b224), WTCP(0x7f8c9572, 0x0abbd5c7),
  WTCP(0x7f89956f, 0x0adf4137), WTCP(0x7f86854d, 0x0b02f494), WTCP(0x7f8364cd, 0x0b26effd), WTCP(0x7f8033ae, 0x0b4b338f),
  WTCP(0x7f7cf1ae, 0x0b6fbf67), WTCP(0x7f799e8b, 0x0b9493a0), WTCP(0x7f763a03, 0x0bb9b056), WTCP(0x7f72c3d2, 0x0bdf15a2),
  WTCP(0x7f6f3bb5, 0x0c04c39c), WTCP(0x7f6ba168, 0x0c2aba5d), WTCP(0x7f67f4a6, 0x0c50f9fa), WTCP(0x7f643529, 0x0c77828a),
  WTCP(0x7f6062ac, 0x0c9e5420), WTCP(0x7f5c7ce8, 0x0cc56ed1), WTCP(0x7f588397, 0x0cecd2ae), WTCP(0x7f547670, 0x0d147fc8),
  WTCP(0x7f50552c, 0x0d3c7630), WTCP(0x7f4c1f83, 0x0d64b5f6), WTCP(0x7f47d52a, 0x0d8d3f26), WTCP(0x7f4375d9, 0x0db611ce),
  WTCP(0x7f3f0144, 0x0ddf2dfa), WTCP(0x7f3a7723, 0x0e0893b4), WTCP(0x7f35d729, 0x0e324306), WTCP(0x7f31210a, 0x0e5c3bf9),
  WTCP(0x7f2c547b, 0x0e867e94), WTCP(0x7f27712e, 0x0eb10add), WTCP(0x7f2276d8, 0x0edbe0da), WTCP(0x7f1d6529, 0x0f07008e),
  WTCP(0x7f183bd3, 0x0f3269fc), WTCP(0x7f12fa89, 0x0f5e1d27), WTCP(0x7f0da0fb, 0x0f8a1a0e), WTCP(0x7f082ed8, 0x0fb660b1),
  WTCP(0x7f02a3d2, 0x0fe2f10f), WTCP(0x7efcff98, 0x100fcb25), WTCP(0x7ef741d9, 0x103ceeee), WTCP(0x7ef16a42, 0x106a5c66),
  WTCP(0x7eeb7884, 0x10981386), WTCP(0x7ee56c4a, 0x10c61447), WTCP(0x7edf4543, 0x10f45ea0), WTCP(0x7ed9031b, 0x1122f288),
  WTCP(0x7ed2a57f, 0x1151cff3), WTCP(0x7ecc2c1a, 0x1180f6d5), WTCP(0x7ec59699, 0x11b06720), WTCP(0x7ebee4a6, 0x11e020c8),
  WTCP(0x7eb815ed, 0x121023ba), WTCP(0x7eb12a18, 0x12406fe8), WTCP(0x7eaa20d1, 0x1271053e), WTCP(0x7ea2f9c2, 0x12a1e3a9),
  WTCP(0x7e9bb494, 0x12d30b15), WTCP(0x7e9450f0, 0x13047b6c), WTCP(0x7e8cce7f, 0x13363497), WTCP(0x7e852ce9, 0x1368367f),
  WTCP(0x7e7d6bd6, 0x139a8109), WTCP(0x7e758aee, 0x13cd141b), WTCP(0x7e6d89d9, 0x13ffef99), WTCP(0x7e65683d, 0x14331368),
  WTCP(0x7e5d25c1, 0x14667f67), WTCP(0x7e54c20b, 0x149a3379), WTCP(0x7e4c3cc3, 0x14ce2f7c), WTCP(0x7e43958e, 0x1502734f),
  WTCP(0x7e3acc11, 0x1536fece), WTCP(0x7e31dff2, 0x156bd1d6), WTCP(0x7e28d0d7, 0x15a0ec41), WTCP(0x7e1f9e63, 0x15d64de9),
  WTCP(0x7e16483d, 0x160bf6a5), WTCP(0x7e0cce08, 0x1641e64c), WTCP(0x7e032f6a, 0x16781cb4), WTCP(0x7df96c05, 0x16ae99b2),
  WTCP(0x7def837e, 0x16e55d18), WTCP(0x7de57579, 0x171c66ba), WTCP(0x7ddb419a, 0x1753b667), WTCP(0x7dd0e784, 0x178b4bef),
  WTCP(0x7dc666d9, 0x17c32721), WTCP(0x7dbbbf3e, 0x17fb47ca), WTCP(0x7db0f056, 0x1833adb5), WTCP(0x7da5f9c3, 0x186c58ae),
  WTCP(0x7d9adb29, 0x18a5487d), WTCP(0x7d8f9429, 0x18de7cec), WTCP(0x7d842467, 0x1917f5c1), WTCP(0x7d788b86, 0x1951b2c2),
  WTCP(0x7d6cc927, 0x198bb3b4), WTCP(0x7d60dced, 0x19c5f85a), WTCP(0x7d54c67c, 0x1a008077), WTCP(0x7d488574, 0x1a3b4bcb),
  WTCP(0x7d3c1979, 0x1a765a17), WTCP(0x7d2f822d, 0x1ab1ab18), WTCP(0x7d22bf32, 0x1aed3e8d), WTCP(0x7d15d02b, 0x1b291432),
  WTCP(0x7d08b4ba, 0x1b652bc1), WTCP(0x7cfb6c82, 0x1ba184f5), WTCP(0x7cedf725, 0x1bde1f86), WTCP(0x7ce05445, 0x1c1afb2c),
  WTCP(0x7cd28386, 0x1c58179c), WTCP(0x7cc48489, 0x1c95748d), WTCP(0x7cb656f3, 0x1cd311b1), WTCP(0x7ca7fa65, 0x1d10eebd),
  WTCP(0x7c996e83, 0x1d4f0b60), WTCP(0x7c8ab2f0, 0x1d8d674c), WTCP(0x7c7bc74f, 0x1dcc0230), WTCP(0x7c6cab44, 0x1e0adbbb),
  WTCP(0x7c5d5e71, 0x1e49f398), WTCP(0x7c4de07c, 0x1e894973), WTCP(0x7c3e3108, 0x1ec8dcf8), WTCP(0x7c2e4fb9, 0x1f08add0),
  WTCP(0x7c1e3c34, 0x1f48bba3), WTCP(0x7c0df61d, 0x1f890618), WTCP(0x7bfd7d18, 0x1fc98cd6), WTCP(0x7becd0cc, 0x200a4f80),
  WTCP(0x7bdbf0dd, 0x204b4dbc), WTCP(0x7bcadcf1, 0x208c872c), WTCP(0x7bb994ae, 0x20cdfb71), WTCP(0x7ba817b9, 0x210faa2c),
  WTCP(0x7b9665bb, 0x215192fc), WTCP(0x7b847e58, 0x2193b57f), WTCP(0x7b726139, 0x21d61153), WTCP(0x7b600e05, 0x2218a614),
  WTCP(0x7b4d8463, 0x225b735d), WTCP(0x7b3ac3fc, 0x229e78c7), WTCP(0x7b27cc79, 0x22e1b5eb), WTCP(0x7b149d82, 0x23252a62),
  WTCP(0x7b0136c1, 0x2368d5c2), WTCP(0x7aed97df, 0x23acb7a0), WTCP(0x7ad9c087, 0x23f0cf92), WTCP(0x7ac5b063, 0x24351d2a),
  WTCP(0x7ab1671e, 0x24799ffc), WTCP(0x7a9ce464, 0x24be5799), WTCP(0x7a8827e1, 0x25034391), WTCP(0x7a733142, 0x25486375),
  WTCP(0x7a5e0033, 0x258db6d2), WTCP(0x7a489461, 0x25d33d35), WTCP(0x7a32ed7c, 0x2618f62c), WTCP(0x7a1d0b31, 0x265ee143),
  WTCP(0x7a06ed2f, 0x26a4fe02), WTCP(0x79f09327, 0x26eb4bf5), WTCP(0x79d9fcc8, 0x2731caa3), WTCP(0x79c329c2, 0x27787995),
  WTCP(0x79ac19c9, 0x27bf5850), WTCP(0x7994cc8d, 0x2806665c), WTCP(0x797d41c1, 0x284da33c), WTCP(0x79657918, 0x28950e74),
  WTCP(0x794d7247, 0x28dca788), WTCP(0x79352d01, 0x29246dfa), WTCP(0x791ca8fc, 0x296c614a), WTCP(0x7903e5ee, 0x29b480f9),
  WTCP(0x78eae38d, 0x29fccc87), WTCP(0x78d1a191, 0x2a454372), WTCP(0x78b81fb1, 0x2a8de537), WTCP(0x789e5da6, 0x2ad6b155),
  WTCP(0x78845b29, 0x2b1fa745), WTCP(0x786a17f5, 0x2b68c684), WTCP(0x784f93c4, 0x2bb20e8c), WTCP(0x7834ce53, 0x2bfb7ed7),
  WTCP(0x7819c75c, 0x2c4516dc), WTCP(0x77fe7e9e, 0x2c8ed615), WTCP(0x77e2f3d7, 0x2cd8bbf7), WTCP(0x77c726c5, 0x2d22c7fa),
  WTCP(0x77ab1728, 0x2d6cf993), WTCP(0x778ec4c0, 0x2db75037), WTCP(0x77722f4e, 0x2e01cb59), WTCP(0x77555695, 0x2e4c6a6d),
  WTCP(0x77383a58, 0x2e972ce6), WTCP(0x771ada5a, 0x2ee21235), WTCP(0x76fd3660, 0x2f2d19cc), WTCP(0x76df4e30, 0x2f78431a),
  WTCP(0x76c12190, 0x2fc38d91), WTCP(0x76a2b047, 0x300ef89d), WTCP(0x7683fa1e, 0x305a83af), WTCP(0x7664fede, 0x30a62e34),
  WTCP(0x7645be51, 0x30f1f798), WTCP(0x76263842, 0x313ddf49), WTCP(0x76066c7e, 0x3189e4b1), WTCP(0x75e65ad1, 0x31d6073d),
  WTCP(0x75c60309, 0x32224657), WTCP(0x75a564f6, 0x326ea168), WTCP(0x75848067, 0x32bb17da), WTCP(0x7563552d, 0x3307a917),
  WTCP(0x7541e31a, 0x33545486), WTCP(0x75202a02, 0x33a1198e), WTCP(0x74fe29b8, 0x33edf798), WTCP(0x74dbe211, 0x343aee09),
  WTCP(0x74b952e3, 0x3487fc48), WTCP(0x74967c06, 0x34d521bb), WTCP(0x74735d51, 0x35225dc7), WTCP(0x744ff69f, 0x356fafcf),
  WTCP(0x742c47c9, 0x35bd173a), WTCP(0x740850ab, 0x360a9369), WTCP(0x73e41121, 0x365823c1), WTCP(0x73bf8909, 0x36a5c7a4),
  WTCP(0x739ab842, 0x36f37e75), WTCP(0x73759eab, 0x37414796), WTCP(0x73503c26, 0x378f2268), WTCP(0x732a9095, 0x37dd0e4c),
  WTCP(0x73049bda, 0x382b0aa4), WTCP(0x72de5ddb, 0x387916d0), WTCP(0x72b7d67d, 0x38c73230), WTCP(0x729105a6, 0x39155c24),
  WTCP(0x7269eb3f, 0x3963940c), WTCP(0x72428730, 0x39b1d946), WTCP(0x721ad964, 0x3a002b31), WTCP(0x71f2e1c5, 0x3a4e892c),
  WTCP(0x71caa042, 0x3a9cf296), WTCP(0x71a214c7, 0x3aeb66cc), WTCP(0x71793f43, 0x3b39e52c), WTCP(0x71501fa6, 0x3b886d14),
  WTCP(0x7126b5e3, 0x3bd6fde1), WTCP(0x70fd01eb, 0x3c2596f1), WTCP(0x70d303b2, 0x3c74379f), WTCP(0x70a8bb2e, 0x3cc2df49),
  WTCP(0x707e2855, 0x3d118d4c), WTCP(0x70534b1e, 0x3d604103), WTCP(0x70282381, 0x3daef9cc), WTCP(0x6ffcb17a, 0x3dfdb702),
  WTCP(0x6fd0f504, 0x3e4c7800), WTCP(0x6fa4ee1a, 0x3e9b3c25), WTCP(0x6f789cbb, 0x3eea02ca), WTCP(0x6f4c00e5, 0x3f38cb4b),
  WTCP(0x6f1f1a9a, 0x3f879505), WTCP(0x6ef1e9da, 0x3fd65f53), WTCP(0x6ec46ea9, 0x40252990), WTCP(0x6e96a90b, 0x4073f318),
  WTCP(0x6e689905, 0x40c2bb46), WTCP(0x6e3a3e9d, 0x41118176), WTCP(0x6e0b99dd, 0x41604504), WTCP(0x6ddcaacc, 0x41af054a),
  WTCP(0x6dad7177, 0x41fdc1a5), WTCP(0x6d7dede8, 0x424c7970), WTCP(0x6d4e202e, 0x429b2c06), WTCP(0x6d1e0855, 0x42e9d8c4),
  WTCP(0x6ceda66f, 0x43387f05), WTCP(0x6cbcfa8d, 0x43871e26), WTCP(0x6c8c04c0, 0x43d5b581), WTCP(0x6c5ac51d, 0x44244474),
  WTCP(0x6c293bb8, 0x4472ca5a), WTCP(0x6bf768a8, 0x44c14690), WTCP(0x6bc54c06, 0x450fb873), WTCP(0x6b92e5e9, 0x455e1f5f),
  WTCP(0x6b60366c, 0x45ac7ab2), WTCP(0x6b2d3dab, 0x45fac9c8), WTCP(0x6af9fbc2, 0x46490bff), WTCP(0x6ac670d1, 0x469740b5),
  WTCP(0x6a929cf6, 0x46e56747), WTCP(0x6a5e8053, 0x47337f13), WTCP(0x6a2a1b0a, 0x47818779), WTCP(0x69f56d3e, 0x47cf7fd6),
  WTCP(0x69c07715, 0x481d678a), WTCP(0x698b38b4, 0x486b3df3), WTCP(0x6955b243, 0x48b90272), WTCP(0x691fe3ec, 0x4906b466),
  WTCP(0x68e9cdd8, 0x49545330), WTCP(0x68b37033, 0x49a1de30), WTCP(0x687ccb29, 0x49ef54c8), WTCP(0x6845dee9, 0x4a3cb657),
  WTCP(0x680eaba3, 0x4a8a0242), WTCP(0x67d73187, 0x4ad737e9), WTCP(0x679f70c7, 0x4b2456af), WTCP(0x67676997, 0x4b715df7),
  WTCP(0x672f1c2b, 0x4bbe4d25), WTCP(0x66f688ba, 0x4c0b239c), WTCP(0x66bdaf7b, 0x4c57e0c2), WTCP(0x668490a6, 0x4ca483fa),
  WTCP(0x664b2c76, 0x4cf10cac), WTCP(0x66118326, 0x4d3d7a3b), WTCP(0x65d794f3, 0x4d89cc0f), WTCP(0x659d621a, 0x4dd6018f),
  WTCP(0x6562eada, 0x4e221a22), WTCP(0x65282f74, 0x4e6e1530), WTCP(0x64ed302b, 0x4eb9f222), WTCP(0x64b1ed40, 0x4f05b061),
  WTCP(0x647666f8, 0x4f514f57), WTCP(0x643a9d99, 0x4f9cce6f), WTCP(0x63fe916a, 0x4fe82d13), WTCP(0x63c242b2, 0x50336aaf),
  WTCP(0x6385b1bc, 0x507e86b0), WTCP(0x6348ded1, 0x50c98082), WTCP(0x630bca3f, 0x51145793), WTCP(0x62ce7451, 0x515f0b51),
  WTCP(0x6290dd57, 0x51a99b2b), WTCP(0x625305a0, 0x51f40692), WTCP(0x6214ed7d, 0x523e4cf5), WTCP(0x61d69541, 0x52886dc5),
  WTCP(0x6197fd3e, 0x52d26875), WTCP(0x615925c9, 0x531c3c77), WTCP(0x611a0f39, 0x5365e93e), WTCP(0x60dab9e3, 0x53af6e3e),
  WTCP(0x609b2621, 0x53f8caed), WTCP(0x605b544c, 0x5441fec0), WTCP(0x601b44bf, 0x548b092e), WTCP(0x5fdaf7d5, 0x54d3e9ae),
  WTCP(0x5f9a6deb, 0x551c9fb7), WTCP(0x5f59a761, 0x55652ac3), WTCP(0x5f18a494, 0x55ad8a4d), WTCP(0x5ed765e6, 0x55f5bdcd),
  WTCP(0x5e95ebb8, 0x563dc4c1), WTCP(0x5e54366d, 0x56859ea3), WTCP(0x5e12466a, 0x56cd4af3), WTCP(0x5dd01c13, 0x5714c92d),
  WTCP(0x5d8db7cf, 0x575c18d0), WTCP(0x5d4b1a05, 0x57a3395e), WTCP(0x5d08431e, 0x57ea2a56), WTCP(0x5cc53384, 0x5830eb3a),
  WTCP(0x5c81eba0, 0x58777b8e), WTCP(0x5c3e6bdf, 0x58bddad5), WTCP(0x5bfab4af, 0x59040893), WTCP(0x5bb6c67c, 0x594a044f),
  WTCP(0x5b72a1b6, 0x598fcd8e), WTCP(0x5b2e46ce, 0x59d563d9), WTCP(0x5ae9b634, 0x5a1ac6b8), WTCP(0x5aa4f05a, 0x5a5ff5b5),
};

RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_WTP SineWindow120[] =
{
  WTCP(0x7fff4c54, 0x00d676eb), WTCP(0x7ff9af04, 0x02835b5a), WTCP(0x7fee74a2, 0x0430238f), WTCP(0x7fdd9dad, 0x05dcbcbe),
  WTCP(0x7fc72ae2, 0x07891418), WTCP(0x7fab1d3d, 0x093516d4), WTCP(0x7f8975f9, 0x0ae0b22c), WTCP(0x7f62368f, 0x0c8bd35e),
  WTCP(0x7f3560b9, 0x0e3667ad), WTCP(0x7f02f66f, 0x0fe05c64), WTCP(0x7ecaf9e5, 0x11899ed3), WTCP(0x7e8d6d91, 0x13321c53),
  WTCP(0x7e4a5426, 0x14d9c245), WTCP(0x7e01b096, 0x16807e15), WTCP(0x7db3860f, 0x18263d36), WTCP(0x7d5fd801, 0x19caed29),
  WTCP(0x7d06aa16, 0x1b6e7b7a), WTCP(0x7ca80038, 0x1d10d5c2), WTCP(0x7c43de8e, 0x1eb1e9a7), WTCP(0x7bda497d, 0x2051a4dd),
  WTCP(0x7b6b45a5, 0x21eff528), WTCP(0x7af6d7e6, 0x238cc85d), WTCP(0x7a7d055b, 0x25280c5e), WTCP(0x79fdd35c, 0x26c1af22),
  WTCP(0x7979477d, 0x28599eb0), WTCP(0x78ef678f, 0x29efc925), WTCP(0x7860399e, 0x2b841caf), WTCP(0x77cbc3f2, 0x2d168792),
  WTCP(0x77320d0d, 0x2ea6f827), WTCP(0x76931bae, 0x30355cdd), WTCP(0x75eef6ce, 0x31c1a43b), WTCP(0x7545a5a0, 0x334bbcde),
  WTCP(0x74972f92, 0x34d3957e), WTCP(0x73e39c49, 0x36591cea), WTCP(0x732af3a7, 0x37dc420c), WTCP(0x726d3dc6, 0x395cf3e9),
  WTCP(0x71aa82f7, 0x3adb21a1), WTCP(0x70e2cbc6, 0x3c56ba70), WTCP(0x701620f5, 0x3dcfadb0), WTCP(0x6f448b7e, 0x3f45ead8),
  WTCP(0x6e6e1492, 0x40b9617d), WTCP(0x6d92c59b, 0x422a0154), WTCP(0x6cb2a837, 0x4397ba32), WTCP(0x6bcdc639, 0x45027c0c),
  WTCP(0x6ae429ae, 0x466a36f9), WTCP(0x69f5dcd3, 0x47cedb31), WTCP(0x6902ea1d, 0x4930590f), WTCP(0x680b5c33, 0x4a8ea111),
  WTCP(0x670f3df3, 0x4be9a3db), WTCP(0x660e9a6a, 0x4d415234), WTCP(0x65097cdb, 0x4e959d08), WTCP(0x63fff0ba, 0x4fe6756a),
  WTCP(0x62f201ac, 0x5133cc94), WTCP(0x61dfbb8a, 0x527d93e6), WTCP(0x60c92a5a, 0x53c3bcea), WTCP(0x5fae5a55, 0x55063951),
  WTCP(0x5e8f57e2, 0x5644faf4), WTCP(0x5d6c2f99, 0x577ff3da), WTCP(0x5c44ee40, 0x58b71632), WTCP(0x5b19a0c8, 0x59ea5454),
};

RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_WTP KBDWindow120[] =
{
  WTCP(0x7ffffffe, 0x00017b6f), WTCP(0x7fffffef, 0x00042d2f), WTCP(0x7fffffbb, 0x000849d0), WTCP(0x7fffff36, 0x000e3494),
  WTCP(0x7ffffe0c, 0x00165efd), WTCP(0x7ffffbac, 0x002149be), WTCP(0x7ffff72e, 0x002f854c), WTCP(0x7fffef24, 0x0041b235),
  WTCP(0x7fffe167, 0x0058814f), WTCP(0x7fffcacd, 0x0074b3af), WTCP(0x7fffa6d0, 0x00971a67), WTCP(0x7fff6f1e, 0x00c0960e),
  WTCP(0x7fff1b12, 0x00f21602), WTCP(0x7ffe9f0b, 0x012c9775), WTCP(0x7ffdebb2, 0x01712428), WTCP(0x7ffced1b, 0x01c0d0f7),
  WTCP(0x7ffb89c2, 0x021cbc12), WTCP(0x7ff9a17c, 0x02860b05), WTCP(0x7ff70c39, 0x02fde875), WTCP(0x7ff398bc, 0x038581b3),
  WTCP(0x7fef0b3b, 0x041e040c), WTCP(0x7fe91bf3, 0x04c899f4), WTCP(0x7fe175ba, 0x05866803), WTCP(0x7fd7b493, 0x065889d5),
  WTCP(0x7fcb6459, 0x07400ed4), WTCP(0x7fbbff82, 0x083df6e9), WTCP(0x7fa8ee09, 0x09532f37), WTCP(0x7f91849a, 0x0a808ed1),
  WTCP(0x7f7503f2, 0x0bc6d381), WTCP(0x7f52989a, 0x0d269eb0), WTCP(0x7f295af4, 0x0ea07270), WTCP(0x7ef84fb6, 0x1034aeb6),
  WTCP(0x7ebe68c5, 0x11e38ed2), WTCP(0x7e7a8686, 0x13ad2733), WTCP(0x7e2b79a3, 0x1591636d), WTCP(0x7dd0053c, 0x179004a7),
  WTCP(0x7d66e18b, 0x19a8a05f), WTCP(0x7ceebef0, 0x1bda9fa2), WTCP(0x7c664953, 0x1e253ea1), WTCP(0x7bcc2be8, 0x20878cce),
  WTCP(0x7b1f1526, 0x23006d5d), WTCP(0x7a5dbb01, 0x258e9848), WTCP(0x7986df3e, 0x28309bc6), WTCP(0x789953e0, 0x2ae4de3e),
  WTCP(0x7793ff88, 0x2da9a0a8), WTCP(0x7675e1cc, 0x307d0163), WTCP(0x753e1763, 0x335cff72), WTCP(0x73ebde10, 0x36477e1f),
  WTCP(0x727e984e, 0x393a48f1), WTCP(0x70f5d09b, 0x3c3317f9), WTCP(0x6f513c60, 0x3f2f945c), WTCP(0x6d90be61, 0x422d5d18),
  WTCP(0x6bb468b1, 0x452a0bf3), WTCP(0x69bc7e1e, 0x48233a81), WTCP(0x67a97317, 0x4b16873e), WTCP(0x657bedfa, 0x4e019a9d),
  WTCP(0x6334c6d2, 0x50e22c0b), WTCP(0x60d50689, 0x53b606cb), WTCP(0x5e5de588, 0x567b0ea7), WTCP(0x5bd0c9c6, 0x592f4460),
};

RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_WTP SineWindow512[] =
{
  WTCP(0x7ffff621, 0x003243f5), WTCP(0x7fffa72c, 0x0096cbc1), WTCP(0x7fff0943, 0x00fb5330), WTCP(0x7ffe1c65, 0x015fda03),
  WTCP(0x7ffce093, 0x01c45ffe), WTCP(0x7ffb55ce, 0x0228e4e2), WTCP(0x7ff97c18, 0x028d6870), WTCP(0x7ff75370, 0x02f1ea6c),
  WTCP(0x7ff4dbd9, 0x03566a96), WTCP(0x7ff21553, 0x03bae8b2), WTCP(0x7feeffe1, 0x041f6480), WTCP(0x7feb9b85, 0x0483ddc3),
  WTCP(0x7fe7e841, 0x04e8543e), WTCP(0x7fe3e616, 0x054cc7b1), WTCP(0x7fdf9508, 0x05b137df), WTCP(0x7fdaf519, 0x0615a48b),
  WTCP(0x7fd6064c, 0x067a0d76), WTCP(0x7fd0c8a3, 0x06de7262), WTCP(0x7fcb3c23, 0x0742d311), WTCP(0x7fc560cf, 0x07a72f45),
  WTCP(0x7fbf36aa, 0x080b86c2), WTCP(0x7fb8bdb8, 0x086fd947), WTCP(0x7fb1f5fc, 0x08d42699), WTCP(0x7faadf7c, 0x09386e78),
  WTCP(0x7fa37a3c, 0x099cb0a7), WTCP(0x7f9bc640, 0x0a00ece8), WTCP(0x7f93c38c, 0x0a6522fe), WTCP(0x7f8b7227, 0x0ac952aa),
  WTCP(0x7f82d214, 0x0b2d7baf), WTCP(0x7f79e35a, 0x0b919dcf), WTCP(0x7f70a5fe, 0x0bf5b8cb), WTCP(0x7f671a05, 0x0c59cc68),
  WTCP(0x7f5d3f75, 0x0cbdd865), WTCP(0x7f531655, 0x0d21dc87), WTCP(0x7f489eaa, 0x0d85d88f), WTCP(0x7f3dd87c, 0x0de9cc40),
  WTCP(0x7f32c3d1, 0x0e4db75b), WTCP(0x7f2760af, 0x0eb199a4), WTCP(0x7f1baf1e, 0x0f1572dc), WTCP(0x7f0faf25, 0x0f7942c7),
  WTCP(0x7f0360cb, 0x0fdd0926), WTCP(0x7ef6c418, 0x1040c5bb), WTCP(0x7ee9d914, 0x10a4784b), WTCP(0x7edc9fc6, 0x11082096),
  WTCP(0x7ecf1837, 0x116bbe60), WTCP(0x7ec14270, 0x11cf516a), WTCP(0x7eb31e78, 0x1232d979), WTCP(0x7ea4ac58, 0x1296564d),
  WTCP(0x7e95ec1a, 0x12f9c7aa), WTCP(0x7e86ddc6, 0x135d2d53), WTCP(0x7e778166, 0x13c0870a), WTCP(0x7e67d703, 0x1423d492),
  WTCP(0x7e57dea7, 0x148715ae), WTCP(0x7e47985b, 0x14ea4a1f), WTCP(0x7e37042a, 0x154d71aa), WTCP(0x7e26221f, 0x15b08c12),
  WTCP(0x7e14f242, 0x16139918), WTCP(0x7e0374a0, 0x1676987f), WTCP(0x7df1a942, 0x16d98a0c), WTCP(0x7ddf9034, 0x173c6d80),
  WTCP(0x7dcd2981, 0x179f429f), WTCP(0x7dba7534, 0x1802092c), WTCP(0x7da77359, 0x1864c0ea), WTCP(0x7d9423fc, 0x18c7699b),
  WTCP(0x7d808728, 0x192a0304), WTCP(0x7d6c9ce9, 0x198c8ce7), WTCP(0x7d58654d, 0x19ef0707), WTCP(0x7d43e05e, 0x1a517128),
  WTCP(0x7d2f0e2b, 0x1ab3cb0d), WTCP(0x7d19eebf, 0x1b161479), WTCP(0x7d048228, 0x1b784d30), WTCP(0x7ceec873, 0x1bda74f6),
  WTCP(0x7cd8c1ae, 0x1c3c8b8c), WTCP(0x7cc26de5, 0x1c9e90b8), WTCP(0x7cabcd28, 0x1d00843d), WTCP(0x7c94df83, 0x1d6265dd),
  WTCP(0x7c7da505, 0x1dc4355e), WTCP(0x7c661dbc, 0x1e25f282), WTCP(0x7c4e49b7, 0x1e879d0d), WTCP(0x7c362904, 0x1ee934c3),
  WTCP(0x7c1dbbb3, 0x1f4ab968), WTCP(0x7c0501d2, 0x1fac2abf), WTCP(0x7bebfb70, 0x200d888d), WTCP(0x7bd2a89e, 0x206ed295),
  WTCP(0x7bb9096b, 0x20d0089c), WTCP(0x7b9f1de6, 0x21312a65), WTCP(0x7b84e61f, 0x219237b5), WTCP(0x7b6a6227, 0x21f3304f),
  WTCP(0x7b4f920e, 0x225413f8), WTCP(0x7b3475e5, 0x22b4e274), WTCP(0x7b190dbc, 0x23159b88), WTCP(0x7afd59a4, 0x23763ef7),
  WTCP(0x7ae159ae, 0x23d6cc87), WTCP(0x7ac50dec, 0x243743fa), WTCP(0x7aa8766f, 0x2497a517), WTCP(0x7a8b9348, 0x24f7efa2),
  WTCP(0x7a6e648a, 0x2558235f), WTCP(0x7a50ea47, 0x25b84012), WTCP(0x7a332490, 0x26184581), WTCP(0x7a151378, 0x26783370),
  WTCP(0x79f6b711, 0x26d809a5), WTCP(0x79d80f6f, 0x2737c7e3), WTCP(0x79b91ca4, 0x27976df1), WTCP(0x7999dec4, 0x27f6fb92),
  WTCP(0x797a55e0, 0x2856708d), WTCP(0x795a820e, 0x28b5cca5), WTCP(0x793a6361, 0x29150fa1), WTCP(0x7919f9ec, 0x29743946),
  WTCP(0x78f945c3, 0x29d34958), WTCP(0x78d846fb, 0x2a323f9e), WTCP(0x78b6fda8, 0x2a911bdc), WTCP(0x789569df, 0x2aefddd8),
  WTCP(0x78738bb3, 0x2b4e8558), WTCP(0x7851633b, 0x2bad1221), WTCP(0x782ef08b, 0x2c0b83fa), WTCP(0x780c33b8, 0x2c69daa6),
  WTCP(0x77e92cd9, 0x2cc815ee), WTCP(0x77c5dc01, 0x2d263596), WTCP(0x77a24148, 0x2d843964), WTCP(0x777e5cc3, 0x2de2211e),
  WTCP(0x775a2e89, 0x2e3fec8b), WTCP(0x7735b6af, 0x2e9d9b70), WTCP(0x7710f54c, 0x2efb2d95), WTCP(0x76ebea77, 0x2f58a2be),
  WTCP(0x76c69647, 0x2fb5fab2), WTCP(0x76a0f8d2, 0x30133539), WTCP(0x767b1231, 0x30705217), WTCP(0x7654e279, 0x30cd5115),
  WTCP(0x762e69c4, 0x312a31f8), WTCP(0x7607a828, 0x3186f487), WTCP(0x75e09dbd, 0x31e39889), WTCP(0x75b94a9c, 0x32401dc6),
  WTCP(0x7591aedd, 0x329c8402), WTCP(0x7569ca99, 0x32f8cb07), WTCP(0x75419de7, 0x3354f29b), WTCP(0x751928e0, 0x33b0fa84),
  WTCP(0x74f06b9e, 0x340ce28b), WTCP(0x74c7663a, 0x3468aa76), WTCP(0x749e18cd, 0x34c4520d), WTCP(0x74748371, 0x351fd918),
  WTCP(0x744aa63f, 0x357b3f5d), WTCP(0x74208150, 0x35d684a6), WTCP(0x73f614c0, 0x3631a8b8), WTCP(0x73cb60a8, 0x368cab5c),
  WTCP(0x73a06522, 0x36e78c5b), WTCP(0x73752249, 0x37424b7b), WTCP(0x73499838, 0x379ce885), WTCP(0x731dc70a, 0x37f76341),
  WTCP(0x72f1aed9, 0x3851bb77), WTCP(0x72c54fc1, 0x38abf0ef), WTCP(0x7298a9dd, 0x39060373), WTCP(0x726bbd48, 0x395ff2c9),
  WTCP(0x723e8a20, 0x39b9bebc), WTCP(0x7211107e, 0x3a136712), WTCP(0x71e35080, 0x3a6ceb96), WTCP(0x71b54a41, 0x3ac64c0f),
  WTCP(0x7186fdde, 0x3b1f8848), WTCP(0x71586b74, 0x3b78a007), WTCP(0x7129931f, 0x3bd19318), WTCP(0x70fa74fc, 0x3c2a6142),
  WTCP(0x70cb1128, 0x3c830a50), WTCP(0x709b67c0, 0x3cdb8e09), WTCP(0x706b78e3, 0x3d33ec39), WTCP(0x703b44ad, 0x3d8c24a8),
  WTCP(0x700acb3c, 0x3de4371f), WTCP(0x6fda0cae, 0x3e3c2369), WTCP(0x6fa90921, 0x3e93e950), WTCP(0x6f77c0b3, 0x3eeb889c),
  WTCP(0x6f463383, 0x3f430119), WTCP(0x6f1461b0, 0x3f9a5290), WTCP(0x6ee24b57, 0x3ff17cca), WTCP(0x6eaff099, 0x40487f94),
  WTCP(0x6e7d5193, 0x409f5ab6), WTCP(0x6e4a6e66, 0x40f60dfb), WTCP(0x6e174730, 0x414c992f), WTCP(0x6de3dc11, 0x41a2fc1a),
  WTCP(0x6db02d29, 0x41f93689), WTCP(0x6d7c3a98, 0x424f4845), WTCP(0x6d48047e, 0x42a5311b), WTCP(0x6d138afb, 0x42faf0d4),
  WTCP(0x6cdece2f, 0x4350873c), WTCP(0x6ca9ce3b, 0x43a5f41e), WTCP(0x6c748b3f, 0x43fb3746), WTCP(0x6c3f055d, 0x4450507e),
  WTCP(0x6c093cb6, 0x44a53f93), WTCP(0x6bd3316a, 0x44fa0450), WTCP(0x6b9ce39b, 0x454e9e80), WTCP(0x6b66536b, 0x45a30df0),
  WTCP(0x6b2f80fb, 0x45f7526b), WTCP(0x6af86c6c, 0x464b6bbe), WTCP(0x6ac115e2, 0x469f59b4), WTCP(0x6a897d7d, 0x46f31c1a),
  WTCP(0x6a51a361, 0x4746b2bc), WTCP(0x6a1987b0, 0x479a1d67), WTCP(0x69e12a8c, 0x47ed5be6), WTCP(0x69a88c19, 0x48406e08),
  WTCP(0x696fac78, 0x48935397), WTCP(0x69368bce, 0x48e60c62), WTCP(0x68fd2a3d, 0x49389836), WTCP(0x68c387e9, 0x498af6df),
  WTCP(0x6889a4f6, 0x49dd282a), WTCP(0x684f8186, 0x4a2f2be6), WTCP(0x68151dbe, 0x4a8101de), WTCP(0x67da79c3, 0x4ad2a9e2),
  WTCP(0x679f95b7, 0x4b2423be), WTCP(0x676471c0, 0x4b756f40), WTCP(0x67290e02, 0x4bc68c36), WTCP(0x66ed6aa1, 0x4c177a6e),
  WTCP(0x66b187c3, 0x4c6839b7), WTCP(0x6675658c, 0x4cb8c9dd), WTCP(0x66390422, 0x4d092ab0), WTCP(0x65fc63a9, 0x4d595bfe),
  WTCP(0x65bf8447, 0x4da95d96), WTCP(0x65826622, 0x4df92f46), WTCP(0x6545095f, 0x4e48d0dd), WTCP(0x65076e25, 0x4e984229),
  WTCP(0x64c99498, 0x4ee782fb), WTCP(0x648b7ce0, 0x4f369320), WTCP(0x644d2722, 0x4f857269), WTCP(0x640e9386, 0x4fd420a4),
  WTCP(0x63cfc231, 0x50229da1), WTCP(0x6390b34a, 0x5070e92f), WTCP(0x635166f9, 0x50bf031f), WTCP(0x6311dd64, 0x510ceb40),
  WTCP(0x62d216b3, 0x515aa162), WTCP(0x6292130c, 0x51a82555), WTCP(0x6251d298, 0x51f576ea), WTCP(0x6211557e, 0x524295f0),
  WTCP(0x61d09be5, 0x528f8238), WTCP(0x618fa5f7, 0x52dc3b92), WTCP(0x614e73da, 0x5328c1d0), WTCP(0x610d05b7, 0x537514c2),
  WTCP(0x60cb5bb7, 0x53c13439), WTCP(0x60897601, 0x540d2005), WTCP(0x604754bf, 0x5458d7f9), WTCP(0x6004f819, 0x54a45be6),
  WTCP(0x5fc26038, 0x54efab9c), WTCP(0x5f7f8d46, 0x553ac6ee), WTCP(0x5f3c7f6b, 0x5585adad), WTCP(0x5ef936d1, 0x55d05faa),
  WTCP(0x5eb5b3a2, 0x561adcb9), WTCP(0x5e71f606, 0x566524aa), WTCP(0x5e2dfe29, 0x56af3750), WTCP(0x5de9cc33, 0x56f9147e),
  WTCP(0x5da5604f, 0x5742bc06), WTCP(0x5d60baa7, 0x578c2dba), WTCP(0x5d1bdb65, 0x57d5696d), WTCP(0x5cd6c2b5, 0x581e6ef1),
  WTCP(0x5c9170bf, 0x58673e1b), WTCP(0x5c4be5b0, 0x58afd6bd), WTCP(0x5c0621b2, 0x58f838a9), WTCP(0x5bc024f0, 0x594063b5),
  WTCP(0x5b79ef96, 0x598857b2), WTCP(0x5b3381ce, 0x59d01475), WTCP(0x5aecdbc5, 0x5a1799d1), WTCP(0x5aa5fda5, 0x5a5ee79a),
};

/* The window coefficients are calculated according to the formula:
 * N=FRAME_LEN_LONG_LD480
 * win[n]=sin(pi*(n+0.5)/(2*N))   for n=0,..,N-1 */
RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_WTP SineWindow480[480] =
{
  WTCP(0x7ffff4c5, 0x00359dd2), WTCP(0x7fff9aef, 0x00a0d951), WTCP(0x7ffee744, 0x010c1460), WTCP(0x7ffdd9c4, 0x01774eb2),
  WTCP(0x7ffc726f, 0x01e287fc), WTCP(0x7ffab147, 0x024dbff4), WTCP(0x7ff8964d, 0x02b8f64e), WTCP(0x7ff62182, 0x03242abf),
  WTCP(0x7ff352e8, 0x038f5cfb), WTCP(0x7ff02a82, 0x03fa8cb8), WTCP(0x7feca851, 0x0465b9aa), WTCP(0x7fe8cc57, 0x04d0e386),
  WTCP(0x7fe49698, 0x053c0a01), WTCP(0x7fe00716, 0x05a72ccf), WTCP(0x7fdb1dd5, 0x06124ba5), WTCP(0x7fd5dad8, 0x067d6639),
  WTCP(0x7fd03e23, 0x06e87c3f), WTCP(0x7fca47b9, 0x07538d6b), WTCP(0x7fc3f7a0, 0x07be9973), WTCP(0x7fbd4dda, 0x0829a00c),
  WTCP(0x7fb64a6e, 0x0894a0ea), WTCP(0x7faeed5f, 0x08ff9bc2), WTCP(0x7fa736b4, 0x096a9049), WTCP(0x7f9f2671, 0x09d57e35),
  WTCP(0x7f96bc9c, 0x0a40653a), WTCP(0x7f8df93c, 0x0aab450d), WTCP(0x7f84dc55, 0x0b161d63), WTCP(0x7f7b65ef, 0x0b80edf1),
  WTCP(0x7f719611, 0x0bebb66c), WTCP(0x7f676cc0, 0x0c56768a), WTCP(0x7f5cea05, 0x0cc12dff), WTCP(0x7f520de6, 0x0d2bdc80),
  WTCP(0x7f46d86c, 0x0d9681c2), WTCP(0x7f3b499d, 0x0e011d7c), WTCP(0x7f2f6183, 0x0e6baf61), WTCP(0x7f232026, 0x0ed63727),
  WTCP(0x7f16858e, 0x0f40b483), WTCP(0x7f0991c4, 0x0fab272b), WTCP(0x7efc44d0, 0x10158ed4), WTCP(0x7eee9ebe, 0x107feb33),
  WTCP(0x7ee09f95, 0x10ea3bfd), WTCP(0x7ed24761, 0x115480e9), WTCP(0x7ec3962a, 0x11beb9aa), WTCP(0x7eb48bfb, 0x1228e5f8),
  WTCP(0x7ea528e0, 0x12930586), WTCP(0x7e956ce1, 0x12fd180b), WTCP(0x7e85580c, 0x13671d3d), WTCP(0x7e74ea6a, 0x13d114d0),
  WTCP(0x7e642408, 0x143afe7b), WTCP(0x7e5304f2, 0x14a4d9f4), WTCP(0x7e418d32, 0x150ea6ef), WTCP(0x7e2fbcd6, 0x15786522),
  WTCP(0x7e1d93ea, 0x15e21445), WTCP(0x7e0b127a, 0x164bb40b), WTCP(0x7df83895, 0x16b5442b), WTCP(0x7de50646, 0x171ec45c),
  WTCP(0x7dd17b9c, 0x17883452), WTCP(0x7dbd98a4, 0x17f193c5), WTCP(0x7da95d6c, 0x185ae269), WTCP(0x7d94ca03, 0x18c41ff6),
  WTCP(0x7d7fde76, 0x192d4c21), WTCP(0x7d6a9ad5, 0x199666a0), WTCP(0x7d54ff2e, 0x19ff6f2a), WTCP(0x7d3f0b90, 0x1a686575),
  WTCP(0x7d28c00c, 0x1ad14938), WTCP(0x7d121cb0, 0x1b3a1a28), WTCP(0x7cfb218c, 0x1ba2d7fc), WTCP(0x7ce3ceb2, 0x1c0b826a),
  WTCP(0x7ccc2430, 0x1c74192a), WTCP(0x7cb42217, 0x1cdc9bf2), WTCP(0x7c9bc87a, 0x1d450a78), WTCP(0x7c831767, 0x1dad6473),
  WTCP(0x7c6a0ef2, 0x1e15a99a), WTCP(0x7c50af2b, 0x1e7dd9a4), WTCP(0x7c36f824, 0x1ee5f447), WTCP(0x7c1ce9ef, 0x1f4df93a),
  WTCP(0x7c02849f, 0x1fb5e836), WTCP(0x7be7c847, 0x201dc0ef), WTCP(0x7bccb4f8, 0x2085831f), WTCP(0x7bb14ac5, 0x20ed2e7b),
  WTCP(0x7b9589c3, 0x2154c2bb), WTCP(0x7b797205, 0x21bc3f97), WTCP(0x7b5d039e, 0x2223a4c5), WTCP(0x7b403ea2, 0x228af1fe),
  WTCP(0x7b232325, 0x22f226f8), WTCP(0x7b05b13d, 0x2359436c), WTCP(0x7ae7e8fc, 0x23c04710), WTCP(0x7ac9ca7a, 0x2427319d),
  WTCP(0x7aab55ca, 0x248e02cb), WTCP(0x7a8c8b01, 0x24f4ba50), WTCP(0x7a6d6a37, 0x255b57e6), WTCP(0x7a4df380, 0x25c1db44),
  WTCP(0x7a2e26f2, 0x26284422), WTCP(0x7a0e04a4, 0x268e9238), WTCP(0x79ed8cad, 0x26f4c53e), WTCP(0x79ccbf22, 0x275adcee),
  WTCP(0x79ab9c1c, 0x27c0d8fe), WTCP(0x798a23b1, 0x2826b928), WTCP(0x796855f9, 0x288c7d24), WTCP(0x7946330c, 0x28f224ab),
  WTCP(0x7923bb01, 0x2957af74), WTCP(0x7900edf2, 0x29bd1d3a), WTCP(0x78ddcbf5, 0x2a226db5), WTCP(0x78ba5524, 0x2a87a09d),
  WTCP(0x78968998, 0x2aecb5ac), WTCP(0x7872696a, 0x2b51ac9a), WTCP(0x784df4b3, 0x2bb68522), WTCP(0x78292b8d, 0x2c1b3efb),
  WTCP(0x78040e12, 0x2c7fd9e0), WTCP(0x77de9c5b, 0x2ce45589), WTCP(0x77b8d683, 0x2d48b1b1), WTCP(0x7792bca5, 0x2dacee11),
  WTCP(0x776c4edb, 0x2e110a62), WTCP(0x77458d40, 0x2e75065e), WTCP(0x771e77f0, 0x2ed8e1c0), WTCP(0x76f70f05, 0x2f3c9c40),
  WTCP(0x76cf529c, 0x2fa03599), WTCP(0x76a742d1, 0x3003ad85), WTCP(0x767edfbe, 0x306703bf), WTCP(0x76562982, 0x30ca3800),
  WTCP(0x762d2038, 0x312d4a03), WTCP(0x7603c3fd, 0x31903982), WTCP(0x75da14ef, 0x31f30638), WTCP(0x75b01329, 0x3255afe0),
  WTCP(0x7585becb, 0x32b83634), WTCP(0x755b17f2, 0x331a98ef), WTCP(0x75301ebb, 0x337cd7cd), WTCP(0x7504d345, 0x33def287),
  WTCP(0x74d935ae, 0x3440e8da), WTCP(0x74ad4615, 0x34a2ba81), WTCP(0x74810499, 0x35046736), WTCP(0x74547158, 0x3565eeb6),
  WTCP(0x74278c72, 0x35c750bc), WTCP(0x73fa5607, 0x36288d03), WTCP(0x73ccce36, 0x3689a348), WTCP(0x739ef51f, 0x36ea9346),
  WTCP(0x7370cae2, 0x374b5cb9), WTCP(0x73424fa0, 0x37abff5d), WTCP(0x73138379, 0x380c7aee), WTCP(0x72e4668f, 0x386ccf2a),
  WTCP(0x72b4f902, 0x38ccfbcb), WTCP(0x72853af3, 0x392d008f), WTCP(0x72552c85, 0x398cdd32), WTCP(0x7224cdd8, 0x39ec9172),
  WTCP(0x71f41f0f, 0x3a4c1d09), WTCP(0x71c3204c, 0x3aab7fb7), WTCP(0x7191d1b1, 0x3b0ab937), WTCP(0x71603361, 0x3b69c947),
  WTCP(0x712e457f, 0x3bc8afa5), WTCP(0x70fc082d, 0x3c276c0d), WTCP(0x70c97b90, 0x3c85fe3d), WTCP(0x70969fca, 0x3ce465f3),
  WTCP(0x706374ff, 0x3d42a2ec), WTCP(0x702ffb54, 0x3da0b4e7), WTCP(0x6ffc32eb, 0x3dfe9ba1), WTCP(0x6fc81bea, 0x3e5c56d8),
  WTCP(0x6f93b676, 0x3eb9e64b), WTCP(0x6f5f02b2, 0x3f1749b8), WTCP(0x6f2a00c4, 0x3f7480dd), WTCP(0x6ef4b0d1, 0x3fd18b7a),
  WTCP(0x6ebf12ff, 0x402e694c), WTCP(0x6e892772, 0x408b1a12), WTCP(0x6e52ee52, 0x40e79d8c), WTCP(0x6e1c67c4, 0x4143f379),
  WTCP(0x6de593ee, 0x41a01b97), WTCP(0x6dae72f7, 0x41fc15a6), WTCP(0x6d770506, 0x4257e166), WTCP(0x6d3f4a40, 0x42b37e96),
  WTCP(0x6d0742cf, 0x430eecf6), WTCP(0x6cceeed8, 0x436a2c45), WTCP(0x6c964e83, 0x43c53c44), WTCP(0x6c5d61f9, 0x44201cb2),
  WTCP(0x6c242960, 0x447acd50), WTCP(0x6beaa4e2, 0x44d54ddf), WTCP(0x6bb0d4a7, 0x452f9e1e), WTCP(0x6b76b8d6, 0x4589bdcf),
  WTCP(0x6b3c519a, 0x45e3acb1), WTCP(0x6b019f1a, 0x463d6a87), WTCP(0x6ac6a180, 0x4696f710), WTCP(0x6a8b58f6, 0x46f0520f),
  WTCP(0x6a4fc5a6, 0x47497b44), WTCP(0x6a13e7b8, 0x47a27271), WTCP(0x69d7bf57, 0x47fb3757), WTCP(0x699b4cad, 0x4853c9b9),
  WTCP(0x695e8fe5, 0x48ac2957), WTCP(0x69218929, 0x490455f4), WTCP(0x68e438a4, 0x495c4f52), WTCP(0x68a69e81, 0x49b41533),
  WTCP(0x6868baec, 0x4a0ba75b), WTCP(0x682a8e0f, 0x4a63058a), WTCP(0x67ec1817, 0x4aba2f84), WTCP(0x67ad592f, 0x4b11250c),
  WTCP(0x676e5183, 0x4b67e5e4), WTCP(0x672f013f, 0x4bbe71d1), WTCP(0x66ef6891, 0x4c14c894), WTCP(0x66af87a4, 0x4c6ae9f2),
  WTCP(0x666f5ea6, 0x4cc0d5ae), WTCP(0x662eedc3, 0x4d168b8b), WTCP(0x65ee3529, 0x4d6c0b4e), WTCP(0x65ad3505, 0x4dc154bb),
  WTCP(0x656bed84, 0x4e166795), WTCP(0x652a5ed6, 0x4e6b43a2), WTCP(0x64e88926, 0x4ebfe8a5), WTCP(0x64a66ca5, 0x4f145662),
  WTCP(0x6464097f, 0x4f688ca0), WTCP(0x64215fe5, 0x4fbc8b22), WTCP(0x63de7003, 0x501051ae), WTCP(0x639b3a0b, 0x5063e008),
  WTCP(0x6357be2a, 0x50b735f8), WTCP(0x6313fc90, 0x510a5340), WTCP(0x62cff56c, 0x515d37a9), WTCP(0x628ba8ef, 0x51afe2f6),
  WTCP(0x62471749, 0x520254ef), WTCP(0x620240a8, 0x52548d59), WTCP(0x61bd253f, 0x52a68bfb), WTCP(0x6177c53c, 0x52f8509b),
  WTCP(0x613220d2, 0x5349daff), WTCP(0x60ec3830, 0x539b2af0), WTCP(0x60a60b88, 0x53ec4032), WTCP(0x605f9b0b, 0x543d1a8e),
  WTCP(0x6018e6eb, 0x548db9cb), WTCP(0x5fd1ef59, 0x54de1db1), WTCP(0x5f8ab487, 0x552e4605), WTCP(0x5f4336a7, 0x557e3292),
  WTCP(0x5efb75ea, 0x55cde31e), WTCP(0x5eb37285, 0x561d5771), WTCP(0x5e6b2ca8, 0x566c8f55), WTCP(0x5e22a487, 0x56bb8a90),
  WTCP(0x5dd9da55, 0x570a48ec), WTCP(0x5d90ce45, 0x5758ca31), WTCP(0x5d47808a, 0x57a70e29), WTCP(0x5cfdf157, 0x57f5149d),
  WTCP(0x5cb420e0, 0x5842dd54), WTCP(0x5c6a0f59, 0x5890681a), WTCP(0x5c1fbcf6, 0x58ddb4b8), WTCP(0x5bd529eb, 0x592ac2f7),
  WTCP(0x5b8a566c, 0x597792a1), WTCP(0x5b3f42ae, 0x59c42381), WTCP(0x5af3eee6, 0x5a107561), WTCP(0x5aa85b48, 0x5a5c880a),
};






/**
 * \brief Helper table containing the length, rasterand shape mapping to individual window slope tables.
 * [0: sine ][0: radix2 raster        ][ceil(log2(length)) length   4    .. 1024 ]
 *           [1: 10ms raster          ][ceil(log2(length)) length   3.25 ..  960 ]
 *           [2: 3/4 of radix 2 raster][ceil(log2(length)) length   3    ..  768 ]
 * [1: KBD  ][0: radix2 raster        ][ceil(log2(length)) length 128    .. 1024 ]
 *           [1: 10ms raster          ][ceil(log2(length)) length 120    ..  960 ]
 *           [2: 3/4 of radix 2 raster][ceil(log2(length)) length  96    ..  768 ]
 */
const FIXP_WTP *const windowSlopes[2][3][9] =
{
  { /* Sine */
    { /* Radix 2 */
      NULL,
      NULL,
      NULL,
      SineWindow32,
      SineWindow64,
      SineWindow128,
      NULL,
      SineWindow512,
      SineWindow1024
    },
    { /* 10ms raster */
      NULL, /* 3.25 */
      NULL, /* 7.5 */
      NULL,
      NULL,
      NULL,
      SineWindow120, 
      NULL,
      SineWindow480,
      SineWindow960
    },
    { /* 3/4 radix2 raster */
      NULL, /* 3 */
      NULL, /* 6 */
#ifdef INCLUDE_SineWindow12
      SineWindow12,
#else
      NULL,
#endif
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    }
  },
  { /* KBD */
    { /* Radix 2 */
      KBDWindow128,
      NULL,
      SineWindow512,
      KBDWindow1024
    },
    { /* 10ms raster */
      KBDWindow120,
      NULL,
      SineWindow480,
      KBDWindow960
    },
    { /* 3/4 radix2 raster */
      NULL,
      NULL,
      NULL,
      NULL
    }
  }
};

const FIXP_WTP * FDKgetWindowSlope(int length, int shape)
{
  const FIXP_WTP * w = NULL;
  int raster, ld2_length;

  /* Get ld2 of length - 2 + 1
     -2: because first table entry is window of size 4
     +1: because we already include +1 because of ceil(log2(length)) */
  ld2_length = DFRACT_BITS-1-fNormz((FIXP_DBL)length) - 1;

  /* Extract sort of "eigenvalue" (the 4 left most bits) of length. */
  switch ( (length) >> (ld2_length-2) ) {
    case 0x8: /* radix 2 */
      raster = 0;
      ld2_length--; /* revert + 1 because of ceil(log2(length)) from above. */
      break;
    case 0xf: /* 10 ms */
      raster = 1;
      break;
    case 0xc: /* 3/4 of radix 2 */
      raster = 2;
      break;
    default:
      raster = 0;
      break;
  }

  /* The table for sine windows (shape == 0) is 5 entries longer. */
  if (shape == 1) {
    ld2_length-=5;
  }

  /* Look up table */
  w = windowSlopes[shape&1][raster][ld2_length];

  FDK_ASSERT(w != NULL);

  return w;
}


/*
 * QMF filter and twiddle tables
 */

#ifdef QMF_COEFF_16BIT
#define QFC(x)  FX_DBL2FXCONST_SGL(x)
#define QTCFL(x)  FL2FXCONST_SGL(x)
#define QTC(x)    FX_DBL2FXCONST_SGL(x)
#else
#define QFC(x) ((FIXP_DBL)(x))
#define QTCFL(x)  FL2FXCONST_DBL(x)
#define QTC(x)  ((FIXP_DBL)(x))
#endif /* ARCH_PREFER_MULT_32x16 */

#ifndef LOW_POWER_SBR_ONLY
/*!
  \name QMF-Twiddle
  \brief QMF twiddle factors

  L=32, gain=2.0, angle = 0.75
*/

const FIXP_QTW qmf_phaseshift_cos32[32] =
{
  QTCFL( 0.99932238458835f),QTCFL( 0.99390697000236f),QTCFL( 0.98310548743122f),QTCFL( 0.96697647104485f),
  QTCFL( 0.94560732538052f),QTCFL( 0.91911385169006f),QTCFL( 0.88763962040285f),QTCFL( 0.85135519310527f),
  QTCFL( 0.81045719825259f),QTCFL( 0.76516726562246f),QTCFL( 0.71573082528382f),QTCFL( 0.66241577759017f),
  QTCFL( 0.60551104140433f),QTCFL( 0.54532498842205f),QTCFL( 0.48218377207912f),QTCFL( 0.41642956009764f),
  QTCFL( 0.34841868024943f),QTCFL( 0.27851968938505f),QTCFL( 0.20711137619222f),QTCFL( 0.13458070850713f),
  QTCFL( 0.06132073630221f),QTCFL(-0.01227153828572f),QTCFL(-0.08579731234444f),QTCFL(-0.15885814333386f),
  QTCFL(-0.23105810828067f),QTCFL(-0.30200594931923f),QTCFL(-0.37131719395184f),QTCFL(-0.43861623853853f),
  QTCFL(-0.50353838372572f),QTCFL(-0.56573181078361f),QTCFL(-0.62485948814239f),QTCFL(-0.68060099779545f)
};

const FIXP_QTW qmf_phaseshift_sin32[32] =
{
  QTCFL( 0.03680722294136f),QTCFL( 0.11022220729388f),QTCFL( 0.18303988795514f),QTCFL( 0.25486565960451f),
  QTCFL( 0.32531029216226f),QTCFL( 0.39399204006105f),QTCFL( 0.46053871095824f),QTCFL( 0.52458968267847f),
  QTCFL( 0.58579785745644f),QTCFL( 0.64383154288979f),QTCFL( 0.69837624940897f),QTCFL( 0.74913639452346f),
  QTCFL( 0.79583690460888f),QTCFL( 0.83822470555484f),QTCFL( 0.87607009419541f),QTCFL( 0.90916798309052f),
  QTCFL( 0.93733901191257f),QTCFL( 0.96043051941557f),QTCFL( 0.97831737071963f),QTCFL( 0.99090263542778f),
  QTCFL( 0.99811811290015f),QTCFL( 0.99992470183914f),QTCFL( 0.99631261218278f),QTCFL( 0.98730141815786f),
  QTCFL( 0.97293995220556f),QTCFL( 0.95330604035419f),QTCFL( 0.92850608047322f),QTCFL( 0.89867446569395f),
  QTCFL( 0.86397285612159f),QTCFL( 0.82458930278503f),QTCFL( 0.78073722857209f),QTCFL( 0.73265427167241f)
};
const FIXP_QTW qmf_phaseshift_cos64[64] = {
  QTC(0x7ff62181), QTC(0x7fa736b3), QTC(0x7f0991c3), QTC(0x7e1d93e9), QTC(0x7ce3ceb1), QTC(0x7b5d039d), QTC(0x798a23b0), QTC(0x776c4eda),
  QTC(0x7504d344), QTC(0x72552c84), QTC(0x6f5f02b1), QTC(0x6c24295f), QTC(0x68a69e80), QTC(0x64e88925), QTC(0x60ec382f), QTC(0x5cb420df),
  QTC(0x5842dd54), QTC(0x539b2aef), QTC(0x4ebfe8a4), QTC(0x49b41533), QTC(0x447acd50), QTC(0x3f1749b7), QTC(0x398cdd32), QTC(0x33def287),
  QTC(0x2e110a62), QTC(0x2826b928), QTC(0x2223a4c5), QTC(0x1c0b826a), QTC(0x15e21444), QTC(0x0fab272b), QTC(0x096a9049), QTC(0x03242abf),
  QTC(0xfcdbd541), QTC(0xf6956fb7), QTC(0xf054d8d5), QTC(0xea1debbc), QTC(0xe3f47d96), QTC(0xdddc5b3b), QTC(0xd7d946d8), QTC(0xd1eef59e),
  QTC(0xcc210d79), QTC(0xc67322ce), QTC(0xc0e8b649), QTC(0xbb8532b0), QTC(0xb64beacd), QTC(0xb140175c), QTC(0xac64d511), QTC(0xa7bd22ac),
  QTC(0xa34bdf21), QTC(0x9f13c7d1), QTC(0x9b1776db), QTC(0x97596180), QTC(0x93dbd6a1), QTC(0x90a0fd4f), QTC(0x8daad37c), QTC(0x8afb2cbc),
  QTC(0x8893b126), QTC(0x8675dc50), QTC(0x84a2fc63), QTC(0x831c314f), QTC(0x81e26c17), QTC(0x80f66e3d), QTC(0x8058c94d), QTC(0x8009de7f)
};

const FIXP_QTW qmf_phaseshift_sin64[64] = {
  QTC(0x03242abf), QTC(0x096a9049), QTC(0x0fab272b), QTC(0x15e21444), QTC(0x1c0b826a), QTC(0x2223a4c5), QTC(0x2826b928), QTC(0x2e110a62),
  QTC(0x33def287), QTC(0x398cdd32), QTC(0x3f1749b7), QTC(0x447acd50), QTC(0x49b41533), QTC(0x4ebfe8a4), QTC(0x539b2aef), QTC(0x5842dd54),
  QTC(0x5cb420df), QTC(0x60ec382f), QTC(0x64e88925), QTC(0x68a69e80), QTC(0x6c24295f), QTC(0x6f5f02b1), QTC(0x72552c84), QTC(0x7504d344),
  QTC(0x776c4eda), QTC(0x798a23b0), QTC(0x7b5d039d), QTC(0x7ce3ceb1), QTC(0x7e1d93e9), QTC(0x7f0991c3), QTC(0x7fa736b3), QTC(0x7ff62181),
  QTC(0x7ff62181), QTC(0x7fa736b3), QTC(0x7f0991c3), QTC(0x7e1d93e9), QTC(0x7ce3ceb1), QTC(0x7b5d039d), QTC(0x798a23b0), QTC(0x776c4eda),
  QTC(0x7504d344), QTC(0x72552c84), QTC(0x6f5f02b1), QTC(0x6c24295f), QTC(0x68a69e80), QTC(0x64e88925), QTC(0x60ec382f), QTC(0x5cb420df),
  QTC(0x5842dd54), QTC(0x539b2aef), QTC(0x4ebfe8a4), QTC(0x49b41533), QTC(0x447acd50), QTC(0x3f1749b7), QTC(0x398cdd32), QTC(0x33def287),
  QTC(0x2e110a62), QTC(0x2826b928), QTC(0x2223a4c5), QTC(0x1c0b826a), QTC(0x15e21444), QTC(0x0fab272b), QTC(0x096a9049), QTC(0x03242abf)
};

//@}

#endif /* #ifdef LOW_POWER_SBR_ONLY */



/*!
  \name QMF
  \brief  QMF-Table
          64 channels, N = 640, optimized by PE 010516

  The coeffs are rearranged compared with the reference in the following
  way, exploiting symmetry :
  sbr_qmf_64[5] = p_64_640_qmf[0];
  sbr_qmf_64[6] = p_64_640_qmf[128];
  sbr_qmf_64[7] = p_64_640_qmf[256];
  sbr_qmf_64[8] = p_64_640_qmf[384];
  sbr_qmf_64[9] = p_64_640_qmf[512];

  sbr_qmf_64[10] = p_64_640_qmf[1];
  sbr_qmf_64[11] = p_64_640_qmf[129];
  sbr_qmf_64[12] = p_64_640_qmf[257];
  sbr_qmf_64[13] = p_64_640_qmf[385];
  sbr_qmf_64[14] = p_64_640_qmf[513];
  .
  .
  .
  sbr_qmf_64_640_qmf[315] = p_64_640_qmf[62];
  sbr_qmf_64_640_qmf[316] = p_64_640_qmf[190];
  sbr_qmf_64_640_qmf[317] = p_64_640_qmf[318];
  sbr_qmf_64_640_qmf[318] = p_64_640_qmf[446];
  sbr_qmf_64_640_qmf[319] = p_64_640_qmf[574];

  sbr_qmf_64_640_qmf[320] = p_64_640_qmf[63];
  sbr_qmf_64_640_qmf[321] = p_64_640_qmf[191];
  sbr_qmf_64_640_qmf[322] = p_64_640_qmf[319];
  sbr_qmf_64_640_qmf[323] = p_64_640_qmf[447];
  sbr_qmf_64_640_qmf[324] = p_64_640_qmf[575];

  sbr_qmf_64_640_qmf[319] = p_64_640_qmf[64];
  sbr_qmf_64_640_qmf[318] = p_64_640_qmf[192];
  sbr_qmf_64_640_qmf[317] = p_64_640_qmf[320];
  sbr_qmf_64_640_qmf[316] = p_64_640_qmf[448];
  sbr_qmf_64_640_qmf[315] = p_64_640_qmf[576];

  sbr_qmf_64_640_qmf[314] = p_64_640_qmf[65];
  sbr_qmf_64_640_qmf[313] = p_64_640_qmf[193];
  sbr_qmf_64_640_qmf[312] = p_64_640_qmf[321];
  sbr_qmf_64_640_qmf[311] = p_64_640_qmf[449];
  sbr_qmf_64_640_qmf[310] = p_64_640_qmf[577];
  .
  .
  .
  sbr_qmf_64[9] = p_64_640_qmf[126]
  sbr_qmf_64[8] = p_64_640_qmf[254];
  sbr_qmf_64[7] = p_64_640_qmf[382];
  sbr_qmf_64[6] = p_64_640_qmf[510];
  sbr_qmf_64[5] = p_64_640_qmf[638];

  sbr_qmf_64[4] = p_64_640_qmf[127]
  sbr_qmf_64[3] = p_64_640_qmf[255];
  sbr_qmf_64[2] = p_64_640_qmf[383];
  sbr_qmf_64[1] = p_64_640_qmf[511];
  sbr_qmf_64[0] = p_64_640_qmf[639];

  Max sum of all FIR filter absolute coefficients is: 0x7FF5B201
  thus, the filter output is not required to be scaled.

  \showinitializer
*/
//@{
#if QMF_NO_POLY==5
LNK_SECTION_CONSTDATA_L1
RAM_ALIGN
const FIXP_PFT qmf_64[QMF640_PFT_TABLE_SIZE+QMF_NO_POLY] =
{
  QFC(0x00000000), QFC(0x01b2e41d), QFC(0x2e3a7532), QFC(0xd1c58ace), QFC(0xfe4d1be3),
  QFC(0xffede50e), QFC(0x01d78bfc), QFC(0x2faa221c), QFC(0xd3337b3e), QFC(0xfe70b8d1),
  QFC(0xffed978a), QFC(0x01fd3ba0), QFC(0x311af3a4), QFC(0xd49fd55f), QFC(0xfe933dc0),
  QFC(0xffefc9b9), QFC(0x02244a24), QFC(0x328cc6f0), QFC(0xd60a46e6), QFC(0xfeb48d0d),
  QFC(0xfff0065d), QFC(0x024bf7a1), QFC(0x33ff670e), QFC(0xd7722f04), QFC(0xfed4bec3),
  QFC(0xffeff6ca), QFC(0x0274ba43), QFC(0x3572ec70), QFC(0xd8d7f220), QFC(0xfef3f6ab),
  QFC(0xffef7b8b), QFC(0x029e35b4), QFC(0x36e69691), QFC(0xda3b176a), QFC(0xff120d70),
  QFC(0xffeedfa4), QFC(0x02c89901), QFC(0x385a49c3), QFC(0xdb9b5b12), QFC(0xff2ef725),
  QFC(0xffee1650), QFC(0x02f3e48d), QFC(0x39ce0477), QFC(0xdcf898fb), QFC(0xff4aabc8),
  QFC(0xffed651d), QFC(0x03201116), QFC(0x3b415115), QFC(0xde529087), QFC(0xff6542d1),
  QFC(0xffecc31b), QFC(0x034d01f1), QFC(0x3cb41219), QFC(0xdfa93ab5), QFC(0xff7ee3f1),
  QFC(0xffebe77b), QFC(0x037ad438), QFC(0x3e25b17e), QFC(0xe0fc421e), QFC(0xff975c01),
  QFC(0xffeb50b2), QFC(0x03a966bb), QFC(0x3f962fb8), QFC(0xe24b8f67), QFC(0xffaea5d6),
  QFC(0xffea9192), QFC(0x03d8afe6), QFC(0x41058bc5), QFC(0xe396a45d), QFC(0xffc4e365),
  QFC(0xffe9ca76), QFC(0x04083fec), QFC(0x4272a385), QFC(0xe4de0cb0), QFC(0xffda17f2),
  QFC(0xffe940f4), QFC(0x043889c6), QFC(0x43de620a), QFC(0xe620c476), QFC(0xffee183b),
  QFC(0xffe88ba8), QFC(0x04694101), QFC(0x4547daea), QFC(0xe75f8bb8), QFC(0x0000e790),
  QFC(0xffe83a07), QFC(0x049aa82f), QFC(0x46aea856), QFC(0xe89971b7), QFC(0x00131c75),
  QFC(0xffe79e16), QFC(0x04cc2fcf), QFC(0x4812f848), QFC(0xe9cea84a), QFC(0x0023b989),
  QFC(0xffe7746e), QFC(0x04fe20be), QFC(0x4973fef1), QFC(0xeafee7f1), QFC(0x0033b927),
  QFC(0xffe6d466), QFC(0x05303f88), QFC(0x4ad237a2), QFC(0xec2a3f60), QFC(0x00426f36),
  QFC(0xffe6afed), QFC(0x05626209), QFC(0x4c2ca3df), QFC(0xed50a31d), QFC(0x00504f41),
  QFC(0xffe65416), QFC(0x05950122), QFC(0x4d83976c), QFC(0xee71b2fe), QFC(0x005d36df),
  QFC(0xffe681c6), QFC(0x05c76fed), QFC(0x4ed62be2), QFC(0xef8d4d7b), QFC(0x006928a0),
  QFC(0xffe66dd0), QFC(0x05f9c051), QFC(0x5024d70e), QFC(0xf0a3959f), QFC(0x007400b8),
  QFC(0xffe66fab), QFC(0x062bf5ec), QFC(0x516eefb8), QFC(0xf1b461ab), QFC(0x007e0393),
  QFC(0xffe69423), QFC(0x065dd56a), QFC(0x52b449dd), QFC(0xf2bf6ea4), QFC(0x00872c63),
  QFC(0xffe6fed4), QFC(0x068f8b44), QFC(0x53f495a9), QFC(0xf3c4e887), QFC(0x008f87aa),
  QFC(0xffe75361), QFC(0x06c0f0c0), QFC(0x552f8ff6), QFC(0xf4c473c6), QFC(0x0096dcc2),
  QFC(0xffe80414), QFC(0x06f1825d), QFC(0x56654bdc), QFC(0xf5be0fa9), QFC(0x009da526),
  QFC(0xffe85b4a), QFC(0x0721bf22), QFC(0x579505f4), QFC(0xf6b1f3c3), QFC(0x00a3508f),
  QFC(0xffe954d0), QFC(0x075112a2), QFC(0x58befacc), QFC(0xf79fa13a), QFC(0x00a85e94),
  QFC(0xffea353a), QFC(0x077fedb3), QFC(0x59e2f69e), QFC(0xf887507c), QFC(0x00acbd2f),
  QFC(0xffeb3849), QFC(0x07ad8c26), QFC(0x5b001db7), QFC(0xf96916f5), QFC(0x00b06b68),
  QFC(0xffec8409), QFC(0x07da2b7f), QFC(0x5c16d0ae), QFC(0xfa44a069), QFC(0x00b36acd),
  QFC(0xffedc418), QFC(0x08061671), QFC(0x5d26be9b), QFC(0xfb19b7bd), QFC(0x00b58c8d),
  QFC(0xffef2395), QFC(0x08303897), QFC(0x5e2f6366), QFC(0xfbe8f5bd), QFC(0x00b73ab0),
  QFC(0xfff0e7ef), QFC(0x08594887), QFC(0x5f30ff5e), QFC(0xfcb1d740), QFC(0x00b85f70),
  QFC(0xfff294c3), QFC(0x0880ffdd), QFC(0x602b0c7e), QFC(0xfd7475d8), QFC(0x00b8c6b0),
  QFC(0xfff48700), QFC(0x08a75da4), QFC(0x611d58a2), QFC(0xfe310657), QFC(0x00b8fe0d),
  QFC(0xfff681d6), QFC(0x08cb4e23), QFC(0x6207f21f), QFC(0xfee723c6), QFC(0x00b8394b),
  QFC(0xfff91fc9), QFC(0x08edfeaa), QFC(0x62ea6473), QFC(0xff96db8f), QFC(0x00b74c37),
  QFC(0xfffb42b0), QFC(0x090ec1fc), QFC(0x63c45243), QFC(0x0040c497), QFC(0x00b5c867),
  QFC(0xfffdfa24), QFC(0x092d7970), QFC(0x64964062), QFC(0x00e42fa2), QFC(0x00b3d15c),
  QFC(0x00007134), QFC(0x0949eaac), QFC(0x655f63f1), QFC(0x01816e06), QFC(0x00b1978d),
  QFC(0x00039609), QFC(0x0963ed46), QFC(0x661fd6b7), QFC(0x02186a92), QFC(0x00af374c),
  QFC(0x0006b1cf), QFC(0x097c1ee8), QFC(0x66d76724), QFC(0x02a99097), QFC(0x00abe79e),
  QFC(0x0009aa3f), QFC(0x099140a7), QFC(0x6785c24c), QFC(0x03343534), QFC(0x00a8739d),
  QFC(0x000d31b5), QFC(0x09a3e163), QFC(0x682b39a3), QFC(0x03b8f8dc), QFC(0x00a520bb),
  QFC(0x0010bc63), QFC(0x09b3d77f), QFC(0x68c7269b), QFC(0x0437fb0a), QFC(0x00a1039c),
  QFC(0x001471f8), QFC(0x09c0e59f), QFC(0x6959709c), QFC(0x04b0adcb), QFC(0x009d10bf),
  QFC(0x0018703f), QFC(0x09cab9f2), QFC(0x69e29783), QFC(0x05237f9d), QFC(0x0098b855),
  QFC(0x001c3549), QFC(0x09d19ca9), QFC(0x6a619c5e), QFC(0x0590a67d), QFC(0x009424c6),
  QFC(0x002064f8), QFC(0x09d52709), QFC(0x6ad73e8d), QFC(0x05f7fb90), QFC(0x008f4bfd),
  QFC(0x0024dd50), QFC(0x09d5560b), QFC(0x6b42a863), QFC(0x06593912), QFC(0x008a7dd7),
  QFC(0x00293718), QFC(0x09d1fa23), QFC(0x6ba4629e), QFC(0x06b559c3), QFC(0x0085c217),
  QFC(0x002d8e42), QFC(0x09caeb0f), QFC(0x6bfbdd97), QFC(0x070bbf58), QFC(0x00807994),
  QFC(0x00329ab6), QFC(0x09c018ce), QFC(0x6c492216), QFC(0x075ca90c), QFC(0x007b3875),
  QFC(0x003745f9), QFC(0x09b18a1d), QFC(0x6c8c4c79), QFC(0x07a8127d), QFC(0x0075fded),
  QFC(0x003c1fa4), QFC(0x099ec3dc), QFC(0x6cc59baa), QFC(0x07ee507c), QFC(0x0070c8a5),
  QFC(0x004103f5), QFC(0x09881dc5), QFC(0x6cf4073d), QFC(0x082f552e), QFC(0x006b47fa),
  QFC(0x00465348), QFC(0x096d0e21), QFC(0x6d18520d), QFC(0x086b1eec), QFC(0x0065fde5),
  QFC(0x004b6c46), QFC(0x094d7ec2), QFC(0x6d32730e), QFC(0x08a24899), QFC(0x006090c4),
  QFC(0x0050b177), QFC(0x09299ead), QFC(0x6d41d963), QFC(0x08d3e41b), QFC(0x005b5371),
  QFC(0x0055dba1), QFC(0x09015651), QFC(0x6d474e1d), QFC(0x09015651), QFC(0x0055dba1),
  QFC(0xfe4d1be3), QFC(0xd1c58ace), QFC(0x2e3a7532), QFC(0x01b2e41d), QFC(0x00000000),
};

RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_QTW qmf_phaseshift_cos_downsamp32[] =
{
  QTC(0x7fd8878e), QTC(0x7e9d55fc), QTC(0x7c29fbee), QTC(0x78848414), QTC(0x73b5ebd1), QTC(0x6dca0d14), QTC(0x66cf8120), QTC(0x5ed77c8a),
  QTC(0x55f5a4d2), QTC(0x4c3fdff4), QTC(0x41ce1e65), QTC(0x36ba2014), QTC(0x2b1f34eb), QTC(0x1f19f97b), QTC(0x12c8106f), QTC(0x0647d97c),
  QTC(0xf9b82684), QTC(0xed37ef91), QTC(0xe0e60685), QTC(0xd4e0cb15), QTC(0xc945dfec), QTC(0xbe31e19b), QTC(0xb3c0200c), QTC(0xaa0a5b2e),
  QTC(0xa1288376), QTC(0x99307ee0), QTC(0x9235f2ec), QTC(0x8c4a142f), QTC(0x877b7bec), QTC(0x83d60412), QTC(0x8162aa04), QTC(0x80277872),
};

RAM_ALIGN
LNK_SECTION_CONSTDATA
const FIXP_QTW qmf_phaseshift_sin_downsamp32[] =
{
  QTC(0x0647d97c), QTC(0x12c8106f), QTC(0x1f19f97b), QTC(0x2b1f34eb), QTC(0x36ba2014), QTC(0x41ce1e65), QTC(0x4c3fdff4), QTC(0x55f5a4d2),
  QTC(0x5ed77c8a), QTC(0x66cf8120), QTC(0x6dca0d14), QTC(0x73b5ebd1), QTC(0x78848414), QTC(0x7c29fbee), QTC(0x7e9d55fc), QTC(0x7fd8878e),
  QTC(0x7fd8878e), QTC(0x7e9d55fc), QTC(0x7c29fbee), QTC(0x78848414), QTC(0x73b5ebd1), QTC(0x6dca0d14), QTC(0x66cf8120), QTC(0x5ed77c8a),
  QTC(0x55f5a4d2), QTC(0x4c3fdff4), QTC(0x41ce1e65), QTC(0x36ba2014), QTC(0x2b1f34eb), QTC(0x1f19f97b), QTC(0x12c8106f), QTC(0x0647d97c),
};

#else /* QMF_NO_POLY == 5 */
#endif /* QMF_NO_POLY==5 */





/*
 * Low Delay QMF aka CLDFB
 */

#if defined(QMF_COEFF_16BIT)
#define QTCFLLD(x)  FL2FXCONST_SGL(x/(float)(1<<QMF_CLDFB_PFT_SCALE))
#define QTCFLLDT(x)  FL2FXCONST_SGL(x)
#else
#define QTCFLLD(x)  FL2FXCONST_DBL(x/(float)(1<<QMF_CLDFB_PFT_SCALE))
#define QTCFLLDT(x)  FL2FXCONST_DBL(x)
#endif


#ifndef LOW_POWER_SBR_ONLY
/*!
  \name QMF-Twiddle
  \brief QMF twiddle factors

  L=32, gain=2.0, angle = 0.75
*/
/* sin/cos (angle) / 2 */
const FIXP_QTW qmf_phaseshift_cos32_cldfb[32] = {
  QTCFLLDT(-7.071067e-01), QTCFLLDT(7.071070e-01), QTCFLLDT(7.071064e-01), QTCFLLDT(-7.071073e-01),
  QTCFLLDT(-7.071061e-01), QTCFLLDT(7.071076e-01), QTCFLLDT(7.071058e-01), QTCFLLDT(-7.071080e-01),
  QTCFLLDT(-7.071055e-01), QTCFLLDT(7.071083e-01), QTCFLLDT(7.071052e-01), QTCFLLDT(-7.071086e-01),
  QTCFLLDT(-7.071049e-01), QTCFLLDT(7.071089e-01), QTCFLLDT(7.071046e-01), QTCFLLDT(-7.071092e-01),
  QTCFLLDT(-7.071042e-01), QTCFLLDT(7.071095e-01), QTCFLLDT(7.071039e-01), QTCFLLDT(-7.071098e-01),
  QTCFLLDT(-7.071036e-01), QTCFLLDT(7.071101e-01), QTCFLLDT(7.071033e-01), QTCFLLDT(-7.071104e-01),
  QTCFLLDT(-7.071030e-01), QTCFLLDT(7.071107e-01), QTCFLLDT(7.071027e-01), QTCFLLDT(-7.071111e-01),
  QTCFLLDT(-7.071024e-01), QTCFLLDT(7.071114e-01), QTCFLLDT(7.071021e-01), QTCFLLDT(-7.071117e-01),
};

const FIXP_QTW qmf_phaseshift_sin32_cldfb[32] = {
  QTCFLLDT(7.071068e-01), QTCFLLDT(7.071065e-01), QTCFLLDT(-7.071072e-01), QTCFLLDT(-7.071062e-01),
  QTCFLLDT(7.071075e-01), QTCFLLDT(7.071059e-01), QTCFLLDT(-7.071078e-01), QTCFLLDT(-7.071056e-01),
  QTCFLLDT(7.071081e-01), QTCFLLDT(7.071053e-01), QTCFLLDT(-7.071084e-01), QTCFLLDT(-7.071050e-01),
  QTCFLLDT(7.071087e-01), QTCFLLDT(7.071047e-01), QTCFLLDT(-7.071090e-01), QTCFLLDT(-7.071044e-01),
  QTCFLLDT(7.071093e-01), QTCFLLDT(7.071041e-01), QTCFLLDT(-7.071096e-01), QTCFLLDT(-7.071038e-01),
  QTCFLLDT(7.071099e-01), QTCFLLDT(7.071034e-01), QTCFLLDT(-7.071103e-01), QTCFLLDT(-7.071031e-01),
  QTCFLLDT(7.071106e-01), QTCFLLDT(7.071028e-01), QTCFLLDT(-7.071109e-01), QTCFLLDT(-7.071025e-01),
  QTCFLLDT(7.071112e-01), QTCFLLDT(7.071022e-01), QTCFLLDT(-7.071115e-01), QTCFLLDT(-7.071019e-01),
};


/* sin/cos (angle) / 128 */
const FIXP_QTW qmf_phaseshift_cos64_cldfb[64] = {
  QTCFLLDT(7.071068e-01), QTCFLLDT(-7.071066e-01), QTCFLLDT(-7.071070e-01), QTCFLLDT(7.071065e-01),
  QTCFLLDT(7.071072e-01), QTCFLLDT(-7.071063e-01), QTCFLLDT(-7.071074e-01), QTCFLLDT(7.071061e-01),
  QTCFLLDT(7.071075e-01), QTCFLLDT(-7.071059e-01), QTCFLLDT(-7.071078e-01), QTCFLLDT(7.071057e-01),
  QTCFLLDT(7.071080e-01), QTCFLLDT(-7.071055e-01), QTCFLLDT(-7.071081e-01), QTCFLLDT(7.071053e-01),
  QTCFLLDT(7.071083e-01), QTCFLLDT(-7.071052e-01), QTCFLLDT(-7.071085e-01), QTCFLLDT(7.071050e-01),
  QTCFLLDT(7.071087e-01), QTCFLLDT(-7.071048e-01), QTCFLLDT(-7.071089e-01), QTCFLLDT(7.071046e-01),
  QTCFLLDT(7.071090e-01), QTCFLLDT(-7.071044e-01), QTCFLLDT(-7.071092e-01), QTCFLLDT(7.071042e-01),
  QTCFLLDT(7.071095e-01), QTCFLLDT(-7.071040e-01), QTCFLLDT(-7.071096e-01), QTCFLLDT(7.071038e-01),
  QTCFLLDT(7.071098e-01), QTCFLLDT(-7.071037e-01), QTCFLLDT(-7.071100e-01), QTCFLLDT(7.071035e-01),
  QTCFLLDT(7.071102e-01), QTCFLLDT(-7.071033e-01), QTCFLLDT(-7.071103e-01), QTCFLLDT(7.071031e-01),
  QTCFLLDT(7.071105e-01), QTCFLLDT(-7.071030e-01), QTCFLLDT(-7.071107e-01), QTCFLLDT(7.071028e-01),
  QTCFLLDT(7.071109e-01), QTCFLLDT(-7.071025e-01), QTCFLLDT(-7.071111e-01), QTCFLLDT(7.071024e-01),
  QTCFLLDT(7.071113e-01), QTCFLLDT(-7.071022e-01), QTCFLLDT(-7.071115e-01), QTCFLLDT(7.071020e-01),
  QTCFLLDT(7.071117e-01), QTCFLLDT(-7.071018e-01), QTCFLLDT(-7.071118e-01), QTCFLLDT(7.071016e-01),
  QTCFLLDT(7.071120e-01), QTCFLLDT(-7.071015e-01), QTCFLLDT(-7.071122e-01), QTCFLLDT(7.071013e-01),
  QTCFLLDT(7.071124e-01), QTCFLLDT(-7.071011e-01), QTCFLLDT(-7.071126e-01), QTCFLLDT(7.071009e-01),
 };
 const FIXP_QTW qmf_phaseshift_sin64_cldfb[64] = {
  QTCFLLDT(7.071067e-01), QTCFLLDT(7.071069e-01), QTCFLLDT(-7.071065e-01), QTCFLLDT(-7.071071e-01),
  QTCFLLDT(7.071064e-01), QTCFLLDT(7.071073e-01), QTCFLLDT(-7.071062e-01), QTCFLLDT(-7.071075e-01),
  QTCFLLDT(7.071060e-01), QTCFLLDT(7.071077e-01), QTCFLLDT(-7.071058e-01), QTCFLLDT(-7.071078e-01),
  QTCFLLDT(7.071056e-01), QTCFLLDT(7.071080e-01), QTCFLLDT(-7.071055e-01), QTCFLLDT(-7.071082e-01),
  QTCFLLDT(7.071053e-01), QTCFLLDT(7.071084e-01), QTCFLLDT(-7.071050e-01), QTCFLLDT(-7.071086e-01),
  QTCFLLDT(7.071049e-01), QTCFLLDT(7.071088e-01), QTCFLLDT(-7.071047e-01), QTCFLLDT(-7.071090e-01),
  QTCFLLDT(7.071045e-01), QTCFLLDT(7.071092e-01), QTCFLLDT(-7.071043e-01), QTCFLLDT(-7.071093e-01),
  QTCFLLDT(7.071041e-01), QTCFLLDT(7.071095e-01), QTCFLLDT(-7.071040e-01), QTCFLLDT(-7.071097e-01),
  QTCFLLDT(7.071038e-01), QTCFLLDT(7.071099e-01), QTCFLLDT(-7.071036e-01), QTCFLLDT(-7.071100e-01),
  QTCFLLDT(7.071034e-01), QTCFLLDT(7.071103e-01), QTCFLLDT(-7.071032e-01), QTCFLLDT(-7.071105e-01),
  QTCFLLDT(7.071030e-01), QTCFLLDT(7.071106e-01), QTCFLLDT(-7.071028e-01), QTCFLLDT(-7.071108e-01),
  QTCFLLDT(7.071027e-01), QTCFLLDT(7.071110e-01), QTCFLLDT(-7.071025e-01), QTCFLLDT(-7.071112e-01),
  QTCFLLDT(7.071023e-01), QTCFLLDT(7.071114e-01), QTCFLLDT(-7.071021e-01), QTCFLLDT(-7.071115e-01),
  QTCFLLDT(7.071019e-01), QTCFLLDT(7.071117e-01), QTCFLLDT(-7.071017e-01), QTCFLLDT(-7.071120e-01),
  QTCFLLDT(7.071015e-01), QTCFLLDT(7.071121e-01), QTCFLLDT(-7.071013e-01), QTCFLLDT(-7.071123e-01),
  QTCFLLDT(7.071012e-01), QTCFLLDT(7.071125e-01), QTCFLLDT(-7.071010e-01), QTCFLLDT(-7.071127e-01),
};

//@}

#endif /* #ifdef LOW_POWER_SBR_ONLY */



/*!
  \name QMF
  \brief  QMF-Table
          64 channels, N = 640, optimized by PE 010516

  The coeffs are rearranged compared with the reference in the following
  way:
  sbr_qmf_64[0] = sbr_qmf_64_reference[0];
  sbr_qmf_64[1] = sbr_qmf_64_reference[128];
  sbr_qmf_64[2] = sbr_qmf_64_reference[256];
  sbr_qmf_64[3] = sbr_qmf_64_reference[384];
  sbr_qmf_64[4] = sbr_qmf_64_reference[512];

  sbr_qmf_64[5] = sbr_qmf_64_reference[1];
  sbr_qmf_64[6] = sbr_qmf_64_reference[129];
  sbr_qmf_64[7] = sbr_qmf_64_reference[257];
  sbr_qmf_64[8] = sbr_qmf_64_reference[385];
  sbr_qmf_64[9] = sbr_qmf_64_reference[513];
  .
  .
  .
  sbr_qmf_64[635] = sbr_qmf_64_reference[127]
  sbr_qmf_64[636] = sbr_qmf_64_reference[255];
  sbr_qmf_64[637] = sbr_qmf_64_reference[383];
  sbr_qmf_64[638] = sbr_qmf_64_reference[511];
  sbr_qmf_64[639] = sbr_qmf_64_reference[639];


  Symmetric properties of qmf coeffs:

       Use point symmetry:

  sbr_qmf_64_640_qmf[320..634] = p_64_640_qmf[314..0]

  Max sum of all FIR filter absolute coefficients is: 0x7FF5B201
  thus, the filter output is not required to be scaled.

  \showinitializer
*/
//@{

LNK_SECTION_CONSTDATA_L1
RAM_ALIGN
const FIXP_PFT qmf_cldfb_640[QMF640_CLDFB_PFT_TABLE_SIZE] =
{
  QTCFLLD(6.571760e-07), QTCFLLD(-8.010079e-06), QTCFLLD(-1.250743e-03), QTCFLLD(8.996371e-03), QTCFLLD(5.128557e-01),
  QTCFLLD(4.118360e-07), QTCFLLD(-1.469933e-05), QTCFLLD(-1.194743e-03), QTCFLLD(9.640299e-03), QTCFLLD(5.299510e-01),
  QTCFLLD(8.109952e-07), QTCFLLD(4.840578e-06), QTCFLLD(-1.151796e-03), QTCFLLD(1.033126e-02), QTCFLLD(5.470652e-01),
  QTCFLLD(7.099633e-07), QTCFLLD(7.167101e-06), QTCFLLD(-1.099001e-03), QTCFLLD(1.106959e-02), QTCFLLD(5.641523e-01),
  QTCFLLD(6.834210e-07), QTCFLLD(1.088325e-05), QTCFLLD(-1.047655e-03), QTCFLLD(1.186211e-02), QTCFLLD(5.811993e-01),
  QTCFLLD(4.292862e-07), QTCFLLD(1.013260e-05), QTCFLLD(-9.862027e-04), QTCFLLD(1.270747e-02), QTCFLLD(5.981877e-01),
  QTCFLLD(-5.426597e-09), QTCFLLD(5.869707e-06), QTCFLLD(-9.294665e-04), QTCFLLD(1.361072e-02), QTCFLLD(6.151031e-01),
  QTCFLLD(6.355303e-08), QTCFLLD(1.125135e-05), QTCFLLD(-9.767709e-04), QTCFLLD(1.456209e-02), QTCFLLD(6.319284e-01),
  QTCFLLD(5.490570e-07), QTCFLLD(2.015445e-05), QTCFLLD(-1.040598e-03), QTCFLLD(1.557759e-02), QTCFLLD(6.486438e-01),
  QTCFLLD(1.620171e-06), QTCFLLD(2.800456e-05), QTCFLLD(-1.146268e-03), QTCFLLD(1.665188e-02), QTCFLLD(6.652304e-01),
  QTCFLLD(-6.025110e-10), QTCFLLD(8.975978e-06), QTCFLLD(-1.292866e-03), QTCFLLD(1.778249e-02), QTCFLLD(6.816668e-01),
  QTCFLLD(-6.325664e-10), QTCFLLD(8.563820e-06), QTCFLLD(-1.196638e-03), QTCFLLD(1.897506e-02), QTCFLLD(6.979337e-01),
  QTCFLLD(-4.013525e-09), QTCFLLD(1.168895e-05), QTCFLLD(-9.726699e-04), QTCFLLD(2.023525e-02), QTCFLLD(7.140087e-01),
  QTCFLLD(-4.244091e-09), QTCFLLD(7.300589e-06), QTCFLLD(-8.029620e-04), QTCFLLD(2.156305e-02), QTCFLLD(7.298746e-01),
  QTCFLLD(-1.846548e-08), QTCFLLD(3.965364e-06), QTCFLLD(-6.754936e-04), QTCFLLD(2.296471e-02), QTCFLLD(7.455112e-01),
  QTCFLLD(-3.870537e-09), QTCFLLD(1.374896e-06), QTCFLLD(-5.791145e-04), QTCFLLD(2.443434e-02), QTCFLLD(7.609051e-01),
  QTCFLLD(-8.883499e-10), QTCFLLD(3.798520e-07), QTCFLLD(-4.733148e-04), QTCFLLD(2.597957e-02), QTCFLLD(7.760386e-01),
  QTCFLLD(5.303528e-08), QTCFLLD(4.469729e-06), QTCFLLD(-2.998740e-04), QTCFLLD(2.760091e-02), QTCFLLD(7.908995e-01),
  QTCFLLD(7.391974e-08), QTCFLLD(2.461877e-05), QTCFLLD(7.882620e-05), QTCFLLD(2.931526e-02), QTCFLLD(8.054701e-01),
  QTCFLLD(1.723217e-09), QTCFLLD(4.005269e-05), QTCFLLD(4.708010e-04), QTCFLLD(3.110861e-02), QTCFLLD(8.197387e-01),
  QTCFLLD(2.443085e-07), QTCFLLD(5.272982e-05), QTCFLLD(8.089812e-04), QTCFLLD(3.298151e-02), QTCFLLD(8.336864e-01),
  QTCFLLD(1.387567e-08), QTCFLLD(4.939392e-05), QTCFLLD(1.127142e-03), QTCFLLD(3.493300e-02), QTCFLLD(8.472987e-01),
  QTCFLLD(-5.690531e-06), QTCFLLD(-4.256442e-05), QTCFLLD(1.417367e-03), QTCFLLD(3.696343e-02), QTCFLLD(8.605543e-01),
  QTCFLLD(3.629067e-06), QTCFLLD(6.582328e-05), QTCFLLD(1.725030e-03), QTCFLLD(3.907138e-02), QTCFLLD(8.734367e-01),
  QTCFLLD(-5.393556e-08), QTCFLLD(6.481921e-05), QTCFLLD(1.948069e-03), QTCFLLD(4.125570e-02), QTCFLLD(8.859232e-01),
  QTCFLLD(1.349944e-07), QTCFLLD(3.367998e-05), QTCFLLD(2.033465e-03), QTCFLLD(4.355568e-02), QTCFLLD(8.979959e-01),
  QTCFLLD(7.326611e-09), QTCFLLD(4.694252e-05), QTCFLLD(2.239143e-03), QTCFLLD(4.599068e-02), QTCFLLD(9.096311e-01),
  QTCFLLD(2.399696e-07), QTCFLLD(6.904415e-05), QTCFLLD(2.470456e-03), QTCFLLD(4.849285e-02), QTCFLLD(9.208195e-01),
  QTCFLLD(3.330982e-07), QTCFLLD(5.643103e-05), QTCFLLD(2.630472e-03), QTCFLLD(5.105621e-02), QTCFLLD(9.315442e-01),
  QTCFLLD(4.767794e-07), QTCFLLD(7.095887e-05), QTCFLLD(2.703019e-03), QTCFLLD(5.368313e-02), QTCFLLD(9.417976e-01),
  QTCFLLD(3.428661e-07), QTCFLLD(7.872593e-05), QTCFLLD(2.729137e-03), QTCFLLD(5.637219e-02), QTCFLLD(9.515675e-01),
  QTCFLLD(8.676848e-06), QTCFLLD(2.666445e-04), QTCFLLD(2.719749e-03), QTCFLLD(5.911363e-02), QTCFLLD(9.608520e-01),
  QTCFLLD(2.722296e-05), QTCFLLD(5.822201e-04), QTCFLLD(2.530907e-03), QTCFLLD(6.192693e-02), QTCFLLD(9.696426e-01),
  QTCFLLD(3.575651e-07), QTCFLLD(7.870355e-05), QTCFLLD(2.225524e-03), QTCFLLD(6.480449e-02), QTCFLLD(9.779405e-01),
  QTCFLLD(6.293002e-07), QTCFLLD(7.245096e-05), QTCFLLD(1.891972e-03), QTCFLLD(6.771675e-02), QTCFLLD(9.857388e-01),
  QTCFLLD(1.070243e-06), QTCFLLD(7.194151e-05), QTCFLLD(1.557112e-03), QTCFLLD(7.064948e-02), QTCFLLD(9.930380e-01),
  QTCFLLD(-3.225913e-07), QTCFLLD(-7.679955e-05), QTCFLLD(1.194731e-03), QTCFLLD(7.360559e-02), QTCFLLD(9.998286e-01),
  QTCFLLD(-9.597516e-09), QTCFLLD(-6.093373e-05), QTCFLLD(6.415402e-04), QTCFLLD(7.657650e-02), QTCFLLD(1.006109e+00),
  QTCFLLD(-8.908041e-08), QTCFLLD(-1.721347e-05), QTCFLLD(1.092526e-04), QTCFLLD(7.955571e-02), QTCFLLD(1.011868e+00),
  QTCFLLD(-2.285563e-05), QTCFLLD(-8.882305e-05), QTCFLLD(2.934876e-04), QTCFLLD(8.251962e-02), QTCFLLD(1.017100e+00),
  QTCFLLD(1.013575e-05), QTCFLLD(6.418658e-05), QTCFLLD(5.721223e-04), QTCFLLD(8.547716e-02), QTCFLLD(1.021799e+00),
  QTCFLLD(-1.706941e-05), QTCFLLD(1.769262e-04), QTCFLLD(6.976561e-04), QTCFLLD(8.841813e-02), QTCFLLD(1.025967e+00),
  QTCFLLD(1.356728e-06), QTCFLLD(2.206341e-05), QTCFLLD(7.376101e-04), QTCFLLD(9.133591e-02), QTCFLLD(1.029601e+00),
  QTCFLLD(-1.398913e-08), QTCFLLD(-6.538879e-06), QTCFLLD(7.154124e-04), QTCFLLD(9.421624e-02), QTCFLLD(1.032713e+00),
  QTCFLLD(3.552992e-08), QTCFLLD(-1.052707e-05), QTCFLLD(7.139920e-04), QTCFLLD(9.705240e-02), QTCFLLD(1.035312e+00),
  QTCFLLD(4.211177e-07), QTCFLLD(-9.075431e-06), QTCFLLD(6.944123e-04), QTCFLLD(9.982958e-02), QTCFLLD(1.037422e+00),
  QTCFLLD(5.433719e-07), QTCFLLD(-1.748285e-05), QTCFLLD(6.766320e-04), QTCFLLD(1.025398e-01), QTCFLLD(1.039062e+00),
  QTCFLLD(8.226600e-08), QTCFLLD(-3.498286e-05), QTCFLLD(6.887784e-04), QTCFLLD(1.051642e-01), QTCFLLD(1.040262e+00),
  QTCFLLD(1.272705e-07), QTCFLLD(-4.489491e-05), QTCFLLD(6.673250e-04), QTCFLLD(1.076972e-01), QTCFLLD(1.041043e+00),
  QTCFLLD(2.542598e-07), QTCFLLD(-5.449816e-05), QTCFLLD(5.970697e-04), QTCFLLD(1.101216e-01), QTCFLLD(1.041434e+00),
  QTCFLLD(6.322770e-07), QTCFLLD(-5.874199e-05), QTCFLLD(4.749931e-04), QTCFLLD(1.124296e-01), QTCFLLD(1.041443e+00),
  QTCFLLD(2.801882e-08), QTCFLLD(-7.934510e-05), QTCFLLD(3.189336e-04), QTCFLLD(1.146042e-01), QTCFLLD(1.041087e+00),
  QTCFLLD(5.891904e-07), QTCFLLD(-8.039232e-05), QTCFLLD(1.218226e-04), QTCFLLD(1.166399e-01), QTCFLLD(1.040350e+00),
  QTCFLLD(7.301957e-07), QTCFLLD(-9.907631e-05), QTCFLLD(-1.324292e-04), QTCFLLD(1.185243e-01), QTCFLLD(1.039228e+00),
  QTCFLLD(-4.518603e-06), QTCFLLD(-2.217025e-04), QTCFLLD(-4.268575e-04), QTCFLLD(1.202546e-01), QTCFLLD(1.037683e+00),
  QTCFLLD(-3.561585e-06), QTCFLLD(-2.415166e-04), QTCFLLD(-7.804546e-04), QTCFLLD(1.218184e-01), QTCFLLD(1.035694e+00),
  QTCFLLD(-1.074717e-07), QTCFLLD(-2.123672e-04), QTCFLLD(-1.156680e-03), QTCFLLD(1.232132e-01), QTCFLLD(1.033206e+00),
  QTCFLLD(1.323268e-06), QTCFLLD(-2.078299e-04), QTCFLLD(-1.525819e-03), QTCFLLD(1.244270e-01), QTCFLLD(1.030199e+00),
  QTCFLLD(3.377815e-06), QTCFLLD(-1.885286e-04), QTCFLLD(-1.914115e-03), QTCFLLD(1.254605e-01), QTCFLLD(1.026616e+00),
  QTCFLLD(5.161607e-06), QTCFLLD(-1.728673e-04), QTCFLLD(-2.292814e-03), QTCFLLD(1.262996e-01), QTCFLLD(1.022470e+00),
  QTCFLLD(5.924001e-06), QTCFLLD(-1.744842e-04), QTCFLLD(-2.658042e-03), QTCFLLD(1.269416e-01), QTCFLLD(1.017729e+00),
  QTCFLLD(6.310208e-06), QTCFLLD(-1.784193e-04), QTCFLLD(-3.000423e-03), QTCFLLD(1.273648e-01), QTCFLLD(1.012508e+00),
  QTCFLLD(3.357219e-06), QTCFLLD(-2.131406e-04), QTCFLLD(-3.318858e-03), QTCFLLD(1.275561e-01), QTCFLLD(1.006893e+00),
  QTCFLLD(5.189087e-06), QTCFLLD(-2.078886e-04), QTCFLLD(-3.597476e-03), QTCFLLD(1.274568e-01), QTCFLLD(1.001463e+00),
  QTCFLLD(4.178050e-06), QTCFLLD(-4.663778e-05), QTCFLLD(-3.870852e-03), QTCFLLD(1.273591e-01), QTCFLLD(9.927544e-01),
  QTCFLLD(5.364807e-06), QTCFLLD(-5.889277e-06), QTCFLLD(-4.135130e-03), QTCFLLD(1.272499e-01), QTCFLLD(9.807692e-01),
  QTCFLLD(4.083719e-06), QTCFLLD(-1.774108e-05), QTCFLLD(-4.351668e-03), QTCFLLD(1.268281e-01), QTCFLLD(9.690017e-01),
  QTCFLLD(3.567581e-06), QTCFLLD(-2.599468e-08), QTCFLLD(-4.517190e-03), QTCFLLD(1.261262e-01), QTCFLLD(9.568886e-01),
  QTCFLLD(3.262754e-06), QTCFLLD(1.260640e-05), QTCFLLD(-4.636228e-03), QTCFLLD(1.251477e-01), QTCFLLD(9.443803e-01),
  QTCFLLD(2.041128e-06), QTCFLLD(2.364519e-05), QTCFLLD(-4.704321e-03), QTCFLLD(1.238869e-01), QTCFLLD(9.313874e-01),
  QTCFLLD(-2.567965e-08), QTCFLLD(2.806963e-05), QTCFLLD(-4.722568e-03), QTCFLLD(1.223371e-01), QTCFLLD(9.179666e-01),
  QTCFLLD(2.714879e-07), QTCFLLD(4.493916e-05), QTCFLLD(-4.663276e-03), QTCFLLD(1.204854e-01), QTCFLLD(9.041286e-01),
  QTCFLLD(2.150884e-06), QTCFLLD(5.408155e-05), QTCFLLD(-4.554811e-03), QTCFLLD(1.183233e-01), QTCFLLD(8.899474e-01),
  QTCFLLD(5.818595e-06), QTCFLLD(3.759630e-05), QTCFLLD(-4.369554e-03), QTCFLLD(1.158359e-01), QTCFLLD(8.754641e-01),
  QTCFLLD(-1.686137e-09), QTCFLLD(2.515118e-05), QTCFLLD(-4.091033e-03), QTCFLLD(1.130180e-01), QTCFLLD(8.607492e-01),
  QTCFLLD(-1.775191e-09), QTCFLLD(2.406517e-05), QTCFLLD(-3.794425e-03), QTCFLLD(1.098551e-01), QTCFLLD(8.458450e-01),
  QTCFLLD(-2.222072e-09), QTCFLLD(3.628511e-05), QTCFLLD(-3.460363e-03), QTCFLLD(1.063455e-01), QTCFLLD(8.308040e-01),
  QTCFLLD(-1.280675e-08), QTCFLLD(2.241546e-05), QTCFLLD(-3.064311e-03), QTCFLLD(1.024805e-01), QTCFLLD(8.156523e-01),
  QTCFLLD(-6.977078e-08), QTCFLLD(1.499170e-05), QTCFLLD(-2.621537e-03), QTCFLLD(9.826251e-02), QTCFLLD(8.004165e-01),
  QTCFLLD(-1.409927e-08), QTCFLLD(5.009913e-06), QTCFLLD(-2.124648e-03), QTCFLLD(9.368652e-02), QTCFLLD(7.851012e-01),
  QTCFLLD(-2.986489e-09), QTCFLLD(1.277184e-06), QTCFLLD(-1.594861e-03), QTCFLLD(8.875756e-02), QTCFLLD(7.697093e-01),
  QTCFLLD(1.876022e-07), QTCFLLD(1.580189e-05), QTCFLLD(-1.061499e-03), QTCFLLD(8.347151e-02), QTCFLLD(7.542294e-01),
  QTCFLLD(1.737277e-07), QTCFLLD(5.533953e-05), QTCFLLD(-6.169855e-04), QTCFLLD(7.783300e-02), QTCFLLD(7.386515e-01),
  QTCFLLD(3.818589e-09), QTCFLLD(8.870182e-05), QTCFLLD(-2.004823e-04), QTCFLLD(7.184074e-02), QTCFLLD(7.229599e-01),
  QTCFLLD(5.143615e-07), QTCFLLD(1.035783e-04), QTCFLLD(2.048499e-04), QTCFLLD(6.550209e-02), QTCFLLD(7.071448e-01),
  QTCFLLD(2.820292e-08), QTCFLLD(9.990758e-05), QTCFLLD(5.621721e-04), QTCFLLD(5.881297e-02), QTCFLLD(6.911982e-01),
  QTCFLLD(4.677016e-06), QTCFLLD(1.181078e-04), QTCFLLD(9.373975e-04), QTCFLLD(5.177965e-02), QTCFLLD(6.751199e-01),
  QTCFLLD(3.361682e-06), QTCFLLD(2.126365e-05), QTCFLLD(1.344657e-03), QTCFLLD(4.439684e-02), QTCFLLD(6.589149e-01),
  QTCFLLD(-4.880845e-08), QTCFLLD(5.861800e-05), QTCFLLD(1.812176e-03), QTCFLLD(3.666943e-02), QTCFLLD(6.425940e-01),
  QTCFLLD(2.267731e-07), QTCFLLD(5.021906e-05), QTCFLLD(2.172866e-03), QTCFLLD(2.857528e-02), QTCFLLD(6.261725e-01),
  QTCFLLD(5.158213e-09), QTCFLLD(4.150075e-05), QTCFLLD(1.985825e-03), QTCFLLD(2.012237e-02), QTCFLLD(6.096690e-01),
  QTCFLLD(-2.066962e-07), QTCFLLD(3.799972e-05), QTCFLLD(1.697653e-03), QTCFLLD(1.132324e-02), QTCFLLD(5.930982e-01),
  QTCFLLD(4.883305e-07), QTCFLLD(6.606462e-05), QTCFLLD(1.471167e-03), QTCFLLD(2.184257e-03), QTCFLLD(5.764735e-01),
  QTCFLLD(8.254430e-07), QTCFLLD(9.755685e-05), QTCFLLD(1.232134e-03), QTCFLLD(-7.298198e-03), QTCFLLD(5.598052e-01),
  QTCFLLD(9.464783e-07), QTCFLLD(1.831121e-04), QTCFLLD(8.990256e-04), QTCFLLD(-1.711324e-02), QTCFLLD(5.430990e-01),
  QTCFLLD(-1.232693e-05), QTCFLLD(-5.901618e-07), QTCFLLD(6.150317e-04), QTCFLLD(-2.726484e-02), QTCFLLD(5.263554e-01),
  QTCFLLD(3.867483e-05), QTCFLLD(-3.595054e-04), QTCFLLD(6.307841e-04), QTCFLLD(-3.775928e-02), QTCFLLD(5.095721e-01),
  QTCFLLD(-9.870548e-07), QTCFLLD(-1.815837e-04), QTCFLLD(4.366447e-04), QTCFLLD(-4.859006e-02), QTCFLLD(4.927464e-01),
  QTCFLLD(-1.089501e-06), QTCFLLD(-9.204876e-05), QTCFLLD(1.498232e-04), QTCFLLD(-5.973742e-02), QTCFLLD(4.758754e-01),
  QTCFLLD(-1.569003e-06), QTCFLLD(-5.192444e-05), QTCFLLD(-9.099723e-05), QTCFLLD(-7.120357e-02), QTCFLLD(4.589583e-01),
  QTCFLLD(-2.778618e-07), QTCFLLD(6.487880e-05), QTCFLLD(-3.337967e-04), QTCFLLD(-8.298103e-02), QTCFLLD(4.420014e-01),
  QTCFLLD(6.757015e-09), QTCFLLD(5.397065e-05), QTCFLLD(-5.599348e-04), QTCFLLD(-9.506967e-02), QTCFLLD(4.250144e-01),
  QTCFLLD(1.496436e-07), QTCFLLD(2.472024e-05), QTCFLLD(-7.677634e-04), QTCFLLD(-1.074631e-01), QTCFLLD(4.080155e-01),
  QTCFLLD(2.068297e-05), QTCFLLD(9.711682e-05), QTCFLLD(-9.730460e-04), QTCFLLD(-1.201629e-01), QTCFLLD(3.910244e-01),
  QTCFLLD(-9.388963e-06), QTCFLLD(5.144969e-05), QTCFLLD(-1.131860e-03), QTCFLLD(-1.331545e-01), QTCFLLD(3.740644e-01),
  QTCFLLD(-1.402925e-05), QTCFLLD(-1.039264e-04), QTCFLLD(-1.283281e-03), QTCFLLD(-1.464389e-01), QTCFLLD(3.571528e-01),
  QTCFLLD(-2.757611e-06), QTCFLLD(2.853437e-06), QTCFLLD(-1.480543e-03), QTCFLLD(-1.600062e-01), QTCFLLD(3.403074e-01),
  QTCFLLD(2.945239e-08), QTCFLLD(1.334091e-05), QTCFLLD(-1.699161e-03), QTCFLLD(-1.738542e-01), QTCFLLD(3.235299e-01),
  QTCFLLD(-7.873304e-08), QTCFLLD(2.443161e-05), QTCFLLD(-1.924845e-03), QTCFLLD(-1.879712e-01), QTCFLLD(3.068187e-01),
  QTCFLLD(-9.897194e-07), QTCFLLD(3.568555e-05), QTCFLLD(-2.152380e-03), QTCFLLD(-2.023548e-01), QTCFLLD(2.901491e-01),
  QTCFLLD(-1.922074e-06), QTCFLLD(6.193370e-05), QTCFLLD(-2.396404e-03), QTCFLLD(-2.169926e-01), QTCFLLD(2.734977e-01),
  QTCFLLD(-2.765650e-07), QTCFLLD(1.176237e-04), QTCFLLD(-2.653819e-03), QTCFLLD(-2.318815e-01), QTCFLLD(2.568176e-01),
  QTCFLLD(-4.636105e-07), QTCFLLD(1.635906e-04), QTCFLLD(-2.927159e-03), QTCFLLD(-2.470098e-01), QTCFLLD(2.400768e-01),
  QTCFLLD(-9.607069e-07), QTCFLLD(2.060394e-04), QTCFLLD(-3.209093e-03), QTCFLLD(-2.623749e-01), QTCFLLD(2.232277e-01),
  QTCFLLD(-1.907927e-06), QTCFLLD(2.346981e-04), QTCFLLD(-3.505531e-03), QTCFLLD(-2.779638e-01), QTCFLLD(2.062605e-01),
  QTCFLLD(-1.551251e-08), QTCFLLD(2.520607e-04), QTCFLLD(-3.811612e-03), QTCFLLD(-2.937725e-01), QTCFLLD(1.891590e-01),
  QTCFLLD(-1.653464e-06), QTCFLLD(2.556450e-04), QTCFLLD(-4.133640e-03), QTCFLLD(-3.097862e-01), QTCFLLD(1.719726e-01),
  QTCFLLD(-2.043464e-06), QTCFLLD(3.157664e-04), QTCFLLD(-4.448993e-03), QTCFLLD(-3.259994e-01), QTCFLLD(1.547461e-01),
  QTCFLLD(1.622786e-05), QTCFLLD(6.205676e-04), QTCFLLD(-4.754192e-03), QTCFLLD(-3.423942e-01), QTCFLLD(1.376150e-01),
  QTCFLLD(1.395221e-05), QTCFLLD(7.847840e-04), QTCFLLD(-5.063851e-03), QTCFLLD(-3.589627e-01), QTCFLLD(1.206924e-01),
  QTCFLLD(4.591010e-07), QTCFLLD(9.019129e-04), QTCFLLD(-5.394570e-03), QTCFLLD(-3.756822e-01), QTCFLLD(1.042033e-01),
  QTCFLLD(-6.261944e-06), QTCFLLD(1.054963e-03), QTCFLLD(-5.741103e-03), QTCFLLD(-3.925409e-01), QTCFLLD(8.829745e-02),
  QTCFLLD(-1.606051e-05), QTCFLLD(1.089429e-03), QTCFLLD(-6.109179e-03), QTCFLLD(-4.095160e-01), QTCFLLD(7.325979e-02),
  QTCFLLD(-2.464228e-05), QTCFLLD(1.122503e-03), QTCFLLD(-6.500503e-03), QTCFLLD(-4.265950e-01), QTCFLLD(5.918678e-02),
  QTCFLLD(-2.976824e-05), QTCFLLD(1.177515e-03), QTCFLLD(-6.925141e-03), QTCFLLD(-4.437530e-01), QTCFLLD(4.634696e-02),
  QTCFLLD(-3.177468e-05), QTCFLLD(1.226113e-03), QTCFLLD(-7.380544e-03), QTCFLLD(-4.609829e-01), QTCFLLD(3.450719e-02),
  QTCFLLD(-4.373302e-05), QTCFLLD(1.263569e-03), QTCFLLD(-7.876393e-03), QTCFLLD(-4.782650e-01), QTCFLLD(2.353060e-02),
  QTCFLLD(-3.299004e-05), QTCFLLD(1.287819e-03), QTCFLLD(-8.407749e-03), QTCFLLD(-4.956175e-01), QTCFLLD(1.129580e-02),
};

RAM_ALIGN
const FIXP_PFT qmf_cldfb_320[QMF320_CLDFB_PFT_TABLE_SIZE] =
{
QTCFLLD(5.345060e-07), QTCFLLD(-1.135471e-05), QTCFLLD(-1.222743e-03), QTCFLLD(9.318335e-03), QTCFLLD(5.214033e-01),
QTCFLLD(7.604792e-07), QTCFLLD(6.003839e-06), QTCFLLD(-1.125398e-03), QTCFLLD(1.070043e-02), QTCFLLD(5.556087e-01),
QTCFLLD(5.563536e-07), QTCFLLD(1.050792e-05), QTCFLLD(-1.016929e-03), QTCFLLD(1.228479e-02), QTCFLLD(5.896935e-01),
QTCFLLD(2.906322e-08), QTCFLLD(8.560527e-06), QTCFLLD(-9.531187e-04), QTCFLLD(1.408640e-02), QTCFLLD(6.235157e-01),
QTCFLLD(1.084614e-06), QTCFLLD(2.407951e-05), QTCFLLD(-1.093433e-03), QTCFLLD(1.611474e-02), QTCFLLD(6.569371e-01),
QTCFLLD(-6.175387e-10), QTCFLLD(8.769899e-06), QTCFLLD(-1.244752e-03), QTCFLLD(1.837877e-02), QTCFLLD(6.898003e-01),
QTCFLLD(-4.128808e-09), QTCFLLD(9.494767e-06), QTCFLLD(-8.878160e-04), QTCFLLD(2.089915e-02), QTCFLLD(7.219416e-01),
QTCFLLD(-1.116801e-08), QTCFLLD(2.670130e-06), QTCFLLD(-6.273041e-04), QTCFLLD(2.369952e-02), QTCFLLD(7.532082e-01),
QTCFLLD(2.607347e-08), QTCFLLD(2.424790e-06), QTCFLLD(-3.865944e-04), QTCFLLD(2.679024e-02), QTCFLLD(7.834691e-01),
QTCFLLD(3.782148e-08), QTCFLLD(3.233573e-05), QTCFLLD(2.748136e-04), QTCFLLD(3.021193e-02), QTCFLLD(8.126044e-01),
QTCFLLD(1.290921e-07), QTCFLLD(5.106187e-05), QTCFLLD(9.680615e-04), QTCFLLD(3.395726e-02), QTCFLLD(8.404925e-01),
QTCFLLD(-1.030732e-06), QTCFLLD(1.162943e-05), QTCFLLD(1.571198e-03), QTCFLLD(3.801740e-02), QTCFLLD(8.669955e-01),
QTCFLLD(4.052940e-08), QTCFLLD(4.924960e-05), QTCFLLD(1.990767e-03), QTCFLLD(4.240569e-02), QTCFLLD(8.919595e-01),
QTCFLLD(1.236481e-07), QTCFLLD(5.799333e-05), QTCFLLD(2.354800e-03), QTCFLLD(4.724177e-02), QTCFLLD(9.152253e-01),
QTCFLLD(4.049388e-07), QTCFLLD(6.369496e-05), QTCFLLD(2.666746e-03), QTCFLLD(5.236967e-02), QTCFLLD(9.366709e-01),
QTCFLLD(4.509857e-06), QTCFLLD(1.726852e-04), QTCFLLD(2.724443e-03), QTCFLLD(5.774291e-02), QTCFLLD(9.562097e-01),
QTCFLLD(1.379026e-05), QTCFLLD(3.304619e-04), QTCFLLD(2.378216e-03), QTCFLLD(6.336571e-02), QTCFLLD(9.737916e-01),
QTCFLLD(8.497715e-07), QTCFLLD(7.219624e-05), QTCFLLD(1.724542e-03), QTCFLLD(6.918311e-02), QTCFLLD(9.893883e-01),
QTCFLLD(-1.660944e-07), QTCFLLD(-6.886664e-05), QTCFLLD(9.181354e-04), QTCFLLD(7.509105e-02), QTCFLLD(1.002969e+00),
QTCFLLD(-1.147235e-05), QTCFLLD(-5.301826e-05), QTCFLLD(2.013701e-04), QTCFLLD(8.103766e-02), QTCFLLD(1.014484e+00),
QTCFLLD(-3.466829e-06), QTCFLLD(1.205564e-04), QTCFLLD(6.348892e-04), QTCFLLD(8.694765e-02), QTCFLLD(1.023883e+00),
QTCFLLD(6.713692e-07), QTCFLLD(7.762268e-06), QTCFLLD(7.265112e-04), QTCFLLD(9.277608e-02), QTCFLLD(1.031157e+00),
QTCFLLD(2.283238e-07), QTCFLLD(-9.801253e-06), QTCFLLD(7.042022e-04), QTCFLLD(9.844099e-02), QTCFLLD(1.036367e+00),
QTCFLLD(3.128189e-07), QTCFLLD(-2.623285e-05), QTCFLLD(6.827052e-04), QTCFLLD(1.038520e-01), QTCFLLD(1.039662e+00),
QTCFLLD(1.907652e-07), QTCFLLD(-4.969654e-05), QTCFLLD(6.321974e-04), QTCFLLD(1.089094e-01), QTCFLLD(1.041239e+00),
QTCFLLD(3.301479e-07), QTCFLLD(-6.904354e-05), QTCFLLD(3.969634e-04), QTCFLLD(1.135169e-01), QTCFLLD(1.041265e+00),
QTCFLLD(6.596931e-07), QTCFLLD(-8.973431e-05), QTCFLLD(-5.303260e-06), QTCFLLD(1.175821e-01), QTCFLLD(1.039789e+00),
QTCFLLD(-4.040094e-06), QTCFLLD(-2.316096e-04), QTCFLLD(-6.036561e-04), QTCFLLD(1.210365e-01), QTCFLLD(1.036689e+00),
QTCFLLD(6.078980e-07), QTCFLLD(-2.100985e-04), QTCFLLD(-1.341249e-03), QTCFLLD(1.238201e-01), QTCFLLD(1.031702e+00),
QTCFLLD(4.269711e-06), QTCFLLD(-1.806979e-04), QTCFLLD(-2.103464e-03), QTCFLLD(1.258800e-01), QTCFLLD(1.024543e+00),
QTCFLLD(6.117105e-06), QTCFLLD(-1.764517e-04), QTCFLLD(-2.829232e-03), QTCFLLD(1.271532e-01), QTCFLLD(1.015119e+00),
QTCFLLD(4.273153e-06), QTCFLLD(-2.105146e-04), QTCFLLD(-3.458167e-03), QTCFLLD(1.275064e-01), QTCFLLD(1.004178e+00),
QTCFLLD(4.771428e-06), QTCFLLD(-2.626353e-05), QTCFLLD(-4.002991e-03), QTCFLLD(1.273045e-01), QTCFLLD(9.867618e-01),
QTCFLLD(3.825650e-06), QTCFLLD(-8.883540e-06), QTCFLLD(-4.434429e-03), QTCFLLD(1.264771e-01), QTCFLLD(9.629451e-01),
QTCFLLD(2.651941e-06), QTCFLLD(1.812579e-05), QTCFLLD(-4.670274e-03), QTCFLLD(1.245173e-01), QTCFLLD(9.378839e-01),
QTCFLLD(1.229041e-07), QTCFLLD(3.650440e-05), QTCFLLD(-4.692922e-03), QTCFLLD(1.214113e-01), QTCFLLD(9.110476e-01),
QTCFLLD(3.984739e-06), QTCFLLD(4.583892e-05), QTCFLLD(-4.462183e-03), QTCFLLD(1.170796e-01), QTCFLLD(8.827057e-01),
QTCFLLD(-1.730664e-09), QTCFLLD(2.460818e-05), QTCFLLD(-3.942729e-03), QTCFLLD(1.114366e-01), QTCFLLD(8.532971e-01),
QTCFLLD(-7.514413e-09), QTCFLLD(2.935029e-05), QTCFLLD(-3.262337e-03), QTCFLLD(1.044130e-01), QTCFLLD(8.232281e-01),
QTCFLLD(-4.193503e-08), QTCFLLD(1.000081e-05), QTCFLLD(-2.373092e-03), QTCFLLD(9.597452e-02), QTCFLLD(7.927589e-01),
QTCFLLD(9.230786e-08), QTCFLLD(8.539538e-06), QTCFLLD(-1.328180e-03), QTCFLLD(8.611453e-02), QTCFLLD(7.619694e-01),
QTCFLLD(8.877312e-08), QTCFLLD(7.202067e-05), QTCFLLD(-4.087339e-04), QTCFLLD(7.483687e-02), QTCFLLD(7.308058e-01),
QTCFLLD(2.712822e-07), QTCFLLD(1.017429e-04), QTCFLLD(3.835110e-04), QTCFLLD(6.215753e-02), QTCFLLD(6.991715e-01),
QTCFLLD(4.019349e-06), QTCFLLD(6.968570e-05), QTCFLLD(1.141027e-03), QTCFLLD(4.808825e-02), QTCFLLD(6.670174e-01),
QTCFLLD(8.898233e-08), QTCFLLD(5.441853e-05), QTCFLLD(1.992521e-03), QTCFLLD(3.262236e-02), QTCFLLD(6.343833e-01),
QTCFLLD(-1.007690e-07), QTCFLLD(3.975024e-05), QTCFLLD(1.841739e-03), QTCFLLD(1.572281e-02), QTCFLLD(6.013836e-01),
QTCFLLD(6.568868e-07), QTCFLLD(8.181074e-05), QTCFLLD(1.351651e-03), QTCFLLD(-2.556970e-03), QTCFLLD(5.681393e-01),
QTCFLLD(-5.690228e-06), QTCFLLD(9.126098e-05), QTCFLLD(7.570286e-04), QTCFLLD(-2.218904e-02), QTCFLLD(5.347272e-01),
QTCFLLD(1.884389e-05), QTCFLLD(-2.705446e-04), QTCFLLD(5.337144e-04), QTCFLLD(-4.317467e-02), QTCFLLD(5.011593e-01),
QTCFLLD(-1.329252e-06), QTCFLLD(-7.198660e-05), QTCFLLD(2.941296e-05), QTCFLLD(-6.547049e-02), QTCFLLD(4.674168e-01),
QTCFLLD(-1.355524e-07), QTCFLLD(5.942472e-05), QTCFLLD(-4.468657e-04), QTCFLLD(-8.902535e-02), QTCFLLD(4.335079e-01),
QTCFLLD(1.041631e-05), QTCFLLD(6.091853e-05), QTCFLLD(-8.704047e-04), QTCFLLD(-1.138130e-01), QTCFLLD(3.995200e-01),
QTCFLLD(-1.170911e-05), QTCFLLD(-2.623833e-05), QTCFLLD(-1.207570e-03), QTCFLLD(-1.397967e-01), QTCFLLD(3.656086e-01),
QTCFLLD(-1.364079e-06), QTCFLLD(8.097173e-06), QTCFLLD(-1.589852e-03), QTCFLLD(-1.669302e-01), QTCFLLD(3.319187e-01),
QTCFLLD(-5.342262e-07), QTCFLLD(3.005858e-05), QTCFLLD(-2.038612e-03), QTCFLLD(-1.951630e-01), QTCFLLD(2.984839e-01),
QTCFLLD(-1.099320e-06), QTCFLLD(8.977871e-05), QTCFLLD(-2.525111e-03), QTCFLLD(-2.244371e-01), QTCFLLD(2.651577e-01),
QTCFLLD(-7.121587e-07), QTCFLLD(1.848150e-04), QTCFLLD(-3.068126e-03), QTCFLLD(-2.546924e-01), QTCFLLD(2.316523e-01),
QTCFLLD(-9.617199e-07), QTCFLLD(2.433794e-04), QTCFLLD(-3.658572e-03), QTCFLLD(-2.858681e-01), QTCFLLD(1.977098e-01),
QTCFLLD(-1.848464e-06), QTCFLLD(2.857057e-04), QTCFLLD(-4.291316e-03), QTCFLLD(-3.178928e-01), QTCFLLD(1.633594e-01),
QTCFLLD(1.509004e-05), QTCFLLD(7.026758e-04), QTCFLLD(-4.909021e-03), QTCFLLD(-3.506784e-01), QTCFLLD(1.291537e-01),
QTCFLLD(-2.901422e-06), QTCFLLD(9.784381e-04), QTCFLLD(-5.567837e-03), QTCFLLD(-3.841116e-01), QTCFLLD(9.625038e-02),
QTCFLLD(-2.035140e-05), QTCFLLD(1.105966e-03), QTCFLLD(-6.304841e-03), QTCFLLD(-4.180555e-01), QTCFLLD(6.622328e-02),
QTCFLLD(-3.077146e-05), QTCFLLD(1.201814e-03), QTCFLLD(-7.152842e-03), QTCFLLD(-4.523680e-01), QTCFLLD(4.042707e-02),
QTCFLLD(-3.836153e-05), QTCFLLD(1.275694e-03), QTCFLLD(-8.142071e-03), QTCFLLD(-4.869413e-01), QTCFLLD(1.741320e-02),
};






//@{
/*!
  \name DCT_II twiddle factors, L=64
*/
/*! sin (3.14159265358979323 / (2*L) * n) , L=64*/
LNK_SECTION_CONSTDATA
RAM_ALIGN
const FIXP_WTP sin_twiddle_L64[]=
{
  WTCP(0x7fffffff, 0x00000000), WTCP(0x7ff62182, 0x03242abf), WTCP(0x7fd8878e, 0x0647d97c), WTCP(0x7fa736b4, 0x096a9049),
  WTCP(0x7f62368f, 0x0c8bd35e), WTCP(0x7f0991c4, 0x0fab272b), WTCP(0x7e9d55fc, 0x12c8106f), WTCP(0x7e1d93ea, 0x15e21445),
  WTCP(0x7d8a5f40, 0x18f8b83c), WTCP(0x7ce3ceb2, 0x1c0b826a), WTCP(0x7c29fbee, 0x1f19f97b), WTCP(0x7b5d039e, 0x2223a4c5),
  WTCP(0x7a7d055b, 0x25280c5e), WTCP(0x798a23b1, 0x2826b928), WTCP(0x78848414, 0x2b1f34eb), WTCP(0x776c4edb, 0x2e110a62),
  WTCP(0x7641af3d, 0x30fbc54d), WTCP(0x7504d345, 0x33def287), WTCP(0x73b5ebd1, 0x36ba2014), WTCP(0x72552c85, 0x398cdd32),
  WTCP(0x70e2cbc6, 0x3c56ba70), WTCP(0x6f5f02b2, 0x3f1749b8), WTCP(0x6dca0d14, 0x41ce1e65), WTCP(0x6c242960, 0x447acd50),
  WTCP(0x6a6d98a4, 0x471cece7), WTCP(0x68a69e81, 0x49b41533), WTCP(0x66cf8120, 0x4c3fdff4), WTCP(0x64e88926, 0x4ebfe8a5),
  WTCP(0x62f201ac, 0x5133cc94), WTCP(0x60ec3830, 0x539b2af0), WTCP(0x5ed77c8a, 0x55f5a4d2), WTCP(0x5cb420e0, 0x5842dd54),
  WTCP(0x5a82799a, 0x5a82799a)
  , WTCP(0x5842dd54, 0x5cb420e0), WTCP(0x55f5a4d2, 0x5ed77c8a), WTCP(0x539b2af0, 0x60ec3830),
  WTCP(0x5133cc94, 0x62f201ac), WTCP(0x4ebfe8a5, 0x64e88926), WTCP(0x4c3fdff4, 0x66cf8120), WTCP(0x49b41533, 0x68a69e81),
  WTCP(0x471cece7, 0x6a6d98a4), WTCP(0x447acd50, 0x6c242960), WTCP(0x41ce1e65, 0x6dca0d14), WTCP(0x3f1749b8, 0x6f5f02b2),
  WTCP(0x3c56ba70, 0x70e2cbc6), WTCP(0x398cdd32, 0x72552c85), WTCP(0x36ba2014, 0x73b5ebd1), WTCP(0x33def287, 0x7504d345),
  WTCP(0x30fbc54d, 0x7641af3d), WTCP(0x2e110a62, 0x776c4edb), WTCP(0x2b1f34eb, 0x78848414), WTCP(0x2826b928, 0x798a23b1),
  WTCP(0x25280c5e, 0x7a7d055b), WTCP(0x2223a4c5, 0x7b5d039e), WTCP(0x1f19f97b, 0x7c29fbee), WTCP(0x1c0b826a, 0x7ce3ceb2),
  WTCP(0x18f8b83c, 0x7d8a5f40), WTCP(0x15e21445, 0x7e1d93ea), WTCP(0x12c8106f, 0x7e9d55fc), WTCP(0x0fab272b, 0x7f0991c4),
  WTCP(0x0c8bd35e, 0x7f62368f), WTCP(0x096a9049, 0x7fa736b4), WTCP(0x0647d97c, 0x7fd8878e), WTCP(0x03242abf, 0x7ff62182)
};

LNK_SECTION_CONSTDATA
RAM_ALIGN
const FIXP_WTP SineWindow64[] =
{
  WTCP(0x7ffd885a, 0x01921d20), WTCP(0x7fe9cbc0, 0x04b6195d), WTCP(0x7fc25596, 0x07d95b9e), WTCP(0x7f872bf3, 0x0afb6805),
  WTCP(0x7f3857f6, 0x0e1bc2e4), WTCP(0x7ed5e5c6, 0x1139f0cf), WTCP(0x7e5fe493, 0x145576b1), WTCP(0x7dd6668f, 0x176dd9de),
  WTCP(0x7d3980ec, 0x1a82a026), WTCP(0x7c894bde, 0x1d934fe5), WTCP(0x7bc5e290, 0x209f701c), WTCP(0x7aef6323, 0x23a6887f),
  WTCP(0x7a05eead, 0x26a82186), WTCP(0x7909a92d, 0x29a3c485), WTCP(0x77fab989, 0x2c98fbba), WTCP(0x76d94989, 0x2f875262),
  WTCP(0x75a585cf, 0x326e54c7), WTCP(0x745f9dd1, 0x354d9057), WTCP(0x7307c3d0, 0x382493b0), WTCP(0x719e2cd2, 0x3af2eeb7),
  WTCP(0x7023109a, 0x3db832a6), WTCP(0x6e96a99d, 0x4073f21d), WTCP(0x6cf934fc, 0x4325c135), WTCP(0x6b4af279, 0x45cd358f),
  WTCP(0x698c246c, 0x4869e665), WTCP(0x67bd0fbd, 0x4afb6c98), WTCP(0x65ddfbd3, 0x4d8162c4), WTCP(0x63ef3290, 0x4ffb654d),
  WTCP(0x61f1003f, 0x5269126e), WTCP(0x5fe3b38d, 0x54ca0a4b), WTCP(0x5dc79d7c, 0x571deefa), WTCP(0x5b9d1154, 0x59646498),
};


LNK_SECTION_CONSTDATA
RAM_ALIGN
const FIXP_WTP SineWindow32[] =
{
  WTCP(0x7ff62182, 0x03242abf), WTCP(0x7fa736b4, 0x096a9049), WTCP(0x7f0991c4, 0x0fab272b), WTCP(0x7e1d93ea, 0x15e21445),
  WTCP(0x7ce3ceb2, 0x1c0b826a), WTCP(0x7b5d039e, 0x2223a4c5), WTCP(0x798a23b1, 0x2826b928), WTCP(0x776c4edb, 0x2e110a62),
  WTCP(0x7504d345, 0x33def287), WTCP(0x72552c85, 0x398cdd32), WTCP(0x6f5f02b2, 0x3f1749b8), WTCP(0x6c242960, 0x447acd50),
  WTCP(0x68a69e81, 0x49b41533), WTCP(0x64e88926, 0x4ebfe8a5), WTCP(0x60ec3830, 0x539b2af0), WTCP(0x5cb420e0, 0x5842dd54),
};








const USHORT sqrt_tab[49]={
0x5a82, 0x5d4b, 0x6000, 0x62a1,
0x6531, 0x67b1, 0x6a21, 0x6c84,
0x6ed9, 0x7123, 0x7360, 0x7593,
0x77bb, 0x79da, 0x7bef, 0x7dfb,
0x8000, 0x81fc, 0x83f0, 0x85dd,
0x87c3, 0x89a3, 0x8b7c, 0x8d4e,
0x8f1b, 0x90e2, 0x92a4, 0x9460,
0x9617, 0x97ca, 0x9977, 0x9b20,
0x9cc4, 0x9e64, 0xa000, 0xa197,
0xa32b, 0xa4ba, 0xa646, 0xa7cf,
0xa953, 0xaad5, 0xac53, 0xadcd,
0xaf45, 0xb0b9, 0xb22b, 0xb399,
0xb504};

LNK_SECTION_CONSTDATA_L1
const FIXP_DBL invCount[80]=  /* This could be 16-bit wide */
{
    0x00000000, 0x7fffffff, 0x40000000, 0x2aaaaaab, 0x20000000,
    0x1999999a, 0x15555555, 0x12492492, 0x10000000, 0x0e38e38e,
    0x0ccccccd, 0x0ba2e8ba, 0x0aaaaaab, 0x09d89d8a, 0x09249249,
    0x08888889, 0x08000000, 0x07878788, 0x071c71c7, 0x06bca1af,
    0x06666666, 0x06186186, 0x05d1745d, 0x0590b216, 0x05555555,
    0x051eb852, 0x04ec4ec5, 0x04bda12f, 0x04924925, 0x0469ee58,
    0x04444444, 0x04210842, 0x04000000, 0x03e0f83e, 0x03c3c3c4,
    0x03a83a84, 0x038e38e4, 0x03759f23, 0x035e50d8, 0x03483483,
    0x03333333, 0x031f3832, 0x030c30c3, 0x02fa0be8, 0x02e8ba2f,
    0x02d82d83, 0x02c8590b, 0x02b93105, 0x02aaaaab, 0x029cbc15,
    0x028f5c29, 0x02828283, 0x02762762, 0x026a439f, 0x025ed098,
    0x0253c825, 0x02492492, 0x023ee090, 0x0234f72c, 0x022b63cc,
    0x02222222, 0x02192e2a, 0x02108421, 0x02082082, 0x02000000,
    0x01f81f82, 0x01f07c1f, 0x01e9131b, 0x01e1e1e2, 0x01dae607,
    0x01d41d42, 0x01cd8569, 0x01c71c72, 0x01c0e070, 0x01bacf91,
    0x01b4e81b, 0x01af286c, 0x01a98ef6, 0x01a41a42, 0x019ec8e9
};


/*
 * Bitstream data lists
 */

/*
 * AOT {2,5,29}
 * epConfig = -1
 */

static const rbd_id_t el_aac_sce[] = {
  adtscrc_start_reg1,
  element_instance_tag,
  global_gain,
  ics_info,
  section_data,
  scale_factor_data,
  pulse,
  tns_data_present,
  tns_data,
  gain_control_data_present,
  /* gain_control_data, */
  spectral_data,
  adtscrc_end_reg1,
  end_of_sequence
};

static const struct element_list node_aac_sce = {
  el_aac_sce,
  { NULL, NULL }
};

static const rbd_id_t el_aac_cpe[] = {
  adtscrc_start_reg1,
  element_instance_tag,
  common_window,
  link_sequence
};

static const rbd_id_t el_aac_cpe0[] =
{
  /*common_window = 0*/
  global_gain,
  ics_info,
  section_data,
  scale_factor_data,
  pulse,
  tns_data_present,
  tns_data,
  gain_control_data_present,
  /*gain_control_data,*/
  spectral_data,
  next_channel,

  adtscrc_start_reg2,
  global_gain,
  ics_info,
  section_data,
  scale_factor_data,
  pulse,
  tns_data_present,
  tns_data,
  gain_control_data_present,
  /*gain_control_data,*/
  spectral_data,
  adtscrc_end_reg1,
  adtscrc_end_reg2,
  end_of_sequence
};

static const rbd_id_t el_aac_cpe1[] =
{
  /* common_window = 1 */
  ics_info,
  ms,

  global_gain,
  section_data,
  scale_factor_data,
  pulse,
  tns_data_present,
  tns_data,
  gain_control_data_present,
  /*gain_control_data,*/
  spectral_data,
  next_channel,

  adtscrc_start_reg2,
  global_gain,
  section_data,
  scale_factor_data,
  pulse,
  tns_data_present,
  tns_data,
  gain_control_data_present,
  /*gain_control_data,*/
  spectral_data,
  adtscrc_end_reg1,
  adtscrc_end_reg2,
  end_of_sequence
};

static const struct element_list node_aac_cpe0 = {
  el_aac_cpe0,
  { NULL, NULL }
};

static const struct element_list node_aac_cpe1 = {
  el_aac_cpe1,
  { NULL, NULL }
};

static const element_list_t node_aac_cpe = {
  el_aac_cpe,
  { &node_aac_cpe0, &node_aac_cpe1 }
};

/*
 * AOT C- {17,23}
 * epConfig = 0,1
 */
static const rbd_id_t el_aac_sce_epc0[] = {
  element_instance_tag,
  global_gain,
  ics_info,
  section_data,
  scale_factor_data,
  pulse,
  tns_data_present,
  gain_control_data_present,
  gain_control_data,
  esc1_hcr,   /*length_of_rvlc_escapes, length_of_rvlc_sf */
  esc2_rvlc,  /* rvlc_cod_sf, rvlc_esc_sf */
  tns_data,
  spectral_data,
  end_of_sequence
};

static const struct element_list node_aac_sce_epc0 = {
  el_aac_sce_epc0,
  { NULL, NULL }
};

static const rbd_id_t el_aac_sce_epc1[] = {
  element_instance_tag,
  global_gain,
  ics_info,
  section_data,
  scale_factor_data,
  pulse,
  tns_data_present,
  gain_control_data_present,
  /*gain_control_data,*/
  esc1_hcr,   /*length_of_rvlc_escapes, length_of_rvlc_sf */
  esc2_rvlc,  /* rvlc_cod_sf, rvlc_esc_sf */
  tns_data,
  spectral_data,
  end_of_sequence
};

static const struct element_list node_aac_sce_epc1 = {
  el_aac_sce_epc1,
  { NULL, NULL }
};

static const rbd_id_t el_aac_cpe0_epc0[] = {
  /* common_window = 0 */
  /* ESC 1: */
  global_gain,
  ics_info,
  /* ltp_data_present,
     ltp_data,
  */
  section_data,
  scale_factor_data,
  pulse,
  tns_data_present,
  gain_control_data_present,
  /*gain_control_data,*/
  esc1_hcr,  /*length_of_rvlc_escapes, length_of_rvlc_sf */
  /* ESC 2: */
  esc2_rvlc, /* rvlc_cod_sf, rvlc_esc_sf */
  /* ESC 3: */
  tns_data,
  /* ESC 4: */
  spectral_data,
  next_channel,

  /* ESC 1: */
  global_gain,
  ics_info,
  /* ltp_data_present,
     ltp_data,
  */
  section_data,
  scale_factor_data,
  pulse,
  tns_data_present,
  gain_control_data_present,
  /*gain_control_data,*/
  esc1_hcr,   /*length_of_rvlc_escapes, length_of_rvlc_sf */
  /* ESC 2: */
  esc2_rvlc,  /* rvlc_cod_sf, rvlc_esc_sf */
  /* ESC 3: */
  tns_data,
  /* ESC 4: */
  spectral_data,
  end_of_sequence
};

static const rbd_id_t el_aac_cpe1_epc0[] = {
  /* common_window = 1 */
  /* ESC 0: */
  ics_info,
  /* ltp_data_present,
     ltp_data,
     next_channel,
     ltp_data_present,
     ltp_data,
     next_channel,
  */
  ms,

  /* ESC 1: */
  global_gain,
  section_data,
  scale_factor_data,
  pulse,
  tns_data_present,
  gain_control_data_present,
  /*gain_control_data,*/
  esc1_hcr,   /* length_of_reordered_spectral_data, length_of_longest_codeword */
  /* ESC 2: */
  esc2_rvlc,  /* rvlc_cod_sf, rvlc_esc_sf */
  /* ESC 3: */
  tns_data,
  /* ESC 4: */
  spectral_data,
  next_channel,

  /* ESC 1: */
  global_gain,
  section_data,
  scale_factor_data,
  pulse,
  tns_data_present,
  gain_control_data_present,
  /*gain_control_data,*/
  esc1_hcr,   /* length_of_reordered_spectral_data, length_of_longest_codeword */
  /* ESC 2: */
  esc2_rvlc,  /* rvlc_cod_sf, rvlc_esc_sf */
  /* ESC 3: */
  tns_data,
  /* ESC 4: */
  spectral_data,
  end_of_sequence
};

static const struct element_list node_aac_cpe0_epc0 = {
  el_aac_cpe0_epc0,
  { NULL, NULL }
};

static const struct element_list node_aac_cpe1_epc0 = {
  el_aac_cpe1_epc0,
  { NULL, NULL }
};

static const element_list_t node_aac_cpe_epc0 = {
  el_aac_cpe,
  { &node_aac_cpe0_epc0, &node_aac_cpe1_epc0 }
};

static const rbd_id_t el_aac_cpe0_epc1[] = {
  global_gain,
  ics_info,
  section_data,
  scale_factor_data,
  pulse,
  tns_data_present,
  gain_control_data_present,
  /*gain_control_data,*/
  next_channel,
  global_gain,
  ics_info,
  section_data,
  scale_factor_data,
  pulse,
  tns_data_present,
  gain_control_data_present,
  /*gain_control_data,*/
  next_channel,
  esc1_hcr,  /*length_of_rvlc_escapes, length_of_rvlc_sf */
  next_channel,
  esc1_hcr,  /*length_of_rvlc_escapes, length_of_rvlc_sf */
  next_channel,
  esc2_rvlc,  /* rvlc_cod_sf, rvlc_esc_sf */
  next_channel,
  esc2_rvlc,  /* rvlc_cod_sf, rvlc_esc_sf */
  next_channel,
  tns_data,
  next_channel,
  tns_data,
  next_channel,
  spectral_data,
  next_channel,
  spectral_data,
  end_of_sequence
};

static const rbd_id_t el_aac_cpe1_epc1[] = {
  ics_info,
  ms,
  ltp_data_present,
  /* ltp_data, */
  global_gain,
  section_data,
  scale_factor_data,
  pulse,
  tns_data_present,
  gain_control_data_present,
  /*gain_control_data,*/
  next_channel,

  ltp_data_present,
  /* ltp_data, */
  global_gain,
  section_data,
  scale_factor_data,
  pulse,
  tns_data_present,
  gain_control_data_present,
  /*gain_control_data,*/
  next_channel,
  esc1_hcr,  /*length_of_rvlc_escapes, length_of_rvlc_sf */
  next_channel,
  esc1_hcr,  /*length_of_rvlc_escapes, length_of_rvlc_sf */
  next_channel,
  esc2_rvlc,  /* rvlc_cod_sf, rvlc_esc_sf */
  next_channel,
  esc2_rvlc,  /* rvlc_cod_sf, rvlc_esc_sf */

  next_channel,
  tns_data,
  next_channel,
  tns_data,
  next_channel,
  spectral_data,
  next_channel,
  spectral_data,
  end_of_sequence
};

static const struct element_list node_aac_cpe0_epc1 = {
  el_aac_cpe0_epc1,
  { NULL, NULL }
};

static const struct element_list node_aac_cpe1_epc1 = {
  el_aac_cpe1_epc1,
  { NULL, NULL }
};

static const element_list_t node_aac_cpe_epc1 = {
  el_aac_cpe,
  { &node_aac_cpe0_epc1, &node_aac_cpe1_epc1 }
};

/*
 * AOT = 20
 * epConfig = 0
 */
static const rbd_id_t el_scal_sce_epc0[] = {
  ics_info,            /* ESC 1 */
  tns_data_present,
  ltp_data_present,
  /* ltp_data, */
  global_gain,
  section_data,
  scale_factor_data,
  esc1_hcr,
  esc2_rvlc,           /* ESC 2 */
  tns_data,            /* ESC 3 */
  spectral_data,       /* ESC 4 */
  end_of_sequence
};

static const struct element_list node_scal_sce_epc0 = {
  el_scal_sce_epc0,
  { NULL, NULL }
};

static const rbd_id_t el_scal_cpe_epc0[] = {
  ics_info,            /* ESC 0 */
  ms,
  tns_data_present,    /* ESC 1 (ch 0) */
  ltp_data_present,
  /* ltp_data, */
  global_gain,
  section_data,
  scale_factor_data,
  esc1_hcr,
  esc2_rvlc,           /* ESC 2 (ch 0) */
  tns_data,            /* ESC 3 (ch 0) */
  spectral_data,       /* ESC 4 (ch 0) */
  next_channel,
  tns_data_present,    /* ESC 1 (ch 1) */
  ltp_data_present,
  global_gain,
  section_data,
  scale_factor_data,
  esc1_hcr,
  esc2_rvlc,           /* ESC 2 (ch 1) */
  tns_data,            /* ESC 3 (ch 1) */
  spectral_data,       /* ESC 4 (ch 1) */
  end_of_sequence
};

static const struct element_list node_scal_cpe_epc0 = {
  el_scal_cpe_epc0,
  { NULL, NULL }
};

/*
 * AOT = 20
 * epConfig = 1
 */
static const rbd_id_t el_scal_sce_epc1[] = {
  ics_info,
  tns_data_present,
  ltp_data_present,
  /* ltp_data, */
  global_gain,
  section_data,
  scale_factor_data,
  esc1_hcr,
  tns_data,
  spectral_data,
  end_of_sequence
};

static const struct element_list node_scal_sce_epc1 = {
  el_scal_sce_epc1,
  { NULL, NULL }
};

static const rbd_id_t el_scal_cpe_epc1[] = {
  ics_info,
  ms,
  tns_data_present,
  ltp_data_present,
  /* ltp_data, */
  global_gain,
  section_data,
  scale_factor_data,
  esc1_hcr,
  next_channel,
  tns_data_present,
  ltp_data_present,
  /* ltp_data, */
  global_gain,
  section_data,
  scale_factor_data,
  esc1_hcr,
  next_channel,
  tns_data,
  next_channel,
  tns_data,
  next_channel,
  spectral_data,
  next_channel,
  spectral_data,
  end_of_sequence
};

static const struct element_list node_scal_cpe_epc1 = {
  el_scal_cpe_epc1,
  { NULL, NULL }
};

/*
 * Pseudo AOT for DRM/DRM+ (similar to AOT 20)
 * Derived from epConfig = 1
 */
static const rbd_id_t el_drm_sce[] = {
  drmcrc_start_reg,
  ics_info,
  tns_data_present,
  ltp_data_present,
  /* ltp_data, */
  global_gain,
  section_data,
  scale_factor_data,
  esc1_hcr,
  tns_data,
  drmcrc_end_reg,
  spectral_data,
  end_of_sequence
};

static const struct element_list node_drm_sce = {
  el_drm_sce,
  { NULL, NULL }
};

static const rbd_id_t el_drm_cpe[] = {
  drmcrc_start_reg,
  ics_info,
  ms,
  tns_data_present,
  ltp_data_present,
  /* ltp_data, */
  global_gain,
  section_data,
  scale_factor_data,
  esc1_hcr,
  next_channel,
  tns_data_present,
  ltp_data_present,
  /* ltp_data, */
  global_gain,
  section_data,
  scale_factor_data,
  esc1_hcr,
  next_channel,
  tns_data,
  next_channel,
  tns_data,
  drmcrc_end_reg,
  next_channel,
  spectral_data,
  next_channel,
  spectral_data,
  end_of_sequence
};

static const struct element_list node_drm_cpe = {
  el_drm_cpe,
  { NULL, NULL }
};

/*
 * AOT = 39
 * epConfig = 0
 */
static const rbd_id_t el_eld_sce_epc0[] = {
  global_gain,
  ics_info,
  section_data,
  scale_factor_data,
  tns_data_present,
  tns_data,
  esc1_hcr,
  esc2_rvlc,
  spectral_data,
  end_of_sequence
};

static const struct element_list node_eld_sce_epc0 = {
  el_eld_sce_epc0,
  { NULL, NULL }
};

#define node_eld_sce_epc1 node_eld_sce_epc0

static const rbd_id_t el_eld_cpe_epc0[] = {
  ics_info,
  ms,
  global_gain,
  section_data,
  scale_factor_data,
  tns_data_present,
  tns_data,
  esc1_hcr,
  esc2_rvlc,
  spectral_data,
  next_channel,
  global_gain,
  section_data,
  scale_factor_data,
  tns_data_present,
  tns_data,
  esc1_hcr,
  esc2_rvlc,
  spectral_data,
  end_of_sequence
};

static const rbd_id_t el_eld_cpe_epc1[] = {
  ics_info,
  ms,
  global_gain,
  section_data,
  scale_factor_data,
  tns_data_present,
  next_channel,
  global_gain,
  section_data,
  scale_factor_data,
  tns_data_present,
  next_channel,
  tns_data,
  next_channel,
  tns_data,
  next_channel,
  esc1_hcr,
  esc2_rvlc,
  spectral_data,
  next_channel,
  esc1_hcr,
  esc2_rvlc,
  spectral_data,
  end_of_sequence
};

static const struct element_list node_eld_cpe_epc0 = {
  el_eld_cpe_epc0,
  { NULL, NULL }
};

static const struct element_list node_eld_cpe_epc1 = {
  el_eld_cpe_epc1,
  { NULL, NULL }
};


const element_list_t * getBitstreamElementList(AUDIO_OBJECT_TYPE aot, SCHAR epConfig, UCHAR nChannels, UCHAR layer)
{
  switch (aot) {
    case AOT_AAC_LC:
    case AOT_SBR:
    case AOT_PS:
      FDK_ASSERT(epConfig == -1);
      if (nChannels == 1) {
        return &node_aac_sce;
      } else {
        return &node_aac_cpe;
      }
      break;
    case AOT_ER_AAC_LC:
    case AOT_ER_AAC_LD:
      if (nChannels == 1) {
        if (epConfig == 0) {
          return &node_aac_sce_epc0;
        } else {
          return &node_aac_sce_epc1;
        }
      } else {
        if (epConfig == 0)
          return &node_aac_cpe_epc0;
        else
          return &node_aac_cpe_epc1;
      }
      break;
    case AOT_ER_AAC_SCAL:
      if (nChannels == 1) {
        if (epConfig <= 0)
          return &node_scal_sce_epc0;
        else
          return &node_scal_sce_epc1;
      } else {
        if (epConfig <= 0)
          return &node_scal_cpe_epc0;
        else
          return &node_scal_cpe_epc1;
      }
      break;
    case AOT_ER_AAC_ELD:
      if (nChannels == 1) {
        if (epConfig <= 0)
          return &node_eld_sce_epc0;
        else
          return &node_eld_sce_epc1;
      } else {
        if (epConfig <= 0)
          return &node_eld_cpe_epc0;
        else
          return &node_eld_cpe_epc1;
      }
    case AOT_DRM_AAC:
    case AOT_DRM_SBR:
    case AOT_DRM_MPEG_PS:
      FDK_ASSERT(epConfig == 1);
      if (nChannels == 1) {
        return &node_drm_sce;
      } else {
        return &node_drm_cpe;
      }
      break;
    default:
      break;
  }
  return NULL;
}


