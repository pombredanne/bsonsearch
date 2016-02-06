/*
 * Copyright (c) 2016 Bauman
 * The MIT License (MIT)
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef MONGOC_MATCHER_OP_GEOJSON_H
#define MONGOC_MATCHER_OP_GEOJSON_H

#include <bson.h>
#include "mongoc-matcher-op-private.h"

BSON_BEGIN_DECLS

#define MONGOC_EARTH_RADIUS_M 6371000
#define RADIAN_MAGIC_NUMBER 0.01745329251 //pi/180

mongoc_matcher_op_t *
_mongoc_matcher_op_geonear_new     ( const char              *path,   /* IN */
                                     bson_iter_t       *child);   /* IN */
bool  _mongoc_matcher_op_geonear_iter_values     ( bson_iter_t           near_iter,  /* IN */
                                                   mongoc_matcher_op_t   *op) ; /*OUT*/
bool _mongoc_matcher_op_geonear_parse_geometry     ( bson_iter_t           near_iter,  /* IN */
                                                     mongoc_matcher_op_t   *op) ; /*OUT*/
bool haversign_distance(double lon1, double lat1, double lon2, double lat2, double *distance);
bool _mongoc_matcher_op_geonear (mongoc_matcher_op_near_t    *near, /* IN */
                                 const bson_t                *bson) ;/* IN */
BSON_END_DECLS


#endif /* MONGOC_MATCHER_OP_GEOJSON_H */