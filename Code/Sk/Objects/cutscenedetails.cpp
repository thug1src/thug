//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       cutscenedetails.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  01/13/2003
//****************************************************************************

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sk/objects/cutscenedetails.h>

#include <core/math.h>

#include <gel/assman/assman.h>
#include <gel/assman/assettypes.h>
#include <gel/assman/skeletonasset.h>
#include <gel/assman/skinasset.h>

#include <gfx/bonedanim.h>
#include <gfx/camera.h>

#include <gel/music/music.h>

#include <gel/scripting/checksum.h>
#include <gel/scripting/file.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <gel/object/compositeobject.h>

#include <gfx/skeleton.h>
#include <gel/components/modelcomponent.h>
#include <gel/components/skeletoncomponent.h>

#include <sys/file/AsyncFilesys.h>
#include <sys/file/FileLibrary.h>

#include <sk/modules/skate/skate.h>
#include <sk/objects/movingobject.h>
#include <gel/objman.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/components/animationcomponent.h>

#include <gfx/nx.h>
#include <gfx/nxmodel.h>
#include <gfx/debuggfx.h>
#include <gfx/facetexture.h>
#include <gfx/nxanimcache.h>
#include <gfx/nxquickanim.h>
#include <gfx/nxlightman.h>
#include <gfx/nxlight.h>

#include <sk/objects/skaterprofile.h>
#include <gfx/modelappearance.h>
#include <sys/mem/memman.h>
#include <sys/timer.h>
#include <sys/config/config.h>
#include <sk/modules/FrontEnd/FrontEnd.h>

#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/vecpair.h>


/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

// make sure the texture dictionary offsets don't
// clash with skater's texture dictionaries
// in case we're using actual skater assets
// (such as the board) 
const int vCUTSCENE_TEXDICT_RANGE = 200;
 
// flags that are set during cifconv
#define vCUTSCENEOBJECTFLAGS_ISSKATER		(1<<31)
#define vCUTSCENEOBJECTFLAGS_ISHEAD			(1<<30)

#ifdef __PLAT_NGC__
extern bool g_in_cutscene;
#endif		// __PLAT_NGC__

/*****************************************************************************
**								DBG Defines									**
*****************************************************************************/

namespace Obj
{

/*
char testWeightIndices[] = {
	30, -1, -1, // vertex 0
	30, -1, -1, // vertex 1
	30, -1, -1, // vertex 2
	30, -1, -1, // vertex 3
	30, -1, -1, // vertex 4
	30, -1, -1, // vertex 5
	30, -1, -1, // vertex 6
	30, -1, -1, // vertex 7
	30, -1, -1, // vertex 8
	30, 32, -1, // vertex 9
	30, 32, -1, // vertex 10
	30, 32, -1, // vertex 11
	30, 32, -1, // vertex 12
	30, 32, -1, // vertex 13
	30, 32, -1, // vertex 14
	30, 32, -1, // vertex 15
	30, 32, -1, // vertex 16
	30, 32, -1, // vertex 17
	30, 32, -1, // vertex 18
	30, 32, -1, // vertex 19
	30, 32, -1, // vertex 20
	30, 32, -1, // vertex 21
	30, -1, -1, // vertex 22
	30, -1, -1, // vertex 23
	30, -1, -1, // vertex 24
	30, -1, -1, // vertex 25
	30, -1, -1, // vertex 26
	30, -1, -1, // vertex 27
	30, 31, -1, // vertex 28
	30, -1, -1, // vertex 29
	30, 32, -1, // vertex 30
	30, -1, -1, // vertex 31
	30, 32, -1, // vertex 32
	30, -1, -1, // vertex 33
	30, 32, -1, // vertex 34
	30, 32, -1, // vertex 35
	30, 32, -1, // vertex 36
	30, 32, -1, // vertex 37
	30, 32, -1, // vertex 38
	30, 32, -1, // vertex 39
	30, 32, -1, // vertex 40
	30, 32, -1, // vertex 41
	30, 32, -1, // vertex 42
	30, -1, -1, // vertex 43
	30, -1, -1, // vertex 44
	30, -1, -1, // vertex 45
	30, -1, -1, // vertex 46
	30, -1, -1, // vertex 47
	30, -1, -1, // vertex 48
	30, -1, -1, // vertex 49
	30, -1, -1, // vertex 50
	30, 32, -1, // vertex 51
	30, 32, -1, // vertex 52
	30, 32, -1, // vertex 53
	30, 32, -1, // vertex 54
	30, 32, -1, // vertex 55
	30, 32, -1, // vertex 56
	30, 32, -1, // vertex 57
	30, 32, -1, // vertex 58
	30, 32, -1, // vertex 59
	30, 32, -1, // vertex 60
	30, 32, -1, // vertex 61
	30, 32, -1, // vertex 62
	30, 32, -1, // vertex 63
	30, 32, -1, // vertex 64
	30, 32, -1, // vertex 65
	30, 32, -1, // vertex 66
	30, 32, -1, // vertex 67
	30, 32, -1, // vertex 68
	30, 32, -1, // vertex 69
	30, 32, -1, // vertex 70
	30, 32, -1, // vertex 71
	30, 32, -1, // vertex 72
	30, -1, -1, // vertex 73
	30, 32, -1, // vertex 74
	30, -1, -1, // vertex 75
	30, 32, -1, // vertex 76
	30, -1, -1, // vertex 77
	30, -1, -1, // vertex 78
	30, -1, -1, // vertex 79
	30, -1, -1, // vertex 80
	30, -1, -1, // vertex 81
	30, -1, -1, // vertex 82
	30, -1, -1, // vertex 83
	30, -1, -1, // vertex 84
	30, -1, -1, // vertex 85
	30, -1, -1, // vertex 86
	30, -1, -1, // vertex 87
	30, -1, -1, // vertex 88
	30, -1, -1, // vertex 89
	30, -1, -1, // vertex 90
	30, -1, -1, // vertex 91
	30, -1, -1, // vertex 92
	30, -1, -1, // vertex 93
	30, -1, -1, // vertex 94
	30, 31, -1, // vertex 95
	30, 31, -1, // vertex 96
	30, -1, -1, // vertex 97
	30, -1, -1, // vertex 98
	30, -1, -1, // vertex 99
	30, 29, -1, // vertex 100
	30, 29, -1, // vertex 101
	30, -1, -1, // vertex 102
	30, -1, -1, // vertex 103
	30, 31, -1, // vertex 104
	30, -1, -1, // vertex 105
	30, -1, -1, // vertex 106
	30, -1, -1, // vertex 107
	30, -1, -1, // vertex 108
	30, -1, -1, // vertex 109
	30, -1, -1, // vertex 110
	30, -1, -1, // vertex 111
	30, -1, -1, // vertex 112
	30, -1, -1, // vertex 113
	30, -1, -1, // vertex 114
	29, 30, -1, // vertex 115
	30, -1, -1, // vertex 116
	29, 30, -1, // vertex 117
	29, 30, -1, // vertex 118
	30, 29, -1, // vertex 119
	30, 29, -1, // vertex 120
	30, 33, -1, // vertex 121
	30, 33, -1, // vertex 122
	33, 30, -1, // vertex 123
	33, 30, -1, // vertex 124
	33, 30, -1, // vertex 125
	33, 30, -1, // vertex 126
	33, 30, -1, // vertex 127
	33, 30, -1, // vertex 128
	33, 30, -1, // vertex 129
	33, 30, -1, // vertex 130
	33, 30, -1, // vertex 131
	33, 30, -1, // vertex 132
	33, 30, -1, // vertex 133
	33, 30, -1, // vertex 134
	30, 33, -1, // vertex 135
	33, 30, -1, // vertex 136
	33, 30, -1, // vertex 137
	33, 30, -1, // vertex 138
	33, 30, -1, // vertex 139
	33, 30, -1, // vertex 140
	30, 33, -1, // vertex 141
	33, 30, -1, // vertex 142
	30, 33, -1, // vertex 143
	33, 30, -1, // vertex 144
	30, 33, -1, // vertex 145
	30, 33, -1, // vertex 146
	30, -1, -1, // vertex 147
	30, -1, -1, // vertex 148
	30, -1, -1, // vertex 149
	30, -1, -1, // vertex 150
	30, -1, -1, // vertex 151
	30, -1, -1, // vertex 152
	30, -1, -1, // vertex 153
	30, 32, -1, // vertex 154
	30, -1, -1, // vertex 155
	30, 32, -1, // vertex 156
	30, 32, -1, // vertex 157
	30, 32, -1, // vertex 158
	30, 32, -1, // vertex 159
	30, 32, -1, // vertex 160
	30, 32, -1, // vertex 161
	30, 32, -1, // vertex 162
	30, 32, -1, // vertex 163
	30, 32, -1, // vertex 164
	30, 32, -1, // vertex 165
	30, 32, -1, // vertex 166
	30, 32, -1, // vertex 167
	30, 32, -1, // vertex 168
	30, 32, -1, // vertex 169
	30, 32, -1, // vertex 170
	30, 32, -1, // vertex 171
	30, 32, -1, // vertex 172
	30, 32, -1, // vertex 173
	30, 32, -1, // vertex 174
	30, 32, -1, // vertex 175
	30, -1, -1, // vertex 176
	30, 32, -1, // vertex 177
	30, -1, -1, // vertex 178
	30, -1, -1, // vertex 179
	30, -1, -1, // vertex 180
	30, -1, -1, // vertex 181
	30, -1, -1, // vertex 182
	30, -1, -1, // vertex 183
	30, -1, -1, // vertex 184
	30, 33, -1, // vertex 185
	30, 33, -1, // vertex 186
	33, 30, -1, // vertex 187
	30, 33, -1, // vertex 188
	33, 30, -1, // vertex 189
	30, 33, -1, // vertex 190
	33, 30, -1, // vertex 191
	33, 30, -1, // vertex 192
	33, 30, -1, // vertex 193
	33, 30, -1, // vertex 194
	33, 30, -1, // vertex 195
	30, 33, -1, // vertex 196
	33, 30, -1, // vertex 197
	30, 31, -1, // vertex 198
	30, 31, -1, // vertex 199
	30, 31, -1, // vertex 200
	30, 31, -1, // vertex 201
	30, 31, -1, // vertex 202
	30, 31, -1, // vertex 203
	30, 31, -1, // vertex 204
	30, 31, -1, // vertex 205
	30, 31, -1, // vertex 206
	30, 31, -1, // vertex 207
	30, 31, -1, // vertex 208
	30, 31, -1, // vertex 209
	30, 31, -1, // vertex 210
	30, 31, -1, // vertex 211
	31, 30, -1, // vertex 212
	31, 30, -1, // vertex 213
	31, 30, -1, // vertex 214
	31, 30, -1, // vertex 215
	30, 31, -1, // vertex 216
	30, 31, -1, // vertex 217
	30, 31, -1, // vertex 218
	30, 31, -1, // vertex 219
	30, 31, -1, // vertex 220
	30, 31, -1, // vertex 221
	30, 31, -1, // vertex 222
	30, -1, -1, // vertex 223
	30, 31, -1, // vertex 224
	30, -1, -1, // vertex 225
	30, 31, -1, // vertex 226
	30, 29, -1, // vertex 227
	30, -1, -1, // vertex 228
	30, -1, -1, // vertex 229
	30, -1, -1, // vertex 230
	30, 31, -1, // vertex 231
	30, -1, -1, // vertex 232
	30, -1, -1, // vertex 233
	30, -1, -1, // vertex 234
	30, -1, -1, // vertex 235
	30, -1, -1, // vertex 236
	30, -1, -1, // vertex 237
	30, -1, -1, // vertex 238
	30, 31, -1, // vertex 239
	30, 31, -1, // vertex 240
	30, -1, -1, // vertex 241
	30, -1, -1, // vertex 242
	30, -1, -1, // vertex 243
	30, -1, -1, // vertex 244
	30, -1, -1, // vertex 245
	30, -1, -1, // vertex 246
	30, -1, -1, // vertex 247
	30, -1, -1, // vertex 248
	30, -1, -1, // vertex 249
	30, -1, -1, // vertex 250
	30, -1, -1, // vertex 251
	30, -1, -1, // vertex 252
	30, 33, -1, // vertex 253
	30, -1, -1, // vertex 254
	30, 33, -1, // vertex 255
	30, -1, -1, // vertex 256
	30, 33, -1, // vertex 257
	30, -1, -1, // vertex 258
	33, 30, -1, // vertex 259
	30, -1, -1, // vertex 260
	33, 30, -1, // vertex 261
	30, 31, -1, // vertex 262
	30, 31, -1, // vertex 263
	30, 31, -1, // vertex 264
	30, -1, -1, // vertex 265
	30, -1, -1, // vertex 266
	30, -1, -1, // vertex 267
	30, -1, -1, // vertex 268
	30, -1, -1, // vertex 269
	30, -1, -1, // vertex 270
	30, -1, -1, // vertex 271
	30, -1, -1, // vertex 272
	30, -1, -1, // vertex 273
	30, -1, -1, // vertex 274
	30, -1, -1, // vertex 275
	30, -1, -1, // vertex 276
	30, -1, -1, // vertex 277
	30, 33, -1, // vertex 278
	30, -1, -1, // vertex 279
	30, 32, -1, // vertex 280
	30, -1, -1, // vertex 281
	30, -1, -1, // vertex 282
	30, -1, -1, // vertex 283
	30, -1, -1, // vertex 284
	30, -1, -1, // vertex 285
	30, -1, -1, // vertex 286
	30, -1, -1, // vertex 287
	30, -1, -1, // vertex 288
	30, -1, -1, // vertex 289
	30, 32, -1, // vertex 290
	30, 32, -1, // vertex 291
	30, 32, -1, // vertex 292
	30, 32, -1, // vertex 293
	30, -1, -1, // vertex 294
	30, -1, -1, // vertex 295
	30, -1, -1, // vertex 296
	30, 32, -1, // vertex 297
	30, -1, -1, // vertex 298
	30, 32, -1, // vertex 299
	30, -1, -1, // vertex 300
	30, 32, -1, // vertex 301
	30, -1, -1, // vertex 302
	30, 32, -1, // vertex 303
	30, 32, -1, // vertex 304
	30, -1, -1, // vertex 305
	30, 33, -1, // vertex 306
	30, -1, -1, // vertex 307
	30, 33, -1, // vertex 308
	30, 32, -1, // vertex 309
	30, 32, -1, // vertex 310
	30, 32, -1, // vertex 311
	30, 32, -1, // vertex 312
	30, 32, -1, // vertex 313
	30, 32, -1, // vertex 314
	30, 32, -1, // vertex 315
	30, 32, -1, // vertex 316
	30, 32, -1, // vertex 317
	30, 32, -1, // vertex 318
	30, 32, -1, // vertex 319
	30, 32, -1, // vertex 320
	30, 32, -1, // vertex 321
	30, 32, -1, // vertex 322
	30, 32, -1, // vertex 323
	30, 32, -1, // vertex 324
	30, 32, -1, // vertex 325
	30, 32, -1, // vertex 326
	30, 32, -1, // vertex 327
	30, 32, -1, // vertex 328
	30, 32, -1, // vertex 329
	30, 32, -1, // vertex 330
	30, 32, -1, // vertex 331
	30, 32, -1, // vertex 332
	30, 32, -1, // vertex 333
	30, 32, -1, // vertex 334
	30, -1, -1, // vertex 335
	30, -1, -1, // vertex 336
	30, -1, -1, // vertex 337
	30, -1, -1, // vertex 338
	30, -1, -1, // vertex 339
	30, -1, -1, // vertex 340
	30, -1, -1, // vertex 341
	30, -1, -1, // vertex 342
	30, -1, -1, // vertex 343
	30, -1, -1, // vertex 344
	30, -1, -1, // vertex 345
	30, 31, -1, // vertex 346
	30, 31, -1, // vertex 347
	30, -1, -1, // vertex 348
	30, -1, -1, // vertex 349
	30, -1, -1, // vertex 350
	30, -1, -1, // vertex 351
	30, -1, -1, // vertex 352
	30, -1, -1, // vertex 353
	30, -1, -1, // vertex 354
	30, -1, -1, // vertex 355
	30, -1, -1, // vertex 356
	30, -1, -1, // vertex 357
	30, -1, -1, // vertex 358
	30, -1, -1, // vertex 359
	30, -1, -1, // vertex 360
	30, -1, -1, // vertex 361
	30, 33, -1, // vertex 362
	30, 33, -1, // vertex 363
	33, 30, -1, // vertex 364
	30, 31, -1, // vertex 365
	30, 31, -1, // vertex 366
	30, 31, -1, // vertex 367
	30, 31, -1, // vertex 368
	30, 31, -1, // vertex 369
	31, 30, -1, // vertex 370
	30, 31, -1, // vertex 371
	30, 31, -1, // vertex 372
	30, 31, -1, // vertex 373
	30, 31, -1, // vertex 374
	30, 31, -1, // vertex 375
	30, 31, -1, // vertex 376
	30, 31, -1, // vertex 377
	30, 31, -1, // vertex 378
	30, 31, -1, // vertex 379
	30, 31, -1, // vertex 380
	30, 31, -1, // vertex 381
	30, -1, -1, // vertex 382
	30, -1, -1, // vertex 383
	30, -1, -1, // vertex 384
	30, -1, -1, // vertex 385
	30, -1, -1, // vertex 386
	30, -1, -1, // vertex 387
	30, -1, -1, // vertex 388
	30, -1, -1, // vertex 389
	30, -1, -1, // vertex 390
	29, 30, -1, // vertex 391
	30, -1, -1, // vertex 392
	30, 29, -1, // vertex 393
	30, 31, -1, // vertex 394
	30, -1, -1, // vertex 395
	30, 31, -1, // vertex 396
	30, -1, -1, // vertex 397
	30, -1, -1, // vertex 398
	30, -1, -1, // vertex 399
	30, -1, -1, // vertex 400
	30, -1, -1, // vertex 401
	30, -1, -1, // vertex 402
	30, -1, -1, // vertex 403
	30, -1, -1, // vertex 404
	30, -1, -1, // vertex 405
	30, 31, -1, // vertex 406
	30, 31, -1, // vertex 407
	30, 31, -1, // vertex 408
	33, 30, -1, // vertex 409
	30, -1, -1, // vertex 410
	33, 30, -1, // vertex 411
	30, -1, -1, // vertex 412
	30, 33, -1, // vertex 413
	30, -1, -1, // vertex 414
	30, 33, -1, // vertex 415
	30, -1, -1, // vertex 416
	30, 33, -1, // vertex 417
	30, -1, -1, // vertex 418
	30, -1, -1, // vertex 419
	30, -1, -1, // vertex 420
	30, -1, -1, // vertex 421
	30, 33, -1, // vertex 422
	30, -1, -1, // vertex 423
	30, -1, -1, // vertex 424
	30, -1, -1, // vertex 425
	30, -1, -1, // vertex 426
	30, -1, -1, // vertex 427
	30, -1, -1, // vertex 428
	30, -1, -1, // vertex 429
	30, -1, -1, // vertex 430
	30, -1, -1, // vertex 431
	30, -1, -1, // vertex 432
	30, -1, -1, // vertex 433
	30, -1, -1, // vertex 434
	30, -1, -1, // vertex 435
	30, 31, -1, // vertex 436
	30, 31, -1, // vertex 437
	30, -1, -1, // vertex 438
	30, -1, -1, // vertex 439
	30, -1, -1, // vertex 440
	30, 32, -1, // vertex 441
	30, -1, -1, // vertex 442
	30, -1, -1, // vertex 443
	30, -1, -1, // vertex 444
	30, -1, -1, // vertex 445
	30, -1, -1, // vertex 446
	30, -1, -1, // vertex 447
	30, -1, -1, // vertex 448
	30, -1, -1, // vertex 449
	30, -1, -1, // vertex 450
	30, 32, -1, // vertex 451
	30, 32, -1, // vertex 452
	30, 32, -1, // vertex 453
	30, 32, -1, // vertex 454
	30, -1, -1, // vertex 455
	30, 32, -1, // vertex 456
	30, -1, -1, // vertex 457
	30, 32, -1, // vertex 458
	30, -1, -1, // vertex 459
	30, 32, -1, // vertex 460
	30, -1, -1, // vertex 461
	30, -1, -1, // vertex 462
	30, 32, -1, // vertex 463
	30, 32, -1, // vertex 464
	30, 32, -1, // vertex 465
	30, 32, -1, // vertex 466
	30, 32, -1, // vertex 467
	30, 32, -1, // vertex 468
	30, 32, -1, // vertex 469
	30, 32, -1, // vertex 470
	30, 32, -1, // vertex 471
	30, 32, -1, // vertex 472
	30, 32, -1, // vertex 473
	30, 32, -1, // vertex 474
	30, 32, -1, // vertex 475
	30, 32, -1, // vertex 476
	30, 32, -1, // vertex 477
	30, 32, -1, // vertex 478
	30, 32, -1, // vertex 479
	30, 32, -1, // vertex 480
	30, 32, -1, // vertex 481
	30, 32, -1, // vertex 482
	30, 32, -1, // vertex 483
	30, 32, -1, // vertex 484
	30, 32, -1, // vertex 485
	30, -1, -1, // vertex 486
	30, -1, -1, // vertex 487
	30, -1, -1, // vertex 488
	30, -1, -1, // vertex 489
	30, -1, -1, // vertex 490
	30, -1, -1, // vertex 491
	30, -1, -1, // vertex 492
	30, -1, -1, // vertex 493
	30, -1, -1, // vertex 494
	30, -1, -1, // vertex 495
	30, -1, -1, // vertex 496
	30, -1, -1, // vertex 497
	30, -1, -1, // vertex 498
	30, -1, -1, // vertex 499
	30, -1, -1, // vertex 500
	29, 30, -1, // vertex 501
	30, -1, -1, // vertex 502
	30, -1, -1, // vertex 503
	30, -1, -1, // vertex 504
	30, -1, -1, // vertex 505
	30, -1, -1, // vertex 506
	30, -1, -1, // vertex 507
	30, -1, -1, // vertex 508
	30, -1, -1, // vertex 509
	30, -1, -1, // vertex 510
	30, -1, -1, // vertex 511
	30, -1, -1, // vertex 512
	30, -1, -1, // vertex 513
	30, -1, -1, // vertex 514
	30, -1, -1, // vertex 515
	30, 32, -1, // vertex 516
	30, 32, -1, // vertex 517
	30, 32, -1, // vertex 518
	30, -1, -1, // vertex 519
	30, -1, -1, // vertex 520
	30, -1, -1, // vertex 521
	30, 31, -1, // vertex 522
	30, 31, -1, // vertex 523
	30, 31, -1, // vertex 524
	30, 31, -1, // vertex 525
	30, 31, -1, // vertex 526
	30, 31, -1, // vertex 527
	30, 31, -1, // vertex 528
	30, 31, -1, // vertex 529
	30, 31, -1, // vertex 530
	30, 31, -1, // vertex 531
	30, 31, -1, // vertex 532
	30, 31, -1, // vertex 533
	30, 31, -1, // vertex 534
	30, 31, -1, // vertex 535
	30, 31, -1, // vertex 536
	30, 31, -1, // vertex 537
	30, 31, -1, // vertex 538
	30, 31, -1, // vertex 539
	30, 31, -1, // vertex 540
	30, 31, -1, // vertex 541
	30, 31, -1, // vertex 542
	30, 31, -1, // vertex 543
	30, 31, -1, // vertex 544
	30, 31, -1, // vertex 545
	30, 31, -1, // vertex 546
	30, 31, -1, // vertex 547
	30, 31, -1, // vertex 548
	30, 31, -1, // vertex 549
	30, 31, -1, // vertex 550
	30, 31, -1, // vertex 551
	30, 31, -1, // vertex 552
	31, 30, -1, // vertex 553
	30, 31, -1, // vertex 554
	31, 30, -1, // vertex 555
	31, 30, -1, // vertex 556
	30, 31, -1, // vertex 557
	30, 31, -1, // vertex 558
	30, 31, -1, // vertex 559
	30, 31, -1, // vertex 560
	30, 31, -1, // vertex 561
	31, 30, -1, // vertex 562
	30, 31, -1, // vertex 563
	30, 31, -1, // vertex 564
	30, 31, -1, // vertex 565
	30, 31, -1, // vertex 566
	30, 31, -1, // vertex 567
	30, 31, -1, // vertex 568
	31, 30, -1, // vertex 569
	31, 30, -1, // vertex 570
	30, -1, -1, // vertex 571
	30, -1, -1, // vertex 572
	30, -1, -1, // vertex 573
	30, -1, -1, // vertex 574
	30, -1, -1, // vertex 575
	30, -1, -1, // vertex 576
	30, -1, -1, // vertex 577
	30, -1, -1, // vertex 578
	30, -1, -1, // vertex 579
	30, -1, -1, // vertex 580
	30, -1, -1, // vertex 581
	30, -1, -1, // vertex 582
	30, -1, -1, // vertex 583
	30, -1, -1, // vertex 584
	30, -1, -1, // vertex 585
	30, -1, -1, // vertex 586
	30, -1, -1, // vertex 587
	30, -1, -1, // vertex 588
	30, -1, -1, // vertex 589
	30, -1, -1, // vertex 590
	30, -1, -1, // vertex 591
	30, 31, -1, // vertex 592
	30, -1, -1, // vertex 593
	30, 31, -1, // vertex 594
	30, 29, -1, // vertex 595
	30, 29, -1, // vertex 596
	30, -1, -1, // vertex 597
	29, 30, -1, // vertex 598
	30, 29, -1, // vertex 599
	30, 29, -1, // vertex 600
	30, 31, -1, // vertex 601
	30, -1, -1, // vertex 602
	30, 31, -1, // vertex 603
	33, 30, -1, // vertex 604
	33, 30, -1, // vertex 605
	33, 30, -1, // vertex 606
	33, 30, -1, // vertex 607
	33, 30, -1, // vertex 608
	30, 33, -1, // vertex 609
	30, 33, -1, // vertex 610
	30, 33, -1, // vertex 611
	30, 32, -1, // vertex 612
	30, 33, -1, // vertex 613
	30, 33, -1, // vertex 614
	33, 30, -1, // vertex 615
	33, 30, -1, // vertex 616
	33, 30, -1, // vertex 617
	33, 30, -1, // vertex 618
	33, 30, -1, // vertex 619
	33, 30, -1, // vertex 620
	33, 30, -1, // vertex 621
	33, 30, -1, // vertex 622
	33, 30, -1, // vertex 623
	33, 30, -1, // vertex 624
	33, 30, -1, // vertex 625
	30, 33, -1, // vertex 626
	30, 33, -1, // vertex 627
	30, 33, -1, // vertex 628
	30, 32, -1, // vertex 629
	30, 32, -1, // vertex 630
	30, -1, -1, // vertex 631
	30, -1, -1, // vertex 632
	30, -1, -1, // vertex 633
	30, -1, -1, // vertex 634
	30, -1, -1, // vertex 635
	30, -1, -1, // vertex 636
	30, 33, -1, // vertex 637
	30, 33, -1, // vertex 638
	30, -1, -1, // vertex 639
	30, 32, -1, // vertex 640
	30, -1, -1, // vertex 641
	30, 32, -1, // vertex 642
	30, -1, -1, // vertex 643
	30, 32, -1, // vertex 644
	30, -1, -1, // vertex 645
	30, -1, -1, // vertex 646
	30, -1, -1, // vertex 647
	30, 32, -1, // vertex 648
	30, 32, -1, // vertex 649
	30, 32, -1, // vertex 650
	30, 32, -1, // vertex 651
	30, 32, -1, // vertex 652
	30, 32, -1, // vertex 653
	30, 32, -1, // vertex 654
	30, 32, -1, // vertex 655
	30, 32, -1, // vertex 656
	30, 32, -1, // vertex 657
	30, 32, -1, // vertex 658
	30, -1, -1, // vertex 659
	30, -1, -1, // vertex 660
	30, -1, -1, // vertex 661
	33, 30, -1, // vertex 662
	33, 30, -1, // vertex 663
	33, 30, -1, // vertex 664
	33, 30, -1, // vertex 665
	33, 30, -1, // vertex 666
	33, 30, -1, // vertex 667
	33, 30, -1, // vertex 668
	33, 30, -1, // vertex 669
	33, 30, -1, // vertex 670
	33, 30, -1, // vertex 671
	33, 30, -1, // vertex 672
	33, 30, -1, // vertex 673
	33, 30, -1, // vertex 674
	33, 30, -1, // vertex 675
	30, 33, -1, // vertex 676
	30, 33, -1, // vertex 677
	33, 30, -1, // vertex 678
	33, 30, -1, // vertex 679
	33, 30, -1, // vertex 680
	33, 30, -1, // vertex 681
	33, 30, -1, // vertex 682
	30, -1, -1, // vertex 683
	30, -1, -1, // vertex 684
	30, -1, -1, // vertex 685
	30, -1, -1, // vertex 686
	30, -1, -1, // vertex 687
	30, -1, -1, // vertex 688
	30, 32, -1, // vertex 689
	30, -1, -1, // vertex 690
	30, 32, -1, // vertex 691
	30, -1, -1, // vertex 692
	30, 32, -1, // vertex 693
	30, -1, -1, // vertex 694
	33, 30, -1, // vertex 695
	30, 33, -1, // vertex 696
	30, 33, -1, // vertex 697
	30, -1, -1, // vertex 698
	30, -1, -1, // vertex 699
	30, -1, -1, // vertex 700
	30, 32, -1, // vertex 701
	30, 32, -1, // vertex 702
	30, 32, -1, // vertex 703
	30, 32, -1, // vertex 704
	30, 32, -1, // vertex 705
	30, 32, -1, // vertex 706
	30, 32, -1, // vertex 707
	30, 32, -1, // vertex 708
	30, 32, -1, // vertex 709
	30, 32, -1, // vertex 710
	30, 32, -1, // vertex 711
	30, -1, -1, // vertex 712
	30, -1, -1, // vertex 713
	30, -1, -1, // vertex 714
	30, 31, -1, // vertex 715
	30, 31, -1, // vertex 716
	30, 31, -1, // vertex 717
	30, 31, -1, // vertex 718
	30, 31, -1, // vertex 719
	30, 31, -1, // vertex 720
	30, 31, -1, // vertex 721
	30, 31, -1, // vertex 722
	30, 31, -1, // vertex 723
	30, 31, -1, // vertex 724
	30, 31, -1, // vertex 725
	30, 31, -1, // vertex 726
	30, 31, -1, // vertex 727
	30, 31, -1, // vertex 728
	30, 31, -1, // vertex 729
	30, 31, -1, // vertex 730
	30, 31, -1, // vertex 731
	30, 31, -1, // vertex 732
	30, 31, -1, // vertex 733
	30, 31, -1, // vertex 734
	30, 31, -1, // vertex 735
	30, 31, -1, // vertex 736
	30, 31, -1, // vertex 737
	30, 31, -1, // vertex 738
	30, 31, -1, // vertex 739
	30, 31, -1, // vertex 740
	30, 31, -1, // vertex 741
	30, 31, -1, // vertex 742
	30, 31, -1, // vertex 743
	30, 31, -1, // vertex 744
	30, 31, -1, // vertex 745
	30, 31, -1, // vertex 746
	30, 31, -1, // vertex 747
	30, 31, -1, // vertex 748
	30, 31, -1, // vertex 749
	30, -1, -1, // vertex 750
	30, -1, -1, // vertex 751
	30, 31, -1, // vertex 752
	30, -1, -1, // vertex 753
	30, -1, -1, // vertex 754
	30, 32, -1, // vertex 755
	30, 32, -1, // vertex 756
	30, 32, -1, // vertex 757
	30, 32, -1, // vertex 758
	30, 32, -1, // vertex 759
	30, 32, -1, // vertex 760
	30, 32, -1, // vertex 761
	30, 32, -1, // vertex 762
	30, -1, -1, // vertex 763
	30, -1, -1, // vertex 764
	30, 31, -1, // vertex 765
	30, -1, -1, // vertex 766
	30, -1, -1, // vertex 767
	30, -1, -1, // vertex 768
	30, -1, -1, // vertex 769
	30, 31, -1, // vertex 770
	30, 31, -1, // vertex 771
	30, 31, -1, // vertex 772
	30, 31, -1, // vertex 773
	30, 31, -1, // vertex 774
	30, 31, -1, // vertex 775
	30, 31, -1, // vertex 776
	30, 33, 31, // vertex 777
	30, 31, -1, // vertex 778
	30, 33, 31, // vertex 779
	30, 31, -1, // vertex 780
	30, 33, 31, // vertex 781
	30, 33, 31, // vertex 782
	30, 33, -1, // vertex 783
	30, 33, -1, // vertex 784
	30, 33, 31, // vertex 785
	30, 33, -1, // vertex 786
	30, 33, 31, // vertex 787
	30, 33, -1, // vertex 788
	30, 33, 31, // vertex 789
	33, 30, -1, // vertex 790
	30, 31, -1, // vertex 791
	30, 31, -1, // vertex 792
	30, 31, -1, // vertex 793
	30, 31, -1, // vertex 794
	30, 31, -1, // vertex 795
	30, -1, -1, // vertex 796
	30, 31, -1, // vertex 797
	30, 31, -1, // vertex 798
	30, 31, -1, // vertex 799
	30, 31, -1, // vertex 800
	30, 31, -1, // vertex 801
	30, 31, -1, // vertex 802
	30, 31, -1, // vertex 803
	30, 31, -1, // vertex 804
	30, 31, -1, // vertex 805
	30, 31, -1, // vertex 806
	30, 31, -1, // vertex 807
	30, 31, -1, // vertex 808
	30, 31, -1, // vertex 809
	30, 31, -1, // vertex 810
	30, 31, -1, // vertex 811
	30, 31, -1, // vertex 812
	30, 31, -1, // vertex 813
	30, 31, -1, // vertex 814
	30, 31, -1, // vertex 815
	30, 31, -1, // vertex 816
	30, 31, -1, // vertex 817
	30, -1, -1, // vertex 818
	30, 31, -1, // vertex 819
	30, 31, -1, // vertex 820
	30, 31, -1, // vertex 821
	30, 31, -1, // vertex 822
	30, 31, -1, // vertex 823
	30, 31, -1, // vertex 824
	30, 31, -1, // vertex 825
	30, 31, -1, // vertex 826
	30, 31, -1, // vertex 827
	30, 31, -1, // vertex 828
	30, 31, -1, // vertex 829
	30, 31, -1, // vertex 830
	30, 33, 31, // vertex 831
	30, 31, -1, // vertex 832
	30, 31, -1, // vertex 833
	30, 31, -1, // vertex 834
	30, 31, -1, // vertex 835
	30, 31, -1, // vertex 836
	30, 31, -1, // vertex 837
	30, 31, -1, // vertex 838
	30, 31, -1, // vertex 839
	30, 31, -1, // vertex 840
	30, -1, -1, // vertex 841
	30, -1, -1, // vertex 842
	30, 31, -1, // vertex 843
	30, 31, -1, // vertex 844
	30, 31, -1, // vertex 845
	30, 31, -1, // vertex 846
	30, 31, -1, // vertex 847
	33, 30, -1, // vertex 848
	30, 31, -1, // vertex 849
	30, 33, -1, // vertex 850
	30, 33, 31, // vertex 851
	30, 31, -1, // vertex 852
	30, 31, -1, // vertex 853
	30, 31, -1, // vertex 854
	30, 31, -1, // vertex 855
	30, 31, -1, // vertex 856
	30, 31, -1, // vertex 857
	30, 31, -1, // vertex 858
	30, 31, -1, // vertex 859
	30, 31, -1, // vertex 860
	30, 31, -1, // vertex 861
	30, 31, -1, // vertex 862
	30, 31, -1, // vertex 863
	30, 31, -1, // vertex 864
	30, 31, -1, // vertex 865
	30, 31, -1, // vertex 866
	30, 31, -1, // vertex 867
	30, 31, -1, // vertex 868
	30, 31, -1, // vertex 869
	30, 31, -1, // vertex 870
	30, 31, -1, // vertex 871
	30, 31, -1, // vertex 872
	30, 31, -1, // vertex 873
	30, 31, -1, // vertex 874
	30, 31, -1, // vertex 875
	30, 31, -1, // vertex 876
	30, 31, -1, // vertex 877
	30, 31, -1, // vertex 878
	30, 31, -1, // vertex 879
	30, 31, -1, // vertex 880
	30, 31, -1, // vertex 881
	30, 31, -1, // vertex 882
	30, 31, -1, // vertex 883
	30, 31, -1, // vertex 884
	30, -1, -1, // vertex 885
	30, 31, -1, // vertex 886
	30, 31, -1, // vertex 887
	30, 31, -1, // vertex 888
	30, 31, -1, // vertex 889
	30, 31, -1, // vertex 890
	30, 31, -1, // vertex 891
	30, 31, -1, // vertex 892
	30, 31, -1, // vertex 893
	30, -1, -1, // vertex 894
	30, 31, -1, // vertex 895
	30, 31, -1, // vertex 896
	30, 31, -1, // vertex 897
	30, 32, -1, // vertex 898
	30, 32, -1, // vertex 899
	30, 32, -1, // vertex 900
	30, 32, -1, // vertex 901
	30, 32, -1, // vertex 902
	30, 32, -1, // vertex 903
	30, 32, -1, // vertex 904
	30, 32, -1, // vertex 905
	30, 32, -1, // vertex 906
	30, 32, -1, // vertex 907
	30, 32, -1, // vertex 908
	30, 32, -1, // vertex 909
	30, 32, -1, // vertex 910
	30, 32, -1, // vertex 911
	30, 32, -1, // vertex 912
	30, 32, -1, // vertex 913
	30, 32, -1, // vertex 914
	30, 32, -1, // vertex 915
	30, 32, -1, // vertex 916
	30, 32, -1, // vertex 917
	30, 32, -1, // vertex 918
	30, 32, -1, // vertex 919
	30, 32, -1, // vertex 920
	30, 32, -1, // vertex 921
	30, 32, -1, // vertex 922
	30, 32, -1, // vertex 923
	30, 32, -1, // vertex 924
	30, 32, -1, // vertex 925
	30, 32, -1, // vertex 926
	30, 32, -1, // vertex 927
	30, 32, -1, // vertex 928
	30, 32, -1, // vertex 929
	30, 32, -1, // vertex 930
	30, 32, -1, // vertex 931
	30, 32, -1, // vertex 932
	30, 32, -1, // vertex 933
	30, 32, -1, // vertex 934
	30, 32, -1, // vertex 935
	30, 32, -1, // vertex 936
	30, 32, -1, // vertex 937
	30, 32, -1, // vertex 938
	30, 32, -1, // vertex 939
	30, 32, -1, // vertex 940
	30, 32, -1, // vertex 941
	30, 32, -1, // vertex 942
	30, 32, -1, // vertex 943
	30, 32, -1, // vertex 944
	30, 32, -1, // vertex 945
	30, 32, -1, // vertex 946
	30, 32, -1, // vertex 947
	30, 32, -1, // vertex 948
	30, 32, -1, // vertex 949
	30, 32, -1, // vertex 950
	30, 32, -1, // vertex 951
	30, 32, -1, // vertex 952
	30, 32, -1, // vertex 953
	30, 32, -1, // vertex 954
	30, 32, -1, // vertex 955
	30, 32, -1, // vertex 956
	30, 32, -1, // vertex 957
	30, 32, -1, // vertex 958
	30, 32, -1, // vertex 959
	30, 32, -1, // vertex 960
	30, 32, -1, // vertex 961
	30, 32, -1, // vertex 962
	30, 32, -1, // vertex 963
	30, 32, -1, // vertex 964
	30, 32, -1, // vertex 965
	30, 32, -1, // vertex 966
	30, 32, -1, // vertex 967
	30, 32, -1, // vertex 968
	30, 32, -1, // vertex 969
	30, 32, -1, // vertex 970
	30, 32, -1, // vertex 971
	30, 32, -1, // vertex 972
	30, 32, -1, // vertex 973
	30, 32, -1, // vertex 974
	30, 32, -1, // vertex 975
	30, -1, -1, // vertex 976
	30, -1, -1, // vertex 977
	30, -1, -1, // vertex 978
	30, -1, -1, // vertex 979
	30, -1, -1, // vertex 980
	30, -1, -1, // vertex 981
	30, -1, -1, // vertex 982
	30, -1, -1, // vertex 983
	30, -1, -1, // vertex 984
	30, -1, -1, // vertex 985
	30, -1, -1, // vertex 986
	30, -1, -1, // vertex 987
	30, -1, -1, // vertex 988
	30, -1, -1, // vertex 989
	30, -1, -1, // vertex 990
	30, -1, -1, // vertex 991
	30, -1, -1, // vertex 992
	30, -1, -1, // vertex 993
	30, -1, -1, // vertex 994
	30, -1, -1, // vertex 995
	30, -1, -1, // vertex 996
	30, -1, -1, // vertex 997
	30, -1, -1, // vertex 998
	30, -1, -1, // vertex 999
	30, -1, -1, // vertex 1000
	30, -1, -1, // vertex 1001
	30, -1, -1, // vertex 1002
	30, -1, -1, // vertex 1003
	30, -1, -1, // vertex 1004
	30, -1, -1, // vertex 1005
	30, -1, -1, // vertex 1006
	30, -1, -1, // vertex 1007
	30, -1, -1, // vertex 1008
	30, -1, -1, // vertex 1009
	30, -1, -1, // vertex 1010
	30, -1, -1, // vertex 1011
	30, -1, -1, // vertex 1012
	30, -1, -1, // vertex 1013
	30, -1, -1, // vertex 1014
	30, -1, -1, // vertex 1015
	30, -1, -1, // vertex 1016
	30, -1, -1, // vertex 1017
	30, -1, -1, // vertex 1018
	30, -1, -1, // vertex 1019
	30, -1, -1, // vertex 1020
	30, -1, -1, // vertex 1021
	30, -1, -1, // vertex 1022
	30, -1, -1, // vertex 1023
	30, -1, -1, // vertex 1024
	30, -1, -1, // vertex 1025
	30, -1, -1, // vertex 1026
	30, -1, -1, // vertex 1027
	30, -1, -1, // vertex 1028
	30, -1, -1, // vertex 1029
	30, -1, -1, // vertex 1030
	30, -1, -1, // vertex 1031
	30, -1, -1, // vertex 1032
	30, -1, -1, // vertex 1033
	30, -1, -1, // vertex 1034
	30, -1, -1, // vertex 1035
	30, -1, -1, // vertex 1036
	30, -1, -1, // vertex 1037
	30, -1, -1, // vertex 1038
	30, -1, -1, // vertex 1039
	30, -1, -1, // vertex 1040
	30, -1, -1, // vertex 1041
	30, -1, -1, // vertex 1042
	30, -1, -1, // vertex 1043
	30, -1, -1, // vertex 1044
	30, -1, -1, // vertex 1045
	31, -1, -1, // vertex 1046
	31, -1, -1, // vertex 1047
	31, -1, -1, // vertex 1048
	31, -1, -1, // vertex 1049
	31, -1, -1, // vertex 1050
	31, -1, -1, // vertex 1051
	31, -1, -1, // vertex 1052
	31, -1, -1, // vertex 1053
	31, -1, -1, // vertex 1054
	31, -1, -1, // vertex 1055
	31, -1, -1, // vertex 1056
	31, -1, -1, // vertex 1057
	31, -1, -1, // vertex 1058
	31, -1, -1, // vertex 1059
	31, -1, -1, // vertex 1060
	31, -1, -1, // vertex 1061
	31, -1, -1, // vertex 1062
	31, -1, -1, // vertex 1063
	31, -1, -1, // vertex 1064
	31, -1, -1, // vertex 1065
	31, -1, -1, // vertex 1066
	31, -1, -1, // vertex 1067
	31, -1, -1, // vertex 1068
	31, -1, -1, // vertex 1069
	31, -1, -1, // vertex 1070
	31, -1, -1, // vertex 1071
	31, -1, -1, // vertex 1072
	31, -1, -1, // vertex 1073
	31, -1, -1, // vertex 1074
	31, -1, -1, // vertex 1075
	31, -1, -1, // vertex 1076
	31, -1, -1, // vertex 1077
	31, -1, -1, // vertex 1078
	31, -1, -1, // vertex 1079
	31, -1, -1, // vertex 1080
	30, -1, -1, // vertex 1081
	30, -1, -1, // vertex 1082
	30, -1, -1, // vertex 1083
	30, -1, -1, // vertex 1084
	30, -1, -1, // vertex 1085
	30, -1, -1, // vertex 1086
	30, -1, -1, // vertex 1087
	30, -1, -1, // vertex 1088
	30, -1, -1, // vertex 1089
	30, -1, -1, // vertex 1090
	30, -1, -1, // vertex 1091
	30, -1, -1, // vertex 1092
	30, -1, -1, // vertex 1093
	30, -1, -1, // vertex 1094
	30, -1, -1, // vertex 1095
	30, -1, -1, // vertex 1096
	30, -1, -1, // vertex 1097
	30, -1, -1, // vertex 1098
	30, -1, -1, // vertex 1099
	30, -1, -1, // vertex 1100
	30, -1, -1, // vertex 1101
	30, -1, -1, // vertex 1102
	30, -1, -1, // vertex 1103
	31, -1, -1, // vertex 1104
	31, -1, -1, // vertex 1105
	31, -1, -1, // vertex 1106
	31, -1, -1, // vertex 1107
	31, -1, -1, // vertex 1108
	31, -1, -1, // vertex 1109
	31, -1, -1, // vertex 1110
	31, -1, -1, // vertex 1111
	31, -1, -1, // vertex 1112
	31, -1, -1, // vertex 1113
	31, -1, -1, // vertex 1114
	31, -1, -1, // vertex 1115
	31, -1, -1, // vertex 1116
	31, -1, -1, // vertex 1117
	31, -1, -1, // vertex 1118
	31, -1, -1, // vertex 1119
	31, -1, -1, // vertex 1120
	31, -1, -1, // vertex 1121
	31, -1, -1, // vertex 1122
	31, -1, -1, // vertex 1123
	31, -1, -1, // vertex 1124
	31, -1, -1, // vertex 1125
	31, -1, -1, // vertex 1126
	31, -1, -1, // vertex 1127
	31, -1, -1, // vertex 1128
	31, -1, -1, // vertex 1129
	31, -1, -1, // vertex 1130
	31, -1, -1, // vertex 1131
	31, -1, -1, // vertex 1132
	31, -1, -1, // vertex 1133
	31, -1, -1, // vertex 1134
	30, -1, -1, // vertex 1135
	30, -1, -1, // vertex 1136
	30, -1, -1, // vertex 1137
	30, -1, -1, // vertex 1138
	30, -1, -1, // vertex 1139
	30, -1, -1, // vertex 1140
	30, -1, -1, // vertex 1141
	30, -1, -1, // vertex 1142
	30, -1, -1, // vertex 1143
	30, -1, -1, // vertex 1144
	30, -1, -1, // vertex 1145
	30, -1, -1, // vertex 1146
	30, -1, -1, // vertex 1147
	30, -1, -1, // vertex 1148
	30, -1, -1, // vertex 1149
	30, -1, -1, // vertex 1150
	30, -1, -1, // vertex 1151
	30, -1, -1, // vertex 1152
	30, -1, -1, // vertex 1153
	30, -1, -1, // vertex 1154
	30, -1, -1, // vertex 1155
	30, -1, -1, // vertex 1156
	30, -1, -1, // vertex 1157
	30, -1, -1, // vertex 1158
	30, -1, -1, // vertex 1159
	30, -1, -1, // vertex 1160
	30, -1, -1, // vertex 1161
	30, -1, -1, // vertex 1162
	30, -1, -1, // vertex 1163
	30, -1, -1, // vertex 1164
	30, -1, -1, // vertex 1165
	30, -1, -1, // vertex 1166
	30, -1, -1, // vertex 1167
	30, -1, -1, // vertex 1168
	30, -1, -1, // vertex 1169
	30, -1, -1, // vertex 1170
	30, -1, -1, // vertex 1171
	30, -1, -1, // vertex 1172
	30, -1, -1, // vertex 1173
	30, -1, -1, // vertex 1174
	30, -1, -1, // vertex 1175
	30, -1, -1, // vertex 1176
	30, -1, -1, // vertex 1177
	30, -1, -1, // vertex 1178
	30, -1, -1, // vertex 1179
	30, 31, -1, // vertex 1180
	30, -1, -1, // vertex 1181
	30, -1, -1, // vertex 1182
	30, 32, -1, // vertex 1183
	30, 32, -1, // vertex 1184
	30, 32, -1, // vertex 1185
	30, 32, -1, // vertex 1186
	30, 32, -1, // vertex 1187
	30, 32, -1, // vertex 1188
	30, 32, -1, // vertex 1189
	30, 32, -1, // vertex 1190
	30, -1, -1, // vertex 1191
	30, -1, -1, // vertex 1192
	30, 31, -1, // vertex 1193
	30, -1, -1, // vertex 1194
	30, -1, -1, // vertex 1195
	30, -1, -1, // vertex 1196
	30, -1, -1, // vertex 1197
	30, 31, -1, // vertex 1198
	30, 31, -1, // vertex 1199
	30, 31, -1, // vertex 1200
	30, 31, -1, // vertex 1201
	30, 31, -1, // vertex 1202
	30, 31, -1, // vertex 1203
	30, 31, -1, // vertex 1204
	30, 33, 31, // vertex 1205
	30, 31, -1, // vertex 1206
	30, 33, 31, // vertex 1207
	30, 31, -1, // vertex 1208
	30, 33, 31, // vertex 1209
	30, 33, 31, // vertex 1210
	30, 33, -1, // vertex 1211
	30, 33, -1, // vertex 1212
	30, 33, 31, // vertex 1213
	30, 33, -1, // vertex 1214
	30, 33, 31, // vertex 1215
	30, 33, -1, // vertex 1216
	30, 33, 31, // vertex 1217
	33, 30, -1, // vertex 1218
	30, 31, -1, // vertex 1219
	30, 31, -1, // vertex 1220
	30, 31, -1, // vertex 1221
	30, 31, -1, // vertex 1222
	30, 31, -1, // vertex 1223
	30, -1, -1, // vertex 1224
	30, 31, -1, // vertex 1225
	30, 31, -1, // vertex 1226
	30, 31, -1, // vertex 1227
	30, 31, -1, // vertex 1228
	30, 31, -1, // vertex 1229
	30, 31, -1, // vertex 1230
	30, 31, -1, // vertex 1231
	30, 31, -1, // vertex 1232
	30, 31, -1, // vertex 1233
	30, 31, -1, // vertex 1234
	30, 31, -1, // vertex 1235
	30, 31, -1, // vertex 1236
	30, 31, -1, // vertex 1237
	30, 31, -1, // vertex 1238
	30, 31, -1, // vertex 1239
	30, 31, -1, // vertex 1240
	30, 31, -1, // vertex 1241
	30, 31, -1, // vertex 1242
	30, 31, -1, // vertex 1243
	30, 31, -1, // vertex 1244
	30, 31, -1, // vertex 1245
	30, -1, -1, // vertex 1246
	30, 31, -1, // vertex 1247
	30, 31, -1, // vertex 1248
	30, 31, -1, // vertex 1249
	30, 31, -1, // vertex 1250
	30, 31, -1, // vertex 1251
	30, 31, -1, // vertex 1252
	30, 31, -1, // vertex 1253
	30, 31, -1, // vertex 1254
	30, 31, -1, // vertex 1255
	30, 31, -1, // vertex 1256
	30, 31, -1, // vertex 1257
	30, 31, -1, // vertex 1258
	30, 33, 31, // vertex 1259
	30, 31, -1, // vertex 1260
	30, 31, -1, // vertex 1261
	30, 31, -1, // vertex 1262
	30, 31, -1, // vertex 1263
	30, 31, -1, // vertex 1264
	30, 31, -1, // vertex 1265
	30, 31, -1, // vertex 1266
	30, 31, -1, // vertex 1267
	30, 31, -1, // vertex 1268
	30, -1, -1, // vertex 1269
	30, -1, -1, // vertex 1270
	30, 31, -1, // vertex 1271
	30, 31, -1, // vertex 1272
	30, 31, -1, // vertex 1273
	30, 31, -1, // vertex 1274
	30, 31, -1, // vertex 1275
	33, 30, -1, // vertex 1276
	30, 31, -1, // vertex 1277
	30, 33, -1, // vertex 1278
	30, 33, 31, // vertex 1279
	30, 31, -1, // vertex 1280
	30, 31, -1, // vertex 1281
	30, 31, -1, // vertex 1282
	30, 31, -1, // vertex 1283
	30, 31, -1, // vertex 1284
	30, 31, -1, // vertex 1285
	30, 31, -1, // vertex 1286
	30, 31, -1, // vertex 1287
	30, 31, -1, // vertex 1288
	30, 31, -1, // vertex 1289
	30, 31, -1, // vertex 1290
	30, 31, -1, // vertex 1291
	30, 31, -1, // vertex 1292
	30, 31, -1, // vertex 1293
	30, 31, -1, // vertex 1294
	30, 31, -1, // vertex 1295
	30, 31, -1, // vertex 1296
	30, 31, -1, // vertex 1297
	30, 31, -1, // vertex 1298
	30, 31, -1, // vertex 1299
	30, 31, -1, // vertex 1300
	30, 31, -1, // vertex 1301
	30, 31, -1, // vertex 1302
	30, 31, -1, // vertex 1303
	30, 31, -1, // vertex 1304
	30, 31, -1, // vertex 1305
	30, 31, -1, // vertex 1306
	30, 31, -1, // vertex 1307
	30, 31, -1, // vertex 1308
	30, 31, -1, // vertex 1309
	30, 31, -1, // vertex 1310
	30, 31, -1, // vertex 1311
	30, 31, -1, // vertex 1312
	30, -1, -1, // vertex 1313
	30, 31, -1, // vertex 1314
	30, 31, -1, // vertex 1315
	30, 31, -1, // vertex 1316
	30, 31, -1, // vertex 1317
	30, 31, -1, // vertex 1318
	30, 31, -1, // vertex 1319
	30, 31, -1, // vertex 1320
	30, 31, -1, // vertex 1321
	30, -1, -1, // vertex 1322
	30, 31, -1, // vertex 1323
	30, 31, -1, // vertex 1324
	30, 31, -1, // vertex 1325
	30, 32, -1, // vertex 1326
	30, 32, -1, // vertex 1327
	30, 32, -1, // vertex 1328
	30, 32, -1, // vertex 1329
	30, 32, -1, // vertex 1330
	30, 32, -1, // vertex 1331
	30, 32, -1, // vertex 1332
	30, 32, -1, // vertex 1333
	30, 32, -1, // vertex 1334
	30, 32, -1, // vertex 1335
	30, 32, -1, // vertex 1336
	30, 32, -1, // vertex 1337
	30, 32, -1, // vertex 1338
	30, 32, -1, // vertex 1339
	30, 32, -1, // vertex 1340
	30, 32, -1, // vertex 1341
	30, 32, -1, // vertex 1342
	30, 32, -1, // vertex 1343
	30, 32, -1, // vertex 1344
	30, 32, -1, // vertex 1345
	30, 32, -1, // vertex 1346
	30, 32, -1, // vertex 1347
	30, 32, -1, // vertex 1348
	30, 32, -1, // vertex 1349
	30, 32, -1, // vertex 1350
	30, 32, -1, // vertex 1351
	30, 32, -1, // vertex 1352
	30, 32, -1, // vertex 1353
	30, 32, -1, // vertex 1354
	30, 32, -1, // vertex 1355
	30, 32, -1, // vertex 1356
	30, 32, -1, // vertex 1357
	30, 32, -1, // vertex 1358
	30, 32, -1, // vertex 1359
	30, 32, -1, // vertex 1360
	30, 32, -1, // vertex 1361
	30, 32, -1, // vertex 1362
	30, 32, -1, // vertex 1363
	30, 32, -1, // vertex 1364
	30, 32, -1, // vertex 1365
	30, 32, -1, // vertex 1366
	30, 32, -1, // vertex 1367
	30, 32, -1, // vertex 1368
	30, 32, -1, // vertex 1369
	30, 32, -1, // vertex 1370
	30, 32, -1, // vertex 1371
	30, 32, -1, // vertex 1372
	30, 32, -1, // vertex 1373
	30, 32, -1, // vertex 1374
	30, 32, -1, // vertex 1375
	30, 32, -1, // vertex 1376
	30, 32, -1, // vertex 1377
	30, 32, -1, // vertex 1378
	30, 32, -1, // vertex 1379
	30, 32, -1, // vertex 1380
	30, 32, -1, // vertex 1381
	30, 32, -1, // vertex 1382
	30, 32, -1, // vertex 1383
	30, 32, -1, // vertex 1384
	30, 32, -1, // vertex 1385
	30, 32, -1, // vertex 1386
	30, 32, -1, // vertex 1387
	30, 32, -1, // vertex 1388
	30, 32, -1, // vertex 1389
	30, 32, -1, // vertex 1390
	30, 32, -1, // vertex 1391
	30, 32, -1, // vertex 1392
	30, 32, -1, // vertex 1393
	30, 32, -1, // vertex 1394
	30, 32, -1, // vertex 1395
	30, 32, -1, // vertex 1396
	30, 32, -1, // vertex 1397
	30, 32, -1, // vertex 1398
	30, 32, -1, // vertex 1399
	30, 32, -1, // vertex 1400
	30, 32, -1, // vertex 1401
	30, 32, -1, // vertex 1402
	30, 32, -1, // vertex 1403
};

float testWeights[] = {
	1.000000f, 0.000000f, 0.000000f, // vertex 0
	1.000000f, 0.000000f, 0.000000f, // vertex 1
	1.000000f, 0.000000f, 0.000000f, // vertex 2
	1.000000f, 0.000000f, 0.000000f, // vertex 3
	1.000000f, 0.000000f, 0.000000f, // vertex 4
	1.000000f, 0.000000f, 0.000000f, // vertex 5
	1.000000f, 0.000000f, 0.000000f, // vertex 6
	1.000000f, 0.000000f, 0.000000f, // vertex 7
	1.000000f, 0.000000f, 0.000000f, // vertex 8
	0.500000f, 0.500000f, 0.000000f, // vertex 9
	0.500000f, 0.500000f, 0.000000f, // vertex 10
	0.500000f, 0.500000f, 0.000000f, // vertex 11
	0.500000f, 0.500000f, 0.000000f, // vertex 12
	0.500000f, 0.500000f, 0.000000f, // vertex 13
	0.500000f, 0.500000f, 0.000000f, // vertex 14
	0.555556f, 0.444444f, 0.000000f, // vertex 15
	0.555556f, 0.444444f, 0.000000f, // vertex 16
	0.833333f, 0.166667f, 0.000000f, // vertex 17
	0.666667f, 0.333333f, 0.000000f, // vertex 18
	0.833333f, 0.166667f, 0.000000f, // vertex 19
	0.833333f, 0.166667f, 0.000000f, // vertex 20
	0.833333f, 0.166667f, 0.000000f, // vertex 21
	1.000000f, 0.000000f, 0.000000f, // vertex 22
	1.000000f, 0.000000f, 0.000000f, // vertex 23
	1.000000f, 0.000000f, 0.000000f, // vertex 24
	1.000000f, 0.000000f, 0.000000f, // vertex 25
	1.000000f, 0.000000f, 0.000000f, // vertex 26
	1.000000f, 0.000000f, 0.000000f, // vertex 27
	0.926651f, 0.073349f, 0.000000f, // vertex 28
	1.000000f, 0.000000f, 0.000000f, // vertex 29
	0.833333f, 0.166667f, 0.000000f, // vertex 30
	1.000000f, 0.000000f, 0.000000f, // vertex 31
	0.833333f, 0.166667f, 0.000000f, // vertex 32
	1.000000f, 0.000000f, 0.000000f, // vertex 33
	0.666667f, 0.333333f, 0.000000f, // vertex 34
	0.833333f, 0.166667f, 0.000000f, // vertex 35
	0.666667f, 0.333333f, 0.000000f, // vertex 36
	0.555556f, 0.444444f, 0.000000f, // vertex 37
	0.500000f, 0.500000f, 0.000000f, // vertex 38
	0.555556f, 0.444444f, 0.000000f, // vertex 39
	0.500000f, 0.500000f, 0.000000f, // vertex 40
	0.555556f, 0.444444f, 0.000000f, // vertex 41
	0.500000f, 0.500000f, 0.000000f, // vertex 42
	1.000000f, 0.000000f, 0.000000f, // vertex 43
	1.000000f, 0.000000f, 0.000000f, // vertex 44
	1.000000f, 0.000000f, 0.000000f, // vertex 45
	1.000000f, 0.000000f, 0.000000f, // vertex 46
	1.000000f, 0.000000f, 0.000000f, // vertex 47
	1.000000f, 0.000000f, 0.000000f, // vertex 48
	1.000000f, 0.000000f, 0.000000f, // vertex 49
	1.000000f, 0.000000f, 0.000000f, // vertex 50
	0.833333f, 0.166667f, 0.000000f, // vertex 51
	0.833333f, 0.166667f, 0.000000f, // vertex 52
	0.833333f, 0.166667f, 0.000000f, // vertex 53
	0.666667f, 0.333333f, 0.000000f, // vertex 54
	0.833333f, 0.166667f, 0.000000f, // vertex 55
	0.555556f, 0.444444f, 0.000000f, // vertex 56
	0.555556f, 0.444444f, 0.000000f, // vertex 57
	0.500000f, 0.500000f, 0.000000f, // vertex 58
	0.500000f, 0.500000f, 0.000000f, // vertex 59
	0.500000f, 0.500000f, 0.000000f, // vertex 60
	0.500000f, 0.500000f, 0.000000f, // vertex 61
	0.500000f, 0.500000f, 0.000000f, // vertex 62
	0.500000f, 0.500000f, 0.000000f, // vertex 63
	0.500000f, 0.500000f, 0.000000f, // vertex 64
	0.555556f, 0.444444f, 0.000000f, // vertex 65
	0.500000f, 0.500000f, 0.000000f, // vertex 66
	0.555556f, 0.444444f, 0.000000f, // vertex 67
	0.500000f, 0.500000f, 0.000000f, // vertex 68
	0.555556f, 0.444444f, 0.000000f, // vertex 69
	0.666667f, 0.333333f, 0.000000f, // vertex 70
	0.833333f, 0.166667f, 0.000000f, // vertex 71
	0.666667f, 0.333333f, 0.000000f, // vertex 72
	1.000000f, 0.000000f, 0.000000f, // vertex 73
	0.833333f, 0.166667f, 0.000000f, // vertex 74
	1.000000f, 0.000000f, 0.000000f, // vertex 75
	0.833333f, 0.166667f, 0.000000f, // vertex 76
	1.000000f, 0.000000f, 0.000000f, // vertex 77
	1.000000f, 0.000000f, 0.000000f, // vertex 78
	1.000000f, 0.000000f, 0.000000f, // vertex 79
	1.000000f, 0.000000f, 0.000000f, // vertex 80
	1.000000f, 0.000000f, 0.000000f, // vertex 81
	1.000000f, 0.000000f, 0.000000f, // vertex 82
	1.000000f, 0.000000f, 0.000000f, // vertex 83
	1.000000f, 0.000000f, 0.000000f, // vertex 84
	1.000000f, 0.000000f, 0.000000f, // vertex 85
	1.000000f, 0.000000f, 0.000000f, // vertex 86
	1.000000f, 0.000000f, 0.000000f, // vertex 87
	1.000000f, 0.000000f, 0.000000f, // vertex 88
	1.000000f, 0.000000f, 0.000000f, // vertex 89
	1.000000f, 0.000000f, 0.000000f, // vertex 90
	1.000000f, 0.000000f, 0.000000f, // vertex 91
	1.000000f, 0.000000f, 0.000000f, // vertex 92
	1.000000f, 0.000000f, 0.000000f, // vertex 93
	1.000000f, 0.000000f, 0.000000f, // vertex 94
	0.926651f, 0.073349f, 0.000000f, // vertex 95
	0.926651f, 0.073349f, 0.000000f, // vertex 96
	1.000000f, 0.000000f, 0.000000f, // vertex 97
	1.000000f, 0.000000f, 0.000000f, // vertex 98
	1.000000f, 0.000000f, 0.000000f, // vertex 99
	0.500000f, 0.500000f, 0.000000f, // vertex 100
	0.500000f, 0.500000f, 0.000000f, // vertex 101
	1.000000f, 0.000000f, 0.000000f, // vertex 102
	1.000000f, 0.000000f, 0.000000f, // vertex 103
	0.926651f, 0.073349f, 0.000000f, // vertex 104
	1.000000f, 0.000000f, 0.000000f, // vertex 105
	1.000000f, 0.000000f, 0.000000f, // vertex 106
	1.000000f, 0.000000f, 0.000000f, // vertex 107
	1.000000f, 0.000000f, 0.000000f, // vertex 108
	1.000000f, 0.000000f, 0.000000f, // vertex 109
	1.000000f, 0.000000f, 0.000000f, // vertex 110
	1.000000f, 0.000000f, 0.000000f, // vertex 111
	1.000000f, 0.000000f, 0.000000f, // vertex 112
	1.000000f, 0.000000f, 0.000000f, // vertex 113
	1.000000f, 0.000000f, 0.000000f, // vertex 114
	0.500000f, 0.500000f, 0.000000f, // vertex 115
	1.000000f, 0.000000f, 0.000000f, // vertex 116
	0.500000f, 0.500000f, 0.000000f, // vertex 117
	0.500000f, 0.500000f, 0.000000f, // vertex 118
	0.666667f, 0.333333f, 0.000000f, // vertex 119
	0.666667f, 0.333333f, 0.000000f, // vertex 120
	0.800000f, 0.200000f, 0.000000f, // vertex 121
	0.800000f, 0.200000f, 0.000000f, // vertex 122
	0.500000f, 0.500000f, 0.000000f, // vertex 123
	0.500000f, 0.500000f, 0.000000f, // vertex 124
	0.500000f, 0.500000f, 0.000000f, // vertex 125
	0.500000f, 0.500000f, 0.000000f, // vertex 126
	0.500000f, 0.500000f, 0.000000f, // vertex 127
	0.500000f, 0.500000f, 0.000000f, // vertex 128
	0.500000f, 0.500000f, 0.000000f, // vertex 129
	0.500000f, 0.500000f, 0.000000f, // vertex 130
	0.500000f, 0.500000f, 0.000000f, // vertex 131
	0.500000f, 0.500000f, 0.000000f, // vertex 132
	0.500000f, 0.500000f, 0.000000f, // vertex 133
	0.500000f, 0.500000f, 0.000000f, // vertex 134
	0.800000f, 0.200000f, 0.000000f, // vertex 135
	0.500000f, 0.500000f, 0.000000f, // vertex 136
	0.500000f, 0.500000f, 0.000000f, // vertex 137
	0.500000f, 0.500000f, 0.000000f, // vertex 138
	0.500000f, 0.500000f, 0.000000f, // vertex 139
	0.500000f, 0.500000f, 0.000000f, // vertex 140
	0.666667f, 0.333333f, 0.000000f, // vertex 141
	0.500000f, 0.500000f, 0.000000f, // vertex 142
	0.666667f, 0.333333f, 0.000000f, // vertex 143
	0.500000f, 0.500000f, 0.000000f, // vertex 144
	0.833333f, 0.166667f, 0.000000f, // vertex 145
	0.833333f, 0.166667f, 0.000000f, // vertex 146
	1.000000f, 0.000000f, 0.000000f, // vertex 147
	1.000000f, 0.000000f, 0.000000f, // vertex 148
	1.000000f, 0.000000f, 0.000000f, // vertex 149
	1.000000f, 0.000000f, 0.000000f, // vertex 150
	1.000000f, 0.000000f, 0.000000f, // vertex 151
	1.000000f, 0.000000f, 0.000000f, // vertex 152
	1.000000f, 0.000000f, 0.000000f, // vertex 153
	0.833333f, 0.166667f, 0.000000f, // vertex 154
	1.000000f, 0.000000f, 0.000000f, // vertex 155
	0.800000f, 0.200000f, 0.000000f, // vertex 156
	0.666667f, 0.333333f, 0.000000f, // vertex 157
	0.666667f, 0.333333f, 0.000000f, // vertex 158
	0.555556f, 0.444444f, 0.000000f, // vertex 159
	0.555556f, 0.444444f, 0.000000f, // vertex 160
	0.500000f, 0.500000f, 0.000000f, // vertex 161
	0.555556f, 0.444444f, 0.000000f, // vertex 162
	0.500000f, 0.500000f, 0.000000f, // vertex 163
	0.555556f, 0.444444f, 0.000000f, // vertex 164
	0.500000f, 0.500000f, 0.000000f, // vertex 165
	0.500000f, 0.500000f, 0.000000f, // vertex 166
	0.555556f, 0.444444f, 0.000000f, // vertex 167
	0.500000f, 0.500000f, 0.000000f, // vertex 168
	0.555556f, 0.444444f, 0.000000f, // vertex 169
	0.500000f, 0.500000f, 0.000000f, // vertex 170
	0.555556f, 0.444444f, 0.000000f, // vertex 171
	0.555556f, 0.444444f, 0.000000f, // vertex 172
	0.666667f, 0.333333f, 0.000000f, // vertex 173
	0.666667f, 0.333333f, 0.000000f, // vertex 174
	0.800000f, 0.200000f, 0.000000f, // vertex 175
	1.000000f, 0.000000f, 0.000000f, // vertex 176
	0.833333f, 0.166667f, 0.000000f, // vertex 177
	1.000000f, 0.000000f, 0.000000f, // vertex 178
	1.000000f, 0.000000f, 0.000000f, // vertex 179
	1.000000f, 0.000000f, 0.000000f, // vertex 180
	1.000000f, 0.000000f, 0.000000f, // vertex 181
	1.000000f, 0.000000f, 0.000000f, // vertex 182
	1.000000f, 0.000000f, 0.000000f, // vertex 183
	1.000000f, 0.000000f, 0.000000f, // vertex 184
	0.833333f, 0.166667f, 0.000000f, // vertex 185
	0.833333f, 0.166667f, 0.000000f, // vertex 186
	0.500000f, 0.500000f, 0.000000f, // vertex 187
	0.666667f, 0.333333f, 0.000000f, // vertex 188
	0.500000f, 0.500000f, 0.000000f, // vertex 189
	0.666667f, 0.333333f, 0.000000f, // vertex 190
	0.500000f, 0.500000f, 0.000000f, // vertex 191
	0.500000f, 0.500000f, 0.000000f, // vertex 192
	0.500000f, 0.500000f, 0.000000f, // vertex 193
	0.500000f, 0.500000f, 0.000000f, // vertex 194
	0.500000f, 0.500000f, 0.000000f, // vertex 195
	0.800000f, 0.200000f, 0.000000f, // vertex 196
	0.500000f, 0.500000f, 0.000000f, // vertex 197
	0.655738f, 0.344262f, 0.000000f, // vertex 198
	0.833333f, 0.166667f, 0.000000f, // vertex 199
	0.819001f, 0.180999f, 0.000000f, // vertex 200
	0.833333f, 0.166667f, 0.000000f, // vertex 201
	0.819001f, 0.180999f, 0.000000f, // vertex 202
	0.819001f, 0.180999f, 0.000000f, // vertex 203
	0.819001f, 0.180999f, 0.000000f, // vertex 204
	0.655738f, 0.344262f, 0.000000f, // vertex 205
	0.819001f, 0.180999f, 0.000000f, // vertex 206
	0.819001f, 0.180999f, 0.000000f, // vertex 207
	0.819001f, 0.180999f, 0.000000f, // vertex 208
	0.819001f, 0.180999f, 0.000000f, // vertex 209
	0.819001f, 0.180999f, 0.000000f, // vertex 210
	0.655738f, 0.344262f, 0.000000f, // vertex 211
	0.512195f, 0.487805f, 0.000000f, // vertex 212
	0.512195f, 0.487805f, 0.000000f, // vertex 213
	0.512195f, 0.487805f, 0.000000f, // vertex 214
	0.512195f, 0.487805f, 0.000000f, // vertex 215
	0.500000f, 0.500000f, 0.000000f, // vertex 216
	0.655738f, 0.344262f, 0.000000f, // vertex 217
	0.500000f, 0.500000f, 0.000000f, // vertex 218
	0.500000f, 0.500000f, 0.000000f, // vertex 219
	0.500000f, 0.500000f, 0.000000f, // vertex 220
	0.500000f, 0.500000f, 0.000000f, // vertex 221
	0.500000f, 0.500000f, 0.000000f, // vertex 222
	1.000000f, 0.000000f, 0.000000f, // vertex 223
	0.500000f, 0.500000f, 0.000000f, // vertex 224
	1.000000f, 0.000000f, 0.000000f, // vertex 225
	0.500000f, 0.500000f, 0.000000f, // vertex 226
	0.500000f, 0.500000f, 0.000000f, // vertex 227
	1.000000f, 0.000000f, 0.000000f, // vertex 228
	1.000000f, 0.000000f, 0.000000f, // vertex 229
	1.000000f, 0.000000f, 0.000000f, // vertex 230
	0.833941f, 0.166059f, 0.000000f, // vertex 231
	1.000000f, 0.000000f, 0.000000f, // vertex 232
	1.000000f, 0.000000f, 0.000000f, // vertex 233
	1.000000f, 0.000000f, 0.000000f, // vertex 234
	1.000000f, 0.000000f, 0.000000f, // vertex 235
	1.000000f, 0.000000f, 0.000000f, // vertex 236
	1.000000f, 0.000000f, 0.000000f, // vertex 237
	1.000000f, 0.000000f, 0.000000f, // vertex 238
	0.952381f, 0.047619f, 0.000000f, // vertex 239
	0.909091f, 0.090909f, 0.000000f, // vertex 240
	1.000000f, 0.000000f, 0.000000f, // vertex 241
	1.000000f, 0.000000f, 0.000000f, // vertex 242
	1.000000f, 0.000000f, 0.000000f, // vertex 243
	1.000000f, 0.000000f, 0.000000f, // vertex 244
	1.000000f, 0.000000f, 0.000000f, // vertex 245
	1.000000f, 0.000000f, 0.000000f, // vertex 246
	1.000000f, 0.000000f, 0.000000f, // vertex 247
	1.000000f, 0.000000f, 0.000000f, // vertex 248
	1.000000f, 0.000000f, 0.000000f, // vertex 249
	1.000000f, 0.000000f, 0.000000f, // vertex 250
	1.000000f, 0.000000f, 0.000000f, // vertex 251
	1.000000f, 0.000000f, 0.000000f, // vertex 252
	0.833333f, 0.166667f, 0.000000f, // vertex 253
	1.000000f, 0.000000f, 0.000000f, // vertex 254
	0.666667f, 0.333333f, 0.000000f, // vertex 255
	1.000000f, 0.000000f, 0.000000f, // vertex 256
	0.666667f, 0.333333f, 0.000000f, // vertex 257
	1.000000f, 0.000000f, 0.000000f, // vertex 258
	0.500000f, 0.500000f, 0.000000f, // vertex 259
	1.000000f, 0.000000f, 0.000000f, // vertex 260
	0.500000f, 0.500000f, 0.000000f, // vertex 261
	0.952381f, 0.047619f, 0.000000f, // vertex 262
	0.909091f, 0.090909f, 0.000000f, // vertex 263
	0.909091f, 0.090909f, 0.000000f, // vertex 264
	1.000000f, 0.000000f, 0.000000f, // vertex 265
	1.000000f, 0.000000f, 0.000000f, // vertex 266
	1.000000f, 0.000000f, 0.000000f, // vertex 267
	1.000000f, 0.000000f, 0.000000f, // vertex 268
	1.000000f, 0.000000f, 0.000000f, // vertex 269
	1.000000f, 0.000000f, 0.000000f, // vertex 270
	1.000000f, 0.000000f, 0.000000f, // vertex 271
	1.000000f, 0.000000f, 0.000000f, // vertex 272
	1.000000f, 0.000000f, 0.000000f, // vertex 273
	1.000000f, 0.000000f, 0.000000f, // vertex 274
	1.000000f, 0.000000f, 0.000000f, // vertex 275
	1.000000f, 0.000000f, 0.000000f, // vertex 276
	1.000000f, 0.000000f, 0.000000f, // vertex 277
	0.833333f, 0.166667f, 0.000000f, // vertex 278
	1.000000f, 0.000000f, 0.000000f, // vertex 279
	0.833333f, 0.166667f, 0.000000f, // vertex 280
	1.000000f, 0.000000f, 0.000000f, // vertex 281
	1.000000f, 0.000000f, 0.000000f, // vertex 282
	1.000000f, 0.000000f, 0.000000f, // vertex 283
	1.000000f, 0.000000f, 0.000000f, // vertex 284
	1.000000f, 0.000000f, 0.000000f, // vertex 285
	1.000000f, 0.000000f, 0.000000f, // vertex 286
	1.000000f, 0.000000f, 0.000000f, // vertex 287
	1.000000f, 0.000000f, 0.000000f, // vertex 288
	1.000000f, 0.000000f, 0.000000f, // vertex 289
	0.555556f, 0.444444f, 0.000000f, // vertex 290
	0.555556f, 0.444444f, 0.000000f, // vertex 291
	0.833333f, 0.166667f, 0.000000f, // vertex 292
	0.833333f, 0.166667f, 0.000000f, // vertex 293
	1.000000f, 0.000000f, 0.000000f, // vertex 294
	1.000000f, 0.000000f, 0.000000f, // vertex 295
	1.000000f, 0.000000f, 0.000000f, // vertex 296
	0.833333f, 0.166667f, 0.000000f, // vertex 297
	1.000000f, 0.000000f, 0.000000f, // vertex 298
	0.833333f, 0.166667f, 0.000000f, // vertex 299
	1.000000f, 0.000000f, 0.000000f, // vertex 300
	0.833333f, 0.166667f, 0.000000f, // vertex 301
	1.000000f, 0.000000f, 0.000000f, // vertex 302
	0.833333f, 0.166667f, 0.000000f, // vertex 303
	0.666667f, 0.333333f, 0.000000f, // vertex 304
	1.000000f, 0.000000f, 0.000000f, // vertex 305
	0.833333f, 0.166667f, 0.000000f, // vertex 306
	1.000000f, 0.000000f, 0.000000f, // vertex 307
	0.833333f, 0.166667f, 0.000000f, // vertex 308
	0.833333f, 0.166667f, 0.000000f, // vertex 309
	0.833333f, 0.166667f, 0.000000f, // vertex 310
	0.714286f, 0.285714f, 0.000000f, // vertex 311
	0.833333f, 0.166667f, 0.000000f, // vertex 312
	0.714286f, 0.285714f, 0.000000f, // vertex 313
	0.833333f, 0.166667f, 0.000000f, // vertex 314
	0.714286f, 0.285714f, 0.000000f, // vertex 315
	0.833333f, 0.166667f, 0.000000f, // vertex 316
	0.800000f, 0.200000f, 0.000000f, // vertex 317
	0.555556f, 0.444444f, 0.000000f, // vertex 318
	0.666667f, 0.333333f, 0.000000f, // vertex 319
	0.666667f, 0.333333f, 0.000000f, // vertex 320
	0.500000f, 0.500000f, 0.000000f, // vertex 321
	0.500000f, 0.500000f, 0.000000f, // vertex 322
	0.555556f, 0.444444f, 0.000000f, // vertex 323
	0.555556f, 0.444444f, 0.000000f, // vertex 324
	0.666667f, 0.333333f, 0.000000f, // vertex 325
	0.833333f, 0.166667f, 0.000000f, // vertex 326
	0.666667f, 0.333333f, 0.000000f, // vertex 327
	0.833333f, 0.166667f, 0.000000f, // vertex 328
	0.833333f, 0.166667f, 0.000000f, // vertex 329
	0.500000f, 0.500000f, 0.000000f, // vertex 330
	0.500000f, 0.500000f, 0.000000f, // vertex 331
	0.555556f, 0.444444f, 0.000000f, // vertex 332
	0.500000f, 0.500000f, 0.000000f, // vertex 333
	0.500000f, 0.500000f, 0.000000f, // vertex 334
	1.000000f, 0.000000f, 0.000000f, // vertex 335
	1.000000f, 0.000000f, 0.000000f, // vertex 336
	1.000000f, 0.000000f, 0.000000f, // vertex 337
	1.000000f, 0.000000f, 0.000000f, // vertex 338
	1.000000f, 0.000000f, 0.000000f, // vertex 339
	1.000000f, 0.000000f, 0.000000f, // vertex 340
	1.000000f, 0.000000f, 0.000000f, // vertex 341
	1.000000f, 0.000000f, 0.000000f, // vertex 342
	1.000000f, 0.000000f, 0.000000f, // vertex 343
	1.000000f, 0.000000f, 0.000000f, // vertex 344
	1.000000f, 0.000000f, 0.000000f, // vertex 345
	0.926651f, 0.073349f, 0.000000f, // vertex 346
	0.833941f, 0.166059f, 0.000000f, // vertex 347
	1.000000f, 0.000000f, 0.000000f, // vertex 348
	1.000000f, 0.000000f, 0.000000f, // vertex 349
	1.000000f, 0.000000f, 0.000000f, // vertex 350
	1.000000f, 0.000000f, 0.000000f, // vertex 351
	1.000000f, 0.000000f, 0.000000f, // vertex 352
	1.000000f, 0.000000f, 0.000000f, // vertex 353
	1.000000f, 0.000000f, 0.000000f, // vertex 354
	1.000000f, 0.000000f, 0.000000f, // vertex 355
	1.000000f, 0.000000f, 0.000000f, // vertex 356
	1.000000f, 0.000000f, 0.000000f, // vertex 357
	1.000000f, 0.000000f, 0.000000f, // vertex 358
	1.000000f, 0.000000f, 0.000000f, // vertex 359
	1.000000f, 0.000000f, 0.000000f, // vertex 360
	1.000000f, 0.000000f, 0.000000f, // vertex 361
	0.800000f, 0.200000f, 0.000000f, // vertex 362
	0.800000f, 0.200000f, 0.000000f, // vertex 363
	0.500000f, 0.500000f, 0.000000f, // vertex 364
	0.655738f, 0.344262f, 0.000000f, // vertex 365
	0.819001f, 0.180999f, 0.000000f, // vertex 366
	0.833333f, 0.166667f, 0.000000f, // vertex 367
	0.833333f, 0.166667f, 0.000000f, // vertex 368
	0.500000f, 0.500000f, 0.000000f, // vertex 369
	0.512195f, 0.487805f, 0.000000f, // vertex 370
	0.500000f, 0.500000f, 0.000000f, // vertex 371
	0.500000f, 0.500000f, 0.000000f, // vertex 372
	0.500000f, 0.500000f, 0.000000f, // vertex 373
	0.655738f, 0.344262f, 0.000000f, // vertex 374
	0.500000f, 0.500000f, 0.000000f, // vertex 375
	0.500000f, 0.500000f, 0.000000f, // vertex 376
	0.500000f, 0.500000f, 0.000000f, // vertex 377
	0.500000f, 0.500000f, 0.000000f, // vertex 378
	0.500000f, 0.500000f, 0.000000f, // vertex 379
	0.500000f, 0.500000f, 0.000000f, // vertex 380
	0.500000f, 0.500000f, 0.000000f, // vertex 381
	1.000000f, 0.000000f, 0.000000f, // vertex 382
	1.000000f, 0.000000f, 0.000000f, // vertex 383
	1.000000f, 0.000000f, 0.000000f, // vertex 384
	1.000000f, 0.000000f, 0.000000f, // vertex 385
	1.000000f, 0.000000f, 0.000000f, // vertex 386
	1.000000f, 0.000000f, 0.000000f, // vertex 387
	1.000000f, 0.000000f, 0.000000f, // vertex 388
	1.000000f, 0.000000f, 0.000000f, // vertex 389
	1.000000f, 0.000000f, 0.000000f, // vertex 390
	0.500000f, 0.500000f, 0.000000f, // vertex 391
	1.000000f, 0.000000f, 0.000000f, // vertex 392
	0.666667f, 0.333333f, 0.000000f, // vertex 393
	0.500000f, 0.500000f, 0.000000f, // vertex 394
	1.000000f, 0.000000f, 0.000000f, // vertex 395
	0.500000f, 0.500000f, 0.000000f, // vertex 396
	1.000000f, 0.000000f, 0.000000f, // vertex 397
	1.000000f, 0.000000f, 0.000000f, // vertex 398
	1.000000f, 0.000000f, 0.000000f, // vertex 399
	1.000000f, 0.000000f, 0.000000f, // vertex 400
	1.000000f, 0.000000f, 0.000000f, // vertex 401
	1.000000f, 0.000000f, 0.000000f, // vertex 402
	1.000000f, 0.000000f, 0.000000f, // vertex 403
	1.000000f, 0.000000f, 0.000000f, // vertex 404
	1.000000f, 0.000000f, 0.000000f, // vertex 405
	0.909091f, 0.090909f, 0.000000f, // vertex 406
	0.952381f, 0.047619f, 0.000000f, // vertex 407
	0.952381f, 0.047619f, 0.000000f, // vertex 408
	0.500000f, 0.500000f, 0.000000f, // vertex 409
	1.000000f, 0.000000f, 0.000000f, // vertex 410
	0.500000f, 0.500000f, 0.000000f, // vertex 411
	1.000000f, 0.000000f, 0.000000f, // vertex 412
	0.666667f, 0.333333f, 0.000000f, // vertex 413
	1.000000f, 0.000000f, 0.000000f, // vertex 414
	0.666667f, 0.333333f, 0.000000f, // vertex 415
	1.000000f, 0.000000f, 0.000000f, // vertex 416
	0.833333f, 0.166667f, 0.000000f, // vertex 417
	1.000000f, 0.000000f, 0.000000f, // vertex 418
	1.000000f, 0.000000f, 0.000000f, // vertex 419
	1.000000f, 0.000000f, 0.000000f, // vertex 420
	1.000000f, 0.000000f, 0.000000f, // vertex 421
	0.833333f, 0.166667f, 0.000000f, // vertex 422
	1.000000f, 0.000000f, 0.000000f, // vertex 423
	1.000000f, 0.000000f, 0.000000f, // vertex 424
	1.000000f, 0.000000f, 0.000000f, // vertex 425
	1.000000f, 0.000000f, 0.000000f, // vertex 426
	1.000000f, 0.000000f, 0.000000f, // vertex 427
	1.000000f, 0.000000f, 0.000000f, // vertex 428
	1.000000f, 0.000000f, 0.000000f, // vertex 429
	1.000000f, 0.000000f, 0.000000f, // vertex 430
	1.000000f, 0.000000f, 0.000000f, // vertex 431
	1.000000f, 0.000000f, 0.000000f, // vertex 432
	1.000000f, 0.000000f, 0.000000f, // vertex 433
	1.000000f, 0.000000f, 0.000000f, // vertex 434
	1.000000f, 0.000000f, 0.000000f, // vertex 435
	0.909091f, 0.090909f, 0.000000f, // vertex 436
	0.909091f, 0.090909f, 0.000000f, // vertex 437
	1.000000f, 0.000000f, 0.000000f, // vertex 438
	1.000000f, 0.000000f, 0.000000f, // vertex 439
	1.000000f, 0.000000f, 0.000000f, // vertex 440
	0.833333f, 0.166667f, 0.000000f, // vertex 441
	1.000000f, 0.000000f, 0.000000f, // vertex 442
	1.000000f, 0.000000f, 0.000000f, // vertex 443
	1.000000f, 0.000000f, 0.000000f, // vertex 444
	1.000000f, 0.000000f, 0.000000f, // vertex 445
	1.000000f, 0.000000f, 0.000000f, // vertex 446
	1.000000f, 0.000000f, 0.000000f, // vertex 447
	1.000000f, 0.000000f, 0.000000f, // vertex 448
	1.000000f, 0.000000f, 0.000000f, // vertex 449
	1.000000f, 0.000000f, 0.000000f, // vertex 450
	0.833333f, 0.166667f, 0.000000f, // vertex 451
	0.833333f, 0.166667f, 0.000000f, // vertex 452
	0.555556f, 0.444444f, 0.000000f, // vertex 453
	0.555556f, 0.444444f, 0.000000f, // vertex 454
	1.000000f, 0.000000f, 0.000000f, // vertex 455
	0.833333f, 0.166667f, 0.000000f, // vertex 456
	1.000000f, 0.000000f, 0.000000f, // vertex 457
	0.833333f, 0.166667f, 0.000000f, // vertex 458
	1.000000f, 0.000000f, 0.000000f, // vertex 459
	0.833333f, 0.166667f, 0.000000f, // vertex 460
	1.000000f, 0.000000f, 0.000000f, // vertex 461
	1.000000f, 0.000000f, 0.000000f, // vertex 462
	0.666667f, 0.333333f, 0.000000f, // vertex 463
	0.833333f, 0.166667f, 0.000000f, // vertex 464
	0.800000f, 0.200000f, 0.000000f, // vertex 465
	0.833333f, 0.166667f, 0.000000f, // vertex 466
	0.714286f, 0.285714f, 0.000000f, // vertex 467
	0.833333f, 0.166667f, 0.000000f, // vertex 468
	0.714286f, 0.285714f, 0.000000f, // vertex 469
	0.833333f, 0.166667f, 0.000000f, // vertex 470
	0.714286f, 0.285714f, 0.000000f, // vertex 471
	0.833333f, 0.166667f, 0.000000f, // vertex 472
	0.833333f, 0.166667f, 0.000000f, // vertex 473
	0.666667f, 0.333333f, 0.000000f, // vertex 474
	0.833333f, 0.166667f, 0.000000f, // vertex 475
	0.666667f, 0.333333f, 0.000000f, // vertex 476
	0.555556f, 0.444444f, 0.000000f, // vertex 477
	0.555556f, 0.444444f, 0.000000f, // vertex 478
	0.500000f, 0.500000f, 0.000000f, // vertex 479
	0.500000f, 0.500000f, 0.000000f, // vertex 480
	0.500000f, 0.500000f, 0.000000f, // vertex 481
	0.500000f, 0.500000f, 0.000000f, // vertex 482
	0.555556f, 0.444444f, 0.000000f, // vertex 483
	0.500000f, 0.500000f, 0.000000f, // vertex 484
	0.500000f, 0.500000f, 0.000000f, // vertex 485
	1.000000f, 0.000000f, 0.000000f, // vertex 486
	1.000000f, 0.000000f, 0.000000f, // vertex 487
	1.000000f, 0.000000f, 0.000000f, // vertex 488
	1.000000f, 0.000000f, 0.000000f, // vertex 489
	1.000000f, 0.000000f, 0.000000f, // vertex 490
	1.000000f, 0.000000f, 0.000000f, // vertex 491
	1.000000f, 0.000000f, 0.000000f, // vertex 492
	1.000000f, 0.000000f, 0.000000f, // vertex 493
	1.000000f, 0.000000f, 0.000000f, // vertex 494
	1.000000f, 0.000000f, 0.000000f, // vertex 495
	1.000000f, 0.000000f, 0.000000f, // vertex 496
	1.000000f, 0.000000f, 0.000000f, // vertex 497
	1.000000f, 0.000000f, 0.000000f, // vertex 498
	1.000000f, 0.000000f, 0.000000f, // vertex 499
	1.000000f, 0.000000f, 0.000000f, // vertex 500
	0.500000f, 0.500000f, 0.000000f, // vertex 501
	1.000000f, 0.000000f, 0.000000f, // vertex 502
	1.000000f, 0.000000f, 0.000000f, // vertex 503
	1.000000f, 0.000000f, 0.000000f, // vertex 504
	1.000000f, 0.000000f, 0.000000f, // vertex 505
	1.000000f, 0.000000f, 0.000000f, // vertex 506
	1.000000f, 0.000000f, 0.000000f, // vertex 507
	1.000000f, 0.000000f, 0.000000f, // vertex 508
	1.000000f, 0.000000f, 0.000000f, // vertex 509
	1.000000f, 0.000000f, 0.000000f, // vertex 510
	1.000000f, 0.000000f, 0.000000f, // vertex 511
	1.000000f, 0.000000f, 0.000000f, // vertex 512
	1.000000f, 0.000000f, 0.000000f, // vertex 513
	1.000000f, 0.000000f, 0.000000f, // vertex 514
	1.000000f, 0.000000f, 0.000000f, // vertex 515
	0.555556f, 0.444444f, 0.000000f, // vertex 516
	0.666667f, 0.333333f, 0.000000f, // vertex 517
	0.666667f, 0.333333f, 0.000000f, // vertex 518
	1.000000f, 0.000000f, 0.000000f, // vertex 519
	1.000000f, 0.000000f, 0.000000f, // vertex 520
	1.000000f, 0.000000f, 0.000000f, // vertex 521
	0.833333f, 0.166667f, 0.000000f, // vertex 522
	0.833333f, 0.166667f, 0.000000f, // vertex 523
	0.833333f, 0.166667f, 0.000000f, // vertex 524
	0.833333f, 0.166667f, 0.000000f, // vertex 525
	0.833333f, 0.166667f, 0.000000f, // vertex 526
	0.833333f, 0.166667f, 0.000000f, // vertex 527
	0.833333f, 0.166667f, 0.000000f, // vertex 528
	0.769231f, 0.230769f, 0.000000f, // vertex 529
	0.769231f, 0.230769f, 0.000000f, // vertex 530
	0.769231f, 0.230769f, 0.000000f, // vertex 531
	0.500000f, 0.500000f, 0.000000f, // vertex 532
	0.769231f, 0.230769f, 0.000000f, // vertex 533
	0.500000f, 0.500000f, 0.000000f, // vertex 534
	0.833333f, 0.166667f, 0.000000f, // vertex 535
	0.500000f, 0.500000f, 0.000000f, // vertex 536
	0.833333f, 0.166667f, 0.000000f, // vertex 537
	0.655738f, 0.344262f, 0.000000f, // vertex 538
	0.833333f, 0.166667f, 0.000000f, // vertex 539
	0.900901f, 0.099099f, 0.000000f, // vertex 540
	0.900901f, 0.099099f, 0.000000f, // vertex 541
	0.833333f, 0.166667f, 0.000000f, // vertex 542
	0.900901f, 0.099099f, 0.000000f, // vertex 543
	0.833333f, 0.166667f, 0.000000f, // vertex 544
	0.655738f, 0.344262f, 0.000000f, // vertex 545
	0.833333f, 0.166667f, 0.000000f, // vertex 546
	0.900901f, 0.099099f, 0.000000f, // vertex 547
	0.833333f, 0.166667f, 0.000000f, // vertex 548
	0.833333f, 0.166667f, 0.000000f, // vertex 549
	0.833333f, 0.166667f, 0.000000f, // vertex 550
	0.655738f, 0.344262f, 0.000000f, // vertex 551
	0.500000f, 0.500000f, 0.000000f, // vertex 552
	0.512195f, 0.487805f, 0.000000f, // vertex 553
	0.500000f, 0.500000f, 0.000000f, // vertex 554
	0.512195f, 0.487805f, 0.000000f, // vertex 555
	0.512195f, 0.487805f, 0.000000f, // vertex 556
	0.833333f, 0.166667f, 0.000000f, // vertex 557
	0.833333f, 0.166667f, 0.000000f, // vertex 558
	0.900901f, 0.099099f, 0.000000f, // vertex 559
	0.655738f, 0.344262f, 0.000000f, // vertex 560
	0.655738f, 0.344262f, 0.000000f, // vertex 561
	0.512195f, 0.487805f, 0.000000f, // vertex 562
	0.500000f, 0.500000f, 0.000000f, // vertex 563
	0.500000f, 0.500000f, 0.000000f, // vertex 564
	0.500000f, 0.500000f, 0.000000f, // vertex 565
	0.500000f, 0.500000f, 0.000000f, // vertex 566
	0.655738f, 0.344262f, 0.000000f, // vertex 567
	0.500000f, 0.500000f, 0.000000f, // vertex 568
	0.512195f, 0.487805f, 0.000000f, // vertex 569
	0.512195f, 0.487805f, 0.000000f, // vertex 570
	1.000000f, 0.000000f, 0.000000f, // vertex 571
	1.000000f, 0.000000f, 0.000000f, // vertex 572
	1.000000f, 0.000000f, 0.000000f, // vertex 573
	1.000000f, 0.000000f, 0.000000f, // vertex 574
	1.000000f, 0.000000f, 0.000000f, // vertex 575
	1.000000f, 0.000000f, 0.000000f, // vertex 576
	1.000000f, 0.000000f, 0.000000f, // vertex 577
	1.000000f, 0.000000f, 0.000000f, // vertex 578
	1.000000f, 0.000000f, 0.000000f, // vertex 579
	1.000000f, 0.000000f, 0.000000f, // vertex 580
	1.000000f, 0.000000f, 0.000000f, // vertex 581
	1.000000f, 0.000000f, 0.000000f, // vertex 582
	1.000000f, 0.000000f, 0.000000f, // vertex 583
	1.000000f, 0.000000f, 0.000000f, // vertex 584
	1.000000f, 0.000000f, 0.000000f, // vertex 585
	1.000000f, 0.000000f, 0.000000f, // vertex 586
	1.000000f, 0.000000f, 0.000000f, // vertex 587
	1.000000f, 0.000000f, 0.000000f, // vertex 588
	1.000000f, 0.000000f, 0.000000f, // vertex 589
	1.000000f, 0.000000f, 0.000000f, // vertex 590
	1.000000f, 0.000000f, 0.000000f, // vertex 591
	0.500000f, 0.500000f, 0.000000f, // vertex 592
	1.000000f, 0.000000f, 0.000000f, // vertex 593
	0.500000f, 0.500000f, 0.000000f, // vertex 594
	0.500000f, 0.500000f, 0.000000f, // vertex 595
	0.500000f, 0.500000f, 0.000000f, // vertex 596
	1.000000f, 0.000000f, 0.000000f, // vertex 597
	0.500000f, 0.500000f, 0.000000f, // vertex 598
	0.666667f, 0.333333f, 0.000000f, // vertex 599
	0.666667f, 0.333333f, 0.000000f, // vertex 600
	0.500000f, 0.500000f, 0.000000f, // vertex 601
	1.000000f, 0.000000f, 0.000000f, // vertex 602
	0.500000f, 0.500000f, 0.000000f, // vertex 603
	0.500000f, 0.500000f, 0.000000f, // vertex 604
	0.500000f, 0.500000f, 0.000000f, // vertex 605
	0.500000f, 0.500000f, 0.000000f, // vertex 606
	0.500000f, 0.500000f, 0.000000f, // vertex 607
	0.500000f, 0.500000f, 0.000000f, // vertex 608
	0.833333f, 0.166667f, 0.000000f, // vertex 609
	0.833333f, 0.166667f, 0.000000f, // vertex 610
	0.714286f, 0.285714f, 0.000000f, // vertex 611
	0.833333f, 0.166667f, 0.000000f, // vertex 612
	0.714286f, 0.285714f, 0.000000f, // vertex 613
	0.714286f, 0.285714f, 0.000000f, // vertex 614
	0.500000f, 0.500000f, 0.000000f, // vertex 615
	0.500000f, 0.500000f, 0.000000f, // vertex 616
	0.500000f, 0.500000f, 0.000000f, // vertex 617
	0.500000f, 0.500000f, 0.000000f, // vertex 618
	0.500000f, 0.500000f, 0.000000f, // vertex 619
	0.500000f, 0.500000f, 0.000000f, // vertex 620
	0.500000f, 0.500000f, 0.000000f, // vertex 621
	0.500000f, 0.500000f, 0.000000f, // vertex 622
	0.500000f, 0.500000f, 0.000000f, // vertex 623
	0.500000f, 0.500000f, 0.000000f, // vertex 624
	0.500000f, 0.500000f, 0.000000f, // vertex 625
	0.833333f, 0.166667f, 0.000000f, // vertex 626
	0.714286f, 0.285714f, 0.000000f, // vertex 627
	0.833333f, 0.166667f, 0.000000f, // vertex 628
	0.833333f, 0.166667f, 0.000000f, // vertex 629
	0.833333f, 0.166667f, 0.000000f, // vertex 630
	1.000000f, 0.000000f, 0.000000f, // vertex 631
	1.000000f, 0.000000f, 0.000000f, // vertex 632
	1.000000f, 0.000000f, 0.000000f, // vertex 633
	1.000000f, 0.000000f, 0.000000f, // vertex 634
	1.000000f, 0.000000f, 0.000000f, // vertex 635
	1.000000f, 0.000000f, 0.000000f, // vertex 636
	0.833333f, 0.166667f, 0.000000f, // vertex 637
	0.833333f, 0.166667f, 0.000000f, // vertex 638
	1.000000f, 0.000000f, 0.000000f, // vertex 639
	0.833333f, 0.166667f, 0.000000f, // vertex 640
	1.000000f, 0.000000f, 0.000000f, // vertex 641
	0.833333f, 0.166667f, 0.000000f, // vertex 642
	1.000000f, 0.000000f, 0.000000f, // vertex 643
	0.833333f, 0.166667f, 0.000000f, // vertex 644
	1.000000f, 0.000000f, 0.000000f, // vertex 645
	1.000000f, 0.000000f, 0.000000f, // vertex 646
	1.000000f, 0.000000f, 0.000000f, // vertex 647
	0.833333f, 0.166667f, 0.000000f, // vertex 648
	0.833333f, 0.166667f, 0.000000f, // vertex 649
	0.555556f, 0.444444f, 0.000000f, // vertex 650
	0.833333f, 0.166667f, 0.000000f, // vertex 651
	0.555556f, 0.444444f, 0.000000f, // vertex 652
	0.500000f, 0.500000f, 0.000000f, // vertex 653
	0.555556f, 0.444444f, 0.000000f, // vertex 654
	0.666667f, 0.333333f, 0.000000f, // vertex 655
	0.833333f, 0.166667f, 0.000000f, // vertex 656
	0.833333f, 0.166667f, 0.000000f, // vertex 657
	0.666667f, 0.333333f, 0.000000f, // vertex 658
	1.000000f, 0.000000f, 0.000000f, // vertex 659
	1.000000f, 0.000000f, 0.000000f, // vertex 660
	1.000000f, 0.000000f, 0.000000f, // vertex 661
	0.500000f, 0.500000f, 0.000000f, // vertex 662
	0.500000f, 0.500000f, 0.000000f, // vertex 663
	0.500000f, 0.500000f, 0.000000f, // vertex 664
	0.500000f, 0.500000f, 0.000000f, // vertex 665
	0.500000f, 0.500000f, 0.000000f, // vertex 666
	0.500000f, 0.500000f, 0.000000f, // vertex 667
	0.500000f, 0.500000f, 0.000000f, // vertex 668
	0.500000f, 0.500000f, 0.000000f, // vertex 669
	0.500000f, 0.500000f, 0.000000f, // vertex 670
	0.500000f, 0.500000f, 0.000000f, // vertex 671
	0.500000f, 0.500000f, 0.000000f, // vertex 672
	0.500000f, 0.500000f, 0.000000f, // vertex 673
	0.500000f, 0.500000f, 0.000000f, // vertex 674
	0.500000f, 0.500000f, 0.000000f, // vertex 675
	0.714286f, 0.285714f, 0.000000f, // vertex 676
	0.714286f, 0.285714f, 0.000000f, // vertex 677
	0.500000f, 0.500000f, 0.000000f, // vertex 678
	0.500000f, 0.500000f, 0.000000f, // vertex 679
	0.500000f, 0.500000f, 0.000000f, // vertex 680
	0.500000f, 0.500000f, 0.000000f, // vertex 681
	0.500000f, 0.500000f, 0.000000f, // vertex 682
	1.000000f, 0.000000f, 0.000000f, // vertex 683
	1.000000f, 0.000000f, 0.000000f, // vertex 684
	1.000000f, 0.000000f, 0.000000f, // vertex 685
	1.000000f, 0.000000f, 0.000000f, // vertex 686
	1.000000f, 0.000000f, 0.000000f, // vertex 687
	1.000000f, 0.000000f, 0.000000f, // vertex 688
	0.833333f, 0.166667f, 0.000000f, // vertex 689
	1.000000f, 0.000000f, 0.000000f, // vertex 690
	0.833333f, 0.166667f, 0.000000f, // vertex 691
	1.000000f, 0.000000f, 0.000000f, // vertex 692
	0.833333f, 0.166667f, 0.000000f, // vertex 693
	1.000000f, 0.000000f, 0.000000f, // vertex 694
	0.500000f, 0.500000f, 0.000000f, // vertex 695
	0.714286f, 0.285714f, 0.000000f, // vertex 696
	0.833333f, 0.166667f, 0.000000f, // vertex 697
	1.000000f, 0.000000f, 0.000000f, // vertex 698
	1.000000f, 0.000000f, 0.000000f, // vertex 699
	1.000000f, 0.000000f, 0.000000f, // vertex 700
	0.555556f, 0.444444f, 0.000000f, // vertex 701
	0.833333f, 0.166667f, 0.000000f, // vertex 702
	0.555556f, 0.444444f, 0.000000f, // vertex 703
	0.833333f, 0.166667f, 0.000000f, // vertex 704
	0.833333f, 0.166667f, 0.000000f, // vertex 705
	0.555556f, 0.444444f, 0.000000f, // vertex 706
	0.500000f, 0.500000f, 0.000000f, // vertex 707
	0.666667f, 0.333333f, 0.000000f, // vertex 708
	0.666667f, 0.333333f, 0.000000f, // vertex 709
	0.833333f, 0.166667f, 0.000000f, // vertex 710
	0.833333f, 0.166667f, 0.000000f, // vertex 711
	1.000000f, 0.000000f, 0.000000f, // vertex 712
	1.000000f, 0.000000f, 0.000000f, // vertex 713
	1.000000f, 0.000000f, 0.000000f, // vertex 714
	0.655738f, 0.344262f, 0.000000f, // vertex 715
	0.833333f, 0.166667f, 0.000000f, // vertex 716
	0.900901f, 0.099099f, 0.000000f, // vertex 717
	0.900901f, 0.099099f, 0.000000f, // vertex 718
	0.833333f, 0.166667f, 0.000000f, // vertex 719
	0.500000f, 0.500000f, 0.000000f, // vertex 720
	0.833333f, 0.166667f, 0.000000f, // vertex 721
	0.500000f, 0.500000f, 0.000000f, // vertex 722
	0.769231f, 0.230769f, 0.000000f, // vertex 723
	0.500000f, 0.500000f, 0.000000f, // vertex 724
	0.833333f, 0.166667f, 0.000000f, // vertex 725
	0.833333f, 0.166667f, 0.000000f, // vertex 726
	0.833333f, 0.166667f, 0.000000f, // vertex 727
	0.833333f, 0.166667f, 0.000000f, // vertex 728
	0.833333f, 0.166667f, 0.000000f, // vertex 729
	0.833333f, 0.166667f, 0.000000f, // vertex 730
	0.833333f, 0.166667f, 0.000000f, // vertex 731
	0.769231f, 0.230769f, 0.000000f, // vertex 732
	0.769231f, 0.230769f, 0.000000f, // vertex 733
	0.769231f, 0.230769f, 0.000000f, // vertex 734
	0.769231f, 0.230769f, 0.000000f, // vertex 735
	0.769231f, 0.230769f, 0.000000f, // vertex 736
	0.833333f, 0.166667f, 0.000000f, // vertex 737
	0.500000f, 0.500000f, 0.000000f, // vertex 738
	0.500000f, 0.500000f, 0.000000f, // vertex 739
	0.500000f, 0.500000f, 0.000000f, // vertex 740
	0.500000f, 0.500000f, 0.000000f, // vertex 741
	0.769231f, 0.230769f, 0.000000f, // vertex 742
	0.833333f, 0.166667f, 0.000000f, // vertex 743
	0.769231f, 0.230769f, 0.000000f, // vertex 744
	0.833333f, 0.166667f, 0.000000f, // vertex 745
	0.833333f, 0.166667f, 0.000000f, // vertex 746
	0.833333f, 0.166667f, 0.000000f, // vertex 747
	0.833333f, 0.166667f, 0.000000f, // vertex 748
	0.833333f, 0.166667f, 0.000000f, // vertex 749
	1.000000f, 0.000000f, 0.000000f, // vertex 750
	1.000000f, 0.000000f, 0.000000f, // vertex 751
	0.909091f, 0.090909f, 0.000000f, // vertex 752
	1.000000f, 0.000000f, 0.000000f, // vertex 753
	1.000000f, 0.000000f, 0.000000f, // vertex 754
	0.833333f, 0.166667f, 0.000000f, // vertex 755
	0.833333f, 0.166667f, 0.000000f, // vertex 756
	0.666667f, 0.333333f, 0.000000f, // vertex 757
	0.666667f, 0.333333f, 0.000000f, // vertex 758
	0.666667f, 0.333333f, 0.000000f, // vertex 759
	0.666667f, 0.333333f, 0.000000f, // vertex 760
	0.833333f, 0.166667f, 0.000000f, // vertex 761
	0.833333f, 0.166667f, 0.000000f, // vertex 762
	1.000000f, 0.000000f, 0.000000f, // vertex 763
	1.000000f, 0.000000f, 0.000000f, // vertex 764
	0.909091f, 0.090909f, 0.000000f, // vertex 765
	1.000000f, 0.000000f, 0.000000f, // vertex 766
	1.000000f, 0.000000f, 0.000000f, // vertex 767
	1.000000f, 0.000000f, 0.000000f, // vertex 768
	1.000000f, 0.000000f, 0.000000f, // vertex 769
	0.500000f, 0.500000f, 0.000000f, // vertex 770
	0.833333f, 0.166667f, 0.000000f, // vertex 771
	0.500000f, 0.500000f, 0.000000f, // vertex 772
	0.833333f, 0.166667f, 0.000000f, // vertex 773
	0.714286f, 0.285714f, 0.000000f, // vertex 774
	0.900901f, 0.099099f, 0.000000f, // vertex 775
	0.655738f, 0.344262f, 0.000000f, // vertex 776
	0.772187f, 0.154437f, 0.073375f, // vertex 777
	0.819001f, 0.180999f, 0.000000f, // vertex 778
	0.772187f, 0.154437f, 0.073375f, // vertex 779
	0.819001f, 0.180999f, 0.000000f, // vertex 780
	0.772187f, 0.154437f, 0.073375f, // vertex 781
	0.772187f, 0.154437f, 0.073375f, // vertex 782
	0.800000f, 0.200000f, 0.000000f, // vertex 783
	0.800000f, 0.200000f, 0.000000f, // vertex 784
	0.772187f, 0.154437f, 0.073375f, // vertex 785
	0.800000f, 0.200000f, 0.000000f, // vertex 786
	0.772187f, 0.154437f, 0.073375f, // vertex 787
	0.800000f, 0.200000f, 0.000000f, // vertex 788
	0.772187f, 0.154437f, 0.073375f, // vertex 789
	0.500000f, 0.500000f, 0.000000f, // vertex 790
	0.900901f, 0.099099f, 0.000000f, // vertex 791
	0.952381f, 0.047619f, 0.000000f, // vertex 792
	0.833333f, 0.166667f, 0.000000f, // vertex 793
	0.909091f, 0.090909f, 0.000000f, // vertex 794
	0.909091f, 0.090909f, 0.000000f, // vertex 795
	1.000000f, 0.000000f, 0.000000f, // vertex 796
	0.500000f, 0.500000f, 0.000000f, // vertex 797
	0.500000f, 0.500000f, 0.000000f, // vertex 798
	0.500000f, 0.500000f, 0.000000f, // vertex 799
	0.500000f, 0.500000f, 0.000000f, // vertex 800
	0.500000f, 0.500000f, 0.000000f, // vertex 801
	0.500000f, 0.500000f, 0.000000f, // vertex 802
	0.500000f, 0.500000f, 0.000000f, // vertex 803
	0.500000f, 0.500000f, 0.000000f, // vertex 804
	0.500000f, 0.500000f, 0.000000f, // vertex 805
	0.500000f, 0.500000f, 0.000000f, // vertex 806
	0.500000f, 0.500000f, 0.000000f, // vertex 807
	0.500000f, 0.500000f, 0.000000f, // vertex 808
	0.500000f, 0.500000f, 0.000000f, // vertex 809
	0.655738f, 0.344262f, 0.000000f, // vertex 810
	0.714286f, 0.285714f, 0.000000f, // vertex 811
	0.500000f, 0.500000f, 0.000000f, // vertex 812
	0.500000f, 0.500000f, 0.000000f, // vertex 813
	0.500000f, 0.500000f, 0.000000f, // vertex 814
	0.500000f, 0.500000f, 0.000000f, // vertex 815
	0.500000f, 0.500000f, 0.000000f, // vertex 816
	0.500000f, 0.500000f, 0.000000f, // vertex 817
	1.000000f, 0.000000f, 0.000000f, // vertex 818
	0.500000f, 0.500000f, 0.000000f, // vertex 819
	0.500000f, 0.500000f, 0.000000f, // vertex 820
	0.500000f, 0.500000f, 0.000000f, // vertex 821
	0.500000f, 0.500000f, 0.000000f, // vertex 822
	0.500000f, 0.500000f, 0.000000f, // vertex 823
	0.500000f, 0.500000f, 0.000000f, // vertex 824
	0.500000f, 0.500000f, 0.000000f, // vertex 825
	0.500000f, 0.500000f, 0.000000f, // vertex 826
	0.500000f, 0.500000f, 0.000000f, // vertex 827
	0.500000f, 0.500000f, 0.000000f, // vertex 828
	0.500000f, 0.500000f, 0.000000f, // vertex 829
	0.819001f, 0.180999f, 0.000000f, // vertex 830
	0.772187f, 0.154437f, 0.073375f, // vertex 831
	0.819001f, 0.180999f, 0.000000f, // vertex 832
	0.819001f, 0.180999f, 0.000000f, // vertex 833
	0.655738f, 0.344262f, 0.000000f, // vertex 834
	0.900901f, 0.099099f, 0.000000f, // vertex 835
	0.714286f, 0.285714f, 0.000000f, // vertex 836
	0.833333f, 0.166667f, 0.000000f, // vertex 837
	0.500000f, 0.500000f, 0.000000f, // vertex 838
	0.833333f, 0.166667f, 0.000000f, // vertex 839
	0.500000f, 0.500000f, 0.000000f, // vertex 840
	1.000000f, 0.000000f, 0.000000f, // vertex 841
	1.000000f, 0.000000f, 0.000000f, // vertex 842
	0.909091f, 0.090909f, 0.000000f, // vertex 843
	0.909091f, 0.090909f, 0.000000f, // vertex 844
	0.833333f, 0.166667f, 0.000000f, // vertex 845
	0.952381f, 0.047619f, 0.000000f, // vertex 846
	0.900901f, 0.099099f, 0.000000f, // vertex 847
	0.500000f, 0.500000f, 0.000000f, // vertex 848
	0.819001f, 0.180999f, 0.000000f, // vertex 849
	0.800000f, 0.200000f, 0.000000f, // vertex 850
	0.772187f, 0.154437f, 0.073375f, // vertex 851
	0.500000f, 0.500000f, 0.000000f, // vertex 852
	0.655738f, 0.344262f, 0.000000f, // vertex 853
	0.500000f, 0.500000f, 0.000000f, // vertex 854
	0.714286f, 0.285714f, 0.000000f, // vertex 855
	0.500000f, 0.500000f, 0.000000f, // vertex 856
	0.500000f, 0.500000f, 0.000000f, // vertex 857
	0.500000f, 0.500000f, 0.000000f, // vertex 858
	0.500000f, 0.500000f, 0.000000f, // vertex 859
	0.500000f, 0.500000f, 0.000000f, // vertex 860
	0.500000f, 0.500000f, 0.000000f, // vertex 861
	0.500000f, 0.500000f, 0.000000f, // vertex 862
	0.500000f, 0.500000f, 0.000000f, // vertex 863
	0.500000f, 0.500000f, 0.000000f, // vertex 864
	0.500000f, 0.500000f, 0.000000f, // vertex 865
	0.500000f, 0.500000f, 0.000000f, // vertex 866
	0.500000f, 0.500000f, 0.000000f, // vertex 867
	0.500000f, 0.500000f, 0.000000f, // vertex 868
	0.500000f, 0.500000f, 0.000000f, // vertex 869
	0.500000f, 0.500000f, 0.000000f, // vertex 870
	0.500000f, 0.500000f, 0.000000f, // vertex 871
	0.500000f, 0.500000f, 0.000000f, // vertex 872
	0.500000f, 0.500000f, 0.000000f, // vertex 873
	0.500000f, 0.500000f, 0.000000f, // vertex 874
	0.500000f, 0.500000f, 0.000000f, // vertex 875
	0.500000f, 0.500000f, 0.000000f, // vertex 876
	0.500000f, 0.500000f, 0.000000f, // vertex 877
	0.500000f, 0.500000f, 0.000000f, // vertex 878
	0.500000f, 0.500000f, 0.000000f, // vertex 879
	0.500000f, 0.500000f, 0.000000f, // vertex 880
	0.500000f, 0.500000f, 0.000000f, // vertex 881
	0.500000f, 0.500000f, 0.000000f, // vertex 882
	0.714286f, 0.285714f, 0.000000f, // vertex 883
	0.500000f, 0.500000f, 0.000000f, // vertex 884
	1.000000f, 0.000000f, 0.000000f, // vertex 885
	0.909091f, 0.090909f, 0.000000f, // vertex 886
	0.833333f, 0.166667f, 0.000000f, // vertex 887
	0.833333f, 0.166667f, 0.000000f, // vertex 888
	0.500000f, 0.500000f, 0.000000f, // vertex 889
	0.500000f, 0.500000f, 0.000000f, // vertex 890
	0.500000f, 0.500000f, 0.000000f, // vertex 891
	0.500000f, 0.500000f, 0.000000f, // vertex 892
	0.500000f, 0.500000f, 0.000000f, // vertex 893
	1.000000f, 0.000000f, 0.000000f, // vertex 894
	0.833333f, 0.166667f, 0.000000f, // vertex 895
	0.909091f, 0.090909f, 0.000000f, // vertex 896
	0.833333f, 0.166667f, 0.000000f, // vertex 897
	0.666667f, 0.333333f, 0.000000f, // vertex 898
	0.800000f, 0.200000f, 0.000000f, // vertex 899
	0.666667f, 0.333333f, 0.000000f, // vertex 900
	0.714286f, 0.285714f, 0.000000f, // vertex 901
	0.666667f, 0.333333f, 0.000000f, // vertex 902
	0.714286f, 0.285714f, 0.000000f, // vertex 903
	0.666667f, 0.333333f, 0.000000f, // vertex 904
	0.714286f, 0.285714f, 0.000000f, // vertex 905
	0.714286f, 0.285714f, 0.000000f, // vertex 906
	0.666667f, 0.333333f, 0.000000f, // vertex 907
	0.555556f, 0.444444f, 0.000000f, // vertex 908
	0.666667f, 0.333333f, 0.000000f, // vertex 909
	0.555556f, 0.444444f, 0.000000f, // vertex 910
	0.666667f, 0.333333f, 0.000000f, // vertex 911
	0.555556f, 0.444444f, 0.000000f, // vertex 912
	0.666667f, 0.333333f, 0.000000f, // vertex 913
	0.666667f, 0.333333f, 0.000000f, // vertex 914
	0.714286f, 0.285714f, 0.000000f, // vertex 915
	0.714286f, 0.285714f, 0.000000f, // vertex 916
	0.800000f, 0.200000f, 0.000000f, // vertex 917
	0.714286f, 0.285714f, 0.000000f, // vertex 918
	0.666667f, 0.333333f, 0.000000f, // vertex 919
	0.666667f, 0.333333f, 0.000000f, // vertex 920
	0.555556f, 0.444444f, 0.000000f, // vertex 921
	0.555556f, 0.444444f, 0.000000f, // vertex 922
	0.800000f, 0.200000f, 0.000000f, // vertex 923
	0.666667f, 0.333333f, 0.000000f, // vertex 924
	0.666667f, 0.333333f, 0.000000f, // vertex 925
	0.555556f, 0.444444f, 0.000000f, // vertex 926
	0.555556f, 0.444444f, 0.000000f, // vertex 927
	0.500000f, 0.500000f, 0.000000f, // vertex 928
	0.500000f, 0.500000f, 0.000000f, // vertex 929
	0.500000f, 0.500000f, 0.000000f, // vertex 930
	0.500000f, 0.500000f, 0.000000f, // vertex 931
	0.500000f, 0.500000f, 0.000000f, // vertex 932
	0.500000f, 0.500000f, 0.000000f, // vertex 933
	0.500000f, 0.500000f, 0.000000f, // vertex 934
	0.500000f, 0.500000f, 0.000000f, // vertex 935
	0.500000f, 0.500000f, 0.000000f, // vertex 936
	0.500000f, 0.500000f, 0.000000f, // vertex 937
	0.500000f, 0.500000f, 0.000000f, // vertex 938
	0.500000f, 0.500000f, 0.000000f, // vertex 939
	0.500000f, 0.500000f, 0.000000f, // vertex 940
	0.500000f, 0.500000f, 0.000000f, // vertex 941
	0.500000f, 0.500000f, 0.000000f, // vertex 942
	0.500000f, 0.500000f, 0.000000f, // vertex 943
	0.500000f, 0.500000f, 0.000000f, // vertex 944
	0.500000f, 0.500000f, 0.000000f, // vertex 945
	0.500000f, 0.500000f, 0.000000f, // vertex 946
	0.500000f, 0.500000f, 0.000000f, // vertex 947
	0.500000f, 0.500000f, 0.000000f, // vertex 948
	0.500000f, 0.500000f, 0.000000f, // vertex 949
	0.500000f, 0.500000f, 0.000000f, // vertex 950
	0.500000f, 0.500000f, 0.000000f, // vertex 951
	0.500000f, 0.500000f, 0.000000f, // vertex 952
	0.500000f, 0.500000f, 0.000000f, // vertex 953
	0.500000f, 0.500000f, 0.000000f, // vertex 954
	0.500000f, 0.500000f, 0.000000f, // vertex 955
	0.500000f, 0.500000f, 0.000000f, // vertex 956
	0.500000f, 0.500000f, 0.000000f, // vertex 957
	0.500000f, 0.500000f, 0.000000f, // vertex 958
	0.500000f, 0.500000f, 0.000000f, // vertex 959
	0.500000f, 0.500000f, 0.000000f, // vertex 960
	0.500000f, 0.500000f, 0.000000f, // vertex 961
	0.500000f, 0.500000f, 0.000000f, // vertex 962
	0.500000f, 0.500000f, 0.000000f, // vertex 963
	0.500000f, 0.500000f, 0.000000f, // vertex 964
	0.500000f, 0.500000f, 0.000000f, // vertex 965
	0.500000f, 0.500000f, 0.000000f, // vertex 966
	0.500000f, 0.500000f, 0.000000f, // vertex 967
	0.500000f, 0.500000f, 0.000000f, // vertex 968
	0.500000f, 0.500000f, 0.000000f, // vertex 969
	0.500000f, 0.500000f, 0.000000f, // vertex 970
	0.500000f, 0.500000f, 0.000000f, // vertex 971
	0.500000f, 0.500000f, 0.000000f, // vertex 972
	0.500000f, 0.500000f, 0.000000f, // vertex 973
	0.500000f, 0.500000f, 0.000000f, // vertex 974
	0.500000f, 0.500000f, 0.000000f, // vertex 975
	1.000000f, 0.000000f, 0.000000f, // vertex 976
	1.000000f, 0.000000f, 0.000000f, // vertex 977
	1.000000f, 0.000000f, 0.000000f, // vertex 978
	1.000000f, 0.000000f, 0.000000f, // vertex 979
	1.000000f, 0.000000f, 0.000000f, // vertex 980
	1.000000f, 0.000000f, 0.000000f, // vertex 981
	1.000000f, 0.000000f, 0.000000f, // vertex 982
	1.000000f, 0.000000f, 0.000000f, // vertex 983
	1.000000f, 0.000000f, 0.000000f, // vertex 984
	1.000000f, 0.000000f, 0.000000f, // vertex 985
	1.000000f, 0.000000f, 0.000000f, // vertex 986
	1.000000f, 0.000000f, 0.000000f, // vertex 987
	1.000000f, 0.000000f, 0.000000f, // vertex 988
	1.000000f, 0.000000f, 0.000000f, // vertex 989
	1.000000f, 0.000000f, 0.000000f, // vertex 990
	1.000000f, 0.000000f, 0.000000f, // vertex 991
	1.000000f, 0.000000f, 0.000000f, // vertex 992
	1.000000f, 0.000000f, 0.000000f, // vertex 993
	1.000000f, 0.000000f, 0.000000f, // vertex 994
	1.000000f, 0.000000f, 0.000000f, // vertex 995
	1.000000f, 0.000000f, 0.000000f, // vertex 996
	1.000000f, 0.000000f, 0.000000f, // vertex 997
	1.000000f, 0.000000f, 0.000000f, // vertex 998
	1.000000f, 0.000000f, 0.000000f, // vertex 999
	1.000000f, 0.000000f, 0.000000f, // vertex 1000
	1.000000f, 0.000000f, 0.000000f, // vertex 1001
	1.000000f, 0.000000f, 0.000000f, // vertex 1002
	1.000000f, 0.000000f, 0.000000f, // vertex 1003
	1.000000f, 0.000000f, 0.000000f, // vertex 1004
	1.000000f, 0.000000f, 0.000000f, // vertex 1005
	1.000000f, 0.000000f, 0.000000f, // vertex 1006
	1.000000f, 0.000000f, 0.000000f, // vertex 1007
	1.000000f, 0.000000f, 0.000000f, // vertex 1008
	1.000000f, 0.000000f, 0.000000f, // vertex 1009
	1.000000f, 0.000000f, 0.000000f, // vertex 1010
	1.000000f, 0.000000f, 0.000000f, // vertex 1011
	1.000000f, 0.000000f, 0.000000f, // vertex 1012
	1.000000f, 0.000000f, 0.000000f, // vertex 1013
	1.000000f, 0.000000f, 0.000000f, // vertex 1014
	1.000000f, 0.000000f, 0.000000f, // vertex 1015
	1.000000f, 0.000000f, 0.000000f, // vertex 1016
	1.000000f, 0.000000f, 0.000000f, // vertex 1017
	1.000000f, 0.000000f, 0.000000f, // vertex 1018
	1.000000f, 0.000000f, 0.000000f, // vertex 1019
	1.000000f, 0.000000f, 0.000000f, // vertex 1020
	1.000000f, 0.000000f, 0.000000f, // vertex 1021
	1.000000f, 0.000000f, 0.000000f, // vertex 1022
	1.000000f, 0.000000f, 0.000000f, // vertex 1023
	1.000000f, 0.000000f, 0.000000f, // vertex 1024
	1.000000f, 0.000000f, 0.000000f, // vertex 1025
	1.000000f, 0.000000f, 0.000000f, // vertex 1026
	1.000000f, 0.000000f, 0.000000f, // vertex 1027
	1.000000f, 0.000000f, 0.000000f, // vertex 1028
	1.000000f, 0.000000f, 0.000000f, // vertex 1029
	1.000000f, 0.000000f, 0.000000f, // vertex 1030
	1.000000f, 0.000000f, 0.000000f, // vertex 1031
	1.000000f, 0.000000f, 0.000000f, // vertex 1032
	1.000000f, 0.000000f, 0.000000f, // vertex 1033
	1.000000f, 0.000000f, 0.000000f, // vertex 1034
	1.000000f, 0.000000f, 0.000000f, // vertex 1035
	1.000000f, 0.000000f, 0.000000f, // vertex 1036
	1.000000f, 0.000000f, 0.000000f, // vertex 1037
	1.000000f, 0.000000f, 0.000000f, // vertex 1038
	1.000000f, 0.000000f, 0.000000f, // vertex 1039
	1.000000f, 0.000000f, 0.000000f, // vertex 1040
	1.000000f, 0.000000f, 0.000000f, // vertex 1041
	1.000000f, 0.000000f, 0.000000f, // vertex 1042
	1.000000f, 0.000000f, 0.000000f, // vertex 1043
	1.000000f, 0.000000f, 0.000000f, // vertex 1044
	1.000000f, 0.000000f, 0.000000f, // vertex 1045
	1.000000f, 0.000000f, 0.000000f, // vertex 1046
	1.000000f, 0.000000f, 0.000000f, // vertex 1047
	1.000000f, 0.000000f, 0.000000f, // vertex 1048
	1.000000f, 0.000000f, 0.000000f, // vertex 1049
	1.000000f, 0.000000f, 0.000000f, // vertex 1050
	1.000000f, 0.000000f, 0.000000f, // vertex 1051
	1.000000f, 0.000000f, 0.000000f, // vertex 1052
	1.000000f, 0.000000f, 0.000000f, // vertex 1053
	1.000000f, 0.000000f, 0.000000f, // vertex 1054
	1.000000f, 0.000000f, 0.000000f, // vertex 1055
	1.000000f, 0.000000f, 0.000000f, // vertex 1056
	1.000000f, 0.000000f, 0.000000f, // vertex 1057
	1.000000f, 0.000000f, 0.000000f, // vertex 1058
	1.000000f, 0.000000f, 0.000000f, // vertex 1059
	1.000000f, 0.000000f, 0.000000f, // vertex 1060
	1.000000f, 0.000000f, 0.000000f, // vertex 1061
	1.000000f, 0.000000f, 0.000000f, // vertex 1062
	1.000000f, 0.000000f, 0.000000f, // vertex 1063
	1.000000f, 0.000000f, 0.000000f, // vertex 1064
	1.000000f, 0.000000f, 0.000000f, // vertex 1065
	1.000000f, 0.000000f, 0.000000f, // vertex 1066
	1.000000f, 0.000000f, 0.000000f, // vertex 1067
	1.000000f, 0.000000f, 0.000000f, // vertex 1068
	1.000000f, 0.000000f, 0.000000f, // vertex 1069
	1.000000f, 0.000000f, 0.000000f, // vertex 1070
	1.000000f, 0.000000f, 0.000000f, // vertex 1071
	1.000000f, 0.000000f, 0.000000f, // vertex 1072
	1.000000f, 0.000000f, 0.000000f, // vertex 1073
	1.000000f, 0.000000f, 0.000000f, // vertex 1074
	1.000000f, 0.000000f, 0.000000f, // vertex 1075
	1.000000f, 0.000000f, 0.000000f, // vertex 1076
	1.000000f, 0.000000f, 0.000000f, // vertex 1077
	1.000000f, 0.000000f, 0.000000f, // vertex 1078
	1.000000f, 0.000000f, 0.000000f, // vertex 1079
	1.000000f, 0.000000f, 0.000000f, // vertex 1080
	1.000000f, 0.000000f, 0.000000f, // vertex 1081
	1.000000f, 0.000000f, 0.000000f, // vertex 1082
	1.000000f, 0.000000f, 0.000000f, // vertex 1083
	1.000000f, 0.000000f, 0.000000f, // vertex 1084
	1.000000f, 0.000000f, 0.000000f, // vertex 1085
	1.000000f, 0.000000f, 0.000000f, // vertex 1086
	1.000000f, 0.000000f, 0.000000f, // vertex 1087
	1.000000f, 0.000000f, 0.000000f, // vertex 1088
	1.000000f, 0.000000f, 0.000000f, // vertex 1089
	1.000000f, 0.000000f, 0.000000f, // vertex 1090
	1.000000f, 0.000000f, 0.000000f, // vertex 1091
	1.000000f, 0.000000f, 0.000000f, // vertex 1092
	1.000000f, 0.000000f, 0.000000f, // vertex 1093
	1.000000f, 0.000000f, 0.000000f, // vertex 1094
	1.000000f, 0.000000f, 0.000000f, // vertex 1095
	1.000000f, 0.000000f, 0.000000f, // vertex 1096
	1.000000f, 0.000000f, 0.000000f, // vertex 1097
	1.000000f, 0.000000f, 0.000000f, // vertex 1098
	1.000000f, 0.000000f, 0.000000f, // vertex 1099
	1.000000f, 0.000000f, 0.000000f, // vertex 1100
	1.000000f, 0.000000f, 0.000000f, // vertex 1101
	1.000000f, 0.000000f, 0.000000f, // vertex 1102
	1.000000f, 0.000000f, 0.000000f, // vertex 1103
	1.000000f, 0.000000f, 0.000000f, // vertex 1104
	1.000000f, 0.000000f, 0.000000f, // vertex 1105
	1.000000f, 0.000000f, 0.000000f, // vertex 1106
	1.000000f, 0.000000f, 0.000000f, // vertex 1107
	1.000000f, 0.000000f, 0.000000f, // vertex 1108
	1.000000f, 0.000000f, 0.000000f, // vertex 1109
	1.000000f, 0.000000f, 0.000000f, // vertex 1110
	1.000000f, 0.000000f, 0.000000f, // vertex 1111
	1.000000f, 0.000000f, 0.000000f, // vertex 1112
	1.000000f, 0.000000f, 0.000000f, // vertex 1113
	1.000000f, 0.000000f, 0.000000f, // vertex 1114
	1.000000f, 0.000000f, 0.000000f, // vertex 1115
	1.000000f, 0.000000f, 0.000000f, // vertex 1116
	1.000000f, 0.000000f, 0.000000f, // vertex 1117
	1.000000f, 0.000000f, 0.000000f, // vertex 1118
	1.000000f, 0.000000f, 0.000000f, // vertex 1119
	1.000000f, 0.000000f, 0.000000f, // vertex 1120
	1.000000f, 0.000000f, 0.000000f, // vertex 1121
	1.000000f, 0.000000f, 0.000000f, // vertex 1122
	1.000000f, 0.000000f, 0.000000f, // vertex 1123
	1.000000f, 0.000000f, 0.000000f, // vertex 1124
	1.000000f, 0.000000f, 0.000000f, // vertex 1125
	1.000000f, 0.000000f, 0.000000f, // vertex 1126
	1.000000f, 0.000000f, 0.000000f, // vertex 1127
	1.000000f, 0.000000f, 0.000000f, // vertex 1128
	1.000000f, 0.000000f, 0.000000f, // vertex 1129
	1.000000f, 0.000000f, 0.000000f, // vertex 1130
	1.000000f, 0.000000f, 0.000000f, // vertex 1131
	1.000000f, 0.000000f, 0.000000f, // vertex 1132
	1.000000f, 0.000000f, 0.000000f, // vertex 1133
	1.000000f, 0.000000f, 0.000000f, // vertex 1134
	1.000000f, 0.000000f, 0.000000f, // vertex 1135
	1.000000f, 0.000000f, 0.000000f, // vertex 1136
	1.000000f, 0.000000f, 0.000000f, // vertex 1137
	1.000000f, 0.000000f, 0.000000f, // vertex 1138
	1.000000f, 0.000000f, 0.000000f, // vertex 1139
	1.000000f, 0.000000f, 0.000000f, // vertex 1140
	1.000000f, 0.000000f, 0.000000f, // vertex 1141
	1.000000f, 0.000000f, 0.000000f, // vertex 1142
	1.000000f, 0.000000f, 0.000000f, // vertex 1143
	1.000000f, 0.000000f, 0.000000f, // vertex 1144
	1.000000f, 0.000000f, 0.000000f, // vertex 1145
	1.000000f, 0.000000f, 0.000000f, // vertex 1146
	1.000000f, 0.000000f, 0.000000f, // vertex 1147
	1.000000f, 0.000000f, 0.000000f, // vertex 1148
	1.000000f, 0.000000f, 0.000000f, // vertex 1149
	1.000000f, 0.000000f, 0.000000f, // vertex 1150
	1.000000f, 0.000000f, 0.000000f, // vertex 1151
	1.000000f, 0.000000f, 0.000000f, // vertex 1152
	1.000000f, 0.000000f, 0.000000f, // vertex 1153
	1.000000f, 0.000000f, 0.000000f, // vertex 1154
	1.000000f, 0.000000f, 0.000000f, // vertex 1155
	1.000000f, 0.000000f, 0.000000f, // vertex 1156
	1.000000f, 0.000000f, 0.000000f, // vertex 1157
	1.000000f, 0.000000f, 0.000000f, // vertex 1158
	1.000000f, 0.000000f, 0.000000f, // vertex 1159
	1.000000f, 0.000000f, 0.000000f, // vertex 1160
	1.000000f, 0.000000f, 0.000000f, // vertex 1161
	1.000000f, 0.000000f, 0.000000f, // vertex 1162
	1.000000f, 0.000000f, 0.000000f, // vertex 1163
	1.000000f, 0.000000f, 0.000000f, // vertex 1164
	1.000000f, 0.000000f, 0.000000f, // vertex 1165
	1.000000f, 0.000000f, 0.000000f, // vertex 1166
	1.000000f, 0.000000f, 0.000000f, // vertex 1167
	1.000000f, 0.000000f, 0.000000f, // vertex 1168
	1.000000f, 0.000000f, 0.000000f, // vertex 1169
	1.000000f, 0.000000f, 0.000000f, // vertex 1170
	1.000000f, 0.000000f, 0.000000f, // vertex 1171
	1.000000f, 0.000000f, 0.000000f, // vertex 1172
	1.000000f, 0.000000f, 0.000000f, // vertex 1173
	1.000000f, 0.000000f, 0.000000f, // vertex 1174
	1.000000f, 0.000000f, 0.000000f, // vertex 1175
	1.000000f, 0.000000f, 0.000000f, // vertex 1176
	1.000000f, 0.000000f, 0.000000f, // vertex 1177
	1.000000f, 0.000000f, 0.000000f, // vertex 1178
	1.000000f, 0.000000f, 0.000000f, // vertex 1179
	0.909091f, 0.090909f, 0.000000f, // vertex 1180
	1.000000f, 0.000000f, 0.000000f, // vertex 1181
	1.000000f, 0.000000f, 0.000000f, // vertex 1182
	0.833333f, 0.166667f, 0.000000f, // vertex 1183
	0.833333f, 0.166667f, 0.000000f, // vertex 1184
	0.666667f, 0.333333f, 0.000000f, // vertex 1185
	0.666667f, 0.333333f, 0.000000f, // vertex 1186
	0.666667f, 0.333333f, 0.000000f, // vertex 1187
	0.666667f, 0.333333f, 0.000000f, // vertex 1188
	0.833333f, 0.166667f, 0.000000f, // vertex 1189
	0.833333f, 0.166667f, 0.000000f, // vertex 1190
	1.000000f, 0.000000f, 0.000000f, // vertex 1191
	1.000000f, 0.000000f, 0.000000f, // vertex 1192
	0.909091f, 0.090909f, 0.000000f, // vertex 1193
	1.000000f, 0.000000f, 0.000000f, // vertex 1194
	1.000000f, 0.000000f, 0.000000f, // vertex 1195
	1.000000f, 0.000000f, 0.000000f, // vertex 1196
	1.000000f, 0.000000f, 0.000000f, // vertex 1197
	0.500000f, 0.500000f, 0.000000f, // vertex 1198
	0.833333f, 0.166667f, 0.000000f, // vertex 1199
	0.500000f, 0.500000f, 0.000000f, // vertex 1200
	0.833333f, 0.166667f, 0.000000f, // vertex 1201
	0.714286f, 0.285714f, 0.000000f, // vertex 1202
	0.900901f, 0.099099f, 0.000000f, // vertex 1203
	0.655738f, 0.344262f, 0.000000f, // vertex 1204
	0.772187f, 0.154437f, 0.073375f, // vertex 1205
	0.819001f, 0.180999f, 0.000000f, // vertex 1206
	0.772187f, 0.154437f, 0.073375f, // vertex 1207
	0.819001f, 0.180999f, 0.000000f, // vertex 1208
	0.772187f, 0.154437f, 0.073375f, // vertex 1209
	0.772187f, 0.154437f, 0.073375f, // vertex 1210
	0.800000f, 0.200000f, 0.000000f, // vertex 1211
	0.800000f, 0.200000f, 0.000000f, // vertex 1212
	0.772187f, 0.154437f, 0.073375f, // vertex 1213
	0.800000f, 0.200000f, 0.000000f, // vertex 1214
	0.772187f, 0.154437f, 0.073375f, // vertex 1215
	0.800000f, 0.200000f, 0.000000f, // vertex 1216
	0.772187f, 0.154437f, 0.073375f, // vertex 1217
	0.500000f, 0.500000f, 0.000000f, // vertex 1218
	0.900901f, 0.099099f, 0.000000f, // vertex 1219
	0.952381f, 0.047619f, 0.000000f, // vertex 1220
	0.833333f, 0.166667f, 0.000000f, // vertex 1221
	0.909091f, 0.090909f, 0.000000f, // vertex 1222
	0.909091f, 0.090909f, 0.000000f, // vertex 1223
	1.000000f, 0.000000f, 0.000000f, // vertex 1224
	0.500000f, 0.500000f, 0.000000f, // vertex 1225
	0.500000f, 0.500000f, 0.000000f, // vertex 1226
	0.500000f, 0.500000f, 0.000000f, // vertex 1227
	0.500000f, 0.500000f, 0.000000f, // vertex 1228
	0.500000f, 0.500000f, 0.000000f, // vertex 1229
	0.500000f, 0.500000f, 0.000000f, // vertex 1230
	0.500000f, 0.500000f, 0.000000f, // vertex 1231
	0.500000f, 0.500000f, 0.000000f, // vertex 1232
	0.500000f, 0.500000f, 0.000000f, // vertex 1233
	0.500000f, 0.500000f, 0.000000f, // vertex 1234
	0.500000f, 0.500000f, 0.000000f, // vertex 1235
	0.500000f, 0.500000f, 0.000000f, // vertex 1236
	0.500000f, 0.500000f, 0.000000f, // vertex 1237
	0.655738f, 0.344262f, 0.000000f, // vertex 1238
	0.714286f, 0.285714f, 0.000000f, // vertex 1239
	0.500000f, 0.500000f, 0.000000f, // vertex 1240
	0.500000f, 0.500000f, 0.000000f, // vertex 1241
	0.500000f, 0.500000f, 0.000000f, // vertex 1242
	0.500000f, 0.500000f, 0.000000f, // vertex 1243
	0.500000f, 0.500000f, 0.000000f, // vertex 1244
	0.500000f, 0.500000f, 0.000000f, // vertex 1245
	1.000000f, 0.000000f, 0.000000f, // vertex 1246
	0.500000f, 0.500000f, 0.000000f, // vertex 1247
	0.500000f, 0.500000f, 0.000000f, // vertex 1248
	0.500000f, 0.500000f, 0.000000f, // vertex 1249
	0.500000f, 0.500000f, 0.000000f, // vertex 1250
	0.500000f, 0.500000f, 0.000000f, // vertex 1251
	0.500000f, 0.500000f, 0.000000f, // vertex 1252
	0.500000f, 0.500000f, 0.000000f, // vertex 1253
	0.500000f, 0.500000f, 0.000000f, // vertex 1254
	0.500000f, 0.500000f, 0.000000f, // vertex 1255
	0.500000f, 0.500000f, 0.000000f, // vertex 1256
	0.500000f, 0.500000f, 0.000000f, // vertex 1257
	0.819001f, 0.180999f, 0.000000f, // vertex 1258
	0.772187f, 0.154437f, 0.073375f, // vertex 1259
	0.819001f, 0.180999f, 0.000000f, // vertex 1260
	0.819001f, 0.180999f, 0.000000f, // vertex 1261
	0.655738f, 0.344262f, 0.000000f, // vertex 1262
	0.900901f, 0.099099f, 0.000000f, // vertex 1263
	0.714286f, 0.285714f, 0.000000f, // vertex 1264
	0.833333f, 0.166667f, 0.000000f, // vertex 1265
	0.500000f, 0.500000f, 0.000000f, // vertex 1266
	0.833333f, 0.166667f, 0.000000f, // vertex 1267
	0.500000f, 0.500000f, 0.000000f, // vertex 1268
	1.000000f, 0.000000f, 0.000000f, // vertex 1269
	1.000000f, 0.000000f, 0.000000f, // vertex 1270
	0.909091f, 0.090909f, 0.000000f, // vertex 1271
	0.909091f, 0.090909f, 0.000000f, // vertex 1272
	0.833333f, 0.166667f, 0.000000f, // vertex 1273
	0.952381f, 0.047619f, 0.000000f, // vertex 1274
	0.900901f, 0.099099f, 0.000000f, // vertex 1275
	0.500000f, 0.500000f, 0.000000f, // vertex 1276
	0.819001f, 0.180999f, 0.000000f, // vertex 1277
	0.800000f, 0.200000f, 0.000000f, // vertex 1278
	0.772187f, 0.154437f, 0.073375f, // vertex 1279
	0.500000f, 0.500000f, 0.000000f, // vertex 1280
	0.655738f, 0.344262f, 0.000000f, // vertex 1281
	0.500000f, 0.500000f, 0.000000f, // vertex 1282
	0.714286f, 0.285714f, 0.000000f, // vertex 1283
	0.500000f, 0.500000f, 0.000000f, // vertex 1284
	0.500000f, 0.500000f, 0.000000f, // vertex 1285
	0.500000f, 0.500000f, 0.000000f, // vertex 1286
	0.500000f, 0.500000f, 0.000000f, // vertex 1287
	0.500000f, 0.500000f, 0.000000f, // vertex 1288
	0.500000f, 0.500000f, 0.000000f, // vertex 1289
	0.500000f, 0.500000f, 0.000000f, // vertex 1290
	0.500000f, 0.500000f, 0.000000f, // vertex 1291
	0.500000f, 0.500000f, 0.000000f, // vertex 1292
	0.500000f, 0.500000f, 0.000000f, // vertex 1293
	0.500000f, 0.500000f, 0.000000f, // vertex 1294
	0.500000f, 0.500000f, 0.000000f, // vertex 1295
	0.500000f, 0.500000f, 0.000000f, // vertex 1296
	0.500000f, 0.500000f, 0.000000f, // vertex 1297
	0.500000f, 0.500000f, 0.000000f, // vertex 1298
	0.500000f, 0.500000f, 0.000000f, // vertex 1299
	0.500000f, 0.500000f, 0.000000f, // vertex 1300
	0.500000f, 0.500000f, 0.000000f, // vertex 1301
	0.500000f, 0.500000f, 0.000000f, // vertex 1302
	0.500000f, 0.500000f, 0.000000f, // vertex 1303
	0.500000f, 0.500000f, 0.000000f, // vertex 1304
	0.500000f, 0.500000f, 0.000000f, // vertex 1305
	0.500000f, 0.500000f, 0.000000f, // vertex 1306
	0.500000f, 0.500000f, 0.000000f, // vertex 1307
	0.500000f, 0.500000f, 0.000000f, // vertex 1308
	0.500000f, 0.500000f, 0.000000f, // vertex 1309
	0.500000f, 0.500000f, 0.000000f, // vertex 1310
	0.714286f, 0.285714f, 0.000000f, // vertex 1311
	0.500000f, 0.500000f, 0.000000f, // vertex 1312
	1.000000f, 0.000000f, 0.000000f, // vertex 1313
	0.909091f, 0.090909f, 0.000000f, // vertex 1314
	0.833333f, 0.166667f, 0.000000f, // vertex 1315
	0.833333f, 0.166667f, 0.000000f, // vertex 1316
	0.500000f, 0.500000f, 0.000000f, // vertex 1317
	0.500000f, 0.500000f, 0.000000f, // vertex 1318
	0.500000f, 0.500000f, 0.000000f, // vertex 1319
	0.500000f, 0.500000f, 0.000000f, // vertex 1320
	0.500000f, 0.500000f, 0.000000f, // vertex 1321
	1.000000f, 0.000000f, 0.000000f, // vertex 1322
	0.833333f, 0.166667f, 0.000000f, // vertex 1323
	0.909091f, 0.090909f, 0.000000f, // vertex 1324
	0.833333f, 0.166667f, 0.000000f, // vertex 1325
	0.666667f, 0.333333f, 0.000000f, // vertex 1326
	0.800000f, 0.200000f, 0.000000f, // vertex 1327
	0.666667f, 0.333333f, 0.000000f, // vertex 1328
	0.714286f, 0.285714f, 0.000000f, // vertex 1329
	0.666667f, 0.333333f, 0.000000f, // vertex 1330
	0.714286f, 0.285714f, 0.000000f, // vertex 1331
	0.666667f, 0.333333f, 0.000000f, // vertex 1332
	0.714286f, 0.285714f, 0.000000f, // vertex 1333
	0.714286f, 0.285714f, 0.000000f, // vertex 1334
	0.666667f, 0.333333f, 0.000000f, // vertex 1335
	0.555556f, 0.444444f, 0.000000f, // vertex 1336
	0.666667f, 0.333333f, 0.000000f, // vertex 1337
	0.555556f, 0.444444f, 0.000000f, // vertex 1338
	0.666667f, 0.333333f, 0.000000f, // vertex 1339
	0.555556f, 0.444444f, 0.000000f, // vertex 1340
	0.666667f, 0.333333f, 0.000000f, // vertex 1341
	0.666667f, 0.333333f, 0.000000f, // vertex 1342
	0.714286f, 0.285714f, 0.000000f, // vertex 1343
	0.714286f, 0.285714f, 0.000000f, // vertex 1344
	0.800000f, 0.200000f, 0.000000f, // vertex 1345
	0.714286f, 0.285714f, 0.000000f, // vertex 1346
	0.666667f, 0.333333f, 0.000000f, // vertex 1347
	0.666667f, 0.333333f, 0.000000f, // vertex 1348
	0.555556f, 0.444444f, 0.000000f, // vertex 1349
	0.555556f, 0.444444f, 0.000000f, // vertex 1350
	0.800000f, 0.200000f, 0.000000f, // vertex 1351
	0.666667f, 0.333333f, 0.000000f, // vertex 1352
	0.666667f, 0.333333f, 0.000000f, // vertex 1353
	0.555556f, 0.444444f, 0.000000f, // vertex 1354
	0.555556f, 0.444444f, 0.000000f, // vertex 1355
	0.500000f, 0.500000f, 0.000000f, // vertex 1356
	0.500000f, 0.500000f, 0.000000f, // vertex 1357
	0.500000f, 0.500000f, 0.000000f, // vertex 1358
	0.500000f, 0.500000f, 0.000000f, // vertex 1359
	0.500000f, 0.500000f, 0.000000f, // vertex 1360
	0.500000f, 0.500000f, 0.000000f, // vertex 1361
	0.500000f, 0.500000f, 0.000000f, // vertex 1362
	0.500000f, 0.500000f, 0.000000f, // vertex 1363
	0.500000f, 0.500000f, 0.000000f, // vertex 1364
	0.500000f, 0.500000f, 0.000000f, // vertex 1365
	0.500000f, 0.500000f, 0.000000f, // vertex 1366
	0.500000f, 0.500000f, 0.000000f, // vertex 1367
	0.500000f, 0.500000f, 0.000000f, // vertex 1368
	0.500000f, 0.500000f, 0.000000f, // vertex 1369
	0.500000f, 0.500000f, 0.000000f, // vertex 1370
	0.500000f, 0.500000f, 0.000000f, // vertex 1371
	0.500000f, 0.500000f, 0.000000f, // vertex 1372
	0.500000f, 0.500000f, 0.000000f, // vertex 1373
	0.500000f, 0.500000f, 0.000000f, // vertex 1374
	0.500000f, 0.500000f, 0.000000f, // vertex 1375
	0.500000f, 0.500000f, 0.000000f, // vertex 1376
	0.500000f, 0.500000f, 0.000000f, // vertex 1377
	0.500000f, 0.500000f, 0.000000f, // vertex 1378
	0.500000f, 0.500000f, 0.000000f, // vertex 1379
	0.500000f, 0.500000f, 0.000000f, // vertex 1380
	0.500000f, 0.500000f, 0.000000f, // vertex 1381
	0.500000f, 0.500000f, 0.000000f, // vertex 1382
	0.500000f, 0.500000f, 0.000000f, // vertex 1383
	0.500000f, 0.500000f, 0.000000f, // vertex 1384
	0.500000f, 0.500000f, 0.000000f, // vertex 1385
	0.500000f, 0.500000f, 0.000000f, // vertex 1386
	0.500000f, 0.500000f, 0.000000f, // vertex 1387
	0.500000f, 0.500000f, 0.000000f, // vertex 1388
	0.500000f, 0.500000f, 0.000000f, // vertex 1389
	0.500000f, 0.500000f, 0.000000f, // vertex 1390
	0.500000f, 0.500000f, 0.000000f, // vertex 1391
	0.500000f, 0.500000f, 0.000000f, // vertex 1392
	0.500000f, 0.500000f, 0.000000f, // vertex 1393
	0.500000f, 0.500000f, 0.000000f, // vertex 1394
	0.500000f, 0.500000f, 0.000000f, // vertex 1395
	0.500000f, 0.500000f, 0.000000f, // vertex 1396
	0.500000f, 0.500000f, 0.000000f, // vertex 1397
	0.500000f, 0.500000f, 0.000000f, // vertex 1398
	0.500000f, 0.500000f, 0.000000f, // vertex 1399
	0.500000f, 0.500000f, 0.000000f, // vertex 1400
	0.500000f, 0.500000f, 0.000000f, // vertex 1401
	0.500000f, 0.500000f, 0.000000f, // vertex 1402
	0.500000f, 0.500000f, 0.000000f, // vertex 1403                                                          
};
*/

Mth::Vector testBonePositions[] = {
	Mth::Vector( 0.000000f, 0.000000f, 0.000000f, 1.000000f ),
	Mth::Vector( 0.003876f, 3.697956f, 1.117333f, 1.000000f ),
	Mth::Vector( 0.003872f, 4.297436f, 2.893996f, 1.000000f ),
	Mth::Vector( 0.003868f, 9.568745f, 1.807596f, 1.000000f ),
	Mth::Vector( 0.003863f, 7.351619f, 5.444900f, 1.000000f ),
};											    

Mth::Vector testBoneScales[] = {
	Mth::Vector( 1.000000f, 1.000000f, 1.000000f, 1.000000f ),
	Mth::Vector( 1.000000f, 1.000000f, 1.000000f, 1.000000f ),
	Mth::Vector( 1.000000f, 1.000000f, 1.000000f, 1.000000f ),
	Mth::Vector( 1.000000f, 1.000000f, 1.000000f, 1.000000f ),
	Mth::Vector( 1.000000f, 1.000000f, 1.000000f, 1.000000f ),
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCutsceneData::CCutsceneData()
{
	mp_fileLibrary = NULL;

	mp_cameraQuickAnim = NULL;
	mp_objectQuickAnim = NULL;

	m_numObjects = 0;
	m_numAssets = 0;

	m_audioStarted = false;
	m_currentTime = 0.0f;

	m_original_params_set = false;

	Pcm::StopMusic();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCutsceneData::~CCutsceneData()
{
	if ( mp_fileLibrary )
	{
		delete mp_fileLibrary;
		mp_fileLibrary = NULL;
	}

	if ( mp_cameraQuickAnim )
	{
		delete mp_cameraQuickAnim;
		mp_cameraQuickAnim = NULL;
	}

	if ( mp_objectQuickAnim )
	{
		delete mp_objectQuickAnim;
		mp_objectQuickAnim = NULL;
	}
	
	for ( int i = 0; i < m_numBonedAnims; i++ )
	{											 
		SCutsceneAnimInfo* pAnimInfo = &m_bonedAnims[i];
	
		Dbg_MsgAssert( pAnimInfo->mp_bonedQuickAnim, ( "No quick anim to delete" ) );
		delete pAnimInfo->mp_bonedQuickAnim;
		pAnimInfo->mp_bonedQuickAnim = NULL;
	}

	for ( int i = 0; i < m_numObjects; i++ )
	{
// 		Don't want to mark 'em as dead, because the assets
//		contained in these dummy objects will be obliterated
//		very soon...  we don't want any dummy objects hanging
//		around without valid assets...
//		mp_object[i]->MarkAsDead();
		
		SCutsceneObjectInfo* pObjectInfo = &m_objects[i];
		
		Dbg_MsgAssert( pObjectInfo->mp_object, ( "No object to delete" ) );
		
		Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( pObjectInfo->mp_object );
		Dbg_Assert( pModelComponent );
		
		if ( pObjectInfo->m_doPlayerSubstitution )
		{
			Dbg_Assert( pModelComponent->HasRefObject() );
			// put back the skater's model component,
			// if we changed the bounding sphere...
			uint32 refObjectName = pModelComponent->GetRefObjectName();
			Obj::CCompositeObject* pRefObject = (Obj::CCompositeObject*)Obj::ResolveToObject( refObjectName );
			Dbg_Assert( pRefObject );
			Obj::CModelComponent* pRefModelComponent = GetModelComponentFromObject( pRefObject );
			Dbg_Assert( pRefModelComponent );
			Nx::CModel* pRefModel = pRefModelComponent->GetModel();
			pRefModel->SetBoundingSphere( m_original_player_bounding_sphere );
			pRefModel->Hide( m_original_hidden );
			pRefModel->EnableScaling( m_original_scale_enabled );
			
			// send the object back to where it came from...
			// (we had to change it for the duration of the cutscene
			// in case the object was affected by a FakeLight)
			pRefObject->SetPos( m_original_pos );
		}

		delete pObjectInfo->mp_object;
		pObjectInfo->mp_object = NULL;
	}
	
	for ( int i = 0; i < m_numAssets; i++ )
	{
		// remove them from the assman
		// (do i need to make sure the objects are completely dead first?)
		Ass::CAssMan* pAssMan = Ass::CAssMan::Instance();
		Ass::CAsset* pAsset = pAssMan->GetAssetNode( m_assetName[i], true );
		pAssMan->UnloadAsset( pAsset );
		m_assetName[i] = NULL;
		
		// TODO:  Need to handle anims slightly
		// if we use references...  fortunately,
		// we're not using references right now
	}

	// stop the stream (may not need this, unless we abort)
	Pcm::StopMusic();
	
	Script::RunScript( CRCD(0x7216d8da,"CutsceneKillObjects") );

	Script::UnloadQB( CRCD(0xa1408e6e,"cutscenefrommemory.qb") );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCutsceneData::OverridesCamera()
{
	return LoadFinished() && m_audioStarted;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector get_scale( Script::CStruct* pAppearance, uint32 bone_group_name )
{
	Script::CStruct* pVirtualDescStruct = NULL;

	if ( pAppearance->GetStructure( bone_group_name, &pVirtualDescStruct, Script::NO_ASSERT ) )
	{
		int use_default_scale = 1;
		pVirtualDescStruct->GetInteger( CRCD(0x5a96985d,"use_default_scale"), &use_default_scale, Script::NO_ASSERT );
		
		if ( !use_default_scale )
		{
			int x, y, z;
			pVirtualDescStruct->GetInteger( CRCD(0x7323e97c,"x"), &x, Script::ASSERT );
			pVirtualDescStruct->GetInteger( CRCD(0x0424d9ea,"y"), &y, Script::ASSERT );
			pVirtualDescStruct->GetInteger( CRCD(0x9d2d8850,"z"), &z, Script::ASSERT );
			return Mth::Vector( ((float)x)/100.0f, ((float)y)/100.0f, ((float)z)/100.0f, 1.0f ); 
		}
		else 
		{
			// use_default_scale parameter
			return Mth::Vector( 1.0f, 1.0f, 1.0f, 1.0f );
		}
	}
	else
	{
		// part not found
		return Mth::Vector( 1.0f, 1.0f, 1.0f, 1.0f );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCutsceneData::load_models()
{
	Dbg_MsgAssert( mp_fileLibrary, ( "No file library?!?" ) );

	int numFiles = mp_fileLibrary->GetNumFiles();
	for ( int i = 0; i < numFiles; i++ )
	{
		const File::SFileInfo* pFileInfo = mp_fileLibrary->GetFileInfo( i );

#ifdef __PLAT_NGC__
		uint32* pData = NULL;
		switch ( pFileInfo->fileExtensionChecksum )
		{
			case 0x5ac14717:	// CIF
			case 0x1512808d:	// TEX
			case 0xffc529f4:	// CAS
			case 0x2cd4107d:	// WGT
				break;
			default:
				pData = mp_fileLibrary->GetFileData( i );
				break;
		}
#else
		uint32* pData = mp_fileLibrary->GetFileData( i );
#endif		// __PLAT_NGC__
			
		// read each file one by one, and do something different based on the file type
		switch ( pFileInfo->fileExtensionChecksum )
		{
			case 0xeab51346:	// SKA
				add_boned_anim( pFileInfo->fileNameChecksum, pData, pFileInfo->fileSize );
				mp_fileLibrary->ClearFile( pFileInfo->fileNameChecksum, pFileInfo->fileExtensionChecksum );				
				break;
		
			case 0x2e4bf21b:	// OBA
			{
				Dbg_MsgAssert( !mp_objectQuickAnim, ( "Cutscene file should have exactly 1 object anim\n" ) );
				add_anim( CRCD(0x3b423a24,"cutscene_oba"), pData, pFileInfo->fileSize );
				mp_objectQuickAnim = new Nx::CQuickAnim;
				mp_objectQuickAnim->SetAnimAssetName( CRCD(0x3b423a24,"cutscene_oba") );
				mp_objectQuickAnim->ResetCustomKeys();
//				m_objectAnimController.PlaySequence( 0, 0.0f, mp_objectAnimData->GetDuration(), Gfx::LOOPING_HOLD, 0.0f, 1.0f );
				mp_fileLibrary->ClearFile( pFileInfo->fileNameChecksum, pFileInfo->fileExtensionChecksum );				
			}
			break;
		
			case 0x05ca1497:	// CAM
			{
				// this presumes that there's only one cutscene_cam asset at a time,
				// which may not be true if we're queueing up multiple cutscenes
				// in this case, we'd need to run an init function before each
				// cutscene is played, not in the cutscene's InitFromStructure,
				// which gets called at the time the cutscene gets added to the list...
				add_anim( CRCD(0x10c3dca8,"cutscene_cam"), pData, pFileInfo->fileSize );
				Dbg_MsgAssert( !mp_cameraQuickAnim, ( "Cutscene file should have exactly 1 Camera\n" ) );
				mp_cameraQuickAnim = new Nx::CQuickAnim;
				mp_cameraQuickAnim->SetAnimAssetName( CRCD(0x10c3dca8,"cutscene_cam") );
				mp_cameraQuickAnim->ResetCustomKeys();
//				m_cameraController.PlaySequence( 0, 0.0f, mp_cameraData->GetDuration(), Gfx::LOOPING_HOLD, 0.0f, 1.0f );
				mp_fileLibrary->ClearFile( pFileInfo->fileNameChecksum, pFileInfo->fileExtensionChecksum );				
			}
			break;
			
			case 0x2bbea5c3:	// QB
			{
				Script::LoadQBFromMemory( "cutscenefrommemory.qb", (uint8*)pData, Script::ASSERT_IF_DUPLICATE_SYMBOLS );
				mp_fileLibrary->ClearFile( pFileInfo->fileNameChecksum, pFileInfo->fileExtensionChecksum );
			}
			break;

			case 0xe7308c1f:	// GEOM
			case 0xfd8697e1:	// SKIN
			case 0x0524fd4e:	// MDL
			{
				char model_asset_filename[512];
				sprintf( model_asset_filename, "%08x", pFileInfo->fileNameChecksum + pFileInfo->fileExtensionChecksum );
				uint32 model_asset_name = pFileInfo->fileNameChecksum + CRCD(0xfd8697e1,"SKIN");// pFileInfo->fileExtensionChecksum;
//				Dbg_Message( "Adding model asset %08x", model_asset_name );
						
				Ass::SCutsceneModelDataInfo theInfo;
				theInfo.modelChecksum = model_asset_name;
				theInfo.pModelData = pData;
				theInfo.modelDataSize = pFileInfo->fileSize;
				theInfo.pTextureData = mp_fileLibrary->GetFileData( pFileInfo->fileNameChecksum, CRCD(0x1512808d,"TEX"), true );
				const File::SFileInfo* pTextureFileInfo;
				pTextureFileInfo = mp_fileLibrary->GetFileInfo( pFileInfo->fileNameChecksum, CRCD(0x1512808d,"TEX"), true );
				Dbg_Assert( pTextureFileInfo );
				theInfo.textureDataSize = pTextureFileInfo->fileSize;
					
				// CAS data isn't always required;  only for certain SKIN files
				theInfo.pCASData = (uint8*)mp_fileLibrary->GetFileData( pFileInfo->fileNameChecksum, CRCD(0xffc529f4,"CAS"), false );;

				theInfo.isSkin = pFileInfo->fileExtensionChecksum == CRCD(0xfd8697e1,"SKIN");		
				theInfo.texDictChecksum = pTextureFileInfo->fileNameChecksum + pTextureFileInfo->fileExtensionChecksum;
				
				// TODO:  Should be true if this is the skater model...
				theInfo.doShadowVolume = false;	

				// GJ:  make sure the texture dictionary offset doesn't clash with skaters
				// there must be a cleaner way to do this...  there must!
				if ( pFileInfo->fileExtensionChecksum == CRCD(0xe7308c1f,"GEOM") )
				{
					// geoms shouldn't specify tex dict offset, because
					// non-zero tex dict offsets will try to load themselves
					// up as a MDL, rather than as a GEOM
					theInfo.texDictOffset = 0;
				}
				else
				{
					theInfo.texDictOffset = vCUTSCENE_TEXDICT_RANGE + i;
				}

				// set up some default parameters
				bool permanent = false;
				int group = 0;

				// these file types get added to the asset manager
				// (if they don't already exist)...  remember to remove
				// them when the cutscene is done!
				Ass::CAssMan* pAssMan = Ass::CAssMan::Instance();

				// by default, load the asset...  the cutscene heads
				// go through an additional piece of code to figure out
				// whether they will actually be needed during the cutscene
				bool should_load_asset = true;

				// used for cutscene head scaling
				uint8* pWeightMapData = (uint8*)mp_fileLibrary->GetFileData( pFileInfo->fileNameChecksum, CRCD(0x2cd4107d,"WGT"), false );
				if ( pWeightMapData )
				{
					Nx::SMeshScalingParameters theParams;
					
#ifdef __NOPT_ASSERT__
					if ( Script::GetInteger( CRCD(0xf28e1301,"CutsceneScaleTest"), Script::NO_ASSERT ) )
					{
						Script::CVector* pVec = NULL;
						pVec = Script::GetVector( CRCD(0x513a55f6,"CutsceneScaleTestNeck") );
						testBoneScales[0] = Mth::Vector( pVec->mX, pVec->mY, pVec->mZ, 1.0f );
						pVec = Script::GetVector( CRCD(0xd6d87539,"CutsceneScaleTestHead") );
						testBoneScales[1] = Mth::Vector( pVec->mX, pVec->mY, pVec->mZ, 1.0f );
						pVec = Script::GetVector( CRCD(0x9dea75ff,"CutsceneScaleTestJaw") );
						testBoneScales[2] = Mth::Vector( pVec->mX, pVec->mY, pVec->mZ, 1.0f );
						pVec = Script::GetVector( CRCD(0x0767f02b,"CutsceneScaleTestHat") );
						testBoneScales[3] = Mth::Vector( pVec->mX, pVec->mY, pVec->mZ, 1.0f );
						pVec = Script::GetVector( CRCD(0xba314cf8,"CutsceneScaleTestBrow") );
						testBoneScales[4] = Mth::Vector( pVec->mX, pVec->mY, pVec->mZ, 1.0f );
					}
					else
#endif
					{
						// get it from the profile
						Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( 0 );
						Gfx::CModelAppearance* pCASModelAppearance = pSkaterProfile->GetAppearance();
						Script::CStruct* pCASModelStruct = pCASModelAppearance->GetStructure();

						testBoneScales[0] = get_scale( pCASModelStruct, CRCD(0xa28fab7f,"head_bone_group") );
						testBoneScales[1] = get_scale( pCASModelStruct, CRCD(0xa28fab7f,"head_bone_group") );
						testBoneScales[2] = get_scale( pCASModelStruct, CRCD(0x0442d3c1,"jaw_bone_group") );
						testBoneScales[3] = get_scale( pCASModelStruct, CRCD(0xb90f08d0,"headtop_bone_group") );
						testBoneScales[4] = get_scale( pCASModelStruct, CRCD(0x1dbcfae8,"nose_bone_group") );
					}

					// skip past version number
					pWeightMapData += sizeof(int); 

					int numVerts = *(int*)pWeightMapData;
					pWeightMapData += sizeof(int);

	    			theParams.pWeights = (float*)pWeightMapData;
					theParams.pWeightIndices = (char*)(pWeightMapData + (numVerts * 3 * sizeof(float)));
					
					//theParams.pWeights = &testWeights[0];
					//theParams.pWeightIndices = &testWeightIndices[0];
					theParams.pBonePositions = &testBonePositions[0];
					theParams.pBoneScales = &testBoneScales[0];

					// it's a cutscene head, so only load the model if it's the head that 
					// we're going to need in the cutscene...  the 4 cutscene heads are always
					// ordered male/nonfacemapped, male/facemapped, female/nonfacemapped, 
					// female/facemapped, so we should be able to figure out which head
					// is which depending on what other files exist...
					int is_male_head = ( mp_fileLibrary->GetFileInfo( pFileInfo->fileNameChecksum + 2, CRCD(0xfd8697e1,"SKIN"), false ) != NULL );
					int is_facemapped_head;
					if ( is_male_head )
					{
						is_facemapped_head = ( mp_fileLibrary->GetFileInfo( pFileInfo->fileNameChecksum - 1, CRCD(0xfd8697e1,"SKIN"), false ) != NULL );
					}
					else
					{
						is_facemapped_head = ( mp_fileLibrary->GetFileInfo( pFileInfo->fileNameChecksum + 1, CRCD(0xfd8697e1,"SKIN"), false ) == NULL );
					}

					int is_male_skater;
					int is_facemapped_skater;
					Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( 0 );
					Gfx::CModelAppearance* pCASModelAppearance = pSkaterProfile->GetAppearance();
					Gfx::CFaceTexture* pCASFaceTexture = pCASModelAppearance->GetFaceTexture();
					is_facemapped_skater = ( pCASFaceTexture && pCASFaceTexture->IsValid() );					
					Script::CStruct* pInfo = pSkaterProfile->GetInfo();
					pInfo->GetInteger( CRCD(0x3f813177,"is_male"), &is_male_skater, Script::ASSERT );

					// only load the correct head from the 4 cutscene heads
					should_load_asset = ( ( is_facemapped_head == is_facemapped_skater ) && ( is_male_head == is_male_skater) );
				
					if ( should_load_asset )
					{
						// only want to set the parameters if we're sure we're going to be loading the asset...
						Nx::CEngine::sSetMeshScalingParameters( &theParams );
					}
				}

				if ( should_load_asset )
				{
					pAssMan->LoadAssetFromStream( model_asset_name, 
												  Ass::ASSET_SKIN, 
												  (uint32*)&theInfo, 
												  sizeof(Ass::SCutsceneModelDataInfo), 
												  permanent,
												  group);

					// keep track of which assets are loaded, 
					// so that we know to remove it in the destructor...
					Dbg_MsgAssert( m_numAssets < vMAX_CUTSCENE_ASSETS, ( "Too many assets (increase vMAX_CUTSCENE_ASSETS > %d)", vMAX_CUTSCENE_ASSETS ) );
					m_assetName[m_numAssets] = model_asset_name;
					m_numAssets++;
				}
				
				mp_fileLibrary->ClearFile( pFileInfo->fileNameChecksum, pFileInfo->fileExtensionChecksum );
				mp_fileLibrary->ClearFile( pFileInfo->fileNameChecksum, CRCD(0x1512808d,"TEX") );
				mp_fileLibrary->ClearFile( pFileInfo->fileNameChecksum, CRCD(0xffc529f4,"CAS") );
				mp_fileLibrary->ClearFile( pFileInfo->fileNameChecksum, CRCD(0x2cd4107d,"WGT") );
		  }
			break;
/*
			case 0xedd8d75f:	// SKE
				{
					char skeleton_asset_filename[512];
					sprintf( skeleton_asset_filename, "%08x", pFileInfo->fileNameChecksum + pFileInfo->fileExtensionChecksum );
					uint32 skeleton_asset_name = pFileInfo->fileNameChecksum + pFileInfo->fileExtensionChecksum;
					Dbg_Message( "Adding asset %08x", skeleton_asset_name );
						
					// set up some default parameters
					bool permanent = false;
					int group = 0;
					 
					// these file types get added to the asset manager
					// (if they don't already exist)...  remember to remove
					// them when the cutscene is done!
					Ass::CAssMan* pAssMan = Ass::CAssMan::Instance();
					pAssMan->LoadAssetFromStream( skeleton_asset_name, 
												  Ass::ASSET_SKELETON, 
												  (uint32*)pData, 
												  pFileInfo->fileSize, 
												  permanent,
												  group);

					// keep track of which assets are loaded, 
					// so that we know to remove it in the destructor...
					Dbg_MsgAssert( m_numAssets < vMAX_CUTSCENE_ASSETS, ( "Too many assets (increase vMAX_CUTSCENE_ASSETS > %d)", vMAX_CUTSCENE_ASSETS ) );
					m_assetName[m_numAssets] = skeleton_asset_name;
					m_numAssets++;
				}
				break;
*/

			case 0x5ac14717:	// CIF
				// do nothing;  cif packet is handled during a separate step
				break;

			case 0x1512808d:	// TEX
				// do nothing;  loading of texture dictionaries is handled
				// automatically by the loading of models/skins
				break;

			case 0xffc529f4:	// CAS
				// do nothing;  loading of CAS flags are handled
				// automatically by the loading of models/skins
				break;

			case 0x2cd4107d:	// WGT
				// do nothing;  loading of mesh scaling weight maps are handled
				// automatically by the loading of models/skins
				break;
			
			default:
				// file type ignored on this pass
				break;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCutsceneData::init_objects()
{
	// now create the objects once the assets have been loaded
	uint32* pCIFData = mp_fileLibrary->GetFileData( 0, CRCD(0x5ac14717,"CIF"), true );
	Dbg_Assert( pCIFData );
	create_objects( pCIFData );
	mp_fileLibrary->ClearFile( 0, CRCD(0x5ac14717,"CIF") );

#ifdef __NOPT_ASSERT__
	/*
	can't do this test any more, because we're clearing out unused data
	
	// check for valid skeleton, should be moved to the exporter
	for ( int i = 0; i < m_numObjects; i++ )
	{
		SCutsceneObjectInfo* pObjectInfo = &m_objects[i];
		
		if ( pObjectInfo->m_doPlayerSubstitution )
		{
			// no checks
		}
		else if ( pObjectInfo->m_skeletonName == 0 )
		{
			// if the object has no skeleton, make sure that this is an MDL, and not a SKIN
			if ( mp_fileLibrary->GetFileData( pObjectInfo->m_objectName, Crc::GenerateCRCFromString("SKIN"), false ) )
			{
				Dbg_MsgAssert( 0, ( "No skeleton specified for object %08x, even though it was listed as a SKIN file", pObjectInfo->m_objectName ) );
			}
		}
		else
		{
			if ( !mp_fileLibrary->GetFileData( pObjectInfo->m_objectName, Crc::GenerateCRCFromString("SKIN"), false ) )
			{
				Dbg_MsgAssert( 0, ( "Skeleton specified for object %08x, even though it was not listed as a SKIN file", pObjectInfo->m_objectName ) );
			}
		}
	}
	*/
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCutsceneData::PostLoad( bool assertOnFail )
{
	Dbg_MsgAssert( mp_fileLibrary, ( "No file library?!?" ) );

	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().CutsceneBottomUpHeap() );

	load_models();
	init_objects();
	
	Mem::Manager::sHandle().PopContext();

	delete mp_fileLibrary;
	mp_fileLibrary = NULL;

	//-------------------------------------------------------------
	// There's a blue glitch that appears for a few frames while
	// the cutscene manager waits for the stream to be preloaded.
	// The following loop will ensure that the stream is ready
	// to go before the video starts...

#ifndef __PLAT_NGC__
	while ( !m_audioStarted )
	{
		Pcm::Update();
		
		if ( Pcm::PreLoadMusicStreamDone() )
		{
			// TODO:  Fill in with appropriate music volume parameter?
			Pcm::StartPreLoadedMusicStream();

			m_audioStarted = true;
			break;
		}

		Tmr::VSync();
	}
#endif		// __PLAT_NGC__
	//-------------------------------------------------------------
	
	return true;
}
			   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCutsceneData::add_anim( uint32 animAssetName, uint32* pData, int fileSize )
{
	// set up some default parameters
	bool permanent = false;
	int group = 0;
	uint32 anim_asset_name = animAssetName;
					 
	// these file types get added to the asset manager
	// (if they don't already exist)...  remember to remove
	// them when the cutscene is done!
	Ass::CAssMan* pAssMan = Ass::CAssMan::Instance();
	pAssMan->LoadAssetFromStream( anim_asset_name, 
								  Ass::ASSET_ANIM, 
								  pData,
								  fileSize,
								  permanent,
								  group);

	// keep track of which assets are loaded, 
	// so that we know to remove it in the destructor...
	Dbg_MsgAssert( m_numAssets < vMAX_CUTSCENE_ASSETS, ( "Too many assets (increase vMAX_CUTSCENE_ASSETS > %d)", vMAX_CUTSCENE_ASSETS ) );
	m_assetName[m_numAssets] = anim_asset_name;
	m_numAssets++;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCutsceneData::add_boned_anim( uint32 animName, uint32* pData, int fileSize )
{
	Dbg_MsgAssert( m_numBonedAnims < vMAX_CUTSCENE_ANIMS, ( "Too many anims in cutscene (increase vMAX_CUTSCENE_ANIMS > %d).", vMAX_CUTSCENE_ANIMS ) );

	SCutsceneAnimInfo* pAnimInfo = &m_bonedAnims[m_numBonedAnims];

	Dbg_MsgAssert( !get_anim_info( animName ), ( "Anim already exists %s", Script::FindChecksumName(animName) ) );

	add_anim( animName, pData, fileSize );

#ifdef __PLAT_NGC__
	int size = sizeof( Nx::CQuickAnim );
	int mem_available;
	bool need_to_pop = false;
	if ( g_in_cutscene )
	{
		Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().FrontEndHeap() );
		mem_available = Mem::Manager::sHandle().Available();
		if ( size < ( mem_available - ( 40 * 1024 ) ) )
		{
			need_to_pop = true;
		}
		else
		{
			Mem::Manager::sHandle().PopContext();
			Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ThemeHeap() );
			mem_available = Mem::Manager::sHandle().Available();
			if ( size < ( mem_available - ( 5 * 1024 ) ) )
			{
				need_to_pop = true;
			}
			else
			{
				Mem::Manager::sHandle().PopContext();
				Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ScriptHeap() );
				mem_available = Mem::Manager::sHandle().Available();
				if ( size < ( mem_available - ( 40 * 1024 ) ) )
				{
					need_to_pop = true;
				}
				else
				{
					Mem::Manager::sHandle().PopContext();
				}
			}
		}
	}
#endif	// __PLAT_NGC__

	Nx::CQuickAnim* pBonedQuickAnim = new Nx::CQuickAnim;

#ifdef __PLAT_NGC__
	if ( need_to_pop )
	{
		Mem::Manager::sHandle().PopContext();
	}
#endif	// __PLAT_NGC__

	pBonedQuickAnim->SetAnimAssetName( animName );
	pAnimInfo->mp_bonedQuickAnim = pBonedQuickAnim;
	
	pAnimInfo->m_animName = animName;

//	Dbg_Message( "Boned anim 0x%08x has %d bones", animName, pBonedQuickAnim->GetNumBones() );

	m_numBonedAnims++;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

SCutsceneObjectInfo* CCutsceneData::get_object_info( uint32 objectName )
{
	for ( int i = 0; i < m_numObjects; i++ )
	{
		if ( m_objects[i].m_objectName == objectName )
		{
			return &m_objects[i];
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

SCutsceneAnimInfo* CCutsceneData::get_anim_info( uint32 animName )
{
	for ( int i = 0; i < m_numBonedAnims; i++ )
	{
		if ( m_bonedAnims[i].m_animName == animName )
		{
			return &m_bonedAnims[i];
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CCompositeObject* create_cutscene_dummy_object( Script::CStruct* pNodeData )
{  
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	CGeneralManager* p_obj_man = skate_mod->GetObjectManager();

	Obj::CCompositeObject* pDummyObject = new CCompositeObject;
	
	pDummyObject->SetType( SKATE_TYPE_COMPOSITE );
	
	p_obj_man->RegisterObject(*pDummyObject);
	
	Script::RunScript( CRCD(0x1f29c5a5,"cutsceneobj_add_components"), pNodeData, pDummyObject );
	
	pDummyObject->Finalize();

	pDummyObject->m_pos = Mth::Vector( 0.0f, 0.0f, 0.0f, 1.0f );

	return pDummyObject;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void apply_cas_scaling( Obj::CCompositeObject* pObject, Gfx::CSkeleton* pSourceSkeleton )
{
	Obj::CSkeletonComponent* pSkeletonComponent = GetSkeletonComponentFromObject( pObject );

	if ( pSkeletonComponent )
	{
		Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( pObject );
		if ( pModelComponent->HasRefObject() )
		{
			Gfx::CSkeleton* pSkeleton = pSkeletonComponent->GetSkeleton();
			Dbg_Assert( pSkeleton );

			pSkeleton->CopyBoneScale( pSourceSkeleton );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCutsceneData::create_objects( uint32* pData )
{
	// skip the version number
	pData++;

	// get the number of objects
	int numObjects = *pData;
	pData++;

	Dbg_Message("Found %d objects in the CIF file", numObjects);

	for ( int i = 0; i < numObjects; i++ )
	{
//		Dbg_Message( "Adding cutscene object #%d:", i );
		
		// remember the objectName
		uint32 objectName = *pData;
//		Dbg_Message( "objectName = %08x", *pData );
		pData++;

		// get the model name
		uint32 modelName = *pData;
		uint32 modelAssetName;
//		Dbg_Message( "modelName = %08x", *pData );
		modelAssetName = *pData + Script::GenerateCRC("SKIN");
//		Dbg_Message( "munged modelAssetName = %08x", modelAssetName );
		pData++;

		// get the skeleton name
		uint32 skeletonName;
		skeletonName = *pData;
		pData++;

		// get the anim name (the associated SKA file
		// which should be found somewhere in this CUT file)
		uint32 animName;
		animName = *pData;
		pData++;

		// get any extra flags
		uint32 flags;
		flags = *pData;
		pData++;

		Script::CStruct* pNodeData = new Script::CStruct;
										
		// This substitutes the	given model with the current player model
		// TODO:  Eventually phase out the name "skater"
		bool do_player_substitution = ( modelName == CRCD(0x5b8ab877,"skater") || modelName == CRCD(0x67e6859a,"player") );
		if ( do_player_substitution )
		{
			uint32 refObjectName = 0;
			pNodeData->AddChecksum( CRCD(0x153a84de,"refObjectName"), refObjectName );
		}
		else
		{
			// if it's not a skater model, then check for correct skeleton/anims
			if ( !mp_fileLibrary->FileExists( modelName, CRCD(0xfd8697e1,"SKIN") ) )
			{
//				Dbg_MsgAssert( skeletonName == 0, ( "Non-skinned model %08x has skeleton name", objectName ) );
				skeletonName = 0;
//				Dbg_MsgAssert( animName == 0, ( "Non-skinned model %08x has anim name", objectName ) );
				animName = 0;
			}
			else
			{
				Dbg_MsgAssert( skeletonName != 0, ( "Skinned model %08x has no skeleton name", objectName ) );
				Dbg_MsgAssert( animName != 0, ( "Skinned model %08x has no anim name", objectName ) );
			}

			// if it's the skater's head model and the player has a face texture,
			// then we need to swap in the eyeball-less head model...
			bool is_head_model = flags & vCUTSCENEOBJECTFLAGS_ISHEAD;
			bool is_skater_model = flags & vCUTSCENEOBJECTFLAGS_ISSKATER;
			if ( is_head_model && is_skater_model )
			{
				Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( 0 );
				Gfx::CModelAppearance* pCASModelAppearance = pSkaterProfile->GetAppearance();
				Gfx::CFaceTexture* pCASFaceTexture = pCASModelAppearance->GetFaceTexture();
				if ( pCASFaceTexture && pCASFaceTexture->IsValid() )
				{
					Dbg_MsgAssert( mp_fileLibrary->FileExists( modelName + 1, CRCD(0xfd8697e1,"SKIN") ), ( "Couldn't find eyeball-less head model" ) );

					// eyeball-less head model checksum = regular head model checksum + 1
					modelAssetName += 1;
				}

				// Load up the correct head, based on the skater's sex
				Script::CStruct* pInfo = pSkaterProfile->GetInfo();
				int is_male = 0;
				pInfo->GetInteger( CRCD(0x3f813177,"is_male"), &is_male, Script::ASSERT );
				if ( !is_male )
				{
					// female eyeball = male eyeball + 2
					// female eyeball-less = male eyeball-less + 2
					modelAssetName += 2;
				}

				// this will allow changing facial hair color (needed only for males)
				pNodeData->AddInteger( CRCD(0x92b43e79,"multicolor"), is_male );
			}
			
			pNodeData->AddChecksum( CRCD(0x86bd5b8f,"assetName"), modelAssetName );
		}

		// if you've got the special-case skull head,
		// then we don't want to subsitute the hi-res skeleton
		{
			bool is_head_model = flags & vCUTSCENEOBJECTFLAGS_ISHEAD;
			bool is_skater_model = flags & vCUTSCENEOBJECTFLAGS_ISSKATER;
			if ( is_head_model && is_skater_model )
			{
				Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( 0 );
				Gfx::CModelAppearance* pCASModelAppearance = pSkaterProfile->GetAppearance();

				Script::CStruct* pHeadStruct = NULL;
				pHeadStruct = pCASModelAppearance->GetActualDescStructure( CRCD(0x650fab6d,"skater_m_head") );
				if ( !pHeadStruct )
				{
					pHeadStruct = pCASModelAppearance->GetActualDescStructure( CRCD(0x0fc85bae,"skater_f_head") );
				}
				if ( pHeadStruct && pHeadStruct->ContainsFlag( CRCD(0xc4c5b2cc,"NoCutsceneHead") ) )
				{
					Script::RunScript( CRCD(0xf37d9f53,"UnhideLoResHeads") );
					delete pNodeData;
					continue;
				}

				// another special case for the paper bag...
				Script::CStruct* pHatStruct = NULL;
				pHatStruct = pCASModelAppearance->GetActualDescStructure( CRCD(0x6df453b6,"hat") );
				if ( pHatStruct && pHatStruct->ContainsFlag( CRCD(0xc4c5b2cc,"NoCutsceneHead") ) )
				{
					Script::RunScript( CRCD(0xf37d9f53,"UnhideLoResHeads") );
					delete pNodeData;
					continue;
				}
#if 0				
				Script::RunScript( CRCD(0xf37d9f53,"UnhideLoResHeads") );
				delete pNodeData;
				continue;
#endif
			}
		}

//		Dbg_Message( "skeletonName = %08x", skeletonName );
//		Dbg_Message( "animName = %08x", animName );
		
		// TODO:  if the skeleton asset were included
		// in the CUT file, then we wouldn't need the
		// artists to fill in this field manually:	
		if ( skeletonName )
		{
			pNodeData->AddChecksum( CRCD(0x09794932,"skeletonName"), skeletonName );

			// this disables the "bone skip list" optimization,
			// which would otherwise make the cutscene object's
			// hands disappear
			pNodeData->AddInteger( CRCD(0xd3982061,"max_bone_skip_lod"), 0 );
		}

		// shouldn't have an animation component, because
		// we want the cutscene details to fill up the skeleton
		// with the appropriate data (we don't want the
		// animation component's update() function to get
		// called once per frame and wipe out the cutscene
		// details' changes
		//pNodeData->AddChecksum( CRCD(0x6c2bfb7f,"animName"), anim_name );

		SCutsceneObjectInfo* pObjectInfo = &m_objects[m_numObjects];

		Dbg_MsgAssert( m_numObjects < vMAX_CUTSCENE_OBJECTS, ( "Too many objects in cutscene (increase vMAX_CUTSCENE_OBJECTS > %d).", vMAX_CUTSCENE_OBJECTS ) );
		pObjectInfo->mp_object = create_cutscene_dummy_object( pNodeData ); 

		// this	prevents the skateboard from clipping out
		// when it gets far enough away from the skater
		// (like in the NJ pool cutscene) 
		Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( pObjectInfo->mp_object );
		if ( pModelComponent )
		{
			Nx::CModel* pModel = pModelComponent->GetModel();
			Dbg_Assert( pModel );
			pModel->SetBoundingSphere( Mth::Vector(0.0f,0.0f,0.0f,50000.0f) );

	//		printf( "pModel has %d bones, transforms = %p\n", pModel->GetNumBones(), pModel->GetBoneTransforms() );

			if ( pModelComponent->HasRefObject() )
			{
				uint32 refObjectName = pModelComponent->GetRefObjectName();
				Obj::CCompositeObject* pRefObject = (Obj::CCompositeObject*)Obj::ResolveToObject( refObjectName );
				Dbg_Assert( pRefObject );
				Obj::CModelComponent* pRefModelComponent = GetModelComponentFromObject( pRefObject );
				Dbg_Assert( pRefModelComponent );
				Nx::CModel* pRefModel = pRefModelComponent->GetModel();
				
				// "m_original_params_set" ensures that we don't try
				// to write the same data out multiple times if there
				// are multiple create-a-skaters (which happens in the
				// final.cut cutscene)
				if ( !m_original_params_set )
				{
					m_original_player_bounding_sphere = pRefModel->GetBoundingSphere();
					m_original_hidden = pRefModel->IsHidden();
					m_original_scale_enabled = pRefModel->IsScalingEnabled();
					m_original_pos = pRefObject->GetPos();
					m_original_params_set = true;
				}
				
				pRefModel->SetBoundingSphere( Mth::Vector(0.0f,0.0f,0.0f,50000.0f) );
				pRefModel->Hide( false );
				pRefModel->EnableScaling( false );

				pRefModelComponent->UpdateBrightness();
			}

			// don't run the normal update() function,
			// or else it will screw up the double-buffer
			pModelComponent->Suspend(true);
		}

		pObjectInfo->m_objectName = objectName;

		// set the id, so that we can run scripts on it
		Dbg_MsgAssert( !Obj::ResolveToObject(objectName), ( "Object with name %s already exists", Script::FindChecksumName(objectName) ) );
		pObjectInfo->mp_object->SetID( objectName );

		// for debugging...
		pObjectInfo->m_skeletonName = skeletonName;
		pObjectInfo->m_doPlayerSubstitution = do_player_substitution;
		pObjectInfo->m_flags = flags;

		pObjectInfo->mp_animInfo = NULL;
		pObjectInfo->mp_parentObject = NULL;
		pObjectInfo->m_obaIndex = -1;

		// the animation doesn't necessarily exist for all the objects
		if ( animName )
		{
			// enforce that the anim with that name exists,
			pObjectInfo->mp_animInfo = get_anim_info( animName );
			Dbg_MsgAssert( pObjectInfo->mp_animInfo, ( "Couldn't find SKA with id 0x%08x in LIB file", animName ) );
		}
		
//		Dbg_Message( "---------------" );

		m_numObjects++;

		// done with temp struct
		delete pNodeData;
	}

	// apply cas scaling to the appropriate parts
	for ( int i = 0; i < m_numObjects; i++ )
	{
		SCutsceneObjectInfo* pObjectInfo = &m_objects[i];

		if ( pObjectInfo->m_doPlayerSubstitution )
		{
			Obj::CModelComponent* pModelComponent = GetModelComponentFromObject(pObjectInfo->mp_object);
			Dbg_Assert( pModelComponent );
			
			uint32 refObjectName = pModelComponent->GetRefObjectName();
			Obj::CCompositeObject* pRefObject = (Obj::CCompositeObject*)Obj::ResolveToObject( refObjectName );
			Dbg_Assert( pRefObject );
			
			// body
			Obj::CSkeletonComponent* pRefSkeletonComponent = GetSkeletonComponentFromObject(pRefObject);
			Dbg_Assert( pRefSkeletonComponent );
			apply_cas_scaling( pObjectInfo->mp_object, pRefSkeletonComponent->GetSkeleton() );
		}
		
		SCutsceneObjectInfo* pParentObjectInfo = get_object_info( pObjectInfo->m_objectName - 1 );
		if ( pParentObjectInfo && pParentObjectInfo->m_doPlayerSubstitution )
		{
			Obj::CModelComponent* pModelComponent = GetModelComponentFromObject(pParentObjectInfo->mp_object);
			Dbg_Assert( pModelComponent );
			
			uint32 refObjectName = pModelComponent->GetRefObjectName();
			Obj::CCompositeObject* pRefObject = (Obj::CCompositeObject*)Obj::ResolveToObject( refObjectName );
			Dbg_Assert( pRefObject );
			
			// head
			Obj::CSkeletonComponent* pRefSkeletonComponent = GetSkeletonComponentFromObject(pRefObject);
			Dbg_Assert( pRefSkeletonComponent );
			apply_cas_scaling( pObjectInfo->mp_object, pRefSkeletonComponent->GetSkeleton() );
		}
	}

	// do basic poly removal on all objects
	// w/ some extra poly removal on the player's head
	for ( int i = 0; i < m_numObjects; i++ )
	{
		SCutsceneObjectInfo* pObjectInfo = &m_objects[i];

		Obj::CModelComponent* pModelComponent = GetModelComponentFromObject(pObjectInfo->mp_object);
		if ( pModelComponent )
		{
			Nx::CModel* pModel = pModelComponent->GetModel();

			uint32 polyRemovalMask = pModel->GetPolyRemovalMask();

			SCutsceneObjectInfo* pParentObjectInfo = get_object_info( pObjectInfo->m_objectName - 1 );
			if ( pParentObjectInfo && pParentObjectInfo->m_doPlayerSubstitution )
			{
				// treat the player's head as a special case
				// because we need to grab the poly removal
				// mask from the player's body

				Obj::CModelComponent* pModelComponent = GetModelComponentFromObject(pParentObjectInfo->mp_object);
				Dbg_Assert( pModelComponent );

				uint32 refObjectName = pModelComponent->GetRefObjectName();
				Obj::CCompositeObject* pRefObject = (Obj::CCompositeObject*)Obj::ResolveToObject( refObjectName );
				Dbg_Assert( pRefObject );

				Obj::CModelComponent* pRefModelComponent = GetModelComponentFromObject(pRefObject);
				Dbg_Assert( pRefModelComponent );

				Nx::CModel* pRefModel = pRefModelComponent->GetModel();
				Dbg_Assert( pRefModel );

				// head
				polyRemovalMask |= pRefModel->GetPolyRemovalMask();
			}

			pModel->HidePolys( polyRemovalMask );
		}
	}

	// relink parents if necessary
	for ( int i = 0; i < m_numObjects; i++ )
	{
		SCutsceneObjectInfo* pObjectInfo = &m_objects[i];

		// checks for parents, if any...  based on naming convention
		SCutsceneObjectInfo* pParentObjectInfo = get_object_info( pObjectInfo->m_objectName - 1 );
		if ( pParentObjectInfo )
		{
			pObjectInfo->mp_parentObject = pParentObjectInfo->mp_object;

			if ( pParentObjectInfo->m_doPlayerSubstitution )
			{
				// it's the hires create-a-skater head,
				// so need to do some extra texture	replacement on it
				Obj::CModelComponent* pCASModelComponent = GetModelComponentFromObject(pObjectInfo->mp_object);
				Dbg_Assert( pCASModelComponent );
				
				Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( 0 );
				Gfx::CModelAppearance* pCASModelAppearance = pSkaterProfile->GetAppearance();

				pCASModelComponent->InitModelFromProfile(pCASModelAppearance,false,0,CRCD(0x8fb01036,"replace_cutscene_skater_from_appearance"));
			
				// apply color from the skater appearance
				// (we can't do this in replace_cutscene_skater_from_appearance
				// because it might have an object color to set, which you can
				// only do if you have the correct geom name)
				Script::CStruct* pHeadStruct = NULL;

				pCASModelAppearance->PrintContents();

				pHeadStruct = pCASModelAppearance->GetVirtualDescStructure( CRCD(0x650fab6d,"skater_m_head") );
				
				if ( !pHeadStruct )
				{
					pHeadStruct = pCASModelAppearance->GetVirtualDescStructure( CRCD(0x0fc85bae,"skater_f_head") );
				}
				
				if ( pHeadStruct )
				{
					Nx::CModel* pCASModel = pCASModelComponent->GetModel();
					Dbg_Assert( pCASModel );

					int use_default_hsv = 1;
					pHeadStruct->GetInteger( CRCD(0x97dbdde6,"use_default_hsv"), &use_default_hsv );
					if ( !use_default_hsv )
					{
						int h, s, v;
						if ( pHeadStruct->GetInteger( CRCD(0x6e94f918,"h"), &h, false )
							 && pHeadStruct->GetInteger( CRCD(0xe4f130f4,"s"), &s, false )
							 && pHeadStruct->GetInteger( CRCD(0x949bc47b,"v"), &v, false ) )
						{
							Script::CStruct* pTempStruct = new Script::CStruct;
							pTempStruct->AddChecksum( CRCD(0xb6f08f39,"part"), 0 );
							pTempStruct->AddChecksum( CRCD(0x83418a6a,"material"), CRCD(0xaa2206ef,"cashead_head") );
							pTempStruct->AddInteger( CRCD(0x318f2bdb,"pass"), 0 );
							pCASModel->SetColor( pTempStruct, (float)h, (float)s / 100.0f, (float)v / 100.0f );
							delete pTempStruct;
						}
					}
				}
			}
		}
		
		Dbg_Assert( mp_objectQuickAnim );
		for ( int j = 0; j < mp_objectQuickAnim->GetNumBones(); j++ )
		{
			uint32 obaObjectName = mp_objectQuickAnim->GetBoneName(j);
			if ( pObjectInfo->m_objectName == obaObjectName )
			{
				pObjectInfo->m_obaIndex	= j;
				break;
			}
		}
	}


	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char* get_cut_name( const char* p_fileName )
{
	static char cutsceneName[256];

	for ( int i = strlen(p_fileName); i >= 0; i-- )
	{
		if ( p_fileName[i] == '\\' || p_fileName[i] == '/' )
		{
			strcpy( cutsceneName, &p_fileName[i+1] );
			break;
		}
	}

	// strip off extension, if one exists
	char* pExt = strstr( cutsceneName, "." );
	if ( pExt )
	{
		*pExt = NULL;
	}

	return cutsceneName;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCutsceneData::Load( const char* p_fileName, bool assertOnFail, bool async_load )
{   
	// now that the async code is working, 
	// there's no reason to load it synchronously
	async_load = true;

	// GJ:  Disabled async load to remove
	// the glitch when starting cutscenes
	async_load = false;
	
	// ...  but turn off async for platforms that
	// don't support it	anyway
	if ( !File::CAsyncFileLoader::sAsyncSupported() )
	{
		async_load = false;
	}
	
	// based on the name of the CUT file, 
	// we can build the name of the stream associated with it
	char* pCutsceneName = get_cut_name( p_fileName );

	//-------------------------------------------------------------
	// Load up the correct stream, based on the skater's sex
	Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( 0 );
	Script::CStruct* pInfo = pSkaterProfile->GetInfo();
	int is_male = 0;
	pInfo->GetInteger( CRCD(0x3f813177,"is_male"), &is_male, Script::ASSERT );
	
	char streamName[256];
	if ( is_male )
	{
		sprintf( streamName, "%s_Male", pCutsceneName );
	}
	else
	{
		sprintf( streamName, "%s_Female", pCutsceneName );
	}
	
	uint32 streamNameChecksum;
	streamNameChecksum = Crc::GenerateCRCFromString(streamName);

#ifndef __PLAT_NGC__
	if ( !Pcm::PreLoadMusicStream( streamNameChecksum ) )
	{
		Dbg_Message( "Couldn't start stream...  stream %s not found?", streamName );
		m_audioStarted = true;
	}
#endif		// __PLAT_NGC__
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	// Create a cutscene heap here...  it will only be used for the 
	// cutscene, and will prevent fragmentation on the bottom up heap
	// by things that might get allocated during the cutscene,
	// such as proxim nodes.
	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().TopDownHeap() );
		
	// reserve about 100K for any allocations that need to go on the 
	// bottom up heap (such as proxim nodes).  Also need to reserve
	// some room on the topdown heap for texture dictionary loading
	// on the PS2
	int mem_available = Mem::Manager::sHandle().Available();
#	if defined( __PLAT_NGC__ )
#ifdef DVDETH
	mem_available -= 6 * 1024 * 1024;
#else
#ifdef __NOPT_ASSERT__
	mem_available = 3 * 1024 * 1024;
#else

# define toupper(c) ( ( (c) >= 'a' ) && ( (c) <= 'z' ) ) ? (c) += ( 'A' - 'a' ) : (c)
	char name[64];
	char * ps = (char *)p_fileName;
	char * pd = name;
	while ( *ps != '\0' )
	{
		*pd = toupper( *ps );
		ps++;
		pd++;
	}

	if ( strstr( name, "FL_03" ) )
	{
		mem_available -= 200 * 1024;
		OSReport( "******* Reducing by 200k\n" );
	}
	else
	{
		if ( strstr( name, "NY_01V" ) )
		{
			mem_available -= 100 * 1024;
			OSReport( "******* Reducing by 80k\n" );
		}
		else
		{
			if ( strstr( name, "NY_02" ) )
			{
				mem_available -= 80 * 1024;
				OSReport( "******* Reducing by 80k\n" );
			}
			else
			{
				mem_available -= 120 * 1024;
				OSReport( "******* Reducing by 120k\n" );
			}
		}
	}
#endif		// __NOPT_ASSERT__ 
#endif		// DVDETH
#	elif defined( __PLAT_XBOX__ )
	// Reserve more for Xbox since audio streams will be allocated from bottom up heap.
	mem_available -= 512 * 1024;
#	else
	mem_available -= 200 * 1024;
#	endif		// __PLAT_NGC__

#ifdef __PLAT_NGC__
		if ( mem_available < ( 150 * 1024 ) )
		{
			// If heap is too small, allocate a 200k heap from the theme region.
			Mem::Manager::sHandle().PopContext();
			Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ThemeHeap() );
			mem_available = 150 * 1024;
		}
#endif

	Mem::Manager::sHandle().InitCutsceneHeap( mem_available );
	Mem::Manager::sHandle().PopContext(); 
	//-------------------------------------------------------------
#	if defined( __PLAT_NGC__ )
	g_in_cutscene = true;
#	endif		// __PLAT_NGC__

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().CutsceneBottomUpHeap());	
	Dbg_MsgAssert( !mp_fileLibrary, ( "File library already exists" ) );
	mp_fileLibrary = new File::CFileLibrary;
	Dbg_MsgAssert( mp_fileLibrary, ( "Couldn't create file library %s", p_fileName ) );
	Mem::Manager::sHandle().PopContext();

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().CutsceneTopDownHeap());	

	bool success = mp_fileLibrary->Load( p_fileName, assertOnFail, async_load );
	
	if ( !success )
	{
		Dbg_MsgAssert( 0, ( "Couldn't load file library %s", p_fileName ) );
	}
	
	Mem::Manager::sHandle().PopContext();

	if ( !async_load )
	{
		PostLoad( assertOnFail );
		m_dataLoaded = true;
	}

#ifdef __PLAT_NGC__
	if ( !Pcm::PreLoadMusicStream( streamNameChecksum ) )
	{
		Dbg_Message( "Couldn't start stream...  stream %s not found?", streamName );
		m_audioStarted = true;
	}

	while ( !m_audioStarted )
	{
		Pcm::Update();
		
		if ( Pcm::PreLoadMusicStreamDone() )
		{
			// TODO:  Fill in with appropriate music volume parameter?
			Pcm::StartPreLoadedMusicStream();

			m_audioStarted = true;
			break;
		}

		Tmr::VSync();
	}
#endif		// __PLAT_NGC__

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCutsceneData::LoadFinished()
{
	if ( m_dataLoaded )
	{
		// already finished before this...
		return true;
	}

	// check to see whether the file has finished loading
	if ( !mp_fileLibrary->LoadFinished() )
	{
		Dbg_Message( "Waiting for async load to finish..." );
		return false;
	}
	else
	{
		PostLoad( true );
		m_dataLoaded = true;
		return true;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
										    
void CCutsceneData::update_video_playback( Gfx::Camera* pCamera, Script::CStruct* pStruct )
{
	if ( !m_videoStarted )
	{
		// need to pass in the fadeintime
//		Script::SpawnScript( CRCD(0x55665639,"FadeInCutscene"), pStruct );
		m_videoStarted = true;
		
		Script::RunScript( CRCD(0xe794a164,"CutsceneStartVideo") );

		// initialize the camera (don't really need to do this,
		// since the camera animation will override it)
		Dbg_Assert( pCamera );
//		Mth::Vector& camPos = pCamera->GetPos();
//		Mth::Matrix& camMatrix = pCamera->GetMatrix();
//		camPos = Mth::Vector(0.0f,0.0f,0.0f,1.0f);
//		camMatrix.Ident();
	
		// reset the camera to some default FOV,
		// or else it will use the last FOV used
		float fov_in_degrees = Script::GetFloat( CRCD(0x99529205, "camera_fov") );
		pCamera->SetHFOV(fov_in_degrees);

		m_oldTime = 0.0f;
		m_initialVBlanks = Tmr::GetVblanks();
	}

	// update universal cutscene time
	// (shouldn't use the animcontroller's update function
	// because it caps the frame length...  although
	// there's no real reason that it needs to... 
	// maybe we can have the animcontroller use the
	// uncapped version as well...  it's only capped
	// for things like the physics so that objects
	// don't travel too far in a single frame...)
	m_oldTime = m_currentTime;
	
#if 0
	// here's some debug code for debugging cutscene glitches...
	// this will loop through 1 second's worth of cutscene data
	// at half speed.  it was handy for tracking down 
	// SK5:TT10351 - "pop" in camera animation when going to
	// a moving camera cut
	static float cutscene_test_time_offset = 0.0f;
	cutscene_test_time_offset += ( 1/120.0f );
	if (cutscene_test_time_offset >= 1.0f )
	{
		cutscene_test_time_offset -= 1.0f;
	}

	m_currentTime = Script::GetFloat( "cutscene_test_time" ) + cutscene_test_time_offset;//(float)( Tmr::GetVblanks() - m_initialVBlanks ) / Config::FPS();
#else
	m_currentTime = (float)( Tmr::GetVblanks() - m_initialVBlanks ) / Config::FPS();
#endif

	// if the CUT file is really small, then chances are
	// the Makes will still be set from choosing the
	// item on the menu...  so, the following makes
	// sure that you don't try to skip it before the
	// cutscene has played for at least 1 frame...
	if ( m_videoStarted && ( m_currentTime != 0.0f ) )
	{
		Mdl::FrontEnd* pFront = Mdl::FrontEnd::Instance();
		
		for ( int i = 0; i < SIO::vMAX_DEVICES; i++ )
		{
			Inp::Handler< Mdl::FrontEnd >* pHandler = pFront->GetInputHandler( i );
			if ( pHandler->m_Device->IsPluggedIn() && pHandler->m_Device->HasValidControlData() && pHandler->m_Input->m_Makes & Inp::Data::mD_X )
			{
				// GJ FIX FOR TT12703:  "If the player disconnects the 
				// controller during any cut scene in the game, the player
				// can reconnect the controller into any of the other 3
				// controller ports and bypass the cut scene by pressing
				// the A button. Since story mode is a one player game, the
				// player should not be able to use multiple ports at the
				// same time."
				if ( i == Mdl::Skate::Instance()->m_device_server_map[0] )
				{
					// skip to the end of the movie...
					m_currentTime = mp_cameraQuickAnim->GetDuration();
					m_videoAborted = true;
				}
				else
				{
					Dbg_Message( "Can only abort cutscenes from first controller" );
				}
			}
		}
	}

	update_camera( pCamera );

	// boned anims should come before moving objects,
	// because some moving objects depend on the 
	// current position of the neck bone
	update_boned_anims();	

	update_moving_objects();

	update_extra_cutscene_objects();

/*
	// display moving objects here
	// (need to do it after the object anims AND
	// the skeletal anims have been updated for
	// this frame)
	for ( int i = 0; i < m_numObjects; i++ )
	{
		if ( m_objects[i].mp_animInfo )
		{
			Obj::CCompositeObject* pCompositeObject = m_objects[i].mp_object;	
			Dbg_Assert( pCompositeObject );

			Obj::CSkeletonComponent* pSkeletonComponent = GetSkeletonComponentFromObject( pCompositeObject );
			if ( pSkeletonComponent )
			{
				Gfx::CSkeleton* pSkeleton = pSkeletonComponent->GetSkeleton();
				Dbg_Assert( pSkeleton );
	
				// if we're going to grab the display matrix, need to grab it
				// *after* the object anims have been updated
				Mth::Matrix theMat = pCompositeObject->GetDisplayMatrix();
				theMat[Mth::POS] = pCompositeObject->GetPos();
				pSkeleton->Display( &theMat, 1.0f, 0.0f, 0.0f );
			}
		}
	}
*/
	if ( Script::GetInteger( CRCD(0x5ecc0073,"debug_cutscenes") ) )
	{
		// pass the pertinent info to the panel:
		Script::CStruct* pTempStruct = new Script::CStruct;

//			char line1[128];
//			sprintf( line1, "camera = %f", m_cameraController.GetCurrentAnimTime() );
//			pTempStruct->AddString( "line1", (const char*)&line1 );

		char line1[128];
		sprintf( line1, "time = %f", m_currentTime );
		pTempStruct->AddString( "line1", (const char*)&line1 );

		Script::RunScript( CRCD(0xd9157174,"draw_cutscene_panel"), pTempStruct );

		delete pTempStruct;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCutsceneData::update_extra_cutscene_objects()
{
	// GJ:  the cutscene manager updates the camera position after
	// the object manager updates the extra peds in the scene,
	// meaning that the extra peds will still be using the
	// previous frame's camera position to calculate whether it
	// should be visible.  effectively, this causes a 1-frame glitch
	// whenever there's a new camera cut and a ped appears for the
	// first time on-screen.  to patch this, i will loop through
	// the list of all the extra peds for the cutscene and re-apply
	// their model positions.
	
	Script::CArray* pArray = Script::GetArray( CRCD(0x67f08671,"CutsceneObjectNames"), Script::NO_ASSERT );
	if ( pArray )
	{
		for ( uint32 i = 0; i < pArray->GetSize(); i++ )
		{
			uint32 objectName = pArray->GetChecksum( i );
			Obj::CCompositeObject* pObject = (Obj::CCompositeObject*)Obj::ResolveToObject( objectName );
			if ( pObject )
			{
				Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( pObject );
				if ( pModelComponent && pModelComponent->GetModel() )
				{
					pModelComponent->GetModel()->SetActive( true );
					pModelComponent->Update();
				}
			}
		}
	}

	/*
	// repeat for the skater
	Obj::CCompositeObject* pObject = (Obj::CCompositeObject*)Obj::ResolveToObject( 0 );
	if ( pObject )
	{
		Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( pObject );
		if ( pModelComponent && pModelComponent->GetModel() )
		{
			pModelComponent->GetModel()->SetActive( true );
//			pModelComponent->Update();
		}
	}
	*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCutsceneData::Update( Gfx::Camera* pCamera, Script::CStruct* pStruct )
{
	if ( !m_audioStarted )
	{
		// TODO:  Maybe have some timeout, so that if it 
		// can't find the stream, it won't wait forever...

		if ( Pcm::PreLoadMusicStreamDone() )
		{
			// TODO:  Fill in with appropriate music volume parameter
			Pcm::StartPreLoadedMusicStream();

			m_audioStarted = true;
			
			// GJ:  This seemed to cause sync problems
			// when the CUT file is really small (quick
			// to load), and the streams aren't ready
			// to go yet...  need to look into it more

			// now that the music stream has started,
			// we should start updating the video
			// on this frame
			update_video_playback( pCamera, pStruct );
		}
		else
		{
			Dbg_Message( "Waiting for streams to preload to finish..." );
		}
	}
	else
	{
		update_video_playback( pCamera, pStruct );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCutsceneData::IsAnimComplete()
{
	// we don't call the Update() function any more
	// (because Update() uses the capped frame rate)
	//return m_cameraController.IsAnimComplete();

	return m_currentTime >= mp_cameraQuickAnim->GetDuration(); 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCutsceneData::update_camera( Gfx::Camera* pCamera )
{
	Dbg_Assert( pCamera );

	// Not sure if this is still needed for the Doppler effect code
	// Maybe we can add the functionality automatically to the Gfx::Camera?
	pCamera->StoreOldPos();

	Mth::Vector& camPos = pCamera->GetPos();
	Mth::Matrix& camMatrix = pCamera->GetMatrix();
	
	Mth::Quat theQuat;
	Mth::Vector theTrans;

	get_current_frame( &theQuat, &theTrans );
		
	// update the camera position
	camPos = theTrans;

	// update the camera orientation
	Mth::Vector nullTrans;
	nullTrans.Set(0.0f,0.0f,0.0f,1.0f);		// Mick: added separate initialization
	Mth::QuatVecToMatrix( &theQuat, &nullTrans, &camMatrix );

	if ( !m_videoAborted )
	{
		// any allocations during the cutscene should go on the
		// cutscene heap (such as pedestrians).  at the end
		// of the cutscene, we require that the cutscene heap
		// is empty before it is deleted...
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().CutsceneBottomUpHeap());	

		// process any new keys since last frame...
		// do this AFTER the camera is updated for
		// the frame, because a custom key might
		// override the normal value (this is a fix
		// for the double-cut problem, which
		// tries to interpolate between the end
		// of one camera and the beginning of the
		// next...  what we really want is to 
		// keep on the end of the previous camera)
		mp_cameraQuickAnim->ProcessCustomKeys( m_oldTime, m_currentTime, pCamera );
		
		Mem::Manager::sHandle().PopContext();
	}

//	printf( "Done for frame\n" );

//	camPos.PrintContents();
//	camMatrix.PrintContents();
}

// TODO:  Check for overflow of temp arrays
static Mth::Quat s_temp_quats[96];
static Mth::Vector s_temp_vectors[96];

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCutsceneData::update_boned_anims()
{
	for ( int i = 0; i < m_numObjects; i++ )
	{
		if ( m_objects[i].mp_animInfo )
		{
			Obj::CCompositeObject* pCompositeObject = m_objects[i].mp_object;	
			Dbg_Assert( pCompositeObject );

			Obj::CSkeletonComponent* pSkeletonComponent = GetSkeletonComponentFromObject( pCompositeObject );
			if ( pSkeletonComponent )
			{
				Gfx::CSkeleton* pSkeleton = pSkeletonComponent->GetSkeleton();
				Dbg_Assert( pSkeleton );

				Nx::CQuickAnim* pBonedQuickAnim = m_objects[i].mp_animInfo->mp_bonedQuickAnim;

				// turn off the bone skipping, because we're
				// not going through the normal animation code
				pSkeleton->SetBoneSkipDistance( 0.0f );

				pBonedQuickAnim->GetInterpolatedFrames( &s_temp_quats[0], &s_temp_vectors[0], pSkeleton->GetBoneSkipList(), pSkeleton->GetBoneSkipIndex(), m_currentTime );

				if ( m_objects[i].m_doPlayerSubstitution && pSkeleton->GetNumBones() == 55 )
				{					
					int bone_index = pSkeleton->GetBoneIndexById( CRCD(0x7268c230,"bone_jaw") );
					
					// GJ:  the hi-res jaw bone and the low-res jaw bone aren't in the
					// exact same position, which is required for the skinning to
					// work properly (otherwise, it would create a gap between the
					// head and body models.)  ideally on the next project, we will
					// change the exporter so that it puts the correct low-res jaw
					// position into the body's SKA, and the correct hi-res jaw into
					// the head's SKA.  For now, since we only have access to the
					// correct hi-res jaw bone position, we need to explicitly
					// offset the position of the low-res jaw bone here.  the offset
					// was derived by subtracting the low-res jaw bone bone-space 
					// translation (-0.000004, -1.776663, 0.599479) from the hi-res
					// jaw's bone-space translation (0, -1.57249, 0.08276)
					s_temp_vectors[bone_index] += Mth::Vector(0.000000f, -1.776663f-(-1.57249f), 0.599479f-0.08276f, 1.000000f);

//					s_temp_vectors[bone_index] = Mth::Vector(0.000000f, -2.531250f, 1.406250f, 1.000000f);
				}

				pSkeleton->Update( &s_temp_quats[0], &s_temp_vectors[0] );
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void update_model_component( Obj::CCompositeObject* pCompositeObject )
{
	CModelComponent* pModelComponent = GetModelComponentFromObject( pCompositeObject );
	if ( pModelComponent )
	{
		// if it's hidden, there's no reason to update it...
		// (as a side benefit, this also allows us to have multiple
		// objects referencing the same skater, such as in final.cut...
		// otherwise, each object would try to update the same model's 
		// position, and only the last object processed would actually
		// look correct)
		Nx::CModel* pModel = pModelComponent->GetModel();
		Dbg_Assert( pModel );
		if ( pModel->IsHidden() )
		{
			return;
		}

		if ( pModelComponent->HasRefObject() )
		{
			uint32 refObjectName = pModelComponent->GetRefObjectName();
			Obj::CCompositeObject* pRefObject = (Obj::CCompositeObject*)Obj::ResolveToObject( refObjectName );
			Dbg_Assert( pRefObject );
			Obj::CModelComponent* pRefModelComponent = GetModelComponentFromObject( pRefObject );
			Dbg_Assert( pRefModelComponent );
			Nx::CModel* pRefModel = pRefModelComponent->GetModel();
			   
			// turn off skater brightness, so that it matches the other objects...
			pRefModelComponent->GetModel()->GetModelLights()->SetBrightness( 0.0f );

			// update brightness in case there's any time of day changes
			pRefModelComponent->UpdateBrightness();
			
			bool should_animate = true;
			Obj::CSkeletonComponent* pSkeletonComponent = GetSkeletonComponentFromObject( pCompositeObject );
			Dbg_Assert( pSkeletonComponent );
			Mth::Matrix theMatrix = pCompositeObject->GetDisplayMatrix();
			theMatrix[Mth::POS] = pCompositeObject->GetPos();
			pRefModel->Render( &theMatrix, !should_animate, pSkeletonComponent->GetSkeleton() );
			pRefModel->SetBoneMatrixData( pSkeletonComponent->GetSkeleton() );

			// TODO:  Should do some check that the object's skeleton matches up with the ref object's models.
		
			// Need to update the fake skater's position with the
			// real skater's position, or else the FakeLights will
			// be set incorrectly (FakeLights works by taking the 
			// closest light to the m_pos that is pointed to by the
			// model lights)
			pRefObject->SetPos( pCompositeObject->GetPos() );
		}
		else
		{
			// update brightness in case there's any time of day changes
			pModelComponent->UpdateBrightness();
			
			// remember:
			// if there's no model specified, then it will draw
			// a red box.  the compositeobject's update() loop
			// will draw a red box with identity parameters, and then
			// this will draw a second one.  so don't freak out
			// when you see more boxes than you are expecting
			pModelComponent->Update();

			// GJ:  If you call the Update() function explicitly,
			// the model's double-buffer will be messed up...
			// in this case, however, we have suspended the model
			// component upon creation, so it doesn't get wonky
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCutsceneData::update_moving_objects()
{
	if ( !mp_objectQuickAnim )
	{
		// No OBA data found in CUT file
		return;
	}

	mp_objectQuickAnim->GetInterpolatedHiResFrames( &s_temp_quats[0], &s_temp_vectors[0], m_currentTime );

	// update all the OBA bones that don't have an object associated with it
	for ( int i = 0; i < mp_objectQuickAnim->GetNumBones(); i++ )
	{
		uint32 boneName = mp_objectQuickAnim->GetBoneName(i);
		
		bool objectExists = false;

		for ( int j = m_numObjects - 1; j >= 0; j-- )
		{
			SCutsceneObjectInfo* pInfo = &m_objects[j];
			if ( pInfo->m_objectName == boneName )
			{
				objectExists = true;
			}
		}

		if ( !objectExists )
		{
			Obj::CCompositeObject* pCompositeObject = (Obj::CCompositeObject*)Obj::ResolveToObject( boneName );
			if ( pCompositeObject )
			{
				// update the object's orientation
				Mth::Matrix theMatrix;

				Mth::Vector nullTrans;
				nullTrans.Set(0.0f,0.0f,0.0f,1.0f);		// Mick: added separate initialization
				Mth::QuatVecToMatrix( &s_temp_quats[i], &nullTrans, &theMatrix );

				//theMatrix.Ident();
				pCompositeObject->SetMatrix( theMatrix );
				pCompositeObject->SetDisplayMatrix( theMatrix );

				// update the object's position
				pCompositeObject->m_pos = s_temp_vectors[i];
			}
		}
	}


	// need to go backwards because the head is dependent
	// on the body's matrices being set up correctly
	// for that frame...
	for ( int i = m_numObjects - 1; i >= 0; i-- )
	{
		SCutsceneObjectInfo* pInfo = &m_objects[i];

		Obj::CCompositeObject* pCompositeObject = pInfo->mp_object;

		if ( pInfo->mp_parentObject )
		{
			// we handle the hi-res heads specially...  if we
			// were to use the OBA data, the hi-res head wouldn't
			// match up exactly with the body, possibly due to
			// accumulated round-off error as we move down the
			// parent-child hierarchy

			Obj::CCompositeObject* pParentObject = pInfo->mp_parentObject;
			Dbg_Assert( pParentObject );

			// if it's a neck bone, then use the previous item to link it
			Obj::CSkeletonComponent* pSkeletonComponent = GetSkeletonComponentFromObject( pParentObject );
			Dbg_Assert( pSkeletonComponent );
			Gfx::CSkeleton* pSkeleton = pSkeletonComponent->GetSkeleton();
			Dbg_Assert( pSkeleton );
			Mth::Vector neckPos;
			bool success = pSkeleton->GetBonePosition( CRCD(0x5a0e0860,"bone_neck"), &neckPos );
			if ( !success )
			{
				Dbg_MsgAssert( 0, ( "Couldn't find the neck bone" ) );
			}

			pCompositeObject->m_pos = pParentObject->m_pos + ( neckPos * pParentObject->GetDisplayMatrix() );
			pCompositeObject->m_pos[W] = 1.0f;

			Mth::Matrix bone_matrix;
			success = pSkeleton->GetBoneMatrix( CRCD(0x5a0e0860,"bone_neck"), &bone_matrix );
			if ( !success )
			{
				Dbg_MsgAssert( 0, ( "Couldn't find the neck bone" ) );
			}

			// build the object's matrix
			Mth::Matrix object_matrix = pParentObject->GetDisplayMatrix();
			object_matrix[W] = pParentObject->GetPos();
			object_matrix[X][W] = 0.0f;
			object_matrix[Y][W] = 0.0f;
			object_matrix[Z][W] = 0.0f;
			
			// rotate by 90:  doesn't seem to be needed
			// any more, perhaps due to an exporter bug
			// being fixed
			//Mth::Vector temp;
			//temp = bone_matrix[Y];
			//bone_matrix[Y] = bone_matrix[Z];
			//bone_matrix[Z] = -temp;

			// map the bone to world space
			bone_matrix *= object_matrix; 
			bone_matrix[W] = Mth::Vector(0.0f,0.0f,0.0f,1.0f);

			pCompositeObject->SetMatrix( bone_matrix );
			pCompositeObject->SetDisplayMatrix( bone_matrix );

			// need to update the component manually,
			// because it is probably out of range for
			// the suspension logic
			update_model_component( pCompositeObject );
		}
		else if ( pInfo->m_obaIndex != -1 )
		{
			// if the object is directly referenced in the OBA file,
			// then use the position and rotation specified in the OBA.

			// update the object's orientation
			Mth::Matrix theMatrix;

			Mth::Vector nullTrans;
			nullTrans.Set(0.0f,0.0f,0.0f,1.0f);		// Mick: added separate initialization
			Mth::QuatVecToMatrix( &s_temp_quats[pInfo->m_obaIndex], &nullTrans, &theMatrix );

			// GJ:  this used to be a kludge to fix objects
			// being rotated incorrectly by 90 degrees.  it
			// was probably due to an exporter bug, which
			// seems to have been fixed.  in any case,
			// it doesn't make sense why we would need
			// to rotate it in code, so i will comment
			// this out permanently and we can fix the
			// bug in the exporter if it ever arises again.
			if ( pInfo->m_skeletonName == 0 )
			{
				// but we still need to do it for MDL files
				// because it doesn't have SKA data that
				// automatically rotates it
				// theMatrix.RotateXLocal( Mth::DegToRad(90.0f) );
			}

			//theMatrix.Ident();
			pCompositeObject->SetMatrix( theMatrix );
			pCompositeObject->SetDisplayMatrix( theMatrix );

			// update the object's position
			pCompositeObject->m_pos = s_temp_vectors[pInfo->m_obaIndex];

//			Dbg_Message( "Object %d:   ", i );
//	 		pCompositeObject->m_pos.PrintContents();
//			pCompositeObject->GetDisplayMatrix().PrintContents();

			// need to update the component manually,
			// because it is probably out of range for
			// the suspension logic
			update_model_component( pCompositeObject );
		}

//		if ( pCompositeObject->GetID() == Script::GenerateCRC( "Skin_CAS2" ) )
//		{
//			printf( "Setting %s at time %d"\n, Script::FindChecksumName(pCompositeObject->GetID()), (int)(m_currentTime/60.0f+0.5f) );
//			pCompositeObject->GetPos().PrintContents();
//		}

#if 0
		// print out world-space position of board for adam:
		{
			Obj::CSkeletonComponent* pSkeletonComponent = GetSkeletonComponentFromObject( pCompositeObject );
			if ( pSkeletonComponent )
			{
				Gfx::CSkeleton* pSkeleton = pSkeletonComponent->GetSkeleton();
				Mth::Vector theVec;
				if ( pSkeleton->GetBonePosition( Script::GenerateCRC("bone_board_root"), &theVec ) )
				{
					printf( "Position of %s at time %f:\n\t", Script::FindChecksumName(pCompositeObject->GetID()), m_currentTime );
					pCompositeObject->GetPos().PrintContents();
					
					printf( "Position of %s's board at time %f:\n\t", Script::FindChecksumName(pCompositeObject->GetID()), m_currentTime );
					theVec = pCompositeObject->GetPos() + ( theVec * pCompositeObject->GetDisplayMatrix() );
					theVec.PrintContents();
             					
					printf( "--------------------------------\n" );
				}
			}
		}
#endif	  

		if ( Script::GetInteger( CRCD(0x5ecc0073,"debug_cutscenes"), Script::NO_ASSERT ) )
		{
			// set up bounding box
			SBBox theBox;
			theBox.m_max.Set(1.0f, 10.0f, 1.0f);
			theBox.m_min.Set(-1.0f, -1.0f, -1.0f);    

			// For now, draw a bounding box
			Gfx::AddDebugBox( pCompositeObject->GetDisplayMatrix(), pCompositeObject->GetPos(), &theBox, NULL, 1, NULL ); 
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCutsceneData::get_current_frame(Mth::Quat* pQuat, Mth::Vector* pTrans)
{
	Dbg_Assert( pQuat );
	Dbg_Assert( pTrans );

	// with cutscenes, i don't actually want to get 
	// the interpolated frames, i actually want the closest frame
	// this is for camera positions that are held on a certain
	// frame for a period of time, and then jumps somewhere else
	// in this case we would want either the old pose or the new pose,
	// not something in the middle

	// this means we will not want compression on it

	// grab the frame from the animation controller
	Dbg_MsgAssert( mp_cameraQuickAnim, ( "Camera data has not been initialized for this cutscene" ) );
	Dbg_Assert( mp_cameraQuickAnim->GetNumBones() == 1 );
	mp_cameraQuickAnim->GetInterpolatedHiResFrames( pQuat, pTrans, m_currentTime );
}

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCutsceneDetails::CCutsceneDetails()
{				  
	mp_cutsceneAsset = NULL;
	m_cutsceneAssetName = 0;

	mp_cutsceneStruct = new Script::CStruct;

	m_numHiddenObjects = 0;
	mp_hiddenObjects = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCutsceneDetails::~CCutsceneDetails()
{
	if ( mp_cutsceneStruct )
	{
		delete mp_cutsceneStruct;
	}

	if ( mp_cutsceneAsset )
	{
		Ass::CAssMan* pAssMan = Ass::CAssMan::Instance();
		Ass::CAsset* pAsset = pAssMan->GetAssetNode( m_cutsceneAssetName, true );
		pAssMan->UnloadAsset( pAsset );
		mp_cutsceneAsset = NULL;
	}

	if ( mp_hiddenObjects )
	{
		delete[] mp_hiddenObjects;
		mp_hiddenObjects = NULL;
	}
	
	// restore the dynamic light modulation factor
	Nx::CLightManager::sSetAmbientLightModulationFactor( m_oldAmbientLightModulationFactor );
	Nx::CLightManager::sSetDiffuseLightModulationFactor( 0, m_oldDiffuseLightModulationFactor[0] );
	Nx::CLightManager::sSetDiffuseLightModulationFactor( 1, m_oldDiffuseLightModulationFactor[1] );

	// don't need to refresh the skater, because
	// it will be applied on the next frame anyway
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCutsceneDetails::InitFromStructure( Script::CStruct* pParams )
{
	bool success = false;

	/// need to create a temp copy of the params...  this is because
	// the PreCutscene script destroys the screen element that launched
	// the cutscene, meaning that afterwards the data inside pParams 
	// will become corrupt...
	Script::CStruct* pTempParams = new Script::CStruct;
	pTempParams->AppendStructure( pParams );

	this->SetParams( pTempParams );

	// TODO:  Handle async loads of cutscene
	bool async = false;
//	bool dont_assert = false;

	// TODO:  Need to account for the possibility that the same
	// cutscene will be loaded twice

	// TODO:  Push the correct heap

	// turn off the modulation for the duration of the cutscene
	m_oldAmbientLightModulationFactor = Nx::CLightManager::sGetAmbientLightModulationFactor();
	m_oldDiffuseLightModulationFactor[0] = Nx::CLightManager::sGetDiffuseLightModulationFactor(0);
	m_oldDiffuseLightModulationFactor[1] = Nx::CLightManager::sGetDiffuseLightModulationFactor(1);

	Nx::CLightManager::sSetAmbientLightModulationFactor( 0.0f );
	Nx::CLightManager::sSetDiffuseLightModulationFactor( 0, 0.0f );
	Nx::CLightManager::sSetDiffuseLightModulationFactor( 1, 0.0f );

	// force the fake lights to allocate its memory now
	// (otherwise, the next call to RenderWorld() would
	// alloc it which would end up fragmenting memory)
	Nx::CLightManager::sUpdateVCLights();

	// also need to turn it off on the skater to refresh it
	// (but that will be done if there's a ref model)

	const char* pCutsceneFileName;
	pTempParams->GetText( CRCD(0xa1dc81f9,"name"), &pCutsceneFileName, Script::ASSERT );
	
	Ass::CAssMan* pAssMan = Ass::CAssMan::Instance();
	if ( pAssMan->GetAsset( pCutsceneFileName, false ) )
	{
		Dbg_Message( "Cutscene %s is already playing!", pCutsceneFileName );
		goto init_failed;
	}
	else
	{
		char* pCutName = get_cut_name( pCutsceneFileName );
		char structName[256];
		strcpy( structName, pCutName );
		strcat( structName, "_params" );

		Script::CStruct* pCutParams = Script::GetStructure(structName, Script::NO_ASSERT);
		if ( pCutParams )
		{
			mp_cutsceneStruct->AppendStructure( pCutParams );
		}

		// first look at "dont_unload_anims" flag in cutscene struct
		int should_unload_anims = !mp_cutsceneStruct->ContainsFlag( CRCD(0xbafa9f1c,"dont_unload_anims") );
		int should_reload_anims = !mp_cutsceneStruct->ContainsFlag( CRCD(0xbafa9f1c,"dont_unload_anims") );

		if ( pParams->ContainsFlag( CRCD(0x7ddde00e,"from_cutscene_menu") ) )
		{
			// if we're coming from the cutscene menu, we never want to unload anims
			should_unload_anims = true;
			should_reload_anims = true;
		}

		mp_cutsceneStruct->AddInteger( CRCD(0xcdf2b13e,"unload_anims"), should_unload_anims );
		mp_cutsceneStruct->AddInteger( CRCD(0x9a4bdff5,"reload_anims"), should_reload_anims );

		// by default, unload the goals
		mp_cutsceneStruct->AddInteger( CRCD(0xed064302,"unload_goals"), 1 );
		mp_cutsceneStruct->AddInteger( CRCD(0xbabf2dc9,"reload_goals"), 1 );

		// "unload_anims" and "reload_anims" can be overridden from playcutscene line
		mp_cutsceneStruct->AppendStructure( pTempParams );

		// hide all moving objects here...
		// (do it before normal hide/unhide scripts happen)
		hide_moving_objects();

		Script::RunScript( CRCD(0xe1ce826a,"PreCutscene"), mp_cutsceneStruct );

		// run the script after the cutscene, so
		// that we can unpause the objects and such...
		char startScriptName[256];
		strcpy( startScriptName, pCutName );
		strcat( startScriptName, "_begin" );
		char endScriptName[256];
		strcpy( endScriptName, pCutName );
		strcat( endScriptName, "_end" );
		m_startScriptName = Crc::GenerateCRCFromString( startScriptName );
		m_endScriptName = Crc::GenerateCRCFromString( endScriptName );

		// GJ:  "use_pip" flag would be handy here!

		mp_cutsceneAsset = (CCutsceneData*)pAssMan->LoadAsset( pCutsceneFileName, async, false, false, 0 );

		// couldn't get cutscene
		Dbg_MsgAssert( mp_cutsceneAsset, ( "Couldn't create cutscene data %s", pCutsceneFileName ) );

		m_cutsceneAssetName = Script::GenerateCRC( pCutsceneFileName );
    }

	success = true;

init_failed:
	delete pTempParams;
	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCutsceneDetails::Cleanup()
{
#ifdef __PLAT_NGC__
	g_in_cutscene = false;
#endif		// __PLAT_NGC__

	// kill the cutscene panel, if it exists
	Script::RunScript( CRCD(0xe7bc7efd,"kill_cutscene_panel") );

	// need to delete the cutscene asset,
	// so that there will be enough room for all
	// the anims reloaded in "PostCutscene"
	if ( mp_cutsceneAsset )
	{
		Ass::CAssMan* pAssMan = Ass::CAssMan::Instance();
		Ass::CAsset* pAsset = pAssMan->GetAssetNode( m_cutsceneAssetName, true );
		pAssMan->UnloadAsset( pAsset );
		mp_cutsceneAsset = NULL;
	}

	if ( m_endScriptName )
	{
		Script::RunScript( m_endScriptName );
	}
		
	// kill special effects, in case the artist forgot to do it 
	// in the individual cutscene end scripts... (need to do this
	// before the cutscene heap is destroyed)
	Script::RunScript( CRCD(0xf49a8c9c,"kill_cutscene_camera_hud") );

	// Clear any currently running FakeLights commands, in case the player
	// aborted while they were still taking effect during the cutscene
	// (in which case the fake lights would have gone on the cutscene heap)
	Nx::CLightManager::sClearCurrentFakeLightsCommand();

	// destroy the cutscene heap (at this point, any allocations made during 
	// the cutscene should have been freed up)
	Mem::Manager::sHandle().DeleteCutsceneHeap();

	Script::RunScript( CRCD(0xbca1791c,"PostCutscene"), mp_cutsceneStruct );

	// unhide moving objects, based on whether they were hidden...
	unhide_moving_objects();

	CMovieDetails::Cleanup();

	Script::RunScript( CRCD(0x15674315,"restore_skater_camera") );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCutsceneDetails::Update()
{
	if ( !mp_cutsceneAsset )
	{
		Dbg_Message( "No cutscene asset!" );
		return;
	}

	if ( mp_cutsceneAsset->LoadFinished() )
	{
		if ( !m_startScriptAlreadyRun && m_startScriptName )
		{
			Script::RunScript( m_startScriptName );
			m_startScriptAlreadyRun = true;
		}

		mp_cutsceneAsset->Update( mp_camera, mp_cutsceneStruct );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCutsceneDetails::HasMovieStarted()
{
	if ( !mp_cutsceneAsset )
	{
		Dbg_MsgAssert( 0, ( "No cutscene asset!" ) );
		return false;
	}

	return mp_cutsceneAsset->HasMovieStarted();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCutsceneDetails::ResetCustomKeys()
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCutsceneDetails::IsComplete()
{
	if ( m_aborted )
	{
		return true;
	}

	if ( m_holdOnLastFrame )
	{
		return false;
	}

	if ( mp_cutsceneAsset )
	{
		return mp_cutsceneAsset->LoadFinished() && mp_cutsceneAsset->IsAnimComplete();
	}
		
	return false;
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCutsceneDetails::IsHeld()
{
	return false;
}
									  
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCutsceneDetails::OverridesCamera()
{
	if ( mp_cutsceneAsset )
	{
		return mp_cutsceneAsset->OverridesCamera();
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCutsceneDetails::hide_moving_objects()
{
	// hides the goal peds
	Script::RunScript( CRCD(0x722d9ca7,"cutscene_hide_objects") );

	// first count the number of objects, so we can allocate the proper number of hide flags
	int numObjects = 0;
	Obj::CBaseComponent *p_component = Obj::CCompositeObjectManager::Instance()->GetFirstComponentByType( CRCD(0x286a8d26,"model") );
	while( p_component )
	{
		Obj::CModelComponent* pModelComponent = ((Obj::CModelComponent*)p_component);
		Nx::CModel* pModel = pModelComponent->GetModel();
		if ( p_component->GetObject()->GetID() == 0 )
		{
			// skip the skater...
		}
		else if ( !pModel->IsHidden() )
		{
			numObjects++;
		}
		p_component = p_component->GetNextSameType();		
	}
	Dbg_MsgAssert( mp_hiddenObjects	== NULL, ( "hidden objects array already exists?" ) );

	if ( !numObjects)
		return;


	mp_hiddenObjects = new uint32[numObjects];
	m_numHiddenObjects = 0;
	
	// now actually go through and hide them
	p_component = Obj::CCompositeObjectManager::Instance()->GetFirstComponentByType( CRCD(0x286a8d26,"model") );
	while( p_component )
	{
		Obj::CModelComponent* pModelComponent = ((Obj::CModelComponent*)p_component);
		Nx::CModel* pModel = pModelComponent->GetModel();
		if ( p_component->GetObject()->GetID() == 0 )
		{
			// skip the skater...
		}
		else if ( !pModel->IsHidden() )
		{
			// if this fires off, that would be really weird because we just counted the exact
			// number we need a few moments ago...
			Dbg_MsgAssert( m_numHiddenObjects < numObjects, ( "Too many objects in scene to hide?" ) );

//			printf( "Found model to hide (%s)\n", Script::FindChecksumName(p_component->GetObject()->GetID()) );
			mp_hiddenObjects[m_numHiddenObjects] = p_component->GetObject()->GetID();
			pModel->Hide( true );
			m_numHiddenObjects++;
		}

		p_component = p_component->GetNextSameType();		
	}

	printf( "Hid %d objects\n", m_numHiddenObjects );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCutsceneDetails::unhide_moving_objects()
{
	if ( mp_hiddenObjects )
	{
		for ( int i = 0; i < m_numHiddenObjects; i++ )
		{
			Obj::CCompositeObject* pObject = (Obj::CCompositeObject*)Obj::ResolveToObject( mp_hiddenObjects[i] );
			if ( pObject )
			{
				Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( pObject );
				Nx::CModel* pModel = pModelComponent->GetModel();
				pModel->Hide( false );
			}
			else
			{
				// it's possible that they won't exist anymore
				// because of the call to uninitialize all goals...
				//Dbg_MsgAssert( pObject, ( "Couldn't find object %s to unhide?", Script::FindChecksumName(mp_hiddenObjects[i]) ) );
			}
		}

		delete[] mp_hiddenObjects;
		mp_hiddenObjects = NULL;
	}

	m_numHiddenObjects = 0;
	
	// unhides the goal peds
	Script::RunScript( CRCD(0x94d0d528,"cutscene_unhide_objects") );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj
