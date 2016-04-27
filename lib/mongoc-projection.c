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
#ifdef WITH_PROJECTION

#include <stddef.h>
#include <uthash.h>
#include "mongoc-matcher.h"
#include "mongoc-matcher-private.h"
#include "mongoc-matcher-op-private.h"
#include "mongoc-projection.h"
#include "mongoc-bson-descendants.h"


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_parse_projection --
 *
 *       Parse an aggregation spec containing a projection operator
 *       $project.
 *
 *       See the following link for more information.
 *
 *       https://docs.mongodb.org/manual/tutorial/project-fields-from-query-results/
 *
 *       Differences, this does not support the 0 value to omit a single field.
 *       If the key is present, it will do it's best to include it in the output
 *
 *
 * Requires:
 *       iter MUST Be a document type, otherwise, outcome undefined.
 *
 * Returns:
 *       A newly allocated mongoc_matcher_op_t if successful; otherwise
 *       NULL and @error may be set.
 *
 * Side effects:
 *       @error may be set.
 *
 *--------------------------------------------------------------------------
 */
mongoc_matcher_op_t *
_mongoc_matcher_parse_projection (mongoc_matcher_opcode_t  opcode,  /* IN */
                                  bson_iter_t             *iter,    /* IN */
                                  bool                     is_root, /* IN */
                                  bson_error_t            *error)   /* OUT */
{
    mongoc_matcher_op_t *op = NULL;
    BSON_ASSERT (opcode == MONGOC_MATCHER_OPCODE_PROJECTION);
    BSON_ASSERT (iter);

    bson_iter_t child;
    if (bson_iter_recurse(iter, &child)) {
        op = _mongoc_matcher_parse_projection_loop(&child, error);
    }
    return op;
}

mongoc_matcher_op_t *
_mongoc_matcher_parse_projection_loop (bson_iter_t             *iter,    /* IN */
                                       bson_error_t            *error)   /* OUT */
{
    mongoc_matcher_op_t *on_the_left=NULL;
    mongoc_matcher_op_t *next_left = NULL;
    if (bson_iter_next(iter)){
        const bson_value_t * value = bson_iter_value(iter);
        char * d_value = NULL;
        switch (value->value_type){
            case BSON_TYPE_UTF8:
            {
                uint32_t vlen=0;
                const char * value = bson_iter_utf8(iter, &vlen);
                d_value = bson_strdup(value);

            }
            case BSON_TYPE_BOOL:
            case BSON_TYPE_INT32:
            {
                const char * key = bson_iter_key (iter);
                next_left = _mongoc_matcher_parse_projection_loop(iter, error);
                on_the_left = (mongoc_matcher_op_t *)bson_malloc0 (sizeof *on_the_left);
                on_the_left->base.opcode = MONGOC_MATCHER_OPCODE_PROJECTION;
                on_the_left->projection.path = bson_strdup(key);
                on_the_left->projection.next = next_left;
                on_the_left->projection.as = d_value; //null or set in UTF8 case
                on_the_left->projection.pathlist = NULL;
                break;
            }
            case BSON_TYPE_DOCUMENT:
            {
                mongoc_matcher_op_str_hashtable_t *multimatch =NULL;
                multimatch =_mongoc_matcher_parse_projection_complex(iter, error);
                const char * key = bson_iter_key (iter);
                next_left = _mongoc_matcher_parse_projection_loop(iter, error);
                on_the_left = (mongoc_matcher_op_t *)bson_malloc0 (sizeof *on_the_left);
                on_the_left->base.opcode = MONGOC_MATCHER_OPCODE_PROJECTION;
                on_the_left->projection.path = NULL;
                on_the_left->projection.next = next_left;
                on_the_left->projection.as = bson_strdup(key);
                on_the_left->projection.pathlist = multimatch;
                break;
            }
            default:
                break;
        }
    }
    return on_the_left;

}
mongoc_matcher_op_str_hashtable_t *
_mongoc_matcher_parse_projection_complex (bson_iter_t             *iter,    /* IN */
                                          bson_error_t            *error)   /* OUT */
{
    mongoc_matcher_op_str_hashtable_t *found_in_set = NULL;
    bson_iter_t child;
    const char * key = "";
    if (bson_iter_recurse(iter, &child)){
        while (bson_iter_next(&child))
        {
            key = bson_iter_key (&child);
            bson_iter_t found_list;
            if (strcmp (key, "foundin") == 0 &&
                    BSON_ITER_HOLDS_ARRAY(&child)&&
                    bson_iter_recurse(&child, &found_list))
            {
                while (bson_iter_next(&found_list)){
                    if (BSON_ITER_HOLDS_UTF8(&found_list))
                    {
                        mongoc_matcher_op_str_hashtable_t *s = NULL;
                        s = ( mongoc_matcher_op_str_hashtable_t *)malloc(sizeof( mongoc_matcher_op_str_hashtable_t ));
                        uint32_t str_len= 0;
                        const char * matcher_hash_key_local = bson_iter_utf8(&found_list, &str_len);
                        char *matcher_hash_key_persist = bson_strdup(matcher_hash_key_local);
                        s->matcher_hash_key = matcher_hash_key_persist;
                        HASH_ADD_STR(found_in_set, matcher_hash_key, s);
                    }
                }
            }
        }
    }
    return found_in_set;
}
static
bool
mongoc_matcher_projection_execute_find(mongoc_matcher_op_t *current,
                                       bson_t              *bson,
                                       bson_t              *arrlist,
                                       int                  *checked,
                                       int                  *skip,
                                       uint32_t             *packed)
{
    bson_iter_t iter, tmp;
    if (strchr (current->projection.path, '.')) {
        if (bson_iter_init (&tmp, bson) )
        {
            if (bson_iter_find_descendant (&tmp, current->projection.path, &iter))
            {
                mongoc_matcher_projection_value_into_array(&iter, arrlist, (*checked));
            } else {
                while (bson_iter_init (&tmp, bson) &&
                       bson_iter_find_descendants (&tmp, current->projection.path, skip, &iter)){
                    (*packed) += mongoc_matcher_projection_value_into_array(&iter, arrlist, (*packed));
                    (*checked) = (*checked) + 1;
                    (*skip) = (*checked);
                }
            }
        }
    } else if (bson_iter_init_find (&iter, bson, current->projection.path)) {
        int objects_added = mongoc_matcher_projection_value_into_array(&iter, arrlist, (*checked));
        (*checked) += objects_added;
        (*packed)  += objects_added;
    }
    return true;
}


bool
mongoc_matcher_projection_execute(mongoc_matcher_op_t *op,     //in
                                  bson_t              *bson,        //in
                                  bson_t              *projected)
{
    assert(op->base.opcode == MONGOC_MATCHER_OPCODE_PROJECTION);
    bson_t arrlist;
    bool result = true;
    bson_init (projected);
    mongoc_matcher_op_t *current = op;

    do {
        int checked = 0, skip=0;
        uint32_t packed = 0;
        if (current->projection.as == NULL){
            bson_append_array_begin (projected, current->projection.path, -1, &arrlist);
        }
        else{
            bson_append_array_begin (projected, current->projection.as, -1, &arrlist);
        }
        if (current->projection.path != NULL)
        {
            mongoc_matcher_projection_execute_find(current, bson, &arrlist, &checked, &skip, &packed);
        } else if (current->projection.pathlist != NULL)
        {
            mongoc_matcher_op_str_hashtable_t *s, *tmp_hash;
            HASH_ITER(hh, current->projection.pathlist, s, tmp_hash)
            {
                current->projection.path = s->matcher_hash_key;
                mongoc_matcher_projection_execute_find(current, bson, &arrlist, &checked, &skip, &packed);
            }
            current->projection.path = NULL;
        }
        bson_append_array_end (projected, &arrlist);
        current = current->projection.next;
    } while (current);
    /*
    char * str;
    str = bson_as_json(projected, NULL);
    printf("%s\n", str); //*/

    return result;
}
/*
 *--------------------------------------------------------------------------
 *
 * mongoc_matcher_projection_value_into_array --
 *
 *       appends the appropriate type given by iterator type to a bson_t
 *
 * Requires:
 *          arrlist should be an array
 *          calling function is responsible for
 *              bson_init
 *              bson_append_array_begin
 *              bson_append_array_end
 *
 * Returns:
 *       unsigned integer number of objects added to bson_t array
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */
uint32_t
mongoc_matcher_projection_value_into_array(bson_iter_t  *iter, bson_t *arrlist, uint32_t i)
{
    /*----------------------------------------------------------------------------
     * Notes on Building Arrays.  Casting int to str is usually NOT optimized
     * http://api.mongodb.org/libbson/current/performance.html  links to
     * http://api.mongodb.org/libbson/current/bson_uint32_to_string.html
     *
     */
    uint32_t objects_added = 0;
    char STR_BUFFER[16]; //Temp space for bson_uint32_to_string
    const char *key;  //key in array, because arrays are really documents with ascending string values in bson
    size_t st = bson_uint32_to_string (i, &key, STR_BUFFER, sizeof STR_BUFFER);
    /* end recommended performance optimiaztion, have the string value of the int counter in "key" var */
    switch (bson_iter_type (iter)) {
        case BSON_TYPE_DOCUMENT:
        {
            uint32_t          document_len;
            const uint8_t     *document;
            bson_iter_document(iter, &document_len, &document);
            bson_t *doc_data = bson_new_from_data(document, document_len);
            bson_append_document(arrlist, key, st, doc_data);//this performs a memcopy
            objects_added += 1;
            bson_free(doc_data);
            break;
        }
        case BSON_TYPE_ARRAY:
        {
            bson_iter_t right_array;
            bson_iter_recurse(iter, &right_array);
            while (bson_iter_next(&right_array)) {
                /* this isnt going to work if it finds an array within an array.
                 * Should probably pass i as a pointer so everyone can track the same i
                 */
                i += mongoc_matcher_projection_value_into_array( &right_array, arrlist, i);
                objects_added += 1;
            }
            break;
        }
        case BSON_TYPE_UTF8:
        {
            uint32_t vlen=-1;
            const char * value = bson_iter_utf8(iter, &vlen);
            bson_append_utf8(arrlist, key, st, value, vlen);
            objects_added += 1;
            break;
        }
        case BSON_TYPE_INT32:
        {
            int32_t num = bson_iter_int32(iter);
            bson_append_int32 (arrlist, key, st, num);
            objects_added += 1;
            break;
        }
        case BSON_TYPE_INT64:
        {
            int64_t num = bson_iter_int64(iter);
            bson_append_int64 (arrlist, key, st, num);
            objects_added += 1;
            break;
        }
        case BSON_TYPE_DOUBLE:
        {
            double dbl = bson_iter_double(iter);
            bson_append_double (arrlist, key, st, dbl);
            objects_added += 1;
            break;
        }
        case BSON_TYPE_OID:
        {
            const bson_oid_t * oid = bson_iter_oid(iter);
            bson_append_oid (arrlist, key, st, oid);
            objects_added += 1;
            break;
        }
        case BSON_TYPE_DATE_TIME:
        {
            int64_t dt = bson_iter_date_time(iter);
            bson_append_date_time (arrlist, key, st, dt);
            objects_added += 1;
            break;
        }
        case BSON_TYPE_BOOL:
        {
            bool bl = bson_iter_bool(iter);
            bson_append_bool (arrlist, key, st, bl);
            objects_added += 1;
            break;
        }
        case BSON_TYPE_BINARY:
        {
            bson_subtype_t subtype;
            uint32_t binary_len=0;
            const uint8_t * binary;
            bson_iter_binary(iter, &subtype, &binary_len, &binary);
            bson_append_binary (arrlist, key, st, subtype, binary, binary_len);
            objects_added += 1;
            break;
        }
        case BSON_TYPE_REGEX:
        {
            const char * regex_pattern, *regex_options;
            regex_pattern = bson_iter_regex (iter, &regex_options);
            bson_append_regex(arrlist, key, st, regex_pattern, regex_options);
            objects_added += 1;
            break;
        }
        default:
            objects_added = 0;
            break;
    }
    return objects_added;
}


#endif //WITH_PROJECTION