/*                                                    -*- coding: utf-8 -*-
 * ec_p256k1.c - Elliptic curve over GF(p256k1)
 *
 * Copyright (C) 2014 Free Software Initiative of Japan
 * Author: NIIBE Yutaka <gniibe@fsij.org>
 *
 * This file is a part of Gnuk, a GnuPG USB Token implementation.
 *
 * Gnuk is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Gnuk is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * Note: we don't take advantage of the specific feature of this curve,
 * but use same method of computation as NIST P-256 curve.  That's due
 * to some software patent(s).
 */

#include <stdint.h>
#include <string.h>
#include "bn.h"
#include "modp256k1.h"
#include "affine.h"
#include "jpc-ac_p256k1.h"
#include "mod.h"
#include "ec_p256k1.h"

#define FIELD p256k1
#define COEFFICIENT_A_IS_ZERO    1

/*
 * a = 0, b = 7
 */
#if 0
static const bn256 coefficient_a[1] = {
  {{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }}
};
#endif

static const bn256 coefficient_b[1] = {
  {{ 0x7, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }}
};


static const ac precomputed_KG[15] = {
  {
    {{{ 0x16f81798, 0x59f2815b, 0x2dce28d9, 0x029bfcdb,
	0xce870b07, 0x55a06295, 0xf9dcbbac, 0x79be667e }}},
    {{{ 0xfb10d4b8, 0x9c47d08f, 0xa6855419, 0xfd17b448,
	0x0e1108a8, 0x5da4fbfc, 0x26a3c465, 0x483ada77 }}}
  }, {
    {{{ 0x42d0e6bd, 0x13b7e0e7, 0xdb0f5e53, 0xf774d163,
	0x104d6ecb, 0x82a2147c, 0x243c4e25, 0x3322d401 }}},
    {{{ 0x6c28b2a0, 0x24f3a2e9, 0xa2873af6, 0x2805f63e,
	0x4ddaf9b7, 0xbfb019bc, 0xe9664ef5, 0x56e70797 }}}
  }, {
    {{{ 0x829d122a, 0xdca81127, 0x67e99549, 0x8f17f314,
	0x6a8a9e73, 0x9b889085, 0x846dd99d, 0x583fdfd9 }}},
    {{{ 0x63c4eac4, 0xf3c7719e, 0xb734b37a, 0xb44685a3,
	0x572a47a6, 0x9f92d2d6, 0x2ff57d81, 0xabc6232f }}}
  }, {
    {{{ 0x9ec4c0da, 0x1b7b444c, 0x723ea335, 0xe88c5678,
	0x981f162e, 0x9239c1ad, 0xf63b5f33, 0x8f68b9d2 }}},
    {{{ 0x501fff82, 0xf23cbf79, 0x95510bfd, 0xbbea2cfe,
	0xb6be215d, 0xde1d90c2, 0xba063986, 0x662a9f2d }}}
  }, {
    {{{ 0x114cbf09, 0x63c5e885, 0x7be77e3e, 0x2f27ce93,
	0xf54a3e33, 0xdaa6d12d, 0x3eff872c, 0x8b300e51 }}},
    {{{ 0xb3b10a39, 0x26c6ff28, 0x9aaf7169, 0x08f6a7aa,
	0x6b8238ea, 0x446f0d46, 0x7f43c0cc, 0x1cec3067 }}}
  }, {
    {{{ 0x075e9070, 0xba16ce6a, 0x9b5cfe37, 0xbc26893d,
	0x9c510774, 0xe1ddadfe, 0xfe3ae2f4, 0x90922d88 }}},
    {{{ 0x5c08824a, 0x653943cc, 0xfce8f4bc, 0x06d74475,
	0x533c615d, 0x8d101fa7, 0x742108a9, 0x7b1903f6 }}}
  }, {
    {{{ 0x6ebdc96c, 0x1bcfa45c, 0x1c7584ba, 0xe400bc04,
	0x74cf531f, 0x6395e20e, 0xc5131b30, 0x1edd0bb1 }}},
    {{{ 0xe358cf9e, 0xa117161b, 0x2724d11c, 0xe490d6f0,
	0xee6dd8c9, 0xf75062f6, 0xfba373e4, 0x31e03b2b }}}
  }, {
    {{{ 0x2120e2b3, 0x7f3b58fa, 0x7f47f9aa, 0x7a58fdce,
	0x4ce6e521, 0xe7be4ae3, 0x1f51bdba, 0xeaa649f2 }}},
    {{{ 0xba5ad93d, 0xd47a5305, 0xf13f7e59, 0x01a6b965,
	0x9879aa5a, 0xc69a80f8, 0x5bbbb03a, 0xbe3279ed }}}
  }, {
    {{{ 0x27bb4d71, 0xcf291a33, 0x33524832, 0x6caf7d6b,
	0x766584ee, 0x6e0ee131, 0xd064c589, 0x160cb0f6 }}},
    {{{ 0x17136e8d, 0x9d5de554, 0x1aab720e, 0xe3f2d468,
	0xccf75cc2, 0xd1378b49, 0xc4ff16e1, 0x6920c375 }}}
  }, {
    {{{ 0x1a9ee611, 0x3eef9e96, 0x9cc37faf, 0xfe4d7bf3,
	0xb321d965, 0x462aa9b3, 0x208736c5, 0x1702da3e }}},
    {{{ 0x3a545ceb, 0xfba57bbf, 0x7ea858f5, 0x6dbcd766,
	0x680d92f1, 0x088e897c, 0xbc626c80, 0x468c1fd8 }}}
  }, {
    {{{ 0xb188660a, 0xb40f85c7, 0x99bc3c36, 0xc5873c19,
	0x7f33b54c, 0x3c7b4541, 0x1f8c9bf8, 0x4cd3a93c }}},
    {{{ 0x33099cb0, 0xf8dce380, 0x2edd2f33, 0x7a167dd6,
	0x0ffe35b7, 0x576d8987, 0xc68ace5c, 0xd2de0386 }}}
  }, {
    {{{ 0x6658bb08, 0x9a9e0a72, 0xc589607b, 0xe23c5f2a,
	0xf2bfb4c8, 0xa048ca14, 0xc62c2291, 0x4d9a0f89 }}},
    {{{ 0x0f827294, 0x427b5f31, 0x9f2c35cd, 0x1ea7a8b5,
	0x85a3c00f, 0x95442e56, 0x9b57975a, 0x8cb83121 }}}
  }, {
    {{{ 0x51f5cf67, 0x4333f0da, 0xf4f0d3cb, 0x6d3ea47c,
	0xa05a831f, 0x442fda14, 0x016d3e81, 0x6a496013 }}},
    {{{ 0xe52e0f48, 0xf647318c, 0x4a0d5ff1, 0x5ff3a66e,
	0x61199ba8, 0x046ed81a, 0x3e79c23a, 0x578edf08 }}}
  }, {
    {{{ 0x3ea01ea7, 0xb8f996f8, 0x7497bb15, 0xc0045d33,
	0x6205647c, 0xc4749dc9, 0x0efd22c9, 0xd8946054 }}},
    {{{ 0x12774ad5, 0x062dcb09, 0x8be06e3a, 0xcb13f310,
	0x235de1a9, 0xca281d35, 0x69c3645c, 0xaf8a7412 }}}
  }, {
    {{{ 0xbeb8b1e2, 0x8808ca5f, 0xea0dda76, 0x0262b204,
	0xddeb356b, 0xb6fffffc, 0xfbb83870, 0x52de253a }}},
    {{{ 0x8f8d21ea, 0x961f40c0, 0x002f03ed, 0x89686278,
	0x38e421ea, 0x0ff834d7, 0xd36fb8db, 0x3a270d6f }}}
  }
};

static const ac precomputed_2E_KG[15] = {
  {
    {{{ 0x39a48db0, 0xefd7835b, 0x9b3c03bf, 0x9f1215a2,
	0x9b7bde45, 0x2791d0a0, 0x696e7167, 0x100f44da }}},
    {{{ 0x2bc65a09, 0x0fbd5cd6, 0xff5195ac, 0xb7ff4a18,
	0x0c090666, 0x2ec8f330, 0x92a00b77, 0xcdd9e131 }}}
  }, {
    {{{ 0x40fb27b6, 0x32427e28, 0xbe430576, 0xc76e3db2,
	0x61686aa5, 0x10f238ad, 0xbe778b1b, 0xfea74e3d }}},
    {{{ 0xf23cb96f, 0x701d3db7, 0x973f7b77, 0x126b596b,
	0xccb6af93, 0x7cf674de, 0x9b0b1329, 0x6e0568db }}}
  }, {
    {{{ 0x2c8118bc, 0x6cac5154, 0x399ddd98, 0x19bd4b34,
	0x2e9c8949, 0x47248a8d, 0x2cefa3b1, 0x734cb6a8 }}},
    {{{ 0x1e410fd5, 0xf1b340ad, 0xc4873539, 0xa2982bee,
	0xd4de4530, 0x7b5a3ea4, 0x42202574, 0xae46e10e }}}
  }, {
    {{{ 0xac1f98cd, 0xcbfc99c8, 0x4d7f0308, 0x52348905,
	0x1cc66021, 0xfaed8a9c, 0x4a474870, 0x9c3919a8 }}},
    {{{ 0xd4fc599d, 0xbe7e5e03, 0x6c64c8e6, 0x905326f7,
	0xf260e641, 0x584f044b, 0x4a4ddd57, 0xddb84f0f }}}
  }, {
    {{{ 0xed7cebed, 0xc4aacaa8, 0x4fae424e, 0xb75d2dce,
	0xba20735e, 0xa01585a2, 0xba122399, 0x3d75f24b }}},
    {{{ 0xd5570dce, 0xcbe4606f, 0x2da192c2, 0x9d00bfd7,
	0xa57b7265, 0x9c3ce86b, 0xec4edf5e, 0x987a22f1 }}}
  }, {
    {{{ 0x73ea0665, 0x211b9715, 0xf3a1abbb, 0x86f485d4,
	0xcd076f0e, 0xabd242d8, 0x0ba5dc88, 0x862332ab }}},
    {{{ 0x7b784911, 0x09af505c, 0xcaf4fae7, 0xc89544e8,
	0xae9a32eb, 0x256625f6, 0x606d1a3f, 0xe2532b72 }}}
  }, {
    {{{ 0x0deaf885, 0x79e9f313, 0x46df21c9, 0x938ff76e,
	0xa953bb2c, 0x1968f5fb, 0x29155f27, 0xdff538bf }}},
    {{{ 0x31d5d020, 0xf7bae0b1, 0x1a676a8d, 0x5afdc787,
	0xfa9d53ff, 0x11b4f032, 0xc5959167, 0x86ba433e }}}
  }, {
    {{{ 0x9475b7ba, 0x884fdff0, 0xe4918b3d, 0xe039e730,
	0xf5018cdb, 0x3d3e57ed, 0x1943785c, 0x95939698 }}},
    {{{ 0x7524f2fd, 0xe9b8abf8, 0xc8709385, 0x9c653f64,
	0x4b9cd684, 0x8ba0386a, 0x88c331dd, 0x2e7e5528 }}}
  }, {
    {{{ 0xeefe79e5, 0x940bef53, 0xbe9b87f3, 0xc518d286,
	0x7833042c, 0x9e0c7c76, 0x11fbe152, 0x104e2cb5 }}},
    {{{ 0x50bbec83, 0xc0d35e0f, 0x4acd0fcc, 0xee4879be,
	0x006085ee, 0xc8d80f5d, 0x72fe1ac1, 0x3c51bc1c }}}
  }, {
    {{{ 0xb2de976e, 0x06187f61, 0xf5e4b4b6, 0x52869e18,
	0x38d332ca, 0x74d4facd, 0xb3a2f8d9, 0x5c1c90b4 }}},
    {{{ 0xdaa37893, 0x98644d09, 0xabe39818, 0x682435a8,
	0x469c53a0, 0x17e46617, 0x77dc2e64, 0x642f9632 }}}
  }, {
    {{{ 0x222f6c54, 0xad2101c5, 0xfa74785e, 0xb05c7a58,
	0x489bcdaf, 0xce55fa79, 0xffe88d54, 0xc1f920fd }}},
    {{{ 0x9065e490, 0x32553ab0, 0x35329f74, 0x7611b9af,
	0xab7b24c0, 0x57df19ef, 0x6181c447, 0xb9a78749 }}}
  }, {
    {{{ 0xa80b7ea8, 0x392f156f, 0x8ae4a8bf, 0x57ab7ca0,
	0x50c4b178, 0xac320747, 0x0e781feb, 0x146041b9 }}},
    {{{ 0x845279b2, 0xd343f075, 0x7387afa5, 0x2d4fe757,
	0xa72f3c39, 0x151e0948, 0x550da168, 0x41a6d54e }}}
  }, {
    {{{ 0x075a0010, 0xb3134ed3, 0x7ae93e23, 0x9fa76f4b,
	0x7bb4daaa, 0xc0db256f, 0x464dd8a3, 0x7668dc27 }}},
    {{{ 0x9f5da977, 0x150063f5, 0x05efce00, 0x3acac5c8,
	0x884493fe, 0xc8e12ffc, 0x88f06bd2, 0x4ab936d8 }}}
  }, {
    {{{ 0x5d09ea98, 0x996fde77, 0x4145da58, 0x16ddf512,
	0xdc2fb225, 0xa97a6ca8, 0xfbdcdf5a, 0xc7331f30 }}},
    {{{ 0x86a86e52, 0x838f99e0, 0x77795edd, 0x68d39b29,
	0x9f412aaa, 0xe4e4f97e, 0x30d25352, 0xe5cc2c0a }}}
  }, {
    {{{ 0x9c21ff71, 0xb3d68650, 0xddbe3884, 0x11e7589d,
	0x423bac67, 0x7efd4055, 0x46957425, 0x587a7293 }}},
    {{{ 0x8f5a8fc6, 0x360adc2e, 0xbd69f12e, 0x6f8bbafb,
	0x0a3f3b4d, 0xf671f423, 0x59942dc3, 0xb49acb47 }}}
  }
};

/*
 * N: order of G
 *    0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141
 */
static const bn256 N[1] = {
  {{ 0xd0364141, 0xbfd25e8c, 0xaf48a03b, 0xbaaedce6,
     0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff }}
};

/*
 * MU = 2^512 / N
 * MU = ( (1 << 256) | MU_lower )
 */
static const bn256 MU_lower[1] = {
  {{ 0x2fc9bec0, 0x402da173, 0x50b75fc4, 0x45512319,
     0x1, 0x0, 0x0, 0x0 }}
};


#include "ecc.c"
