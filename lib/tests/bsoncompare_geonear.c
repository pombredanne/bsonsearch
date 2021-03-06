#include <stdio.h>
#include <bsoncompare.h>

int compare_json(const char *json,
                 const char *jsonspec ){
    bson_error_t error;
    bson_error_t error2;
    bson_t      *spec;
    bson_t      *doc;
    doc = bson_new_from_json (json, -1, &error);
    spec = bson_new_from_json (jsonspec, -1, &error2);
    const uint8_t *spec_bson = bson_get_data(spec);
    const uint8_t *doc_bson = bson_get_data(doc);
    int yes = compare(spec_bson, spec->len, doc_bson, doc->len);
    bson_free(spec);
    bson_free(doc);
    return yes;
}


int
main (int   argc,
      char *argv[])
{

    BSON_ASSERT(compare_json("{\"loc\": {\"type\": \"Point\", \"coordinates\": [-61.08080307722216, -9.057610600760512]}}",
                             "{\"loc\": {\"$near\": {\"$minDistance\": 0.0, \"$geometry\": {\"type\": \"Point\", \"coordinates\": [61.08080307722216, -12.057610600760512]}, \"$maxDistance\": 100000000.0}}}"));
    BSON_ASSERT(!compare_json("{\"loc\": [{\"type\": \"Point\", \"coordinates\": [-61.08080307722216, -9.057610600760512]}]}",
                        "{\"loc\": {\"$near\": {\"$minDistance\": 0.0, \"$geometry\": {\"type\": \"Point\", \"coordinates\": [61.08080307722216, -12.057610600760512]}, \"$maxDistance\": 100000000.0}}}"));
    return 0;
}