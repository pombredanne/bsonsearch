#include <stdio.h>


#include <bsoncompare.h>
#include <uthash.h>

int compare_rgx_good(){
    bson_error_t error;
    bson_error_t error2;
    bson_t      *spec;
    bson_t      *doc;

    const char *json = "{\"hello\": \"world\"}";
    const char *jsonspec = "{\"hello\": {\"$options\": \"\", \"$regex\": \"orl\"}}";
    doc = bson_new_from_json (json, -1, &error);
    spec = bson_new_from_json (jsonspec, -1, &error2);
    const uint8_t *spec_bson = bson_get_data(spec);
    const uint8_t *doc_bson = bson_get_data(doc);
    int yes = compare(spec_bson, spec->len, doc_bson, doc->len);
    bson_free(spec);
    bson_free(doc);
    return yes;

}
int compare_rgx_good_case_insensitive(){
    bson_error_t error;
    bson_error_t error2;
    bson_t      *spec;
    bson_t      *doc;

    const char *json = "{\"hello\": \"world\"}";
    const char *jsonspec = "{\"hello\": {\"$options\": \"i\", \"$regex\": \"ld\"}}";
    doc = bson_new_from_json (json, -1, &error);
    spec = bson_new_from_json (jsonspec, -1, &error2);
    const uint8_t *spec_bson = bson_get_data(spec);
    const uint8_t *doc_bson = bson_get_data(doc);
    int yes = compare(spec_bson, spec->len, doc_bson, doc->len);
    bson_free(spec);
    bson_free(doc);
    return yes;

}
int check_precompiled_regex(){
    int i = 0;
    for (i = 0; i < 100000; ++i) {
        compare_rgx_good();
    }
    for (i = 0; i < 100000; ++i) {
        compare_rgx_good_case_insensitive();
    }
    regex_destroy();
}
int
main (int   argc,
      char *argv[])
{
    check_precompiled_regex();
}